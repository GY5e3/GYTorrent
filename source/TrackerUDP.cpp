#include "TrackerUDP.hpp"

TrackerUDP::TrackerUDP(boost::asio::io_context &io, const std::string &trackerURL) : Tracker(io, trackerURL),
                                                                                     m_socket(io, boost::asio::ip::udp::v4()) {}

void TrackerUDP::Connect(boost::asio::yield_context yield, boost::system::error_code &ec)
{
    boost::asio::ip::udp::resolver resolver(m_io);
    auto endpoints = resolver.async_resolve(m_host, m_port, yield[ec]);
    if (ec) return;

    m_endpoint = *endpoints.begin();

    std::vector<unsigned char> request(16);

    uint64_t protocolID = 0x41727101980;
    build_request(request, 0, protocolID);

    uint32_t incomingAction = 0x0000;
    build_request(request, 8, incomingAction);

    int32_t incomingTransactionID = 0;
    setRandomIntValue(incomingTransactionID);
    build_request(request, 12, incomingTransactionID);

    m_socket.async_send_to(boost::asio::buffer(request), m_endpoint, yield[ec]);
    if (ec) return;

    std::vector<unsigned char> response(16, '0');
    m_socket.async_receive_from(boost::asio::buffer(response), m_endpoint, yield[ec]);
    if (ec) return;

    uint32_t outgoingAction = 0;
    for (int i = 0; i < 4; i++)
        outgoingAction |= static_cast<uint32_t>(response[i]) << (8 * (3 - i));

    uint32_t outgoingTransactionID = 0;
    for (int i = 0; i < 4; i++)
        outgoingTransactionID |= static_cast<uint32_t>(response[i + 4]) << (8 * (3 - i));

    m_connectionID = 0;
    for (int i = 0; i < 8; i++)
        m_connectionID |= static_cast<uint64_t>(response[i + 8]) << (8 * (7 - i));

    if(incomingAction != outgoingAction || incomingTransactionID != outgoingTransactionID) {
        ec.assign(boost::system::errc::bad_message, boost::system::system_category());
        return;
    }
}

AnnounceResponse TrackerUDP::Get(boost::asio::yield_context yield,
                     std::unordered_map<std::string, std::string> &data,
                     boost::system::error_code &ec)

{
    std::vector<unsigned char> request(98);

    build_request(request, 0, m_connectionID);

    uint32_t incomingAction = 1;
    build_request(request, 8, incomingAction);

    int32_t incomingTransactionID = 0;
    setRandomIntValue(incomingTransactionID);
    build_request(request, 12, incomingTransactionID);

    auto infoHash = data["info_hash"];
    build_request(request, 16, begin(infoHash), end(infoHash));

    auto peerID = data["peer_id"];
    build_request(request, 36, begin(peerID), end(peerID));

    build_request(request, 56, std::stoull(data["downloaded"]));

    build_request(request, 64, std::stoull(data["left"]));

    build_request(request, 72, std::stoull(data["uploaded"]));

    build_request(request, 80, eventMapping[data["event"]]);
    
    int32_t ip_address = data.find("IP address") != end(data) ? std::stoi(data["IP address"]) : 0;
    build_request(request, 84, ip_address);

    int32_t key = 0;
    if (data.find("key") != end(data))
        key = std::stoi(data["key"]);
    else        
        setRandomIntValue(key);
    build_request(request, 88, key);

    int32_t num_want = data.find("num_want") != end(data) ? std::stoi(data["num_want"]) : -1;
    build_request(request, 92, num_want);

    build_request(request, 96, static_cast<int16_t>(std::stoi(data["port"])));

    m_socket.async_send_to(boost::asio::buffer(request), m_endpoint, yield[ec]);
    if (ec) return {};
    
    std::vector<unsigned char> response(20 + 50 * 6, '\0');
    size_t responseSize = m_socket.async_receive_from(boost::asio::buffer(response), m_endpoint, yield[ec]);
    if (ec) return {};
    else if(responseSize < 20) {
        ec.assign(boost::system::errc::bad_message, boost::system::system_category());
        return {};
    }

    response.resize(responseSize);

    uint32_t outgoingAction = 0;
    for (int i = 0; i < 4; i++)
        outgoingAction |= static_cast<uint32_t>(response[i]) << (8 * (3 - i));

    uint32_t outgoingTransactionID = 0;
    for (int i = 0; i < 4; i++)
        outgoingTransactionID |= static_cast<uint32_t>(response[i + 4]) << (8 * (3 - i));

    if(incomingAction != outgoingAction || incomingTransactionID != outgoingTransactionID) {
        ec.assign(boost::system::errc::bad_message, boost::system::system_category());
        return {};
    }
    
    std::vector<unsigned char> responseBody(response.begin() + 8, response.end());
    return AnnounceResponseUDP{responseBody};
}