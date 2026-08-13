#pragma once

#include <string>
#include <string_view>

namespace ikaros
{
    [[nodiscard]] bool contains_hash_image_sequence_format(std::string_view pattern);
    [[nodiscard]] std::string format_hash_image_sequence_filename(
        std::string_view pattern, int image_index);
    void validate_hash_image_sequence_filecount(std::string_view pattern, int filecount);
}
