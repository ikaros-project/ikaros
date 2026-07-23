#include "MidiEndpointSelection.h"


namespace ikaros::midi
{
    namespace
    {
        EndpointMatch
        FindMatches(std::span<const EndpointNames> endpoints,
                    std::string_view requestedName, bool exact)
        {
            EndpointMatch result;
            std::size_t matchCount = 0;
            for(std::size_t index = 0; index < endpoints.size(); ++index)
            {
                const EndpointNames & endpoint = endpoints[index];
                const bool nameMatches = exact
                    ? endpoint.name == requestedName
                    : endpoint.name.find(requestedName) != std::string::npos;
                const bool displayNameMatches = exact
                    ? endpoint.displayName == requestedName
                    : endpoint.displayName.find(requestedName) !=
                          std::string::npos;
                if(!nameMatches && !displayNameMatches)
                    continue;

                result.index = index;
                ++matchCount;
            }

            if(matchCount == 1)
                result.status = EndpointMatchStatus::matched;
            else if(matchCount > 1)
                result.status = EndpointMatchStatus::ambiguous;
            return result;
        }
    }


    EndpointMatch
    matchEndpoint(std::span<const EndpointNames> endpoints,
                  std::string_view requestedName)
    {
        if(requestedName.empty())
            return {};

        const EndpointMatch exact =
            FindMatches(endpoints, requestedName, true);
        if(exact.status != EndpointMatchStatus::notFound)
            return exact;
        return FindMatches(endpoints, requestedName, false);
    }
}
