#pragma once

#include <fstream>
#include <sstream>
#include <iostream>

#include "Abstraction/AnnounceResponse.hpp"

class AnnounceResponseUDP : public AnnounceResponse
{
public:
    AnnounceResponseUDP();

    AnnounceResponseUDP(const std::vector<unsigned char>& responseBody);
};

