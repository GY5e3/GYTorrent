#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <openssl/sha.h>

#include <TorrentMetaData.hpp>

#include "Entities/Message.hpp"
#include "BitTorrentConstants.hpp"

class PieceManager
{
public:
    /// @brief Auxiliary structure for data of a specific piece
    struct PieceData
    {
        /// @brief Numbers of blocks that must to be downloaded
        int32_t RemainingBlocksCount;

        /// @brief Maps block offsets (relative to the piece start) to their binary data
        std::unordered_map<int32_t, std::vector<unsigned char>> Data;

        /// @brief Set of ip-port concatenations of peers that have sent at least 1 block of a specific piece
        std::unordered_set<std::string> Senders;

        PieceData() = default;
        ~PieceData() = default;
        PieceData(int32_t remainingBlocksCount) : RemainingBlocksCount(remainingBlocksCount) {}
    };

    PieceManager() = delete;

    PieceManager(const TorrentMetaData &torrentMetaData, const std::string &absolutePath = "");

    ~PieceManager();

    std::vector<utils::Message> LoadNextPiece(bool isSequential = false);

    void AcceptBlock(const std::string& peer, utils::Message& message);

    void SavePieceOnDisk(int32_t index, const std::vector<unsigned char>& piece);

    bool IsIncomplete() const;

    const std::unordered_map<int32_t, PieceData>& GetOnDownloading() const;
private:
    TorrentMetaData m_torrentMetaData;

    std::vector<int32_t> m_pieceRarity;

    /// @brief Set of indexes of downloaded pieces
    std::unordered_set<int32_t> m_downloadedPieces;

    
    /// @brief Stores indexes of pieces that are being loaded right now
    std::unordered_map<int32_t, PieceData> m_onDownloadingPieces; 

    /// @brief Stores indexes of missing pieces
    std::map<int32_t, int32_t> m_missingPieces;

    std::queue<std::pair<int32_t, std::vector<unsigned char>>> m_writeQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::thread m_writerThread;
    bool m_stop;

    void WriteThread();

};
