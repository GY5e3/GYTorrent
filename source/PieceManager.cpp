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

std::vector<utils::Message> PieceManager::LoadNextPiece()
{
    int32_t nextPiece = -1;
    if (m_isSequential)
    {
        nextPiece = begin(m_missingPieces)->first;
    }
    else
    {
        //rarity
    }
    m_missingPieces.erase(nextPiece);
    m_onDownloadingPieces.insert(nextPiece, {});

    std::vector<utils::Message> messages;
    int64_t pieceSize = m_torrentMetaData.GetPieces()[nextPiece].GetLength();
    for(int32_t blockOffset = 0; blockOffset < pieceSize; blockOffset += utils::BLOCK_SIZE)
    {
        messages.push_back(utils::Message{utils::MessageID::request, nextPiece, blockOffset, utils::BLOCK_SIZE});

        m_onDownloadingPieces[nextPiece].Data.insert({blockOffset, {}});
    }
    m_onDownloadingPieces[nextPiece].RemainingBlocksCount = m_onDownloadingPieces[nextPiece].Data.size();

    return messages;
}

void PieceManager::AcceptBlock(const std::string& peer, utils::Message& message)
{
    auto& pieceData =  m_onDownloadingPieces[message.PieceIndex];

    pieceData.Senders.insert(peer);

    pieceData.RemainingBlocksCount -= pieceData.Data[message.Offset].empty();

    pieceData.Data[message.Offset] = std::move(message.Block);

    if(pieceData.RemainingBlocksCount)
        return;

    std::vector<unsigned char> resultPiece;
    resultPiece.reserve(m_torrentMetaData.GetPieces()[message.PieceIndex].GetLength());

    for(auto& block : pieceData.Data)
    {
        resultPiece.insert(resultPiece.begin() + block.first, 
                           std::make_move_iterator(block.second.begin()), 
                           std::make_move_iterator(block.second.end()));
    }
    std::vector<unsigned char> hash(SHA_DIGEST_LENGTH);

    SHA1(resultPiece.data(), resultPiece.size(), hash.data());

    std::string hashStr(begin(hash), end(hash));

    if(hashStr == m_torrentMetaData.GetPieces()[message.PieceIndex].GetHash())
    {
        SavePieceOnDisk(resultPiece);
    }
    else
    {
        m_missingPieces.insert({message.PieceIndex, 0});
        ///TODO: implement a request entire piece for each sender
    }
    m_onDownloadingPieces.erase(message.PieceIndex);
}

void PieceManager::SavePieceOnDisk(const std::vector<unsigned char>& piece)
{
}