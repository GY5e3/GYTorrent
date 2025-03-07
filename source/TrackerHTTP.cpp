#include "TrackerHTTP.hpp"

TrackerHTTP::TrackerHTTP(boost::asio::io_context &io, const std::string &trackerURL) : Tracker(io, trackerURL),
                                                                                       m_stream(io)
{
}

void TrackerHTTP::Connect(boost::asio::yield_context yield, boost::system::error_code &ec)
{
    boost::asio::ip::tcp::resolver resolver(m_io);
    const auto endpoints = resolver.async_resolve(m_host, m_port, yield[ec]);

    boost::asio::async_connect(m_stream.socket(), endpoints, yield[ec]);
    if (ec)
    {
        std::cerr << "Connection error: " << ec.message() << std::endl;
        return;
    }
    std::cout << "Connected to tracker at " << m_host << ":" << m_port << std::endl;
}

AnnounceResponseHTTP TrackerHTTP::Get(boost::asio::yield_context yield,
                      std::unordered_map<std::string, std::string> &data,
                      boost::system::error_code &ec)
{
    std::string requestBody = "/announce?info_hash=" + encodeURL(data["info_hash"]) +
                              "&peer_id=" + encodeURL(data["peer_id"]) +
                              "&port=" + data["port"] +
                              "&uploaded=" + data["uploaded"] +
                              "&downloaded=" + data["downloaded"] +
                              "&left=" + data["left"] +
                              "&compact=" + data["compact"] +
                              "&event=" + data["event"];
    boost::beast::http::request<boost::beast::http::string_body> request{boost::beast::http::verb::get, requestBody, 11};
    request.set(boost::beast::http::field::host, m_host);
    request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    boost::beast::http::async_write(m_stream, request, yield[ec]);
    if (ec)
    {
        std::cerr << "Error sending request: " << ec.message() << std::endl;
        return {};
    }

    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::string_body> res;
    boost::beast::http::async_read(m_stream, buffer, res, yield[ec]);
    if (ec)
    {
        std::cerr << "Error reading response: " << ec.message() << std::endl;
        return {};
    }
    
    return AnnounceResponseHTTP{res.body()};
}