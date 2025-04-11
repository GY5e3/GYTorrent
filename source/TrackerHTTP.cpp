#include "TrackerHTTP.hpp"

TrackerHTTP::TrackerHTTP(boost::asio::io_context &io, const std::string &trackerURL) : Tracker(io, trackerURL), m_stream(io) {}

AnnounceResponse TrackerHTTP::Get(boost::asio::yield_context yield,
                                  std::unordered_map<std::string, std::string> &data,
                                  boost::system::error_code &ec)
{
    auto [host, port] = getHostAndPortFromURL(m_url);
    boost::asio::ip::tcp::resolver resolver(m_io);

    /// TODO: async_resolve doesnt guarante async DNS-resolution, probably i should create another thread for this?
    const auto endpoints = resolver.async_resolve(host, port, yield[ec]);
    if (ec) return {};

    boost::asio::async_connect(m_stream.socket(), endpoints, yield[ec]);
    if (ec) return {};

    if (std::string param = checkRequiredParams(data); !param.empty())
    {
        ec.assign(boost::system::errc::bad_message, boost::system::system_category());
        return {};
    }

    std::string requestBody = "/announce?";

    requestBody.append("info_hash=" + encodeURL(data["info_hash"]));
    requestBody.append("&peer_id=" + encodeURL(data["peer_id"]));
    requestBody.append("&port=" + data["port"]);
    requestBody.append("&uploaded=" + data["uploaded"]);
    requestBody.append("&downloaded=" + data["downloaded"]);
    requestBody.append("&left=" + data["left"]);
    requestBody.append("&compact=" + (data.find("compact") != end(data) ? data["compact"] : "1"));
    requestBody.append((data.find("no_peer_id") != end(data) ? "&no_peer_id=" + data["no_peer_id"] : ""));
    requestBody.append((data.find("event") != end(data) ? "&event=" + data["event"] : ""));
    requestBody.append((data.find("ip") != end(data) ? "&ip=" + data["ip"] : ""));
    requestBody.append((data.find("numwant") != end(data) ? "&numwant=" + data["numwant"] : ""));
    requestBody.append((data.find("key") != end(data) ? "&key=" + data["key"] : ""));
    requestBody.append((data.find("trackerid") != end(data) ? "&trackerid=" + data["trackerid"] : ""));

    boost::beast::http::request<boost::beast::http::string_body> request{boost::beast::http::verb::get, requestBody, 11};
    //request.set(boost::beast::http::field::host, m_stream.socket().remote_endpoint().address().to_string());
    request.set(boost::beast::http::field::host, host);
    request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    boost::beast::http::async_write(m_stream, request, yield[ec]);
    if (ec) return {};

    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::string_body> res;
    boost::beast::http::async_read(m_stream, buffer, res, yield[ec]);
    if (ec) return {};

    return AnnounceResponseHTTP{res.body()};
}

std::string TrackerHTTP::encodeURL(const std::string &str)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (unsigned char c : str)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            escaped << c;
        else
            escaped << '%' << std::setw(2) << int((unsigned char)c);
    }
    return escaped.str();
}