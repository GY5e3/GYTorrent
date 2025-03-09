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

#include "../utils/BitTorrentConstants.hpp"

int main(int argc, char *argv[])
{

    TorrentMetaData tmd("../_input/28777.torrent");

    std::string peerID = "-GYTORR-123456789101";

    boost::asio::io_context io;
    auto tracker = std::make_shared<TrackerHTTP>(io, tmd.GetTrackerURLs()[0]);
    std::cout << tmd.GetTrackerURLs()[0] << std::endl;

    auto connectionManager = std::make_shared<ConnectionManager>(io, tmd, peerID);

    // for(auto i :  tmd.GetTrackerURLs())
    //   std::cout << i << std::endl;

    boost::asio::spawn(io, [&io, tracker, connectionManager, &peerID, &tmd](boost::asio::yield_context yield)
                       {
                           boost::system::error_code ec;

                           tracker->Connect(yield, ec);
                           if (ec)
                           {
                               std::cout << ec.message() << std::endl;
                               return;
                           }
                           auto buffer = tmd.GetInfoHash();
                           std::string infoHash(begin(buffer), end(buffer));

                           std::string port = "6881";
                           std::string uploaded = "0";
                           std::string downloaded = "0";
                           std::string left = "0";
                           std::string compact = "0";
                           std::string event = "completed";
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

                           auto response = tracker->Get(yield, data, ec);

                           std::cout << std::endl;
                           std::cout << "complete: " << response.GetSeeders() << std::endl
                                     << std::endl;
                           std::cout << "incomlete: " << response.GetLeechers() << std::endl
                                     << std::endl;
                           std::cout << "interval: " << response.GetInterval() << std::endl
                                     << std::endl;
                           std::cout << "peers:" << std::endl;
                           for (const auto &peer : response.GetPeers())
                               std::cout << peer.GetID() << " " << peer.ToString() << std::endl;

                           auto peers = response.GetPeers();
                           for (size_t i = 0; i < peers.size(); i++)
                           {
                               boost::asio::spawn(io, [&, peer = peers[i]](boost::asio::yield_context yield)
                                                  {
                boost::system::error_code ec;
                auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io);

                connectionManager->Init(*socket, peer, yield, ec);
                if (ec) {
                    std::cout << ec.message() + ": " + peer.ToString() << std::endl;
                }
                else {
                    std::cout << "Connection w/ " + peer.ToString() + " is success!" << std::endl;
                    
                    std::vector<unsigned char> messageLengthBuffer(4);
                    size_t recievedBytes = boost::asio::async_read(*socket, boost::asio::buffer(messageLengthBuffer), yield[ec]);
                    int32_t messageLength = recievedBytes ? (static_cast<int32_t>(messageLengthBuffer[0]) << 24) |
                                                    (static_cast<int32_t>(messageLengthBuffer[1]) << 16) |
                                                    (static_cast<int32_t>(messageLengthBuffer[2]) << 8) |
                                                    static_cast<int32_t>(messageLengthBuffer[3])
                                                    : 0;

                    std::vector<unsigned char> messageBuffer(messageLength);
                    boost::asio::async_read(*socket, boost::asio::buffer(messageBuffer), yield[ec]);

                    int messageID = messageBuffer.size() ? static_cast<int>(messageBuffer[0]) : static_cast<int>(utils::MessageID::keepAlive);
                    std::ostringstream os;
                    if(messageID == static_cast<int>(utils::MessageID::bitField))
                    {
                        os << "BitField by " + peer.ToString() << ":";
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
                } });
                           }

                           // auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io);
                           // connectionManager->Listen(*socket, yield, ec, 6881);
                       });
    io.run();
    return 0;
}