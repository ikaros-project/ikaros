// Ikaros 3.0

#pragma once

#include <string>

#include "dictionary.h"

namespace ikaros
{
    struct Request
    {
        long session_id;
        long client_id;
        dictionary parameters;
        value json_body;
        std::string url;
        std::string command;
        std::string component_path;
        std::string body;

        Request(std::string uri, long sid = 0, std::string body = "",
                std::string content_type = "", long cid = 0);
        bool HasJsonBody() const;
        void MergeJsonBodyIntoParameters(bool overwrite = true);
    };

    bool operator==(Request & request, const std::string value);
}
