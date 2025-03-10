#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include <TorrentMetaData.hpp>

class PieceManager
{
public:
    PieceManager(bool isSequential, const TorrentMetaData &torrentMetaData, const std::string &absolutePath = "");

    void LoadNextPiece();

    void SavePieceOnDisk(const std::vector<unsigned char>& piece);

private:
    bool m_isSequential;

    TorrentMetaData m_torrentMetaData;
};
