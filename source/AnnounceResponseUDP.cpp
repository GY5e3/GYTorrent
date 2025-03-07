#include "../include/AnnounceResponseUDP.hpp"

AnnounceResponseUDP::AnnounceResponseUDP() = default;

AnnounceResponseUDP::AnnounceResponseUDP(const std::vector<unsigned char> &responseBody)
{
    for (int i = 0; i < 4; i++)
        m_interval |= static_cast<uint32_t>(responseBody[i]) << (8 * (3 - i));

    for (int i = 0; i < 4; i++)
        m_incomplete |= static_cast<uint32_t>(responseBody[i + 4]) << (8 * (3 - i));

    for (int i = 0; i < 4; i++)
        m_complete |= static_cast<uint32_t>(responseBody[i + 8]) << (8 * (3 - i));

    for (int i = 12; i < responseBody.size(); i = i + 6)
    {
        std::ostringstream ip;
        ip << static_cast<uint32_t>(responseBody[i]) << "."
           << static_cast<uint32_t>(responseBody[i + 1]) << "."
           << static_cast<uint32_t>(responseBody[i + 2]) << "."
           << static_cast<uint32_t>(responseBody[i + 3]);

        uint16_t port = (static_cast<uint16_t>(responseBody[i + 4]) << 8) |
                        static_cast<uint16_t>(responseBody[i + 5]);
        m_peers.push_back(utils::Peer{ip.str(), std::to_string(port)});
    }
}