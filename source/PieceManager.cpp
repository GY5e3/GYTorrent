#include "PieceManager.hpp"

PieceManager::PieceManager(bool isSequential,
                           const TorrentMetaData &torrentMetaData,
                           const std::string &absolutePath) : m_isSequential(isSequential),
                                                              m_torrentMetaData(torrentMetaData)
{
    std::filesystem::current_path(absolutePath);

    if (!m_torrentMetaData.GetName().empty())
    {
        std::string currentPath = absolutePath + "/" + m_torrentMetaData.GetName();
        std::filesystem::current_path(currentPath);

        std::filesystem::create_directory(m_torrentMetaData.GetName());
    }
}

void PieceManager::LoadNextPiece()
{
    if (m_isSequential)
    {
    }
}

void PieceManager::SavePieceOnDisk(const std::vector<unsigned char>& piece)
{
}