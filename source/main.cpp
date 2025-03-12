#include <iostream>
#include <utility>
#include <filesystem>

#include "../include/PeerSession.hpp"
#include <boost/asio/spawn.hpp>

#include "../include/TrackerAnnouncer.hpp"

#include "../include/TorrentMetaData.hpp"
#include "../include/TrackerHTTP.hpp"
#include "../include/TrackerUDP.hpp"
#include "../include/AnnounceResponseHTTP.hpp"
#include "../include/AnnounceResponseUDP.hpp"
#include "../include/ConnectionManager.hpp"

#include "../utils/BitTorrentConstants.hpp"

#include "../include/TorrentClient.hpp"


void CallBack(boost::asio::ip::tcp::socket &socket, std::vector<unsigned char> messageBuffer)
{
    std::string peer = socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port());

    int messageID = messageBuffer.size() ? static_cast<int>(messageBuffer[0]) : static_cast<int>(utils::MessageID::keepAlive);
    std::ostringstream os;
    if (messageID == static_cast<int>(utils::MessageID::bitField))
    {
        os << "BitField by " + peer << ":";
        std::vector<bool> bifi;
        for (size_t i = 1; i < messageBuffer.size(); i++)
        {
            unsigned char byte = messageBuffer[i];
            for (int j = 7; j >= 0; --j)
            {
                bifi.push_back(byte & (1 << j));
            }
        }
        for (auto mes : bifi)
        {
            os << " " << mes;
        }
        os << std::dec << "\n";
        std::cout << os.str();
    }
}
void call_back(const std::vector<utils::Peer> &peers, boost::system::error_code ec)
{
    if (ec)
    {
        std::cout << ec.message() + " do smth w/ this info lol" << std::endl;
        return;
    }
    std::cout << "peers:" << std::endl;
    for (const auto &peer : peers)
        std::cout << peer.GetID() << " " << peer.ToString() << std::endl;
}
int main(int argc, char *argv[])
{
    boost::asio::io_context io;

    TorrentMetaData tmd("../_input/28777.torrent");
    auto buffer = tmd.GetInfoHash();
    std::string infoHash(begin(buffer), end(buffer));
    std::string peerID = "-GYTORR-123456789101";

    std::filesystem::create_directory(tmd.GetName());

    auto connectionManager = std::make_shared<ConnectionManager>(io, tmd, peerID);
    auto timer = std::make_shared<boost::asio::steady_timer>(io);

    std::string trackerURL = tmd.GetTrackerURLs()[0];
    std::shared_ptr<Tracker> exampleTracker;
    if (trackerURL.substr(0, 3) == "udp")
        exampleTracker = std::make_shared<TrackerUDP>(io, trackerURL);
    else
        exampleTracker = std::make_shared<TrackerHTTP>(io, trackerURL);
    
    boost::asio::ip::tcp::socket socket(io);
    PeerSession ps(io, std::move(socket));

    TorrentClient tc("../_input/28777.torrent", "", 6881);

    tc.Execute();
/*
boost::asio::spawn(io,
                       [&io, exampleTracker, connectionManager, announcer, timer, &peerID, &tmd](boost::asio::yield_context yield)
                       {
                           boost::system::error_code ec;

                           announcer->Start(exampleTracker, ec);
                           if (ec)
                           {
                               ec.message();
                           }
                           timer->expires_after(std::chrono::seconds(15));
                           timer->async_wait(yield);
                           announcer->SetEvent("stopped");
                           announcer->StopAll();
                          
                       });
*/
    
    io.run();
    return 0;
}