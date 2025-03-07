#pragma once

#include "Abstraction/AnnounceResponse.hpp"

#include "../utils/Functors/bencode.hpp"

class AnnounceResponseHTTP : public AnnounceResponse
{
public:
    AnnounceResponseHTTP();
    AnnounceResponseHTTP(const std::string& responseBody);
};
