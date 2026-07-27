// Ikaros 3.0

#include "ikaros.h"

using namespace ikaros;

namespace ikaros
{
    // Request

    Request::Request(std::string uri, long sid, std::string b, std::string content_type, long cid):
        body(b)
    {
        session_id = sid;
        client_id = cid;
        std::string params = tail(uri, "?");
        url = decode_url_component(uri);

        if(!uri.empty() && uri[0] == '/')
            uri.erase(0, 1);
        uri = decode_url_component(uri);
        command = head(uri, "/");
        component_path = uri;
        parameters.parse_url(params);

        if(!body.empty())
        {
            if(!starts_with(content_type, "application/json"))
                throw exception("Request body must have Content-Type application/json.");

            json_body = parse_json(body);
        }
    }

    bool
    Request::HasJsonBody() const
    {
        return !json_body.is_null();
    }

    void
    Request::MergeJsonBodyIntoParameters(bool overwrite)
    {
        if(!HasJsonBody())
            return;
        if(!json_body.is_dictionary())
            throw exception("JSON request body must be an object.");
        parameters.merge(json_body.as_dictionary(), overwrite);
    }

bool operator==(Request & r, const std::string s)
{
    return r.command == s;
}

}; // namespace ikaros
