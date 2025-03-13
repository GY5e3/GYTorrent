#pragma once

#include <cstdint>
#include <vector>

#include "BitTorrentConstants.hpp"
#include "Functors/build_request.hpp"

namespace utils
{
    struct Message
    {
    public:
        utils::MessageID MessageID = utils::MessageID::keepAlive;
        int32_t PieceIndex = 0;
        int32_t Offset = 0;
        int32_t Length = 0;
        std::vector<bool> BitField;
        std::vector<unsigned char> Block;
        int16_t Port = 0;
    };
}
