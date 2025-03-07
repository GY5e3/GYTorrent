#pragma once

#include <string>
#include <cstdint>

namespace utils
{

    class Piece
    {
    public:
        Piece() = default;
        Piece(int64_t length, int64_t offset, std::string hash) : m_length(length),
                                                                  m_offset(offset),
                                                                  m_hash(hash) {}
        int64_t GetLength() const { return m_length; }
        int64_t GetOffset() const { return m_offset; }
        std::string GetHash() const { return m_hash; }

    private:
        int64_t m_length;
        int64_t m_offset;
        std::string m_hash;
    };
}