#pragma once

#include <utility>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <memory>
#include <enable_shared_from_this.hpp>

#include "TorrentMetaData.hpp"
#include "Piece.hpp"
#include "utils.hpp"
#include "Mediator.hpp"

class Mediator;

class PieceManager : public std::enable_shared_from_this<PieceManager>
{
public:
    PieceManager(int piecesCount, TorrentMetaData torrentMetaData);

    void PieceRequest();

    void PieceRecieve(int pieceIndex, std::vector<unsigned char> data);

    void SetMediator(std::shared_ptr<Mediator> mediator);

    bool isDownloading;
private:
    TorrentMetaData m_torrentMetaData;

    std::shared_ptr<Mediator> m_mediator;

    std::vector<Piece> m_pieces;

    std::unordered_set<int32_t> m_missingPieces;

    std::unordered_set<int32_t> m_downloadedPieces;

    std::shared_ptr<PieceManager> getSharedPtr();

    int downloaded;
};