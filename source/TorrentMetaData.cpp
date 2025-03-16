#include "../include/TorrentMetaData.hpp"

TorrentMetaData::TorrentMetaData(const std::string &filepath)
{
    bencode::FunctorDecode decode;
    std::ifstream fin(filepath, std::ios::binary);

    bencode::bencode_object rawData = decode(fin);

    auto data = rawData.GetDict();
    auto info = data["info"].GetDict();

    m_announce = data["announce"].GetString();

    for (auto trackers : rawData.GetDict()["announce-list"].GetList())
    {
        for (auto tracker : trackers.GetList())
        {
            m_announceList.push_back(tracker.GetString());
        }
    }

    m_comment = data["comment"].GetString();

    m_createdBy = data["created by"].GetString();

    m_creationDate = data["creation date"].GetInteger();

    m_encoding = data["encoding"].GetString();

    m_name = info["name"].GetString();

    m_pieceLength = info["piece length"].GetInteger();

    int64_t bufferOffset = 0;
    if (info.find("length") != end(info))
    {
        m_length = info["length"].GetInteger();

        m_files.push_back(utils::File{info["name"].GetString(),
                                      "",
                                      m_length,
                                      0});
    }
    else
    {
        m_name = info["name"].GetString();

        for (auto file : info["files"].GetList())
        {
            auto filePath = file.GetDict()["path"].GetList();
            std::string buffer;
            for (size_t i = 0; i + 1 < filePath.size(); i++)
            {
                buffer.append(filePath[i].GetString() + "/");
            }
            m_files.push_back(utils::File{filePath.back().GetString(),
                                          buffer,
                                          file.GetDict()["length"].GetInteger(),
                                          bufferOffset});
            bufferOffset += m_files.back().GetSize();
        }
    }

    int64_t fullSize = m_files.back().GetOffset() + m_files.back().GetSize();
    std::string hashConcat = info["pieces"].GetString();
    bufferOffset = 0;
    size_t fileIndex = 0;
    size_t index = 0;
    for (; index + 20 < hashConcat.size(); index += 20)
    {
        while(m_files[fileIndex].GetOffset() + m_files[fileIndex].GetSize() <= bufferOffset)
            fileIndex++;

        m_pieces.push_back(utils::Piece{m_pieceLength,
                                        bufferOffset,
                                        hashConcat.substr(index, 20),
                                        fileIndex});
        bufferOffset += m_pieceLength;
    }
    m_pieces.push_back(utils::Piece{fullSize - bufferOffset,
                                    bufferOffset,
                                    hashConcat.substr(index, 20),
                                    fileIndex});

    std::ostringstream out_stream;
    bencode::FunctorEncode encoder;

    encoder(data["info"], out_stream);

    std::string encoded_str = out_stream.str();

    std::vector<unsigned char> info_bencoded(encoded_str.begin(), encoded_str.end());

    m_infoHash.resize(SHA_DIGEST_LENGTH);

    SHA1(info_bencoded.data(), info_bencoded.size(), m_infoHash.data());
}

std::string TorrentMetaData::GetName() const { return m_name; }
std::string TorrentMetaData::GetComment() const { return m_comment; }
std::string TorrentMetaData::GetCreator() const { return m_createdBy; }

int64_t TorrentMetaData::GetCreationDate() const { return m_creationDate; }
int64_t TorrentMetaData::GetPieceLength() const { return m_pieceLength; }

const std::vector<std::string> &TorrentMetaData::GetTrackerURLs() const { return m_announceList; }
const std::vector<utils::File> &TorrentMetaData::GetFiles() const { return m_files; }
const std::vector<utils::Piece> &TorrentMetaData::GetPieces() const { return m_pieces; }

std::vector<unsigned char> TorrentMetaData::GetInfoHash() const { return m_infoHash; }