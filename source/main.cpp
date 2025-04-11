#include <iostream>
#include <utility>
#include <filesystem>

#include <boost/asio/io_context.hpp>

#include "../include/TorrentClient.hpp"

int main(int argc, char *argv[])
{
    TorrentClient tc("../_input/28777.torrent", "", 6881);

    tc.Execute();

    return 0;
}