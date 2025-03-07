#include <iostream>
#include <utility>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "../include/TorrentMetaData.hpp"
#include "../include/TrackerHTTP.hpp"
#include "../include/TrackerUDP.hpp"
#include "../include/AnnounceResponseHTTP.hpp"
#include "../include/AnnounceResponseUDP.hpp"
#include "../include/ConnectionManager.hpp"

#include "../utils/Functors/GeneratorPeerID.hpp"

int main(int argc, char *argv[])
{
    TorrentMetaData tmd("../_input/28777.torrent");

    utils::GeneratorPeerID gpid;
    std::string peerID = gpid();

    boost::asio::io_context io;
    auto tracker = std::make_shared<TrackerHTTP>(io, tmd.GetTrackerURLs()[0]);

    auto connectionManager = std::make_shared<ConnectionManager>(io, tmd, peerID);

    for(auto i :  tmd.GetTrackerURLs())
        std::cout << i << std::endl;

   // auto tracker1 = std::make_shared<TrackerUDP>(io, tmd.GetTrackerURLs()[3]);
    boost::asio::spawn(io, [&io, tracker, connectionManager, &peerID, &tmd](boost::asio::yield_context yield)
                       {
        boost::system::error_code ec;

        tracker->Connect(yield, ec);
        if (ec) return;
        auto buffer = tmd.GetInfoHash();
        std::string infoHash(begin(buffer), end(buffer));
        
        std::string port = "6881";
        std::string uploaded = "0";
        std::string downloaded = "0";
        std::string left = "0";
        std::string compact = "0";
        std::string event = "started";
        std::unordered_map<std::string, std::string> data = {
            {"info_hash", infoHash},
            {"peer_id", peerID},
            {"port", port},
            {"uploaded", uploaded},
            {"downloaded", downloaded},
            {"left", left},
            {"compact", compact},
            {"event", event},
        };

       // tracker1->Connect(yield, ec);
        if(ec) {
            std::cout << ec.message() << std::endl;
            return;
        }

       // AnnounceResponseUDP responseUDP = tracker1->Get(yield, data, ec);
        if(ec) {
            std::cout << ec.message() << std::endl;
            return;
        }

       // std::cout << std::endl;
       // std::cout << "complete: " << responseUDP.GetSeeders() << std::endl << std::endl;
      //  std::cout << "incomlete: " << responseUDP.GetLeechers() << std::endl << std::endl;
        //std::cout << "interval: " << responseUDP.GetInterval() << std::endl << std::endl;
        //std::cout << "peers:" << std::endl;
        //for (const auto& peer : responseUDP.GetPeers())
          // std::cout << peer.ToString() << std::endl;

        AnnounceResponseHTTP response = tracker->Get(yield, data, ec);

        std::cout << std::endl;

        std::cout << "complete: " << response.GetSeeders() << std::endl << std::endl;

        std::cout << "incomlete: " << response.GetLeechers() << std::endl << std::endl;

        std::cout << "interval: " << response.GetInterval() << std::endl << std::endl;

        std::cout << "peers:" << std::endl;
        for (const auto& peer : response.GetPeers())
            std::cout << peer.GetID() << " " << peer.ToString() << std::endl;
         
        boost::asio::ip::tcp::socket socket(io); 
        connectionManager->Init(socket, response.GetPeers()[0], yield, ec);
        if(ec) {
            std::cout << ec.message() << std::endl;
        }
        });

    io.run();
    return 0;
}
//-qB5030-9-FOMqC-GGA6 93.81.249.41:22049