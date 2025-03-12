#include "TorrentClient.hpp"

TorrentClient::TorrentClient(std::string torrentFilePath,
                             std::string downloadPath,
                             uint16_t port) : m_torrentFilePath(std::move(torrentFilePath)),
                                              m_downloadPath(std::move(downloadPath)),
                                              m_port(port)
{
    m_peerID = generatePeerID();
}

void TorrentClient::Execute()
{
    boost::asio::io_context io;

    TorrentMetaData tmd(m_torrentFilePath);

    // if is_open(downloadPath/resume.json) then recover bitfield and peer list probably

    auto buffer = tmd.GetInfoHash();
    std::string infoHash(begin(buffer), end(buffer));

    auto announcer = std::make_shared<TrackerAnnouncer>(io, infoHash, m_peerID, m_port, trackerCallback);
    auto connectionManager = std::make_shared<ConnectionManager>(io, tmd, m_peerID);

    boost::asio::spawn(io, [this, &io, announcer, connectionManager, &tmd](boost::asio::yield_context yield)
                       {   
        boost::system::error_code ec;

        auto trackerURLs = tmd.GetTrackerURLs();
        for (auto trackerURL : trackerURLs)
        {
            boost::system::error_code ecTracker;

            std::shared_ptr<Tracker> tracker;
            if (trackerURL.substr(0, 3) == "udp")
                tracker = std::make_shared<TrackerUDP>(io, trackerURL);
            else
                tracker = std::make_shared<TrackerHTTP>(io, trackerURL);

            announcer->Start(tracker, ec);
        } 

        boost::asio::steady_timer cooldown(io);
        cooldown.expires_after(std::chrono::seconds(10));
        cooldown.async_wait(yield[ec]);

        for (size_t i = 0; i < m_peersList.size(); i++)
        {
            boost::asio::spawn(io, [&, peer = m_peersList[i]](boost::asio::yield_context yield)
            {
                boost::system::error_code ec;
                auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io);

                auto session = connectionManager->Init(peer, yield, ec);
                if (ec) 
                    std::cout << ec.message() + ": " + peer.ToString() << std::endl;                      
                else 
                {
                    std::cout << "Connection w/ " + peer.ToString() + " is success!" << std::endl;

                    auto response = session.Recieve(yield, ec);


                    int messageID = response.size() ? static_cast<int>(response[0]) : static_cast<int>(utils::MessageID::keepAlive);
                    std::ostringstream os;
                    if (messageID == static_cast<int>(utils::MessageID::bitField))
                    {
                        os << "BitField by " + peer.ToString() << ":";
                        std::vector<bool> bf;
                        for (size_t i = 1; i < response.size(); i++)
                        {
                            unsigned char byte = response[i];
                            for (int j = 7; j >= 0; --j)
                                bf.push_back(byte & (1 << j));
                        }
                        for (auto mes : bf)
                            os << " " << mes;
                        os << std::dec << "\n";
                        std::cout << os.str();
                    }
                }
            });
        } });

    io.run();
}
void TorrentClient::trackerCallback(const std::string &trackerURL,
                                    const std::vector<utils::Peer> &peers,
                                    boost::system::error_code ec)
{
    if (ec)
    {
        std::cout << "Tracker error: " + ec.message() << std::endl;
        return;
    }
    else
    {
        std::cout << "Update for " + trackerURL << std::endl;
    }
    m_peersList.insert(end(m_peersList), begin(peers), end(peers));
}

std::string TorrentClient::generatePeerID() const
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
