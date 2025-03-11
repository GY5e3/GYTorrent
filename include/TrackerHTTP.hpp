#pragma once

#include <utility>
#include <iostream>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>

#include "Abstraction/Tracker.hpp"
#include "Abstraction/AnnounceResponse.hpp"
#include "AnnounceResponseHTTP.hpp"

class TrackerHTTP : public Tracker
{
public:
    TrackerHTTP(boost::asio::io_context &io, const std::string &trackerURL);

    AnnounceResponse Get(boost::asio::yield_context yield,
                         std::unordered_map<std::string, std::string> &data,
                         boost::system::error_code &ec) override;

private:
    boost::beast::tcp_stream m_stream;

    std::string encodeURL(const std::string &str);


};