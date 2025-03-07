#pragma once

#include <string>
#include <cstdint>

namespace utils
{
    class File
    {
    public:
        File() = default;
        File(std::string fileName, std::string filePath, int64_t fileSize, int64_t fileOffset) : m_name(fileName),
                                                                                                 m_path(filePath),
                                                                                                 m_size(fileSize),
                                                                                                 m_offset(fileOffset) {}
        std::string GetName() const { return m_name; }
        std::string GetPath() const { return m_path; }
        int64_t GetSize() const { return m_size; }
        int64_t GetOffset() const { return m_offset; }

        std::string ToString() const
        {
            return "Name: " + m_name +
                   "\n Size: " + std::to_string(m_size) +
                   " Offset: " + std::to_string(m_offset);
        }

    private:
        std::string m_path;
        std::string m_name;
        int64_t m_size;
        int64_t m_offset;
    };
}
