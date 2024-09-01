#pragma once

#include <string>
#include <random>
#include <iomanip>
#include <regex>

namespace utils
{
    const int BLOCK_SIZE = 16 * 1024;
    /// @brief BitTorrent protocol ID
    const int PROTOCOL_ID = 19;

    const int HANDSHAKE_LENGTH = 68;

    const int FREE_BYTES_LENGTH = 8;

    const int TORRENT_HASH_LENGTH = 20;

    const int PEER_ID_LENGTH = 20;

    enum MessageID
    {
        keepAlive = -1,
        choke = 0,
        unchoke = 1,
        interested = 2,
        notInterested = 3,
        have = 4,
        bitField = 5,
        request = 6,
        piece = 7,
        cancel = 8,
        port = 9
    };

    struct Peer
    {
    public:
        Peer()
        {
            m_peerIP = "";
            m_peerPort = "";
        }
        Peer(std::string peerIP, std::string peerPort) : m_peerIP(peerIP), m_peerPort(peerPort) {}

        std::string ToString() { return m_peerIP + " " + m_peerPort; }

        std::string GetPeerIP() { return m_peerIP; }
        std::string GetPeerPort() { return m_peerPort; }

    private:
        std::string m_peerIP;
        std::string m_peerPort;
    };
    /*
        @brief Generate PeerID for client
        @return PeerID in the format: -PC0001-dddddddddddd, where 'd' - digit in the range from 0 to 9
    */
    std::string GeneratorPeerID();
    
    std::string EncodeURL(const std::string &value);

    std::pair<std::string, std::string> GetHostAndPortFromURL(std::string url);
}