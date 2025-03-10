#include "TorrentClient.hpp"

TorrentClient::TorrentClient(std::string torrentFilePath,
                             std::string downloadPath,
                             uint16_t port) : m_torrentFilePath(std::move(torrentFilePath)),
                                              m_downloadPath(std::move(downloadPath)),
                                              m_port(port)
{
}

void TorrentClient::Execute()
{
    boost::asio::io_context io;

    TorrentMetaData tmd(m_torrentFilePath);

    // if is_open(downloadPath/resume.json) then recover bitfield and peer list probably

    auto trackerURLs = tmd.GetTrackerURLs();
    for (auto trackerURL : trackerURLs)
    {
        boost::system::error_code ecTracker;

        std::shared_ptr<Tracker> tracker;
        if (trackerURL.substr(0, 3) == "udp")
            tracker = std::make_shared<TrackerUDP>(io);
        else
            tracker = std::make_shared<TrackerHTTP>(io);

        boost::asio::spawn(io, [&io, tracker, &trackerURL, &ecTracker, &tmd](boost::asio::yield_context yield)
                           {
                               try
                               {
                                   tracker->Connect(yield, trackerURL, ecTracker);
                                   if (ecTracker)
                                   {
                                       // Logger obj -> write() instead of displaying it in the console
                                       std::cout << trackerURL << ": " << ecTracker.message() << std::endl;
                                       return;
                                   }
                                   std::unordered_map<std::string, std::string> request;
                                   tracker->Get(yield, request, ecTracker);
                               }
                               catch (const std::exception &e)
                               {
                                   // Logger obj -> write() instead of displaying it in the console
                                   std::cout << trackerURL << ": " << e.what() << std::endl;
                                   return;
                               }
                           });
    }
}

std::string TorrentClient::generatePeerID()
{
    std::string peerID = "-GYTORR-";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);

    for (int i = 0; i < 12; i++)
    {
        peerID += std::to_string(dis(gen));
    }

    return peerID;
}
