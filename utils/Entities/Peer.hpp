#pragma once

#include <string>

namespace utils
{
    class Peer
    {
    public:
        Peer() = default;
        Peer(const std::string &peerIP, const std::string &peerPort, const std::string &peerID = "") : m_IP(peerIP),
                                                                                                       m_port(peerPort),
                                                                                                       m_ID(peerID) {}

        std::string ToString() const { return m_IP + ":" + m_port; }

        std::string GetID() const { return m_ID; }
        std::string GetIP() const { return m_IP; }
        std::string GetPort() const { return m_port; }

    private:
        std::string m_ID;
        std::string m_IP;
        std::string m_port;
    };
}