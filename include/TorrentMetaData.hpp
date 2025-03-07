#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

#include <openssl/sha.h>

#include "../utils/Functors/bencode.hpp"
#include "../utils/Entities/File.hpp"
#include "../utils/Entities/Piece.hpp"

class TorrentMetaData
{
public:
    TorrentMetaData(const std::string &filepath);

    std::string GetName() const;
    std::string GetComment() const;
    std::string GetCreator() const;

    int64_t GetCreationDate() const;
    int64_t GetPieceLength() const;

    std::vector<std::string> GetTrackerURLs() const;
    std::vector<utils::File> GetFiles() const;
    std::vector<utils::Piece> GetPieces() const;

    std::vector<unsigned char> GetInfoHash() const;
private:
    std::string m_name;
    std::string m_announce;
    std::string m_comment;
    std::string m_createdBy;
    std::string m_encoding;

    int64_t m_length;
    int64_t m_creationDate;
    int64_t m_pieceLength;

    std::vector<std::string> m_announceList;
    std::vector<utils::File> m_files;
    std::vector<utils::Piece> m_pieces;

    std::vector<unsigned char> m_infoHash;
};
