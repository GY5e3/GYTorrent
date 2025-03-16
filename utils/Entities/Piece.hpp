#pragma once

#include <string>
#include <cstdint>

namespace utils
{

    class Piece
    {
    public:
        Piece() = default;
        Piece(int64_t length, int64_t offset, const std::string &hash, size_t fileIndex) : m_length(length),
                                                                                            m_offset(offset),
                                                                                            m_hash(hash),
                                                                                            m_fileIndex(fileIndex) {}
        int64_t GetLength() const { return m_length; }
        int64_t GetOffset() const { return m_offset; }
        std::string GetHash() const { return m_hash; }
        size_t GetFileIndex() const { return m_fileIndex; }

    private:
        int64_t m_length;
        int64_t m_offset;
        std::string m_hash;

        size_t m_fileIndex;
    };
}