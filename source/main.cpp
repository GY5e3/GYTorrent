#include <iostream>
#include <utility>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include <libtorrent/torrent_info.hpp>

#include "../include/TorrentMetaData.hpp"
#include "../include/TrackerHTTP.hpp"
#include "../include/TrackerUDP.hpp"

#include "../utils/Functors/GeneratorPeerID.hpp"

int main(int argc, char *argv[])
{
    //lt::error_code ec;
    //lt::torrent_info ti("../_input/duna.torrent");


    TorrentMetaData tmd("../_input/duna.torrent");
    //ti.trackers()[0].url;
    boost::asio::io_context io;
    auto tracker = std::make_shared<TrackerHTTP>(io, tmd.GetTrackerURLs()[2]);

    for(auto i :  tmd.GetTrackerURLs())
        std::cout << i << std::endl;

    auto tracker1 = std::make_shared<TrackerUDP>(io, tmd.GetTrackerURLs()[3]);
    boost::asio::spawn(io, [tracker, tracker1, &tmd](boost::asio::yield_context yield)
                       {
        boost::system::error_code ec;

        tracker->Connect(yield, ec);
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
        tracker->Get(yield, data, ec);
        
        tracker1->Connect(yield, ec);
        tracker1->Get(yield, data, ec); 
        });

    io.run();
    /*
        auto hash = tmd.GetInfoHash();

    for(auto c : hash)
        std::cout << c;
    std::cout << std::endl;

    std::string strhash(begin(hash), end(hash));

    if(strhash == ti.info_hash().to_string()) {
        std::cout << "YEH BOOOY!!!" << std::endl;
    }
    else
        std::cout << "BUCK FOOOY!!!" << std::endl;

    auto files = ti.files();

        auto tmdFiles = tmd.GetFiles();

        for (int i = 0; i < ti.num_files(); i++)
        {
            std::cout << files.file_name(i).to_string() << std::endl;
            std::cout << files.file_offset(i) << std::endl;
            std::cout << files.file_size(i) << std::endl
                      << std::endl;

            std::cout << tmdFiles[i].GetName() << std::endl;
            std::cout << tmdFiles[i].GetOffset() << std::endl;
            std::cout << tmdFiles[i].GetSize() << std::endl;

            std::cout << "===========================================================" << std::endl
                      << std::endl;
        }
        for (size_t i = 0; i < ti.num_pieces(); i++)
    {
        std::cout << ti.piece_length() << std::endl << std::endl;

        std::cout << tmd.GetPieces()[i].GetLength() << std::endl;
        std::cout << tmd.GetPieces()[i].GetOffset() << std::endl << std::endl;

        if(ti.hash_for_piece(i).to_string() == tmd.GetPieces()[i].GetHash()) {
            std::cout << "Noice hashes!!" << std::endl;
        }
        else {
            std::cout << "ZALUPKA!!!" << std::endl;
        }

        std::cout << "===========================================================" << std::endl
                      << std::endl;
    }
    */

    return 0;
}
