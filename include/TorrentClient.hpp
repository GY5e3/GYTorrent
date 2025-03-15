#pragma once

#include <utility>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>

#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>

#include "TorrentMetaData.hpp"
#include "TrackerHTTP.hpp"
#include "TrackerUDP.hpp"

#include "PieceManager.hpp"
#include "TrackerAnnouncer.hpp"
#include "ConnectionManager.hpp"
#include "SessionManager.hpp"

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

    std::queue<utils::Message> m_messageQueue;

    inline static std::shared_ptr<SessionManager> m_sessionManager;

    inline static std::vector<utils::Peer> m_peersList;

    static void trackerCallback(const std::string &trackerURL,
                                const std::vector<utils::Peer> &,
                                boost::system::error_code);
    
    static void peerCallback(const std::string& peer, const utils::Message& message, bool isIncoming, boost::system::error_code ec);

    /// @brief Generate PeerID for client
    /// @return PeerID in the format: -GYTORR-dddddddddddd, where 'd' - digit in the range from 0 to 9
    std::string generatePeerID() const;
};
