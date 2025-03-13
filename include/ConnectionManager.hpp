#pragma once

#include <iostream>
#include <utility>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/spawn.hpp>

#include "TorrentMetaData.hpp"
#include "NetworkAction.hpp"

#include "Entities/Peer.hpp"
#include "Functors/build_request.hpp"
#include "BitTorrentConstants.hpp"

class ConnectionManager
{
public:
    ConnectionManager(boost::asio::io_context &io, const TorrentMetaData &torrentInfo, const std::string &clientPeerID);
    NetworkAction Init(const utils::Peer &peer,
                     boost::asio::yield_context yield,
                     boost::system::error_code &ec);

    NetworkAction Listen(boost::asio::yield_context yield,
                       boost::system::error_code &ec,
                       int16_t port = 6881);

private:
    utils::build_request build_request;

    boost::asio::io_context &m_io;
    boost::asio::ip::tcp::acceptor m_acceptor;
    TorrentMetaData m_torrentInfo;
    std::string m_clientPeerID;
};
