#pragma once

#include <string>

namespace utils
{
    class Peer
    {
    public:
        Peer() = default;
        Peer(const std::string& peerIP, const std::string& peerPort) : m_peerIP(peerIP), m_peerPort(peerPort) {}

        std::string ToString() const { return m_peerIP + " " + m_peerPort; }

        std::string GetPeerIP() const { return m_peerIP; }
        std::string GetPeerPort() const { return m_peerPort; }

    private:
        std::string m_peerIP;
        std::string m_peerPort;
    };
}