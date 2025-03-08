#pragma once

#include <iostream>
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "TorrentMetaData.hpp"

#include "../utils/Entities/Peer.hpp"
#include "../utils/BitTorrentConstants.hpp"

#include "../utils/Functors/build_request.hpp"

class ConnectionManager
{
public:
    ConnectionManager(boost::asio::io_context &io, const TorrentMetaData &torrentInfo, const std::string &clientPeerID);
    void Init(boost::asio::ip::tcp::socket &socket,
              const utils::Peer &peer,
              boost::asio::yield_context yield,
              boost::system::error_code &ec);

    void Listen(boost::asio::ip::tcp::socket &socket,
                boost::asio::yield_context yield,
                boost::system::error_code &ec,
                int32_t port = 6881);

private:
    utils::build_request build_request;

    boost::asio::io_context &m_io;
    boost::asio::ip::tcp::acceptor m_acceptor;
    TorrentMetaData m_torrentInfo;
    std::string m_clientPeerID;
};
