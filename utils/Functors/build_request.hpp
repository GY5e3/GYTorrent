#pragma once

#include <vector>
#include <string>

namespace utils
{
    class build_request
    {
    public:
        template <typename integer, typename = std::enable_if_t<std::is_integral_v<integer>>>
        void operator()(std::vector<unsigned char> &request, size_t offset, integer value) const
        {
            if (offset + sizeof(integer) > request.size())
                throw std::out_of_range("Bad offset");

            for (size_t i = 0; i < sizeof(integer); i++)
                request[offset + i] = (value >> (8 * (sizeof(integer) - 1 - i))) & 0xFF;
        }
        template <typename iterator>
        void operator()(std::vector<unsigned char>& request, size_t offset, iterator begin, iterator end) const
        {
            size_t dataSize = std::distance(begin, end);
            if (offset + dataSize > request.size())
                throw std::out_of_range("Bad offset");

            std::copy(begin, end, request.begin() + offset);
        }
    };
}
