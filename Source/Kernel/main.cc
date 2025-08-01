// Ikaros 3.0

#include "ikaros.h"

using namespace ikaros;

extern bool global_terminate;

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

        o.add_option("b", "batch_mode", "start automatically and quit when execution terminates");
        o.add_option("d", "tick_duration", "duration of each tick");
        o.add_option("i", "info", "print model info");
        o.add_option("r", "real_time", "run in real-time mode");
        o.add_option("S", "start", " start-up automatically without waiting for commands from WebUI");
        o.add_option("s", "stop", "stop Ikaros after this tick", "-1");
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
        k.InitSocket(o.get_long("webui_port"));

        while(k.run_mode != run_mode_quit && !global_terminate)
        {
            try
            {
                if(k.options_.filename().empty())
                    k.New();
                else if(k.needs_reload)
                    k.LoadFile();

                if(k.info_.is_set("start"))
                {
                    if(k.info_.is_set("real_time"))
                        k.run_mode = run_mode_realtime;
                    else
                        k.run_mode = run_mode_play;
                }
                k.Run();
            }

            catch(load_failed & e)
            {
                std::cout << "Load failed. "+e.message() << std::endl;
                k.options_.path_.clear();
            }

            catch(fatal_error & e)
            {
                std::cerr << "Ikaros:: Fatal error: " << e.what() << std::endl;
                if(o.is_set("batch_mode"))
                    exit(1);
                else
                {   
                    // Assuming load failed or other error that can be handled by WebUI
                    // Stop if running but retain data for WebUI
                }                    
            }
        }
    }
    catch(std::exception & e)
    {
        std::cerr << std::string(e.what()) << std::endl;
        exit(1);
    }
    catch(...)
    {
        std::cout << "Ikaros: Internal Error" << std::endl;
        exit(1);
    }

    std::cout << "\nIkaros 3.0 Ended" << std::endl;
    return 0;
}

