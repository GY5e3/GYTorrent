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
    m_writerThread = std::thread(&PieceManager::WriteThread, this);

    m_pieceRarity.resize(m_torrentMetaData.GetPieces().size(), 0);

   // m_missingPieces.insert({0,0});
}

PieceManager::~PieceManager()
{
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }
    m_condition.notify_all();

    if (m_writerThread.joinable())
        m_writerThread.join();
}

bool PieceManager::IsIncomplete() const
{
    return m_missingPieces.size() || m_onDownloadingPieces.size();
}

std::vector<utils::Message> PieceManager::LoadNextPiece(bool isSequential)
{
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

void PieceManager::AcceptBlock(const std::string &peer, utils::Message &message)
{
    auto &pieceData = m_onDownloadingPieces[message.PieceIndex];

    pieceData.Senders.insert(peer);
    pieceData.RemainingBlocksCount -= pieceData.Data[message.Offset].empty();
    pieceData.Data[message.Offset] = std::move(message.Block);

    if (pieceData.RemainingBlocksCount)
        return;

    std::vector<unsigned char> resultPiece;
    resultPiece.reserve(m_torrentMetaData.GetPieces()[message.PieceIndex].GetLength());
    for (auto &block : pieceData.Data)
    {
        resultPiece.insert(resultPiece.begin() + block.first,
                           std::make_move_iterator(block.second.begin()),
                           std::make_move_iterator(block.second.end()));
    }
    std::vector<unsigned char> hash(SHA_DIGEST_LENGTH);
    SHA1(resultPiece.data(), resultPiece.size(), hash.data());
    std::string hashStr(begin(hash), end(hash));
    if (hashStr != m_torrentMetaData.GetPieces()[message.PieceIndex].GetHash())
    {
        m_missingPieces.insert({message.PieceIndex, m_pieceRarity[message.PieceIndex]});
        m_onDownloadingPieces.erase(message.PieceIndex);

        /// TODO: implement a request entire piece for each sender

        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_writeQueue.emplace(message.PieceIndex, std::move(resultPiece));
    }
    m_condition.notify_one();
}

void PieceManager::SavePieceOnDisk(int32_t index, const std::vector<unsigned char> &data)
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

        std::filesystem::create_directories(targetFile.GetPath());

        std::ofstream ofs(targetFile.GetPath() + targetFile.GetName(), std::ios::binary | std::ios::in | std::ios::out);
        if (!ofs)
        {
            /// TODO:
            // auto ec = make_error_code(utils::torrent_errc::custom_error);
            std::cerr << "Smth went wrong..." << std::endl;
            return;
        }

        ofs.seekp(fileOffset);
        if (!ofs)
        {
            /// TODO:
            // auto ec = make_error_code(utils::torrent_errc::custom_error);
            std::cerr << "Error: Could not seek to position " << fileOffset << " in file " << targetFile.GetName() << std::endl;
            return;
        }

        ofs.write(reinterpret_cast<const char *>(data.data() + dataOffset), toWrite);
        if (!ofs)
        {
            /// TODO:
            // auto ec = make_error_code(utils::torrent_errc::custom_error);
            std::cerr << "Error: Failed to write data to file " << targetFile.GetName() << std::endl;
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

void PieceManager::WriteThread()
{
    while (true)
    {
        std::pair<int32_t, std::vector<unsigned char>> task;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this]
                             { return !m_writeQueue.empty() || m_stop; });

            if (m_stop && m_writeQueue.empty())
                return;

            task = std::move(m_writeQueue.front());
            m_writeQueue.pop();
        }

        SavePieceOnDisk(task.first, task.second);
    }
}

const std::unordered_map<int32_t, PieceManager::PieceData>& PieceManager::GetOnDownloading() const
{
    return m_onDownloadingPieces;
}