#include "Mediator.hpp"

Mediator::Mediator(TorrentMetaData torrentMetaData,
                   std::shared_ptr<PieceManager> pieceManager) : m_torrentMetaData(torrentMetaData),
                                                                 m_pieceManager(pieceManager) {}
//

void Mediator::MessageHandler(std::vector<unsigned char> messageBuffer,
                              std::shared_ptr<Connection> connection,
                              boost::system::error_code errorCode)
{
    if (errorCode)
    {
        std::cout << connection->GetPeer().ToString() + ": " << errorCode.message() << std::endl;
        m_activeSockets.erase(connection);
        m_isChoked.erase(connection);
        m_isBusy.erase(connection);
        m_hasPiece.erase(connection);
        return;
    }
    int messageID = messageBuffer.size() ? static_cast<int>(messageBuffer[0]) : utils::MessageID::keepAlive;
    std::ostringstream os;
    switch (messageID)
    {
    case utils::MessageID::keepAlive:
        std::cout << connection->GetPeer().ToString() + " keep alive with me" << std::endl;
        break;
    case utils::MessageID::choke:
        os << connection->GetPeer().ToString() + " choke me" << std::endl;
        std::cout << os.str();

        m_isChoked[connection] = true;
        break;
    case utils::MessageID::unchoke:
        os << connection->GetPeer().ToString() + " unchoke me" << std::endl;
        std::cout << os.str();

        m_isChoked[connection] = false;
        break;
    case utils::MessageID::interested:
        /// TODO: send unchoke message
        break;
    case utils::MessageID::notInterested:
        /// TODO: send choke message, probably
        break;
    case utils::MessageID::have:
    {
        int32_t pieceInd = (static_cast<int32_t>(messageBuffer[1]) << 24) |
                           (static_cast<int32_t>(messageBuffer[2]) << 16) |
                           (static_cast<int32_t>(messageBuffer[3]) << 8) |
                           static_cast<int32_t>(messageBuffer[4]);
        os << connection->GetPeer().ToString() + " have piece" << ": " << pieceInd << std::endl;
        std::cout << os.str();
        /// TODO: update bitfield info
        break;
    }
    case utils::MessageID::bitField:
        os << "BitField by " + connection->GetPeer().ToString() << ":";
        for (size_t i = 1; i < messageBuffer.size(); i++)
        {
            unsigned char byte = messageBuffer[i];
            for (int i = 7; i >= 0; --i)
            {
                m_hasPiece[connection].push_back(byte & (1 << i));
            }
        }
        for (auto mes : m_hasPiece[connection])
        {
            os << " " << mes;
        }
        os << std::dec << "\n";
        std::cout << os.str();

        break;
    case utils::MessageID::request:
        /// TODO: send it to PieceManager-chan
        break;
    case utils::MessageID::piece:
    {
        int32_t pieceIndex = (static_cast<int32_t>(messageBuffer[1]) << 24) |
                             (static_cast<int32_t>(messageBuffer[2]) << 16) |
                             (static_cast<int32_t>(messageBuffer[3]) << 8) |
                             static_cast<int32_t>(messageBuffer[4]);

        int32_t blockOffset = (static_cast<int32_t>(messageBuffer[5]) << 24) |
                              (static_cast<int32_t>(messageBuffer[6]) << 16) |
                              (static_cast<int32_t>(messageBuffer[7]) << 8) |
                              static_cast<int32_t>(messageBuffer[8]);

        std::copy(messageBuffer.begin() + 9, messageBuffer.end(), m_downloadingPieces[pieceIndex].first.begin() + blockOffset);

          //   std::cout << connection->GetPeer().ToString() +
          //                    " send to me piece" + std::to_string(pieceIndex) +
          //                    " block offset " + std::to_string(blockOffset) +
          //                    " left blocks: " + std::to_string(m_downloadingPieces[pieceIndex].second - 1)
          //             << std::endl;

        m_activeRequests[connection]--;

        if (--m_downloadingPieces[pieceIndex].second == 0)
        {
            std::cout << "Piece " << pieceIndex << " download is completed!" << std::endl;

            std::vector<unsigned char> hash(SHA_DIGEST_LENGTH);

            SHA1(m_downloadingPieces[pieceIndex].first.data(), m_downloadingPieces[pieceIndex].first.size(), hash.data());

            std::string hashStr(begin(hash), end(hash));
            if (hashStr == m_torrentMetaData.GetPieceHash(pieceIndex))
            {
                std::cout << "Noice hashes!!!!" << std::endl;
                m_isBusy[connection] = false;

                m_pieceManager->PieceRecieve(pieceIndex, m_downloadingPieces[pieceIndex].first);
                m_downloadingPieces.erase(pieceIndex);
            }
            else
            {
                std::cout << "Bad hashes!!!!" << std::endl;
            }
        }
        break;
    }
    case utils::MessageID::cancel:
        /// TODO: dunno what to do with it
        break;
    default:
        break;
    }
}

bool Mediator::Send(utils::MessageID messageID, Piece piece, std::shared_ptr<PieceManager> pieceManager)
{
    if (m_activeSockets.size() == 0)
        return false;

    std::shared_ptr<Connection> unchokeConnection;

    std::mutex m_mutex;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto connection : m_activeSockets)
        {
            if (!m_isChoked[connection] && !m_isBusy[connection] && m_hasPiece[connection][piece.m_pieceIndex] &&
                connection->GetPeer().ToString() != "80.64.170.90 6889")
            {
                unchokeConnection = connection;
            }
        }
    }
    if (unchokeConnection == nullptr)
    {
        return false;
    }
    m_isBusy[unchokeConnection] = true;
    std::thread([this, messageID, piece, pieceManager, unchokeConnection]()
                {
        int32_t currentSize = std::min(static_cast<int64_t>(m_torrentMetaData.GetPieceSize()),
                                       m_torrentMetaData.GetTotalSize() - piece.m_offset);
        int32_t blockCount = currentSize / utils::BLOCK_SIZE + (currentSize % utils::BLOCK_SIZE > 0);
        m_downloadingPieces.insert({piece.m_pieceIndex, {std::vector<unsigned char>(currentSize), blockCount}});

        for (int32_t i = 0, j = 0; i < blockCount; i++) {
            if(m_activeRequests.find(unchokeConnection) == end(m_activeRequests))
                return false;
            std::vector<int32_t> payload{piece.m_pieceIndex,
                                         i * utils::BLOCK_SIZE,
                                         std::min(utils::BLOCK_SIZE, currentSize - i * utils::BLOCK_SIZE)};
            while(m_activeRequests[unchokeConnection] >= max_concurrent_requests);
            if(m_activeRequests[unchokeConnection] < max_concurrent_requests) 
            {
                m_activeRequests[unchokeConnection]++;
                unchokeConnection->SendMessage(utils::MessageID::request, payload);
            }
        }

        return true; })
        .detach();

    return true;
}

void Mediator::AddActiveConnection(std::shared_ptr<Connection> connection)
{
    m_activeSockets.insert(connection);
    m_isChoked.insert({connection, true});
    m_isBusy.insert({connection, false});
    m_activeRequests.insert({connection, 0});

}

void Mediator::RemoveConnection(std::shared_ptr<Connection> connection)
{
    m_activeSockets.erase(connection);
    m_isChoked.erase(connection);
    m_isBusy.erase(connection);
    m_activeRequests.erase(connection);
}