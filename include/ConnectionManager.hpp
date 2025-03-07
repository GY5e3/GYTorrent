#pragma once

#include <iostream>
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "TorrentMetaData.hpp"

#include "../utils/Entities/Peer.hpp"
#include "../utils/BitTorrentConstants.hpp"

class ConnectionManager
{
public:
    ConnectionManager(boost::asio::io_context &io, const TorrentMetaData& torrentInfo, const std::string& clientPeerID);
    void Init(boost::asio::ip::tcp::socket &socket,
              const utils::Peer &peer,
              boost::asio::yield_context yield,
              boost::system::error_code &ec);
    boost::asio::ip::tcp::socket Listen();

private:
    boost::asio::io_context &m_io;
    TorrentMetaData m_torrentInfo;
    std::string m_clientPeerID;
};
