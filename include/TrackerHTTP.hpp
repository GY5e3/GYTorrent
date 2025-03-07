#pragma once

#include <utility>
#include <iostream>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>

#include "AbstractTracker.hpp"

class TrackerHTTP : public AbstractTracker
{
public:
    TrackerHTTP(boost::asio::io_context &io, const std::string &trackerURL);

    void Connect(boost::asio::yield_context yield, boost::system::error_code &ec);

    void Get(boost::asio::yield_context yield,
             std::unordered_map<std::string, std::string> &data,
             boost::system::error_code &ec);
             
private:
    boost::beast::tcp_stream m_stream;
};