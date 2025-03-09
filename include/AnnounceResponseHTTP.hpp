#pragma once

#include "Abstraction/AnnounceResponse.hpp"

#include "Functors/bencode.hpp"

class AnnounceResponseHTTP : public AnnounceResponse
{
public:
    AnnounceResponseHTTP();
    AnnounceResponseHTTP(const std::string& responseBody);
};
