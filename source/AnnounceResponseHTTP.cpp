#include "AnnounceResponseHTTP.hpp"

AnnounceResponseHTTP::AnnounceResponseHTTP() = default;

AnnounceResponseHTTP::AnnounceResponseHTTP(const std::string &responseBody)
{
    bencode::FunctorDecode decode;
    bencode::bencode_object bResponse = decode(responseBody);

    auto data = bResponse.GetDict();

    m_complete = data["complete"].GetInteger();

    m_incomplete = data["incomlete"].GetInteger();

    m_interval = data["interval"].GetInteger();

    for (const auto &peer : data["peers"].GetList())
    {
        auto info = peer.GetDict();
        m_peers.push_back(utils::Peer{info["ip"].GetString(),
                                      std::to_string(info["port"].GetInteger()),
                                      info["peer id"].GetString()});
    }
    ///TODO: need to add logic for extract peers from compact response
}