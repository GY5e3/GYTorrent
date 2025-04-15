#include "SessionManager.hpp"

using callback_function = std::function<void(const std::string &, const utils::Message &, bool isIncoming, boost::system::error_code)>;

SessionManager::SessionManager(boost::asio::io_context &io, callback_function callback) : m_io(io),
                                                                                          m_callback(callback) {}

void SessionManager::Start(const std::string &peer, NetworkAction communication)
{
    m_sessions.insert({peer, Session{m_io, communication}});

    AwaitMessage(peer);
}

void SessionManager::SendMessage(const std::string &peer, const utils::Message &message)
{
    auto it = m_sessions.find(peer);
    if (it == m_sessions.end())
    {
        /// TODO:
        // auto ec = make_error_code(utils::torrent_errc::custom_error);
        // m_callback(peer, {}, false, ec);
        return;
    }
    std::vector<unsigned char> data;
    switch (message.MessageID)
    {
    case utils::MessageID::keepAlive:
    {
        int32_t mesLen = 0;
        data.resize(4);
        build_request(data, 0, mesLen);
        break;
    }
    case utils::MessageID::choke:
    case utils::MessageID::unchoke:
    case utils::MessageID::interested:
    case utils::MessageID::notInterested:
    {
        int32_t mesLen = 1;
        data.resize(4 + mesLen);
        build_request(data, 0, mesLen);
        data[4] = static_cast<int32_t>(message.MessageID);
        break;
    }
    case utils::MessageID::have:
    {
        int32_t mesLen = 5;
        data.resize(4 + mesLen);
        build_request(data, 0, mesLen);
        data[4] = static_cast<int32_t>(message.MessageID);
        build_request(data, 5, mesLen);
        break;
    }
    case utils::MessageID::bitField:
    {
        int32_t mesLen = 1 + message.BitField.size();
        data.resize(4 + mesLen);
        build_request(data, 0, mesLen);
        data[4] = static_cast<int32_t>(message.MessageID);
        build_request(data, 5, begin(message.BitField), end(message.BitField));
        break;
    }
    case utils::MessageID::request:
    {
        awaitBlock(peer, message.PieceIndex, message.Offset);
    }
    case utils::MessageID::cancel:
    {
        int32_t mesLen = 13;
        data.resize(4 + mesLen);
        build_request(data, 0, mesLen);
        data[4] = static_cast<int32_t>(message.MessageID);
        build_request(data, 5, message.PieceIndex);
        build_request(data, 9, message.Offset);
        build_request(data, 13, message.Length);
        break;
    }
    case utils::MessageID::piece:
    {
        int32_t mesLen = 9 + message.Block.size();
        data.resize(4 + mesLen);
        build_request(data, 0, mesLen);
        data[4] = static_cast<int32_t>(message.MessageID);
        build_request(data, 5, message.PieceIndex);
        build_request(data, 9, message.Offset);
        build_request(data, 13, begin(message.BitField), end(message.BitField));
        break;
    }
    case utils::MessageID::port:
    {
        int32_t mesLen = 3;
        data.resize(4 + mesLen);
        build_request(data, 0, mesLen);
        data[4] = static_cast<int32_t>(message.MessageID);
        build_request(data, 5, message.Port);
        break;
    }
    }
    boost::asio::spawn(m_io, [this, it, peer, message, data = std::move(data)](boost::asio::yield_context yield)
    {
        boost::system::error_code ec;
        it->second.Interact.Send(yield, data, ec);
        if(ec)
        {
            m_callback(peer, {}, false, ec);
            return;
        }
        m_sessions.find(peer)->second.KeepAliveTimer.cancel();
        m_callback(peer, message, false, ec);
        Update(peer); 
    });
}

