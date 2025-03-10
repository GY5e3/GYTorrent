#pragma once

#include <iomanip>
#include <string>
#include <regex>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asio.hpp>

#include "AnnounceResponse.hpp"

class Tracker : public std::enable_shared_from_this<Tracker>
{
public:
    Tracker(boost::asio::io_context &io) : m_io(io) {}

    virtual void Connect(boost::asio::yield_context yield, const std::string &trackerURL, boost::system::error_code &ec) = 0;

    virtual AnnounceResponse Get(boost::asio::yield_context yield,
                                 std::unordered_map<std::string, std::string> &data,
                                 boost::system::error_code &ec) = 0;

protected:
    boost::asio::io_context &m_io;

    std::string checkRequiredParams(const std::unordered_map<std::string, std::string> &data) const
    {
        std::vector<std::string> requiredParams{
            "info_hash", "peer_id", "port", "uploaded", "downloaded", "left", "event"};
        for (std::string param : requiredParams)
        {
            if (data.find(param) == end(data))
                return param;
        }
        return "";
    }

    std::pair<std::string, std::string> getHostAndPortFromURL(const std::string &url) const
    {
        std::string host, port;
        
        std::regex url_regex(R"(^(\w+):\/\/([^\/:]+):?(\d+)?\/?.*$)");

        std::smatch url_match_result;

        if (std::regex_match(url, url_match_result, url_regex))
        {
            host = url_match_result[2].str();
            port = url_match_result[3].matched ? url_match_result[3].str() : "80";
        }
        return {host, port};
    }
};
