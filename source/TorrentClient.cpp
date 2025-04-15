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
    TorrentMetaData tmd(m_torrentFilePath);

    // if is_open(downloadPath/resume.json) then recover bitfield and peer list probably

    auto buffer = tmd.GetInfoHash();
    std::string infoHash(begin(buffer), end(buffer));

    auto announcer = std::make_shared<TrackerAnnouncer>(m_io, infoHash, m_peerID, m_port, 
        [this](const std::string& trackerURL, const std::vector<utils::Peer> &peers, boost::system::error_code ec)
        {
            this->trackerCallback(trackerURL, peers, ec);
        }
    );

    m_sessionManager = std::make_shared<SessionManager>(m_io, 
        [this](const std::string& peer, const utils::Message& msg, bool isIncoming, boost::system::error_code ec) 
        {
            this->peerCallback(peer, msg, isIncoming, ec);
        }
    );

    m_connectionManager = std::make_shared<ConnectionManager>(m_io, tmd, m_peerID);
    m_pieceManager = std::make_shared<PieceManager>(tmd, m_downloadPath);

    auto trackerURLs = tmd.GetTrackerURLs();
    for (size_t i = 0; i < trackerURLs.size() && i < 1; i++)
    {
        boost::system::error_code ec;

        std::shared_ptr<Tracker> tracker;
        if (trackerURLs[i].substr(0, 3) == "udp")
            tracker = std::make_shared<TrackerUDP>(m_io, trackerURLs[i]);
        else
            tracker = std::make_shared<TrackerHTTP>(m_io, trackerURLs[i]);

        announcer->Start(tracker, ec);
            
    } 

    std::thread leecherThread(&TorrentClient::LeecherMode, this);

    m_io.run();

    if(leecherThread.joinable())
        leecherThread.join();    
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
    for (size_t i = 0; i < peers.size(); i++)
    {   
        std::string cur = peers[i].ToString();

        if(m_sessionManager->IsActive(cur))
            continue;
        boost::asio::spawn(m_io, [this, peer = peers[i]](boost::asio::yield_context yield)
        {
            boost::system::error_code ec;

            auto session = m_connectionManager->Init(peer, yield, ec);
            if (ec) 
                std::cout << ec.message() + ": " + peer.ToString() << std::endl;                      
            else 
            {
                std::cout << "Connection w/ " + peer.ToString() + " is success!" << std::endl;

                m_sessionManager->Start(peer.ToString(), session);

                utils::Message interested{utils::MessageID::interested};

                m_sessionManager->SendMessage(peer.ToString(), interested);
            }
        });
    } 
}

void TorrentClient::peerCallback(const std::string &peer, const utils::Message &message, bool isIncoming, boost::system::error_code ec)
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    if (ec == torrent_errc::block_request_timeout)
    {
        m_sessionManager->Stop(peer);
        std::cout << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "-----------Peer " + peer + " error: " + ec.message() << std::endl;
        /// TODO: implement logic to return blocks to the general queue
    }
    else if (ec)
    {
        m_sessionManager->Stop(peer);
        std::cout << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "-----------Peer " + peer + " error: " + ec.message() << std::endl;
    }
    else if (isIncoming)
    {
        
        if (message.MessageID == utils::MessageID::piece)
        {
            //std::cout << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "-----------Peer " + peer + " send to me message " << static_cast<int>(message.MessageID) << std::endl;
            //std::cout << "\tindex:" << message.PieceIndex << " offset:" << message.Offset << std::endl;

            m_pieceManager->AcceptBlock(peer, message);

            if(m_pieceManager->IsDownloadedPiece(message.PieceIndex))
            {
                std::vector<unsigned char> piece;
                boost::system::error_code ec;

                if(m_pieceManager->CheckPieceHash(message.PieceIndex, piece))
                {   
                    m_pieceManager->SavePieceOnDisk(message.PieceIndex, piece, ec);
                    
                }
                /// TODO: implement a request entire piece for each sender
                
            }
        }
        else if (message.MessageID == utils::MessageID::bitField)
        {
            std::cout << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "-----------Peer " + peer + " send to me message " << static_cast<int>(message.MessageID) << std::endl;
            std::cout << "BitField: " << std::endl;
            for (auto i : message.BitField)
            {
                std::cout << i << " ";
            }
            std::cout << std::endl;
        }
        else 
        {
            std::cout << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "-----------Peer " + peer + " send to me message " << static_cast<int>(message.MessageID) << std::endl;
        }
    }
    else
    {
        if(message.MessageID != utils::MessageID::request)
            std::cout << std::put_time(std::localtime(&now_time), "%H:%M:%S") << "-----------I has sent to " + peer + " message " << static_cast<int>(message.MessageID) << std::endl;
    }

    /// TODO: smth w/ incoming message in here :/
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

void TorrentClient::LeecherMode()
{
    while(m_pieceManager->IsIncomplete())
    {
        if(m_pieceManager->GetOnDownloading().size() < 5) /// TODO: It must be a constant
        {
            auto responses = m_pieceManager->LoadNextPiece(true);
            
            for(auto request : responses)
            {
                m_messageQueue.push(request);
            }
        }
        if(m_messageQueue.empty())
            continue;

        auto request = m_messageQueue.front();

        std::string peerConcat = m_sessionManager->GetAvailablePeer(request.PieceIndex, 5); /// TODO: It must be a constant 

        if(peerConcat == "")
            continue;

        m_messageQueue.pop();
        
        m_sessionManager->SendMessage(peerConcat, request);
    }
}
