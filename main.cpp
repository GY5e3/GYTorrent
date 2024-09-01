#include <iostream>
#include <utility>
#include <memory>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <locale>
#include <ctime>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <libtorrent/session.hpp>

#include "PieceManager.hpp"
#include "Connection.hpp"
#include "TorrentMetaData.hpp"
#include "utils.hpp"

void start_timer(boost::asio::io_context &io, std::shared_ptr<PieceManager> pieceManager, std::shared_ptr<Mediator> mediator)
{
    boost::asio::spawn(io, [&io, mediator, pieceManager](boost::asio::yield_context yield)
                       {
        boost::asio::steady_timer timer(io, std::chrono::seconds(15));

        boost::system::error_code ec;
        timer.async_wait(yield[ec]);
        
        if (!ec) {
            std::cout << "Timer expired after 15 seconds" << std::endl;
        } else {
            std::cout << "Timer error: " << ec.message() << std::endl;
        } 
       pieceManager->PieceRequest(); });
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <torrent-file-path>" << std::endl;
        return 1;
    }

    libtorrent::error_code ec;
    lt::torrent_info my_torrent(argv[1], ec);

    TorrentMetaData torrentMetaData(my_torrent);

    auto trackers = torrentMetaData.GetTrackers();

    auto files = torrentMetaData.GetFiles();

    std::cout << "TRACKERS" << std::endl;
    for (auto tracker : trackers)
    {
        std::cout << tracker << std::endl;
    }
    std::cout << "FILES" << std::endl;
    for (auto file : files)
    {
        std::cout << file.ToString() << std::endl;
    }


    boost::asio::io_context io;

    auto [tracker_host, tracker_port] = utils::GetHostAndPortFromURL(my_torrent.trackers()[0].url);
    std::cout << "Host " << tracker_host << " Port " << tracker_port << std::endl;
    std::string info_hash = my_torrent.info_hash().to_string();
    std::string peer_id = utils::GeneratorPeerID(); //"-PC0001-860159384319";
    std::string port = "6881";
    std::string uploaded = "0";
    std::string downloaded = "0";
    std::string left = "0";
    std::string compact = "0";
    std::string event = "started";

    std::string request_path = "/announce?info_hash=" + utils::EncodeURL(info_hash) +
                               "&peer_id=" + utils::EncodeURL(peer_id) +
                               "&port=" + port +
                               "&uploaded=" + uploaded +
                               "&downloaded=" + downloaded +
                               "&left=" + left +
                               "&compact=" + compact +
                               "&event=" + event;

    boost::asio::ip::tcp::resolver resolver(io);
    auto const results = resolver.resolve(tracker_host, tracker_port);

    boost::beast::tcp_stream stream(io);
    boost::asio::connect(stream.socket(), results.begin(), results.end());

    boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::get, request_path, 11};
    req.set(boost::beast::http::field::host, tracker_host);
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    boost::beast::http::write(stream, req);
    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::string_body> res;
    boost::beast::http::read(stream, buffer, res);
    stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

    std::cout << res.base() << std::endl;

    std::string body = res.body();

    libtorrent::span<char const> bencoded_data(body.data(), body.size());
    libtorrent::bdecode_node tracker_response = libtorrent::bdecode(bencoded_data, ec);
    if (ec)
    {
        std::cerr << "Parsing error: " << ec.message() << std::endl;
        return 1;
    }


    int complete = tracker_response.dict_find_int_value("complete", -1);
    int incomplete = tracker_response.dict_find_int_value("incomplete", -1);
    int interval = tracker_response.dict_find_int_value("interval", -1);
    libtorrent::bdecode_node peers = tracker_response.dict_find_list("peers");

    std::cout << "Complete: " << complete << std::endl;
    std::cout << "Incomplete: " << incomplete << std::endl;
    std::cout << "Interval: " << interval << std::endl;
    std::vector<utils::Peer> peersInfo;
    if (peers)
    {
        for (int i = 0; i < peers.list_size(); i++)
        {
            auto peer_id = peers.list_at(i).dict_find_string("peer id");
            auto ip = peers.list_at(i).dict_find_string("ip");
            auto port = peers.list_at(i).dict_find_int("port");

            peersInfo.push_back(utils::Peer(ip.string_value().to_string(), std::to_string(port.int_value())));
        }
    }


    std::shared_ptr<PieceManager> pieceManager = std::make_shared<PieceManager>(torrentMetaData.GetPieceCount(), torrentMetaData);

    std::vector<std::shared_ptr<Connection>> connections;

    std::shared_ptr<Mediator> mediator = std::make_shared<Mediator>(torrentMetaData, pieceManager);

    pieceManager->SetMediator(mediator);

    for (size_t i = 0; i < peersInfo.size(); i++)
    {
        connections.push_back(std::make_shared<Connection>(Connection(torrentMetaData, io, mediator)));
    }
    for (size_t i = 0; i < peersInfo.size(); i++)
    {
        connections[i]->InitializeConnection(peersInfo[i]);
    }
    start_timer(io, pieceManager, mediator);

    io.run();

    return 0;
}