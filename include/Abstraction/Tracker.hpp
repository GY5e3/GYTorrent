#pragma once

#include <iomanip>
#include <string>
#include <regex>
#include <memory>
// #include <enable_shared_from_this.hpp>

#include <boost/asio.hpp>

#include "AnnounceResponse.hpp"

class Tracker : public std::enable_shared_from_this<Tracker>
{
public:
    Tracker(boost::asio::io_context &io, const std::string &trackerURL) : m_io(io), m_url(trackerURL) {}

    virtual AnnounceResponse Get(boost::asio::yield_context yield,
                                 std::unordered_map<std::string, std::string> &data,
                                 boost::system::error_code &ec) = 0;

protected:
    boost::asio::io_context &m_io;

    std::string m_url;

    std::string checkRequiredParams(const std::unordered_map<std::string, std::string> &data) const
    {
        std::vector<std::string> requiredParams{
            "info_hash", "peer_id", "port", "uploaded", "downloaded", "left"};
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
