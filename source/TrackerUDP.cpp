#include "TrackerUDP.hpp"

TrackerUDP::TrackerUDP(boost::asio::io_context &io, const std::string &trackerURL) : AbstractTracker(io, trackerURL),
                                                                                     m_socket(io, boost::asio::ip::udp::v4()) {}

void TrackerUDP::Connect(boost::asio::yield_context yield, boost::system::error_code &ec)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    boost::asio::ip::udp::resolver resolver(m_io);
    auto endpoints = resolver.async_resolve(m_host, m_port, yield[ec]);
    if (ec)
    {
        std::cerr << "resolve error: " << ec.message() << std::endl;
        return;
    }
    m_endpoint = *endpoints.begin();

    std::vector<unsigned char> request(16);

    uint64_t protocolID = 0x41727101980;
    buildRequest(request, 0, protocolID);

    uint32_t incomingAction = 0x0000;
    buildRequest(request, 8, incomingAction);

    int32_t incomingTransactionID = 0;
    setRandomIntValue(incomingTransactionID);
    buildRequest(request, 12, incomingTransactionID);

    for (unsigned char byte : request)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    std::cout << std::endl;

    m_socket.async_send_to(boost::asio::buffer(request), m_endpoint, yield[ec]);
    if (ec)
    {
        std::cerr << "send error: " << ec.message() << std::endl;
        return;
    }

    std::vector<unsigned char> response(16, '0');
    m_socket.async_receive_from(boost::asio::buffer(response), m_endpoint, yield[ec]);
    if (ec)
    {
        std::cerr << "receive error: " << ec.message() << std::endl;
        return;
    }
    uint32_t outgoingAction = 0;
    for (int i = 0; i < 4; i++)
        outgoingAction |= static_cast<uint32_t>(response[i]) << (8 * (3 - i));

    uint32_t outgoingTransactionID = 0;
    for (int i = 0; i < 4; i++)
        outgoingTransactionID |= static_cast<uint32_t>(response[i + 4]) << (8 * (3 - i));

    m_connectionID = 0;
    for (int i = 0; i < 8; i++)
        m_connectionID |= static_cast<uint64_t>(response[i + 8]) << (8 * (7 - i));

    std::cout << "Offset  Size            Name            Value" << std::endl;
    std::cout << "0       32-bit integer  action          " << outgoingAction << " // connect" << std::endl;
    std::cout << "4       32-bit integer  transaction_id  " << outgoingTransactionID << std::endl;
    std::cout << "8       64-bit integer  connection_id   " << m_connectionID << std::endl;
}

void TrackerUDP::Get(boost::asio::yield_context yield,
                     std::unordered_map<std::string, std::string> &data,
                     boost::system::error_code &ec)

{
    std::vector<unsigned char> request(98);

    buildRequest(request, 0, m_connectionID);

    uint32_t incomingAction = 1;
    buildRequest(request, 8, incomingAction);

    int32_t incomingTransactionID = 0;
    setRandomIntValue(incomingTransactionID);
    buildRequest(request, 12, incomingTransactionID);

    buildRequest(request, 16, data["info_hash"]);

    buildRequest(request, 36, data["peer_id"]);

    buildRequest(request, 56, std::stoull(data["downloaded"]));

    buildRequest(request, 64, std::stoull(data["left"]));

    buildRequest(request, 72, std::stoull(data["uploaded"]));

    buildRequest(request, 80, eventMapping[data["event"]]);
    
    int32_t ip_address = data.find("IP address") != end(data) ? std::stoi(data["IP address"]) : 0;
    buildRequest(request, 84, ip_address);

    int32_t key = 0;
    if (data.find("key") != end(data))
        key = std::stoi(data["key"]);
    else        
        setRandomIntValue(key);
    buildRequest(request, 88, key);

    int32_t num_want = data.find("num_want") != end(data) ? std::stoi(data["num_want"]) : -1;
    buildRequest(request, 92, num_want);

    buildRequest(request, 96, static_cast<int16_t>(std::stoi(data["port"])));

    for (unsigned char byte : request)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    std::cout << std::endl;
    std::cout << std::endl;

    m_socket.async_send_to(boost::asio::buffer(request), m_endpoint, yield[ec]);
    if (ec)
    {
        std::cerr << "send error: " << ec.message() << std::endl;
        return;
    }
    
    std::vector<unsigned char> response(1024, '\0');
    m_socket.async_receive_from(boost::asio::buffer(response), m_endpoint, yield[ec]);
    if (ec)
    {
        std::cerr << "receive error: " << ec.message() << std::endl;
        return;
    }

    for (unsigned char byte : response)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    std::cout << std::endl;
}
//e4 5c 37 6e b1 9b 69 2f 00 00 00 01 44 9c 4b 57 8b b5 f3 93 a1 a9 34 8e f2 2c 2e 06 cc 3d 67 15 66 4d ba 59 2d 50 43 30 30 30 31 2d 38 34 36 38 39 31 39 33 39 38 32 38 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 02 00 00 00 00 6f cb 80 19 ff ff ff ff 1a e1
//e4 5c 37 6e b1 9b 69 2f 00 00 00 01 2b b1 ae 74 8b b5 f3 93 a1 a9 34 8e f2 2c 2e 06 cc 3d 67 15 66 4d ba 59 2d 50 43 30 30 30 31 2d 33 31 32 34 32 31 30 35 33 33 37 38 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 02 00 00 00 00 99 e6 64 0d ff ff ff ff 1a e1
//e4 5c 37 6e b1 9b 69 2f 00 00 00 01 11 51 2e f1 8b b5 f3 93 a1 a9 34 8e f2 2c 2e 06 cc 3d 67 15 66 4d ba 59 2d 50 43 30 30 30 31 2d 37 31 30 32 39 31 39 34 38 39 38 36 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 02 00 00 00 00 c8 58 df 03 ff ff ff ff 1a e1