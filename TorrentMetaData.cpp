#include "TorrentMetaData.hpp"

TorrentMetaData::TorrentMetaData(std::string dotTorrentFilePath)
{
    libtorrent::torrent_info torrentInfo(dotTorrentFilePath);

    initialization(torrentInfo);
}

TorrentMetaData::TorrentMetaData(libtorrent::torrent_info torrentInfo)
{
    initialization(torrentInfo);
}

void TorrentMetaData::initialization(libtorrent::torrent_info torrentInfo)
{
    for (const auto &tracker : torrentInfo.trackers())
        m_trackers.push_back(tracker.url);

    auto files = torrentInfo.files();

    for (int i = 0; i < torrentInfo.num_files(); i++)
        m_files.push_back(File(files.file_name(i).to_string(), files.file_size(i), files.file_offset(i)));

    m_torrentHash = torrentInfo.info_hash().to_string();

    m_myPeerID = utils::GeneratorPeerID();

    m_piecesCount = torrentInfo.num_pieces();

    m_totalSize = torrentInfo.total_size();

    m_pieceSize = torrentInfo.piece_length();

    m_pieceHash.resize(m_piecesCount);
    for(int i = 0; i < m_piecesCount; i++)
        m_pieceHash[i] = torrentInfo.hash_for_piece(i).to_string();

}

std::vector<std::string> TorrentMetaData::GetTrackers()
{
    return m_trackers;
}
std::vector<File> TorrentMetaData::GetFiles()
{
    return m_files;
}
std::string TorrentMetaData::GetTorrentHash()
{
    return m_torrentHash;
}
int TorrentMetaData::GetPieceCount()
{
    return m_piecesCount;
}

std::string TorrentMetaData::GetMyPeerID()
{
    return m_myPeerID;
}
int64_t TorrentMetaData::GetTotalSize()
{
    return m_totalSize;
}
int32_t TorrentMetaData::GetPieceSize()
{
    return m_pieceSize;
}
std::string TorrentMetaData::GetPieceHash(int32_t index) {
    return m_pieceHash[index];
}