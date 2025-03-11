#pragma once

#include <utility>
#include <memory>
#include <unordered_set>

#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>

#include "Abstraction/Tracker.hpp"
#include "TrackerHTTP.hpp"
#include "TrackerUDP.hpp"

#include "Abstraction/AnnounceResponse.hpp"

class TrackerAnnouncer : public std::enable_shared_from_this<TrackerAnnouncer>
{
public:
    TrackerAnnouncer(boost::asio::io_context &io,
                     const std::string &infoHash,
                     const std::string &peerID,
                     uint16_t port,
                     std::function<void(const std::vector<utils::Peer> &, boost::system::error_code)> callback);

    void Start(std::shared_ptr<Tracker> tracker, boost::system::error_code &ecTracker);

    void Update(std::shared_ptr<Tracker> tracker, uint32_t interval = 0);

    void StopAll();

    void SetEvent(const std::string &event);
    void SetUploaded(uint64_t uploaded);
    void SetDownloaded(uint64_t downloaded);
    void SetLeft(uint64_t left);

private:
    boost::asio::io_context &m_io;

    std::string m_infoHash;
    std::string m_peerID;
    uint16_t m_port;
    uint64_t m_uploaded = 0;
    uint64_t m_downloaded = 0;
    uint64_t m_left = 0;
    bool m_compact = false;
    std::string m_event = "started";

    std::function<void(const std::vector<utils::Peer> &, boost::system::error_code)> m_callback;

    std::unordered_set<std::shared_ptr<boost::asio::steady_timer>> m_activeTimers;
};

