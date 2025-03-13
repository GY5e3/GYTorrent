#include "NetworkAction.hpp"

NetworkAction::NetworkAction(boost::asio::io_context &io) : m_io(io) {}

NetworkAction::NetworkAction(boost::asio::io_context &io,
                                           boost::asio::ip::tcp::socket socket) : m_io(io)
{
    m_socket = std::make_unique<boost::asio::ip::tcp::socket>(std::move(socket));
}

void NetworkAction::Send(boost::asio::yield_context yield,
                          const std::vector<unsigned char> &message,
                          boost::system::error_code &ec)
{
    boost::asio::async_write(*m_socket, boost::asio::buffer(message), yield[ec]);
}

std::vector<unsigned char> NetworkAction::Recieve(boost::asio::yield_context yield,
                                                   boost::system::error_code &ec)
{
    std::vector<unsigned char> messageLengthBuffer(4);
    size_t recievedBytes = boost::asio::async_read(*m_socket, boost::asio::buffer(messageLengthBuffer), yield[ec]);
    if (ec) return {};

    int32_t messageLength = recievedBytes ? (static_cast<int32_t>(messageLengthBuffer[0]) << 24) |
                                            (static_cast<int32_t>(messageLengthBuffer[1]) << 16) |
                                            (static_cast<int32_t>(messageLengthBuffer[2]) << 8) |
                                             static_cast<int32_t>(messageLengthBuffer[3]): 0;

    std::vector<unsigned char> messageBuffer(messageLength);
    boost::asio::async_read(*m_socket, boost::asio::buffer(messageBuffer), yield[ec]);

    return messageBuffer;
}