#pragma once

#include <random>

namespace utils
{
    /*
    @brief Generate PeerID for client
    @return PeerID in the format: -PC0001-dddddddddddd, where 'd' - digit in the range from 0 to 9
    */
    class GeneratorPeerID
    {
    public:
        std::string operator()()
        {
            std::string peerID = "-PC0001-";

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 9);

            for (int i = 0; i < 12; i++)
            {
                peerID += std::to_string(dis(gen));
            }

            return peerID;
        }
    };
}