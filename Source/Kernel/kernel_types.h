// Ikaros 3.0

#pragma once

namespace ikaros
{
    inline constexpr int run_mode_quit = 0;
    inline constexpr int run_mode_stop = 1;
    inline constexpr int run_mode_pause = 2;
    inline constexpr int run_mode_play = 3;
    inline constexpr int run_mode_realtime = 4;
    inline constexpr int run_mode_fast_forward = 5;
    inline constexpr int run_mode_restart = 6;

    inline constexpr int msg_inherit = 0;
    inline constexpr int msg_quiet = 1;
    inline constexpr int msg_exception = 2;
    inline constexpr int msg_end_of_file = 3;
    inline constexpr int msg_terminate = 4;
    inline constexpr int msg_fatal_error = 5;
    inline constexpr int msg_warning = 6;
    inline constexpr int msg_print = 7;
    inline constexpr int msg_debug = 8;
    inline constexpr int msg_trace = 9;

    using tick_count = long long int;
}
