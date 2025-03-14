#include "TrackerAnnouncer.hpp"

using callback_function = std::function<void(const std::string &, const std::vector<utils::Peer> &, boost::system::error_code)>;
TrackerAnnouncer::TrackerAnnouncer(boost::asio::io_context &io,
                                   const std::string &infoHash,
                                   const std::string &peerID, 
                                   uint16_t port,
                                   callback_function callback) : m_io(io),
                                                                 m_infoHash(infoHash),
                                                                 m_peerID(peerID),
                                                                 m_port(port),
                                                                 m_callback(callback) {}

void TrackerAnnouncer::Start(std::shared_ptr<Tracker> tracker, boost::system::error_code &ecTracker)
{
    Update(tracker);
}

void TrackerAnnouncer::Update(std::shared_ptr<Tracker> tracker)
{
    auto cooldown = std::make_shared<boost::asio::steady_timer>(m_io);
    m_activeTimers.insert(cooldown);
    boost::asio::spawn(m_io, [this, tracker, cooldown](boost::asio::yield_context yield)
    {
        uint32_t interval = 0;
        boost::system::error_code ecTracker;
        while (true) 
        {
            cooldown->expires_after(std::chrono::seconds(interval));
            cooldown->async_wait(yield[ecTracker]);
            if (ecTracker) break;

            std::unordered_map<std::string, std::string> data = {
                {"info_hash", m_infoHash},
                {"peer_id", m_peerID},
                {"port", std::to_string(m_port)},
                {"uploaded", std::to_string(m_uploaded)},
                {"downloaded", std::to_string(m_downloaded)},
                {"left", std::to_string(m_left)},
                {"compact", std::to_string(m_compact)}};
            data["event"] = interval == 0 ? "started" : m_event;

            auto response = tracker->Get(yield, data, ecTracker);
            if (ecTracker)
            {
                m_activeTimers.erase(cooldown);
                m_callback(tracker->GetURL(), {}, ecTracker);
                return;
            }
            m_callback(tracker->GetURL(), response.GetPeers(), ecTracker);


            interval = response.GetInterval();
        } 
    });
}

void TrackerAnnouncer::StopAll()
{
    for (auto &timer : m_activeTimers)
    {
        timer->cancel();
    }
}

void TrackerAnnouncer::SetEvent(const std::string &event)
{
    m_event = event;
}

void TrackerAnnouncer::SetUploaded(uint64_t uploaded)
{
    m_uploaded = uploaded;
}
void TrackerAnnouncer::SetDownloaded(uint64_t downloaded)
{
    m_downloaded = downloaded;
}
void TrackerAnnouncer::SetLeft(uint64_t left)
{
    m_left = left;
}