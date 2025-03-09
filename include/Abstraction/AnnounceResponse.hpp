#pragma once

#include <vector>
#include <string>

#include "Entities/Peer.hpp"

class AnnounceResponse
{
public:
    AnnounceResponse() = default;

    std::vector<utils::Peer> GetPeers() const { return m_peers; }
    uint32_t GetInterval() const { return m_interval; }
    uint32_t GetSeeders() const { return m_complete; }
    uint32_t GetLeechers() const { return m_incomplete; }

protected:
    std::vector<utils::Peer> m_peers;
    uint32_t m_interval;
    uint32_t m_complete;
    uint32_t m_incomplete;
};