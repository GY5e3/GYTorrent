#pragma once

#include <string>
#include <vector>

#include <TorrentMetaData.hpp>

class PieceManager
{
public:
    PieceManager(bool isSequential);

    void LoadNextPiece();

    void SavePieceOnDisk(std::vector<unsigned char> piece);

private:

    bool m_sequential;

    std::string m_absolutePath;

    TorrentMetaData m_torrentMetaData;
};

