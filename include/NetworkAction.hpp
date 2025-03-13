#pragma once

#include <boost/core/scoped_enum.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include "BitTorrentConstants.hpp"
#include "Functors/build_request.hpp"

class NetworkAction
{
public:
    NetworkAction() = delete;

    NetworkAction(boost::asio::io_context &io);

    NetworkAction(boost::asio::io_context &io,
                boost::asio::ip::tcp::socket socket);

    void Send(boost::asio::yield_context yield,
              const std::vector<unsigned char> &message,
              boost::system::error_code &ec);

    std::vector<unsigned char> Recieve(boost::asio::yield_context yield,
                 boost::system::error_code &ec);

private:
    utils::build_request build_request;

    boost::asio::io_context &m_io;
    std::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
};


