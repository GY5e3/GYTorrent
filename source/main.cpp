#include <iostream>
#include <utility>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "../include/TorrentMetaData.hpp"
#include "../include/TrackerHTTP.hpp"
#include "../include/TrackerUDP.hpp"
#include "../include/AnnounceResponseHTTP.hpp"
#include "../include/AnnounceResponseUDP.hpp"

#include "../utils/Functors/GeneratorPeerID.hpp"

int main(int argc, char *argv[])
{
    TorrentMetaData tmd("../_input/duna.torrent");

    boost::asio::io_context io;
    //auto tracker = std::make_shared<TrackerHTTP>(io, tmd.GetTrackerURLs()[0]);

    for(auto i :  tmd.GetTrackerURLs())
        std::cout << i << std::endl;

    auto tracker1 = std::make_shared<TrackerUDP>(io, tmd.GetTrackerURLs()[3]);
    boost::asio::spawn(io, [tracker1, &tmd](boost::asio::yield_context yield)
                       {
        boost::system::error_code ec;

        //tracker1->Connect(yield, ec);
        //if (ec) return;
        auto buffer = tmd.GetInfoHash();
        std::string infoHash(begin(buffer), end(buffer));
        utils::GeneratorPeerID gpid;
        std::string peerID = gpid();
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

        tracker1->Connect(yield, ec);
        if(ec) {
            std::cout << ec.message() << std::endl;
            return;
        }

        AnnounceResponseUDP responseUDP = tracker1->Get(yield, data, ec);
        if(ec) {
            std::cout << ec.message() << std::endl;
            return;
        }

        std::cout << std::endl;
        std::cout << "complete: " << responseUDP.GetSeeders() << std::endl << std::endl;
        std::cout << "incomlete: " << responseUDP.GetLeechers() << std::endl << std::endl;
        std::cout << "interval: " << responseUDP.GetInterval() << std::endl << std::endl;
        std::cout << "peers:" << std::endl;
        for (const auto& peer : responseUDP.GetPeers())
           std::cout << peer.ToString() << std::endl;

        //AnnounceResponseHTTP response = tracker->Get(yield, data, ec);

        //std::cout << std::endl;

       // std::cout << "complete: " << response.GetSeeders() << std::endl << std::endl;

      //  std::cout << "incomlete: " << response.GetLeechers() << std::endl << std::endl;

      //  std::cout << "interval: " << response.GetInterval() << std::endl << std::endl;

      //  std::cout << "peers:" << std::endl;
        //for (const auto& peer : response.GetPeers())
         //   std::cout << peer.GetID() << " " << peer.ToString() << std::endl;
         
        });

    io.run();
    return 0;
}
