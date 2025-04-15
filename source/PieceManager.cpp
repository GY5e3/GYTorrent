#include "PieceManager.hpp"

PieceManager::PieceManager(const TorrentMetaData &torrentMetaData,
                           const std::string &absolutePath) : m_torrentMetaData(torrentMetaData)
{
/*
    std::filesystem::current_path(absolutePath);

     if (!m_torrentMetaData.GetName().empty())
    {
        std::string currentPath = absolutePath + "/" + m_torrentMetaData.GetName();
        std::filesystem::current_path(currentPath);

        std::filesystem::create_directory(m_torrentMetaData.GetName());
    }

*/
    m_pieceRarity.resize(m_torrentMetaData.GetPieces().size(), 0);

    m_missingPieces.insert({0,0});
}

PieceManager::~PieceManager()
{
}

bool PieceManager::IsIncomplete() const
{
    return m_missingPieces.size() || m_onDownloadingPieces.size();
}

std::vector<utils::Message> PieceManager::LoadNextPiece(bool isSequential)
{
    if(m_missingPieces.empty())
        return {};

    int32_t nextPiece = -1;
    if (isSequential)
    {
        nextPiece = begin(m_missingPieces)->first;
    }
    else
    {
        // rarity
    }
    m_missingPieces.erase(nextPiece);
    m_onDownloadingPieces.insert({nextPiece, PieceData()});

    std::vector<utils::Message> messages;
    int64_t pieceSize = m_torrentMetaData.GetPieces()[nextPiece].GetLength();
    for (int32_t blockOffset = 0; blockOffset < pieceSize; blockOffset += utils::BLOCK_SIZE)
    {
        messages.push_back(utils::Message{utils::MessageID::request, nextPiece, blockOffset, utils::BLOCK_SIZE});

        m_onDownloadingPieces[nextPiece].Data.insert({blockOffset, std::vector<unsigned char>()});
    }
    m_onDownloadingPieces[nextPiece].RemainingBlocksCount = m_onDownloadingPieces[nextPiece].Data.size();

    return messages;
}

void PieceManager::AcceptBlock(const std::string &peer, const utils::Message &message)
{
    auto &pieceData = m_onDownloadingPieces[message.PieceIndex];

    pieceData.Senders.insert(peer);
    pieceData.RemainingBlocksCount -= pieceData.Data[message.Offset].empty();
    pieceData.Data[message.Offset] = std::move(message.Block);
}

bool PieceManager::IsDownloadedPiece(int32_t index)
{
    auto &pieceData = m_onDownloadingPieces[index];

    return pieceData.RemainingBlocksCount == 0; 
}

bool PieceManager::CheckPieceHash(int32_t index, std::vector<unsigned char>& resultPiece)
{
    auto &pieceData = m_onDownloadingPieces[index];

    resultPiece.resize(m_torrentMetaData.GetPieces()[index].GetLength());
    for (auto &[offset, data] : pieceData.Data)
    {
        std::copy(data.begin(), data.end(), resultPiece.begin() + offset);
    }
    std::vector<unsigned char> hash(SHA_DIGEST_LENGTH);
    SHA1(resultPiece.data(), resultPiece.size(), hash.data());
    std::string hashStr(begin(hash), end(hash));

    return hashStr == m_torrentMetaData.GetPieces()[index].GetHash();
}

void PieceManager::SavePieceOnDisk(int32_t index, const std::vector<unsigned char> &data, boost::system::error_code& ec)
{
    utils::Piece piece = m_torrentMetaData.GetPieces()[index];

    int64_t pieceOffset = piece.GetOffset();
    int32_t fileIndex = piece.GetFileIndex();

    int64_t remainingData = data.size();
    int64_t dataOffset = 0;

    while (remainingData > 0 && fileIndex < m_torrentMetaData.GetFiles().size())
    {
        const utils::File &targetFile = m_torrentMetaData.GetFiles()[fileIndex];

        int64_t fileOffset = pieceOffset - targetFile.GetOffset();
        int64_t fileAvailableSpace = targetFile.GetSize() - fileOffset;
        int64_t toWrite = std::min(remainingData, fileAvailableSpace);

        if(!targetFile.GetPath().empty())
            std::filesystem::create_directories(targetFile.GetPath());

        std::string fullPath = targetFile.GetPath() + targetFile.GetName();

        if (!std::filesystem::exists(fullPath)) {
            std::ofstream create(fullPath, std::ios::binary);
            create.close();
        }
        std::ofstream ofs(fullPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!ofs)
        {
            ec = make_error_code(torrent_errc::opening_file_error);
            return;
        }

        ofs.seekp(fileOffset);
        if (!ofs)
        {
            ec = make_error_code(torrent_errc::seeking_position_error);
            return;
        }

        ofs.write(reinterpret_cast<const char *>(data.data() + dataOffset), toWrite);
        if (!ofs)
        {
            ec = make_error_code(torrent_errc::writing_error);
            return;
        }

        ofs.close();

        pieceOffset += toWrite;
        dataOffset += toWrite;
        remainingData -= toWrite;
        fileIndex++;
    }

    m_onDownloadingPieces.erase(index);
    m_downloadedPieces.insert(index);
}

const std::unordered_map<int32_t, PieceManager::PieceData>& PieceManager::GetOnDownloading() const
{
    return m_onDownloadingPieces;
}