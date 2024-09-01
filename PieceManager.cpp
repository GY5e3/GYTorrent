#include "PieceManager.hpp"

PieceManager::PieceManager(int pieceCount, TorrentMetaData torrentMetaData) : m_torrentMetaData(torrentMetaData)
{
    m_pieces.resize(pieceCount);
    downloaded = 0;
}
void PieceManager::PieceRequest()
{
    // for (size_t i = 14; i < m_pieces.size() && i <= 14; i++)
    //   {
    //      Piece piece;
    //     piece.m_pieceIndex = i;
    //     piece.m_blockCount = 1024;
    //     piece.m_offset = i * m_torrentMetaData.GetPieceSize();

    //   m_mediator->Send(utils::MessageID::request, piece, getSharedPtr());
    // }

    std::thread([this]()
                {
        std::queue<int> q;
        for (size_t i = 0; i < m_pieces.size() && i <= 14; i++)
        {
            q.push(i);
        }
        while(downloaded < m_pieces.size() && downloaded < 3) {
            if(q.size()){
                Piece piece;
                piece.m_pieceIndex = q.front(); q.pop();
                piece.m_blockCount = 1024;
                piece.m_offset = piece.m_pieceIndex * m_torrentMetaData.GetPieceSize();

                while(!m_mediator->Send(utils::MessageID::request, piece, getSharedPtr()));
            }
            
        } })
        .detach();
}
void PieceManager::PieceRecieve(int32_t pieceIndex, std::vector<unsigned char> data)
{
    int64_t pieceOffset = pieceIndex * m_torrentMetaData.GetPieceSize();

    std::vector<File> files = m_torrentMetaData.GetFiles();

    size_t left = 0, right = files.size() - 1;
    while(files[left++].GetFileOffset() > pieceOffset);
    left--;
    int64_t remainingData = data.size();
    int64_t dataOffset = 0;

    while (remainingData > 0 && left < files.size())
    {
        File &targetFile = files[left];
        int64_t fileOffset = pieceOffset - targetFile.GetFileOffset();
        int64_t fileAvailableSpace = targetFile.GetFileSize() - fileOffset;
        int64_t toWrite = std::min(remainingData, fileAvailableSpace);

        std::ofstream ofs(targetFile.GetFileName(), std::ios::binary | std::ios::in | std::ios::out);
        if (!ofs)
        {
            ofs.open(targetFile.GetFileName(), std::ios::binary | std::ios::out | std::ios::trunc);
            ofs.close();
            ofs.open(targetFile.GetFileName(), std::ios::binary | std::ios::in | std::ios::out);
        }

        ofs.seekp(fileOffset);
        if (!ofs)
        {
            std::cerr << "Error: Could not seek to position " << fileOffset << " in file " << targetFile.GetFileName() << std::endl;
            return;
        }

        ofs.write(reinterpret_cast<const char *>(data.data() + dataOffset), toWrite);
        if (!ofs)
        {
            std::cerr << "Error: Failed to write data to file " << targetFile.GetFileName() << std::endl;
            return;
        }

        ofs.close();

        pieceOffset += toWrite;
        dataOffset += toWrite;
        remainingData -= toWrite;
        left++; 
    }
    
    if (remainingData > 0 && left == files.size())
        std::cout << "smth was wrong..." << std::endl;
}

void PieceManager::SetMediator(std::shared_ptr<Mediator> mediator)
{
    m_mediator = mediator;
}
std::shared_ptr<PieceManager> PieceManager::getSharedPtr()
{
    return shared_from_this();
}