#include "SessionManager.hpp"

using callback_function = std::function<void(const std::string &, const utils::Message &, bool isIncoming, boost::system::error_code)>;

SessionManager::SessionManager(boost::asio::io_context &io, callback_function callback) : m_io(io),
                                                                                          m_callback(callback) {}

void SessionManager::Start(const std::string &peer, NetworkAction communication)
{
    m_temp.insert({peer, Session{m_io, communication}});

    AwaitMessage(peer);
}

void SessionManager::SendMessage(const std::string &peer, const utils::Message &message)
{
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
    boost::asio::spawn(m_io, [this, peer, message, data](boost::asio::yield_context yield)
    {
        boost::system::error_code ec;
        m_temp.find(peer)->second.Interact.Send(yield, data, ec);
        if(ec)
        {
            m_callback(peer, {}, false, ec);
            return;
        }
        m_temp.find(peer)->second.KeepAliveTimer.cancel();
        m_callback(peer, message, false, ec);
        Update(peer); 
    });
}

void SessionManager::AwaitMessage(const std::string &peer)
{
    boost::asio::spawn(m_io, [this, peer](boost::asio::yield_context yield)
    {
        boost::system::error_code ec;
        auto it = m_temp.find(peer);
        if (it == m_temp.end()) return;
        while (true) 
        {
            auto temp = it->second.Interact.Recieve(yield, ec);
            if (ec)
            {
                m_callback(peer, {}, true, ec);
                break; 
            }

            it->second.KeepAliveTimer.cancel();
            utils::Message incoming;
            incoming.MessageID = temp.empty() ? utils::MessageID::keepAlive : static_cast<utils::MessageID>(temp[0]);

            switch (incoming.MessageID)
            {
            case utils::MessageID::have:
            {
                incoming.PieceIndex = (static_cast<int32_t>(temp[1]) << 24) |
                                      (static_cast<int32_t>(temp[2]) << 16) |
                                      (static_cast<int32_t>(temp[3]) << 8)  |
                                       static_cast<int32_t>(temp[4]);
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
                
                incoming.Length = (static_cast<int32_t>(temp[9])  << 24)  |
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
            
            Update(peer); 
        }
    
    });
}

void SessionManager::Update(const std::string &peer)
{
    boost::asio::spawn(m_io, [this, peer](boost::asio::yield_context yield)
    {
        boost::system::error_code ec;
        auto it = m_temp.find(peer);
        if (it == m_temp.end()) return; 
        while (true)
        {
            it->second.KeepAliveTimer.expires_after(std::chrono::seconds(60));
            it->second.KeepAliveTimer.async_wait(yield[ec]);
            if (ec) break; 

            SendMessage(peer, utils::Message{}); 
        }
    });
}

void SessionManager::Stop(const std::string &peer)
{
    m_temp.find(peer)->second.KeepAliveTimer.cancel();

    m_temp.erase(peer);
}