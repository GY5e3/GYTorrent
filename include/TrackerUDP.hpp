#pragma once

#include <utility>
#include <iostream>
#include <unordered_map>
#include <span>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>

#include "Abstraction/Tracker.hpp"
#include "Abstraction/AnnounceResponse.hpp"
#include "AnnounceResponseUDP.hpp"

#include "Functors/build_request.hpp"

class TrackerUDP : public Tracker
{
public:
    TrackerUDP(boost::asio::io_context &io);

    void Connect(boost::asio::yield_context yield, const std::string &trackerURL, boost::system::error_code &ec) override;

    AnnounceResponse Get(boost::asio::yield_context yield,
                         std::unordered_map<std::string, std::string> &data,
                         boost::system::error_code &ec) override;

private:
    utils::build_request build_request;

    boost::asio::ip::udp::socket m_socket;
    boost::asio::ip::udp::endpoint m_endpoint;

    uint64_t m_connectionID;

    std::unordered_map<std::string, int32_t> eventMapping{
        {"none", 0},
        {"completed", 1},
        {"started", 2},
        {"stopped", 3}};

    template <typename integer, typename = std::enable_if_t<std::is_integral_v<integer>>>
    integer getRandInt()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        integer number = 0;
        for (int i = 0; i < sizeof(integer); i++)
            number |= (dis(gen) << (8 * i));

        return number;
    }
};
