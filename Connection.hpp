#pragma once

#define BOOST_ALLOW_DEPRECATED_HEADERS

#include <utility>
#include <vector>
#include <memory>
#include <enable_shared_from_this.hpp>
#include <unordered_map>
#include <iostream>
#include <iomanip>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "TorrentMetaData.hpp"
#include "utils.hpp"
#include "Mediator.hpp"

class Mediator;
/// TODO: I should move error handling to higher layer of my program
class Connection : public std::enable_shared_from_this<Connection>
{
public:
    Connection(const TorrentMetaData &torrentMetaData,
               boost::asio::io_context &io,
               std::shared_ptr<Mediator> mediator);

    bool InitializeConnection(utils::Peer &peer);
    /// NODE: The current implementation does not work for bitfield-messages
    void SendMessage(utils::MessageID messageID, std::vector<int32_t> payload);

    void RecieveMessage();

    std::shared_ptr<boost::asio::ip::tcp::socket> GetSocket();

    utils::Peer GetPeer();

private:
    TorrentMetaData m_torrentMetaData;

    boost::asio::io_context &m_io;

    std::shared_ptr<Mediator> m_mediator;

    std::shared_ptr<boost::asio::ip::tcp::socket> m_socket;

    utils::Peer m_peer;

    bool initiateHandshake();

    std::shared_ptr<Connection> getSharedPtr(); 
};