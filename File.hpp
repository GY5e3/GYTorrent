#pragma once

#include <string>

class File
{
public:
    File(std::string fileName, int64_t fileSize, int64_t fileOffset) : mFileName(fileName),
                                                                       mFileSize(fileSize),
                                                                       mFileOffset(fileOffset) {}
    std::string GetFileName()
    {
        return mFileName;
    }
    int64_t GetFileSize()
    {
        return mFileSize;
    }
    int64_t GetFileOffset()
    {
        return mFileOffset;
    }

    std::string ToString()
    {
        return "Name: " + mFileName +
               "\n Size: " + std::to_string(mFileSize) +
               " Offset: " + std::to_string(mFileOffset);
    }

private:
    std::string mFileName;
    int64_t mFileSize;
    int64_t mFileOffset;
};