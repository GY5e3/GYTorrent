#pragma once

namespace utils
{
    const int BLOCK_SIZE = 16 * 1024;
    /// @brief BitTorrent protocol ID
    const int8_t PROTOCOL_ID = 19;

    const int HANDSHAKE_LENGTH = 68;

    const int FREE_BYTES_LENGTH = 8;

    const int TORRENT_HASH_LENGTH = 20;

    const int PEER_ID_LENGTH = 20;

    enum class MessageID
    {
        keepAlive = -1,
        choke = 0,
        unchoke = 1,
        interested = 2,
        notInterested = 3,
        have = 4,
        bitField = 5,
        request = 6,
        piece = 7,
        cancel = 8,
        port = 9
    };

    enum class TrackerEvent
    {
        none = 0,
        completed = 1,
        started = 2,
        stopped = 3
    };
}