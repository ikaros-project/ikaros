#pragma once

#include <string>
#include <string_view>

namespace ikaros
{
    [[nodiscard]] bool contains_image_sequence_format(std::string_view pattern);
    [[nodiscard]] std::string format_image_sequence_filename(std::string_view pattern,
                                                             int image_index);
}
