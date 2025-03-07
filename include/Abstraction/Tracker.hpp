#pragma once

#include <iomanip>
#include <string>
#include <regex>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asio.hpp>

class Tracker : public std::enable_shared_from_this<Tracker>
{
public:
    Tracker(boost::asio::io_context &io, const std::string &trackerURL) : m_io(io)
    {
        getHostAndPortFromURL(trackerURL);
    }

protected:
    std::string m_host;
    std::string m_port;

    boost::asio::io_context &m_io;

    void getHostAndPortFromURL(const std::string &url)
    {
        // Регулярное выражение для парсинга URL
        std::regex url_regex(R"(^(\w+):\/\/([^\/:]+):?(\d+)?\/?.*$)");
        std::smatch url_match_result;

        // Проверяем, соответствует ли URL регулярному выражению
        if (std::regex_match(url, url_match_result, url_regex))
        {
            // Получаем хост (второй подшаблон)
            m_host = url_match_result[2].str();

            // Получаем порт (третий подшаблон), если он есть
            if (url_match_result[3].matched)
            {
                m_port = url_match_result[3].str();
            }
            else
            {
                m_port = "80"; // DEFAULT_PORT;
            }
        }
    }
    std::string encodeURL(const std::string &str)
    {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : str)
        {
            if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                escaped << c;
            }
            else
            {
                escaped << '%' << std::setw(2) << int((unsigned char)c);
            }
        }
        return escaped.str();
    }
};
