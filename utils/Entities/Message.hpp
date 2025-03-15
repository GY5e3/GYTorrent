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
        int32_t Length = utils::BLOCK_SIZE;
        std::vector<bool> BitField;
        std::vector<unsigned char> Block;
        int16_t Port = 0;

        Message() = default;

        /// @brief Can be used for initialize choke-, unchoke-, interested-, not interested- messages
        /// @param messageID 
        Message(utils::MessageID messageID) : MessageID(messageID) {}

        /// @brief Can be used for initialize have- message
        /// @param messageID 
        /// @param pieceIndex 
        Message(utils::MessageID messageID, int32_t pieceIndex) : MessageID(messageID), PieceIndex(pieceIndex) {}

        /// @brief Can be used for initialize bitfield- message
        /// @param messageID 
        /// @param bitField 
        Message(utils::MessageID messageID, std::vector<bool> bitField) : MessageID(messageID), BitField(bitField) {}

        /// @brief Can be used for initialize request- and cancel- messages
        /// @param messageID 
        /// @param pieceIndex 
        /// @param offset 
        /// @param length 
        Message(utils::MessageID messageID,
                int32_t pieceIndex,
                int32_t offset,
                int32_t length) : MessageID(messageID), PieceIndex(pieceIndex), Offset(offset), Length(length) {}

        /// @brief Can be used for initialize piece- message
        /// @param messageID 
        /// @param pieceIndex 
        /// @param offset 
        /// @param block 
        Message(utils::MessageID messageID,
                int32_t pieceIndex,
                int32_t offset,
                std::vector<unsigned char> block) : MessageID(messageID), PieceIndex(pieceIndex), Offset(offset), Block(block) {}

        /// @brief Can be used for initialize port- message
        /// @param messageID 
        /// @param port 
        Message(utils::MessageID messageID, int16_t port) : MessageID(messageID), Port(port) {}
    };
}
