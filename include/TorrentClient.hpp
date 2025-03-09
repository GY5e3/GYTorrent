#pragma once

#include <utility>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "TorrentMetaData.hpp"
#include "Abstraction/Tracker.hpp"
#include "TrackerHTTP.hpp"
#include "TrackerUDP.hpp"

class TorrentClient
{

public:
    TorrentClient(std::string torrentFilePath, std::string downloadPath, uint16_t port = 6881);

    void Execute();
private:
    std::string m_torrentFilePath;
    std::string m_downloadPath;
    uint16_t m_port;

    std::string m_peerID;

    std::vector<bool> m_bitField;

    std::unordered_map<std::string, boost::asio::ip::tcp::socket> m_connections;
    /// @brief Generate PeerID for client
    /// @return PeerID in the format: -GYTORR-dddddddddddd, where 'd' - digit in the range from 0 to 9
    std::string generatePeerID();
};

