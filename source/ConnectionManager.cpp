#include "ConnectionManager.hpp"

ConnectionManager::ConnectionManager(boost::asio::io_context &io,
                                     const TorrentMetaData &torrentInfo,
                                     const std::string &clientPeerID) : m_io(io),
                                                                        m_torrentInfo(torrentInfo),
                                                                        m_clientPeerID(clientPeerID) {}

void ConnectionManager::Init(boost::asio::ip::tcp::socket &socket,
                             const utils::Peer &peer,
                             boost::asio::yield_context yield,
                             boost::system::error_code &ec)
{
    boost::asio::ip::tcp::resolver resolver(m_io);
    auto endpoints = resolver.async_resolve(peer.GetIP(), peer.GetPort(), yield[ec]);

    boost::asio::async_connect(socket, endpoints, yield[ec]);
    if (ec)
        return;

    std::vector<unsigned char> request(utils::HANDSHAKE_LENGTH, '\0');

    request[0] = utils::PROTOCOL_ID;

    std::string requstPrtcl = "BitTorrent protocol";

    for (size_t i = 0; i < requstPrtcl.size(); i++)
        request[i + 1] = requstPrtcl[i];

    for (size_t i = 0; i < utils::TORRENT_HASH_LENGTH; i++)
        request[i + 1 + utils::PROTOCOL_ID + 8] = m_torrentInfo.GetInfoHash()[i];
        
    for (size_t i = 0; i < m_clientPeerID.size(); i++)
        request[i + 1 + utils::PROTOCOL_ID + 8 + utils::TORRENT_HASH_LENGTH] = m_clientPeerID[i];

    size_t sendBytes = boost::asio::async_write(socket, boost::asio::buffer(request), yield[ec]);
    if (ec) return;

    std::string response(utils::HANDSHAKE_LENGTH, '\0');
    size_t responseBytes = boost::asio::async_read(socket, boost::asio::buffer(response), yield[ec]);
    if(ec) return;
    if (sendBytes != responseBytes)
    {
        ec.assign(boost::system::errc::bad_message, boost::system::system_category());
        return;
    }

    int8_t responseProtocolID = static_cast<int8_t>(response[0]);

    std::string responsePrtcl(responseProtocolID, ' ');
    for (int i = 0; i < responseProtocolID; i++)
        responsePrtcl[i] = response[i + 1];

    std::vector<unsigned char> responseHash(utils::TORRENT_HASH_LENGTH);
    for (int i = 0; i < utils::TORRENT_HASH_LENGTH; i++)
        responseHash[i] = response[i + 1 + utils::PROTOCOL_ID + 8];

    std::string responsePeerID(20, ' ');
    for (size_t i = 0; i < responsePeerID.size(); i++)
        responsePeerID[i] = response[i + 1 + utils::PROTOCOL_ID + 8 + utils::TORRENT_HASH_LENGTH];

    if(requstPrtcl != responsePrtcl || m_torrentInfo.GetInfoHash() != responseHash) 
    {
        ec.assign(boost::system::errc::bad_message, boost::system::system_category());
        return;
    }
}