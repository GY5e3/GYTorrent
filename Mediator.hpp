#pragma once
#define BOOST_ALLOW_DEPRECATED_HEADERS

#include <utility>
#include <memory>
#include <unordered_map>
#include <queue>
#include <enable_shared_from_this.hpp>
#include <openssl/sha.h>

#include <boost/asio.hpp>

#include "PieceManager.hpp"
#include "TorrentMetaData.hpp"
#include "Connection.hpp"
#include "utils.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>

class PieceManager;
class Connection;

struct ConnectionParams {
public:
    int32_t activeRequests = 0;
    bool isBusy = false;
    bool isChoke = true; 
};

class Mediator
{
public:
    Mediator(TorrentMetaData torrentMetaData, std::shared_ptr<PieceManager> pieceManager);

    void MessageHandler(std::vector<unsigned char> messageBuffer,
              std::shared_ptr<Connection> connection,
              boost::system::error_code errorCode);

    bool Send(utils::MessageID messageID, Piece piece, std::shared_ptr<PieceManager> pieceManager);

    void AddActiveConnection(std::shared_ptr<Connection> connection);

    void RemoveConnection(std::shared_ptr<Connection> connection);

private:
    TorrentMetaData m_torrentMetaData;

    std::shared_ptr<PieceManager> m_pieceManager;

    std::unordered_set<std::shared_ptr<Connection>> m_activeSockets;

    std::unordered_map<std::shared_ptr<Connection>, bool> m_isChoked;

    std::unordered_map<std::shared_ptr<Connection>, int> m_activeRequests;

    std::unordered_map<std::shared_ptr<Connection>, bool> m_isBusy;

    std::unordered_map<std::shared_ptr<Connection>, std::vector<bool>> m_hasPiece;

    /// @brief Mapping pieceID - piece data and remaining blocks
    std::unordered_map<int32_t, std::pair<std::vector<unsigned char>, int32_t>> m_downloadingPieces;

    std::mutex mtx;
    std::condition_variable cv;
    int max_concurrent_requests = 5;
    std::atomic<int> active_requests = 0;
};