#pragma once

#include <utility>
#include <iostream>
#include <unordered_map>
#include <span>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>

#include "Abstraction/Tracker.hpp"

#include "AnnounceResponseUDP.hpp"

class TrackerUDP : public Tracker
{
public:
    TrackerUDP(boost::asio::io_context &io, const std::string &trackerURL);

    void Connect(boost::asio::yield_context yield, boost::system::error_code &ec);

    AnnounceResponseUDP Get(boost::asio::yield_context yield,
             std::unordered_map<std::string, std::string> &data,
             boost::system::error_code &ec);

private:
    boost::asio::ip::udp::socket m_socket;
    boost::asio::ip::udp::endpoint m_endpoint;

    uint64_t m_connectionID;

    std::unordered_map<std::string, int32_t> eventMapping{
        {"none", 0},
        {"completed", 1},
        {"started", 2},
        {"stopped", 3}};

    template <typename T>
    void buildRequest(std::vector<unsigned char> &request, size_t offset, T value)
    {
        if constexpr (std::is_integral_v<T>)
        {
            if (offset + sizeof(T) > request.size())
                throw std::out_of_range("Bad offset");

            for (size_t i = 0; i < sizeof(T); i++)
                request[offset + i] = (value >> (8 * (sizeof(T) - 1 - i))) & 0xFF;
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            if (offset + value.size() > request.size())
                throw std::out_of_range("Bad offset");

            for (size_t i = 0; i < value.size(); i++)
                request[offset + i] = static_cast<unsigned char>(value[i]);
        }
        else
            throw std::invalid_argument("Wrong argument");
    }
    template <typename T>
    void setRandomIntValue(T &number)
    {
        static_assert(std::is_integral<T>::value, "T must be an integer type");

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        number = 0;
        for (int i = 0; i < sizeof(T); i++)
            number |= (dis(gen) << (8 * i));
    }
};
