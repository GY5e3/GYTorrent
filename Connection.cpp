#include "Connection.hpp"

Connection::Connection(const TorrentMetaData &torrentMetaData,
                       boost::asio::io_context &io,
                       std::shared_ptr<Mediator> mediator) : m_torrentMetaData(torrentMetaData),
                                                             m_io(io),
                                                             m_mediator(mediator)
{
}

std::shared_ptr<Connection> Connection::getSharedPtr()
{
    return shared_from_this();
}

bool Connection::InitializeConnection(utils::Peer &peer)
{
    m_socket = std::make_shared<boost::asio::ip::tcp::socket>(m_io);

    m_peer = utils::Peer(peer.GetPeerIP(), peer.GetPeerPort());
    boost::asio::spawn(m_io, [this](boost::asio::yield_context yield)
                       {
        try 
        {
            boost::asio::ip::tcp::resolver resolver(m_io);
            boost::system::error_code ec;
            auto endpoints = resolver.resolve(m_peer.GetPeerIP(), m_peer.GetPeerPort(), ec);
            if (ec)
            {
                std::cout << "Failed to resolve: " << ec.message() + " " + m_peer.GetPeerIP() + " " + m_peer.GetPeerPort() << std::endl;
                return false;
            }

            boost::asio::async_connect(*m_socket, endpoints, yield[ec]);
            if (!ec)
            {
                std::cout << "Successfully connected! " + m_peer.ToString() << std::endl;

                if (initiateHandshake())
                {
                    /// TODO: Probably, i should move it in Mediator
                    m_mediator->AddActiveConnection(getSharedPtr());
                    SendMessage(utils::MessageID::interested, std::vector<int>());
                    RecieveMessage();
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                std::cout << "Failed to connect: " << ec.message() + " " + m_peer.GetPeerIP() + " " + m_peer.GetPeerPort() << std::endl;
                return false;
            }
        } 
        catch (const boost::system::system_error &e)
        {
            std::cout << "Error in InitializeConnection: " << e.what() << " for peer " + m_peer.ToString() << std::endl;
        } 
        return true;});
    return true;
}

bool Connection::initiateHandshake()
{
    std::stringstream send;
    send << static_cast<char>(utils::PROTOCOL_ID);
    send << "BitTorrent protocol";
    std::string reserved(8, '\0');
    send << reserved;
    send << m_torrentMetaData.GetTorrentHash();
    send << m_torrentMetaData.GetMyPeerID();

    boost::asio::write(*m_socket, boost::asio::buffer(send.str()));

    /// TODO: Logging sended bytes

    std::string response(utils::HANDSHAKE_LENGTH, '\0');
    boost::system::error_code ec;
    boost::asio::read(*m_socket, boost::asio::buffer(response), boost::asio::transfer_all(), ec);

    /// TODO: Logging recieved bytes

    std::istringstream response_stream(std::string(response.begin(), response.end()));

    unsigned char responseProtocolID;
    std::string responseProtocolStr;
    std::vector<unsigned char> responseTorrentHash(utils::TORRENT_HASH_LENGTH);
    std::vector<unsigned char> responsePeerID(utils::PEER_ID_LENGTH);

    response_stream.read(reinterpret_cast<char *>(&responseProtocolID), 1);
    responseProtocolStr.resize(responseProtocolID);
    response_stream.read(&responseProtocolStr[0], responseProtocolID);
    response_stream.ignore(utils::FREE_BYTES_LENGTH);
    response_stream.read(reinterpret_cast<char *>(responseTorrentHash.data()), responseTorrentHash.size());
    response_stream.read(reinterpret_cast<char *>(responsePeerID.data()), responsePeerID.size());

    std::string responseTorrentHashStr(begin(responseTorrentHash), end(responseTorrentHash));

    /// TODO: Probably I should move it to Mediator
    if (ec)
    {
        std::cout << "Failed to handshake: " << ec.message() + " " + m_peer.ToString() << std::endl;
        return false;
    }
    if (m_torrentMetaData.GetTorrentHash() != responseTorrentHashStr)
    {
        std::cout << "Failed to handshake: Hashes don't match" << std::endl;
        return false;
    }
    std::cout << "Successfully handshaked! " + m_peer.ToString() << std::endl;
    return true;
}
std::shared_ptr<boost::asio::ip::tcp::socket> Connection::GetSocket()
{
    return m_socket;
}
utils::Peer Connection::GetPeer()
{
    return m_peer;
}

void Connection::SendMessage(utils::MessageID messageID, std::vector<int32_t> payload)
{
    boost::asio::spawn(m_io, [this, messageID, payload](boost::asio::yield_context yield)
                       {
        try 
        {
            int32_t messageLength = (messageID != utils::MessageID::keepAlive) + 4 * payload.size();

            std::vector<unsigned char> request(4);
            request[0] = (messageLength >> 24) & 0xFF;
            request[1] = (messageLength >> 16) & 0xFF;
            request[2] = (messageLength >> 8) & 0xFF;
            request[3] = messageLength & 0xFF;

            if(messageLength > 0)
                request.push_back(messageID);

            for(int32_t num : payload) {
                request.push_back((num >> 24) & 0xFF);
                request.push_back((num >> 16) & 0xFF);
                request.push_back((num >> 8) & 0xFF);
                request.push_back(num & 0xFF);
            }
            boost::system::error_code ec;
             boost::asio::async_write(*m_socket, boost::asio::buffer(request), yield[ec]);
            if(ec) {
                m_mediator->MessageHandler(request, getSharedPtr(), ec);
                return;
            }

        }
        catch(const boost::system::system_error &e) {

        } });
}

void Connection::RecieveMessage()
{
    boost::asio::spawn(m_io, [this](boost::asio::yield_context yield)
                       {
        try
        {
            std::vector<unsigned char> messageLengthBuffer(4);
            boost::system::error_code ec;
            size_t recievedBytes = boost::asio::async_read(*m_socket, boost::asio::buffer(messageLengthBuffer), yield[ec]);
            if (ec)
            {
                m_mediator->MessageHandler(messageLengthBuffer, getSharedPtr(), ec);
                return;
            }
            int32_t messageLength = recievedBytes ? (static_cast<int32_t>(messageLengthBuffer[0]) << 24) |
                                                    (static_cast<int32_t>(messageLengthBuffer[1]) << 16) |
                                                    (static_cast<int32_t>(messageLengthBuffer[2]) << 8) |
                                                    static_cast<int32_t>(messageLengthBuffer[3])
                                                    : 0;

            std::vector<unsigned char> messageBuffer(messageLength);
            boost::asio::async_read(*m_socket, boost::asio::buffer(messageBuffer), yield[ec]);
            m_mediator->MessageHandler(messageBuffer, getSharedPtr(), ec);
                               
        }
        catch (const boost::system::system_error &e)
        {
            std::cout << "Error in messageListener: " << e.what() << " for peer " + m_peer.ToString() << std::endl;
        }
        RecieveMessage(); });
}