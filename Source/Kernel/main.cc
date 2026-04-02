// Ikaros 3.0

#include "ikaros.h"
#include <atomic>

using namespace ikaros;

extern std::atomic<bool> global_terminate;

int
main(int argc, char *argv[])
{
    bool debug_mode = false;
#if DEBUG
    debug_mode = true;
#endif
    try
    { 
        Kernel & k = kernel();
        options o;

        //o.add_option("l", "loglevel", "what to print to the log");
        //o.add_option("q", "quiet", "do not print log to terminal; equal to loglevel=0");
        //o.add_option("c", "lagcutoff", "reset lag and restart timing if it exceed this value", "10s");

        o.add_option("b", "batch_mode", "start automatically and quit when execution terminates; no WebUI unless explicitly set with -w");
        o.add_option("d", "tick_duration", "duration of each tick");
        o.add_option("i", "info", "print model info");
        o.add_option("r", "real_time", "run in real-time mode; also implies S");
        o.add_option("S", "start", " start-up automatically without waiting for commands from WebUI");
        o.add_option("s", "stop", "stop Ikaros after this tick", "-1");
        o.add_option("p", "python_executable", "default Python interpreter for python-backed classes");
        o.add_option("w", "webui_port", "port for ikaros WebUI", "8000");
        o.add_option("h", "help", "list command line options");
        o.add_option("x", "experimental", "run with experimental features");

        o.parse_args(argc, argv);
        if(o.is_set("help"))
        {
            o.print_help();
            return 0;
        }

        k.webui_dir = o.ikaros_root+"/Source/WebUI/";   // FIXME: Use consistent file paths without "/" at end
        k.user_dir = o.ikaros_root+"/UserData/";    // FIXME: Use consistent file paths without "/" at end
        k.ScanClasses(o.ikaros_root+"/Source/Modules");
        k.ScanClasses(o.ikaros_root+"/Source/UserModules"); // FIXME: Can probably be removed here

        std::filesystem::current_path(k.user_dir);

        if(o.is_set("batch_mode"))
            o.set("start");

#if DEBUG
        std::cout << "Ikaros 3.0 Starting (Debug)\n" << std::endl;
#else
        std::cout << "Ikaros 3.0 Starting\n" << std::endl;
#endif

        k.options_ = o;
        k.LogProcessStart();
        bool socket_initialized = false;
        auto shutdown_batch_http = [&]()
        {
            if(socket_initialized)
            {
                k.StopHTTPServer();
                socket_initialized = false;
            }
        };
        auto shutdown_and_exit = [&](int code) -> int
        {
            k.LogProcessExit();
            shutdown_batch_http();
            return code;
        };

    while(k.run_mode.load() != run_mode_quit && !global_terminate.load())
        {
            try
            {
                if(k.options_.filename().empty())
                    k.New();
                else if(k.needs_reload)
                    k.LoadFile();

                bool should_start_socket =
                    !o.is_set("batch_mode")
                    || o.is_explicitly_set("webui_port")
                    || k.info_.contains("webui_port");

                if(should_start_socket && !socket_initialized)
                {
                    long port = k.options_.get_long("webui_port");
                    if(k.info_.contains("webui_port"))
                        port = long(k.info_["webui_port"]);
                    k.InitSocket(port);
                    socket_initialized = true;
                }

                if(o.is_set("batch_mode"))
                    k.info_["start"] = true;

                if(k.options_.filename().empty() && o.is_set("batch_mode") && k.info_.contains("stop") && long(k.info_["stop"]) == 0)
                {
                    k.run_mode = run_mode_quit;
                    continue;
                }

                if(k.info_.is_set("real_time"))
                    k.info_["start"] = true;
            
                if(k.info_.is_set("start"))
                {
                    if(k.info_.is_set("real_time"))
                        k.Realtime();
                        //k.run_mode = run_mode_realtime;
                    else
                        k.Play();
                       // k.run_mode = run_mode_play;
                }
                k.Run();

                if(k.options_.filename().empty() && o.is_set("batch_mode"))
                    k.run_mode = run_mode_quit;
            }

            catch(load_failed & e)
            {
                std::cout << "Load failed. "+e.message() << std::endl;
                if(o.is_set("batch_mode"))
                {
                    return shutdown_and_exit(1);
                }
                k.options_.path_.clear();
            }

            catch(fatal_error & e)
            {
                std::cerr << "Ikaros:: Fatal error: " << e.what() << std::endl;
                if(o.is_set("batch_mode"))
                {
                    return shutdown_and_exit(1);
                }
                else
                {   
                    // Assuming load failed or other error that can be handled by WebUI
                    // Stop if running but retain data for WebUI
                }                    
            }
        }

        k.LogProcessExit();
        shutdown_batch_http();
        std::cout << "\nIkaros 3.0 Ended" << std::endl;
        return 0;
    }
    catch(std::exception & e)
    {
        kernel().LogProcessExit();
        std::cerr << std::string(e.what()) << std::endl;
        return 1;
    }
    catch(...)
    {
        kernel().LogProcessExit();
        std::cout << "Ikaros: Internal Error" << std::endl;
        return 1;
    }
}
