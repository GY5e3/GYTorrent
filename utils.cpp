#include "utils.hpp"

namespace utils
{
    std::string GeneratorPeerID()
    {
        std::string peer_id = "-PC0001-";

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 9);

        for (int i = 0; i < 12; i++)
        {
            peer_id += std::to_string(dis(gen));
        }

        return peer_id;
    }

    std::string EncodeURL(const std::string &value)
    {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : value)
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

    std::pair<std::string, std::string> GetHostAndPortFromURL(std::string url)
    {
        std::string host;
        std::string port;

        // Регулярное выражение для парсинга URL
        std::regex url_regex(R"(^(\w+):\/\/([^\/:]+):?(\d+)?\/?.*$)");
        std::smatch url_match_result;

        // Проверяем, соответствует ли URL регулярному выражению
        if (std::regex_match(url, url_match_result, url_regex))
        {
            // Получаем хост (второй подшаблон)
            host = url_match_result[2].str();

            // Получаем порт (третий подшаблон), если он есть
            if (url_match_result[3].matched)
            {
                port = url_match_result[3].str();
            }
            else
            {
                port = "80"; // DEFAULT_PORT;
            }
        }
        return {host, port};
    }
}