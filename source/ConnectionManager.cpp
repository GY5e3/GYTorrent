#include "ConnectionManager.hpp"

ConnectionManager::ConnectionManager(boost::asio::io_context &io,
                                     const TorrentMetaData &torrentInfo,
                                     const std::string &clientPeerID) : m_io(io),
                                                                        m_acceptor(io),
                                                                        m_torrentInfo(torrentInfo),
                                                                        m_clientPeerID(clientPeerID)
{
}

NetworkAction ConnectionManager::Init(const utils::Peer &peer,
                                    boost::asio::yield_context yield,
                                    boost::system::error_code &ec)
{
    boost::asio::ip::tcp::socket socket(m_io);

    boost::asio::ip::tcp::resolver resolver(m_io);
    auto endpoints = resolver.async_resolve(peer.GetIP(), peer.GetPort(), yield[ec]);

    boost::asio::async_connect(socket, endpoints, yield[ec]);
    if (ec) return {m_io};

    std::vector<unsigned char> request(utils::HANDSHAKE_LENGTH, '\0');

    request[0] = utils::PROTOCOL_ID;

    std::string requstProtocol = "BitTorrent protocol";
    build_request(request, 1, begin(requstProtocol), end(requstProtocol));

    auto infoHash = m_torrentInfo.GetInfoHash();
    build_request(request, 1 + utils::PROTOCOL_ID + 8, begin(infoHash), end(infoHash));

    build_request(request, 1 + utils::PROTOCOL_ID + 8 + utils::TORRENT_HASH_LENGTH,
                  begin(m_clientPeerID), end(m_clientPeerID));

    size_t sendBytes = boost::asio::async_write(socket, boost::asio::buffer(request), yield[ec]);
    if (ec) return {m_io};

    std::string response(utils::HANDSHAKE_LENGTH, '\0');

    size_t responseBytes = boost::asio::async_read(socket, boost::asio::buffer(response), yield[ec]);
    if (ec) return {m_io};

    if (sendBytes != responseBytes)
    {
        ec.assign(boost::system::errc::bad_message, boost::system::system_category());
        if (ec) return {m_io};
    }

    int8_t responseProtocolID = static_cast<int8_t>(response[0]);

    std::string responseProtocol(responseProtocolID, ' ');
    for (int i = 0; i < responseProtocolID; i++)
        responseProtocol[i] = response[i + 1];

    std::vector<unsigned char> responseHash(utils::TORRENT_HASH_LENGTH);
    for (int i = 0; i < utils::TORRENT_HASH_LENGTH; i++)
        responseHash[i] = response[i + 1 + utils::PROTOCOL_ID + 8];

    std::string responsePeerID(20, ' ');
    for (size_t i = 0; i < responsePeerID.size(); i++)
        responsePeerID[i] = response[i + 1 + utils::PROTOCOL_ID + 8 + utils::TORRENT_HASH_LENGTH];

    if (requstProtocol != responseProtocol || m_torrentInfo.GetInfoHash() != responseHash)
    {
        ec.assign(boost::system::errc::bad_message, boost::system::system_category());
        if (ec) return {m_io};
    }

    return NetworkAction{m_io, std::move(socket)};
}

NetworkAction ConnectionManager::Listen(boost::asio::yield_context yield,
                                      boost::system::error_code &ec,
                                      int16_t port)
{
    boost::asio::ip::tcp::socket socket(m_io);

    m_acceptor.open(boost::asio::ip::tcp::v4());
    m_acceptor.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
    m_acceptor.listen();

    m_acceptor.async_accept(socket, yield[ec]);

    if (ec) return {m_io};

    return NetworkAction{m_io, std::move(socket)};
}