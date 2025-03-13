#pragma once

#include <iostream>
#include <memory>

#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>

#include "NetworkAction.hpp"

#include "Entities/Message.hpp"

class SessionManager : std::enable_shared_from_this<SessionManager>
{
public:
    SessionManager(boost::asio::io_context &io,
                   std::function<void(const std::string &peer, const utils::Message&, bool isIncoming, boost::system::error_code)> callback);

    void Start(const std::string &peer, NetworkAction interact);

    void SendMessage(const std::string &peer, const utils::Message& message);

    void AwaitMessage(const std::string &peer);

    void Update(const std::string &peer);

    void Stop(const std::string &peer);

private:
    utils::build_request build_request;

    boost::asio::io_context &m_io;

    struct Session
    {
    public:
        bool IsChokeMe = true;
        bool IChokeIt = true;
        bool IsInterested = false;
        bool AmInterested = false;

        NetworkAction Interact;

        std::vector<bool> BitField;

        Session(boost::asio::io_context &io, NetworkAction interact) : Interact(interact), KeepAliveTimer(io) {}

        boost::asio::steady_timer KeepAliveTimer;
    };

    std::unordered_map<std::string, Session> m_temp;

    std::function<void(const std::string&, const utils::Message&, bool isIncoming, boost::system::error_code)> m_callback;
};
