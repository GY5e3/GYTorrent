#pragma once

#include <vector>
#include <string>

#include <libtorrent/torrent_info.hpp>

#include "File.hpp"
#include "utils.hpp"

class TorrentMetaData
{
public:
    TorrentMetaData(std::string dotTorrentFilePath);
    TorrentMetaData(libtorrent::torrent_info torrentInfo);

    std::vector<std::string> GetTrackers();
    std::vector<File> GetFiles();
    std::string GetTorrentHash();
    std::string GetMyPeerID();
    int GetPieceCount();
    int64_t GetTotalSize();
    int32_t GetPieceSize();

    std::string GetPieceHash(int32_t index);
private:
    std::vector<std::string> m_trackers;
    std::vector<File> m_files;
    std::string m_torrentHash;
    std::vector<std::string> m_pieceHash;
    int m_piecesCount;
    int64_t m_totalSize;
    int32_t m_pieceSize;

    /// TODO: move this field to another place
    std::string m_myPeerID;

    void initialization(libtorrent::torrent_info torrentInfo);
};