void SessionManager::AwaitMessage(const std::string &peer)
{
    auto it = m_sessions.find(peer);
    if (it == m_sessions.end())
    {
        /// TODO:
        // auto ec = make_error_code(utils::torrent_errc::custom_error);
        // m_callback(peer, {}, false, ec);
        return;
    }
    boost::asio::spawn(m_io, [this, it, peer](boost::asio::yield_context yield)
    {
        boost::system::error_code ec;
        
        while (true)
        {
            auto temp = it->second.Interact.Recieve(yield, ec);
            if (ec)
            {
                m_callback(peer, {}, true, ec);
                break;
            }

            utils::Message incoming;
            incoming.MessageID = temp.empty() ? utils::MessageID::keepAlive : static_cast<utils::MessageID>(temp[0]);
            switch (incoming.MessageID)
            {
            case utils::MessageID::choke:
            {
                it->second.IsChokeMe = true;
                break;
            }
            case utils::MessageID::unchoke:
            {
                it->second.IsChokeMe = false;
                break;
            }
            case utils::MessageID::interested:
            {
               it->second.IsInterested = true;
               break;
            }
            case utils::MessageID::notInterested:
            {
               it->second.IsInterested = false;
               break;
            }
            case utils::MessageID::have:
            {
                incoming.PieceIndex = (static_cast<int32_t>(temp[1]) << 24) |
                                      (static_cast<int32_t>(temp[2]) << 16) |
                                      (static_cast<int32_t>(temp[3]) << 8)  |
                                       static_cast<int32_t>(temp[4]);

                if (it->second.BitField.size() > incoming.PieceIndex)
                    it->second.BitField[incoming.PieceIndex] = true;
                break;
            }
            case utils::MessageID::bitField:
            {
                for (size_t i = 1; i < temp.size(); i++)
                {
                    unsigned char byte = temp[i];
                    for (int j = 7; j >= 0; j--)
                        incoming.BitField.push_back(byte & (1 << j));
                }
                it->second.BitField.assign(begin(incoming.BitField), end(incoming.BitField));
                break;
            }
            case utils::MessageID::request:
            case utils::MessageID::cancel:
            {
                incoming.PieceIndex = (static_cast<int32_t>(temp[1]) << 24) |
                                      (static_cast<int32_t>(temp[2]) << 16) |
                                      (static_cast<int32_t>(temp[3]) << 8)  |
                                       static_cast<int32_t>(temp[4]);

                incoming.Offset = (static_cast<int32_t>(temp[5]) << 24) |
                                  (static_cast<int32_t>(temp[6]) << 16) |
                                  (static_cast<int32_t>(temp[7]) << 8)  |
                                   static_cast<int32_t>(temp[8]);

                incoming.Length = (static_cast<int32_t>(temp[9]) << 24)  |
                                  (static_cast<int32_t>(temp[10]) << 16) |
                                  (static_cast<int32_t>(temp[11]) << 8)  |
                                   static_cast<int32_t>(temp[12]);
                break;
            }
            case utils::MessageID::piece:
            {
                incoming.PieceIndex = (static_cast<int32_t>(temp[1]) << 24) |
                                      (static_cast<int32_t>(temp[2]) << 16) |
                                      (static_cast<int32_t>(temp[3]) << 8)  |
                                       static_cast<int32_t>(temp[4]);

                incoming.Offset = (static_cast<int32_t>(temp[5]) << 24) |
                                  (static_cast<int32_t>(temp[6]) << 16) |
                                  (static_cast<int32_t>(temp[7]) << 8)  |
                                   static_cast<int32_t>(temp[8]);
                it->second.RequestedBlocks[{incoming.PieceIndex, incoming.Offset}]->cancel();

                incoming.Length = temp.size() - 9;
                incoming.Block.assign(temp.begin() + 9, temp.end());
                break;
            }
            case utils::MessageID::port:
            {
                incoming.Port = (static_cast<int16_t>(temp[1]) << 8) |
                                 static_cast<int16_t>(temp[2]);
                break;
            }
            }
            m_callback(peer, incoming, true, ec);
        } 
    });
}

void SessionManager::Update(const std::string &peer)
{
    auto it = m_sessions.find(peer);
    if (it == m_sessions.end())
    {
        /// TODO:
        // auto ec = make_error_code(utils::torrent_errc::custom_error);
        // m_callback(peer, {}, false, ec);
        return;
    }
    boost::asio::spawn(m_io, [this, it, peer](boost::asio::yield_context yield)
    {
        boost::system::error_code ec;
        
        while (true)
        {
            it->second.KeepAliveTimer.expires_after(std::chrono::seconds(120));
            it->second.KeepAliveTimer.async_wait(yield[ec]);
            if (ec) break; 

            SendMessage(peer, utils::Message{}); 
        } 
    });
}

void SessionManager::Stop(const std::string &peer)
{
    auto it = m_sessions.find(peer);
    if (it == m_sessions.end())
    {
        /// TODO:
        // auto ec = make_error_code(utils::torrent_errc::custom_error);
        // m_callback(peer, {}, false, ec);
        return;
    }

    it->second.KeepAliveTimer.cancel();
    for (auto timers : it->second.RequestedBlocks)
    {
        timers.second->cancel();

        boost::system::error_code torrentEC = make_error_code(torrent_errc::block_request_timeout);
        m_callback(peer, {}, false, torrentEC);
    }

    m_sessions.erase(peer);
}

void SessionManager::awaitBlock(const std::string &peer, int32_t pieceIndex, int32_t offset)
{
    auto blockTimer = std::make_shared<boost::asio::steady_timer>(m_io);
    m_sessions.find(peer)->second.RequestedBlocks.insert({{pieceIndex, offset}, blockTimer});
    boost::asio::spawn(m_io, [this, blockTimer, peer, pieceIndex, offset](boost::asio::yield_context yield)
    {
        boost::system::error_code timerEC;
        blockTimer->expires_after(std::chrono::seconds(10));
        blockTimer->async_wait(yield[timerEC]);
        m_sessions.find(peer)->second.RequestedBlocks.erase({pieceIndex, offset});
        if(!timerEC)
        {
            boost::system::error_code torrentEC = make_error_code(torrent_errc::block_request_timeout);
            m_callback(peer, {}, false, torrentEC);

            utils::Message message;
            message.MessageID = utils::MessageID::cancel;
            message.PieceIndex = pieceIndex;
            message.Offset = offset;
            SendMessage(peer, message);
        } 
    });
}

std::string SessionManager::GetAvailablePeer(int32_t pieceIndex, int32_t requestedBlocksCount) const
{
    auto comparator = [](const auto &a, const auto &b)
    {
        if (a.second != b.second)
            return a.second < b.second;
        return rand() % 2 == 0;
    };

    std::priority_queue<std::pair<std::string, int32_t>,
                        std::vector<std::pair<std::string, int32_t>>,
                        decltype(comparator)>
        q(comparator);

    for (const auto &session : m_sessions)
    {
        if (session.second.IsChokeMe == false &&
            session.second.BitField[pieceIndex] == true &&
            session.second.RequestedBlocks.size() < requestedBlocksCount)
        {
            q.push({session.first, session.second.RequestedBlocks.size()});
        }
    }
    return q.size() ? q.top().first : "";
}

bool SessionManager::IsActive(const std::string &peer) const
{
    return m_sessions.find(peer) != end(m_sessions);
}