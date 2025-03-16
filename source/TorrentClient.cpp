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
    auto pieceManager = std::make_shared<PieceManager>(true, tmd, "");
    
    m_sessionManager = std::make_shared<SessionManager>(io, peerCallback);

    boost::asio::spawn(io, [this, &io, announcer, connectionManager, pieceManager, &tmd](boost::asio::yield_context yield)
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
        cooldown.expires_after(std::chrono::seconds(7));
        cooldown.async_wait(yield[ec]);

        for (size_t i = 0; i < m_peersList.size(); i++)
        {
            boost::asio::spawn(io, [this, connectionManager, peer = m_peersList[i]](boost::asio::yield_context yield)
            {
                boost::system::error_code ec;

                auto session = connectionManager->Init(peer, yield, ec);
                if (ec) 
                    std::cout << ec.message() + ": " + peer.ToString() << std::endl;                      
                else 
                {
                    std::cout << "Connection w/ " + peer.ToString() + " is success!" << std::endl;

                    TorrentClient::m_sessionManager->Start(peer.ToString(), session);

                    utils::Message interested{utils::MessageID::interested};

                    TorrentClient::m_sessionManager->SendMessage(peer.ToString(), interested);
                }
            });
        }
    });

    io.run();
}
void TorrentClient::trackerCallback(const std::string &trackerURL,
                                    const std::vector<utils::Peer> &peers,
                                    boost::system::error_code ec)
{
    auto now = std::chrono::system_clock::now();  
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);  
    if (ec)
    {
        std::cout << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "-----------Tracker " + trackerURL + " error: " + ec.message() << std::endl;
        return;
    }
    else
    {
        std::cout << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "-----------Update for " + trackerURL << std::endl;
    }
    m_peersList.insert(end(m_peersList), begin(peers), end(peers));
}

void TorrentClient::peerCallback(const std::string &peer, const utils::Message &message, bool isIncoming, boost::system::error_code ec)
{
    auto now = std::chrono::system_clock::now();  
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    if(ec == torrent_errc::block_request_timeout)
    {
        m_sessionManager->Stop(peer);
        std::cout << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "-----------Peer " + peer + " error: " + ec.message() << std::endl;
        ///TODO: implement logic to return blocks to the general queue
    }
    else if (ec)
    {
        m_sessionManager->Stop(peer);
        std::cout << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "-----------Peer " + peer + " error: " + ec.message() << std::endl;
    }
    else if(isIncoming)
    {
        std::cout << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "-----------Peer " + peer + " send to me message " << static_cast<int>(message.MessageID) << std::endl;
        if(message.MessageID == utils::MessageID::piece)
        {
            
        }
    }
    else
    {
        std::cout << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "-----------I has sent to " + peer + " message " << static_cast<int>(message.MessageID) << std::endl;
    }
    
    ///TODO: smth w/ incoming message in here :/
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
