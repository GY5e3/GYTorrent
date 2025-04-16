#pragma once

#include <iostream>
#include <memory>
#include <queue>

#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>

#include "NetworkAction.hpp"

#include "CustomNetworkErrors/torrent_error.hpp"
#include "Entities/Message.hpp"

class SessionManager : std::enable_shared_from_this<SessionManager>
{
public:
    SessionManager(boost::asio::io_context &io,
                   std::function<void(const std::string &peer, const utils::Message &, bool isIncoming, boost::system::error_code)> callback);

    void Start(const std::string &peer, NetworkAction interact);

    void SendMessage(const std::string &peer, const utils::Message &message);

    void AwaitMessage(const std::string &peer);

    void Update(const std::string &peer);

    void Stop(const std::string &peer);

    std::string GetAvailablePeer(int32_t pieceIndex, int32_t requstedBlocksCount) const;

    std::vector<utils::Message> GetRequestedBlocks(const std::string &peer) const;

    bool IsActive(const std::string &peer) const;

private:
    utils::build_request build_request;

    boost::asio::io_context &m_io;

    struct Session
    {
    private:
        struct PairHash
        {
            template <typename T1, typename T2>
            std::size_t operator()(const std::pair<T1, T2> &p) const
            {
                std::size_t h1 = std::hash<T1>{}(p.first);
                std::size_t h2 = std::hash<T2>{}(p.second);
                return h1 ^ (h2 << 1);
            }
        };

    public:
        bool IsChokeMe = true;
        bool IChokeIt = true;
        bool IsInterested = false;
        bool AmInterested = false;

        NetworkAction Interact;

        boost::asio::steady_timer KeepAliveTimer;
        std::vector<bool> BitField;
        std::unordered_map<std::pair<int32_t, int32_t>, std::shared_ptr<boost::asio::steady_timer>, PairHash> RequestedBlocks;

        Session(boost::asio::io_context &io, NetworkAction interact) : Interact(interact), KeepAliveTimer(io) {}        
    };

    std::unordered_map<std::string, Session> m_sessions;

    std::function<void(const std::string &, const utils::Message &, bool isIncoming, boost::system::error_code)> m_callback;

    void awaitBlock(const std::string &peer, int32_t pieceIndex, int32_t offset);
};
