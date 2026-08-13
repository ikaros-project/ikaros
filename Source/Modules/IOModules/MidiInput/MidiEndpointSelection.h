#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>


namespace ikaros::midi
{
    struct EndpointNames
    {
        std::string name;
        std::string displayName;
    };


    enum class EndpointMatchStatus
    {
        matched,
        notFound,
        ambiguous,
    };


    struct EndpointMatch
    {
        EndpointMatchStatus status = EndpointMatchStatus::notFound;
        std::size_t index = 0;
    };


    EndpointMatch matchEndpoint(std::span<const EndpointNames> endpoints,
                                std::string_view requestedName);
}
