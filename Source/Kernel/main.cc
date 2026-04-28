// Ikaros 3.0

#include "ikaros.h"
#include <atomic>
#include <cstdlib>

using namespace ikaros;

extern std::atomic<bool> global_terminate;

namespace
{
    std::string ResolveUserDirectory(const options & o)
    {
        std::filesystem::path user_path = o.ikaros_root + "/UserData/";
        if(o.is_explicitly_set("user_data"))
            user_path = std::filesystem::absolute(o.get("user_data"));

        user_path = user_path.lexically_normal();
        std::filesystem::create_directories(user_path);
        return user_path.string();
    }

    void ConfigureOptions(options & o)
    {
        o.add_option("b", "batch_mode", "start automatically and quit when execution terminates; no WebUI unless explicitly set with -w");
        o.add_option("d", "tick_duration", "duration of each tick");
        o.add_option("i", "info", "print model info");
        o.add_option("r", "real_time", "run in real-time mode; also implies S");
        o.add_option("S", "start", " start-up automatically without waiting for commands from WebUI");
        o.add_option("s", "stop", "stop Ikaros after this tick", "-1");
        o.add_option("p", "python_executable", "default Python interpreter for python-backed classes");
        o.add_option("t", "threads", "number of worker threads for the kernel thread pool");
        o.add_option("u", "user_data", "alternative directory for user data files");
        o.add_option("w", "webui_port", "port for ikaros WebUI", "8000");
        o.add_option("B", "bind_address", "bind WebUI/API server to a specific IPv4 address, for example 127.0.0.1");
        o.add_option("a", "auth_password", "enable optional WebUI/API authentication using the provided password");
        o.add_option("A", "agent", "set the agent identifier included in remote session logging");
        o.add_option("h", "help", "list command line options");
    }

    void InitializeKernelPaths(Kernel & k, const options & o)
    {
        k.webui_dir = o.ikaros_root+"/Source/WebUI/";
        k.user_dir = ResolveUserDirectory(o);
        k.ScanClasses(o.ikaros_root+"/Source/Modules");
        k.ScanClasses(o.ikaros_root+"/Source/UserModules");

        std::filesystem::current_path(k.user_dir);
    }

    void PrintStartupBanner()
    {
#if DEBUG
        std::cout << "Ikaros 3.0 Starting (Debug)\n\n";
#else
        std::cout << "Ikaros 3.0 Starting\n\n";
#endif
    }

    class MainLoopController
    {
    public:
        MainLoopController(Kernel & kernel, options & opts)
            : k(kernel), o(opts)
        {
            k.SetOptions(o);
            k.notify_stop_requested = false;
            k.process_exit_code = 0;
            k.LogProcessStart();
        }

        bool ShouldKeepRunning() const
        {
            return k.run_mode.load() != run_mode_quit && !global_terminate.load();
        }

        int Finish()
        {
            return Shutdown(k.process_exit_code.load(), true);
        }

        int FailFast(int code)
        {
            return Shutdown(code, false);
        }

        int HandleLoopException(const socket_startup_error & e)
        {
            LogLoopError("Ikaros error: " + e.message(), e.path());
            return FailFast(1);
        }

        int HandleLoopException(const exception & e)
        {
            return RecoverOrExit("Ikaros error: " + e.message(), e.path());
        }

        int HandleLoopException(const std::exception & e)
        {
            return RecoverOrExit("Standard exception: " + std::string(e.what()));
        }

        int HandleUnknownLoopException()
        {
            return RecoverOrExit("Unknown exception.");
        }

        template<typename Fn>
        int RunProtected(Fn && fn)
        {
            try
            {
                fn();
                return -1;
            }
            catch(const socket_startup_error & e)
            {
                return HandleLoopException(e);
            }
            catch(const exception & e)
            {
                return HandleLoopException(e);
            }
            catch(const std::exception & e)
            {
                return HandleLoopException(e);
            }
            catch(...)
            {
                return HandleUnknownLoopException();
            }
        }

        void RunIteration()
        {
            LoadModelIfNeeded();
            EnsureSocketStarted();
            ApplyAutoStartFlags();

            if(ShouldQuitEmptyBatchModel())
            {
                k.run_mode = run_mode_quit;
                return;
            }

            StartRequestedRunMode();
            k.Run();

            if(k.GetOptionFilename().empty() && o.is_set("batch_mode"))
                k.run_mode = run_mode_quit;
        }

    private:
        Kernel & k;
        options & o;
        bool socket_initialized = false;

        int Shutdown(int code, bool print_banner)
        {
            k.LogProcessExit();
            ShutdownHttp();
            if(print_banner)
                std::cout << "\nIkaros 3.0 Ended\n";
            return code;
        }

        void ShutdownHttp()
        {
            if(socket_initialized)
            {
                k.StopHTTPServer();
                socket_initialized = false;
            }
        }

        void LogLoopError(const std::string & message, const std::string & path = std::string())
        {
            try
            {
                k.Notify(msg_warning, message, path);
            }
            catch(...)
            {
                std::cerr << message;
                if(!path.empty())
                    std::cerr << " (" << path << ")";
                std::cerr << '\n';
            }
        }

        int RecoverOrExit(const std::string & message, const std::string & path = std::string())
        {
            LogLoopError(message, path);

            if(o.is_set("batch_mode"))
                return FailFast(1);

            try
            {
                k.Stop();
            }
            catch(const exception & e)
            {
                LogLoopError("Failed to stop after error: " + e.message(), e.path());
                k.run_mode = run_mode_stop;
                k.needs_reload = true;
            }
            catch(const std::exception & e)
            {
                LogLoopError("Failed to stop after error: " + std::string(e.what()));
                k.run_mode = run_mode_stop;
                k.needs_reload = true;
            }
            catch(...)
            {
                LogLoopError("Failed to stop after error: Unknown error.");
                k.run_mode = run_mode_stop;
                k.needs_reload = true;
            }

            return -1;
        }

        void LoadModelIfNeeded()
        {
            if(k.GetOptionFilename().empty())
                k.New();
            else if(k.needs_reload)
                k.LoadFile();
        }

        void EnsureSocketStarted()
        {
            bool should_start_socket =
                !o.is_set("batch_mode")
                || o.is_explicitly_set("webui_port")
                || k.info_.contains("webui_port");

            if(!should_start_socket || socket_initialized)
                return;

            long port = k.GetOptionLong("webui_port");
            if(k.info_.contains("webui_port"))
                port = long(k.info_["webui_port"]);
            k.InitSocket(port);
            socket_initialized = true;
        }

        void ApplyAutoStartFlags()
        {
            if(o.is_set("batch_mode"))
                k.info_["start"] = true;

            if(k.info_.is_set("real_time"))
                k.info_["start"] = true;
        }

        bool ShouldQuitEmptyBatchModel() const
        {
            return
                k.GetOptionFilename().empty()
                && o.is_set("batch_mode")
                && k.info_.contains("stop")
                && long(k.info_["stop"]) == 0;
        }

        void StartRequestedRunMode()
        {
            if(!k.info_.is_set("start"))
                return;

            if(k.info_.is_set("real_time"))
                k.Realtime();
            else
                k.Play();
        }
    };
}

int
main(int argc, char *argv[])
{
#if defined(__APPLE__)
    // Keep Apple framework unified logging from flooding stderr while
    // preserving normal IKAROS stdout/stderr messages.
    setenv("OS_ACTIVITY_MODE", "disable", 0);
#endif

    try
    { 
        options o;
        ConfigureOptions(o);

        o.parse_args(argc, argv);
        if(o.is_set("help"))
        {
            o.print_help();
            return 0;
        }

        InitializeKernelPaths(kernel(), o);

        if(o.is_set("batch_mode"))
            o.set("start");

        PrintStartupBanner();
        MainLoopController session(kernel(), o);

        while(session.ShouldKeepRunning())
        {
            int exit_code = session.RunProtected([&]() { session.RunIteration(); });
            if(exit_code >= 0)
                return exit_code;
        }

        return session.Finish();
    }
    catch(std::exception & e)
    {
        kernel().LogProcessExit();
        std::cerr << std::string(e.what()) << '\n';
        return 1;
    }
    catch(...)
    {
        kernel().LogProcessExit();
        std::cout << "Ikaros: Internal Error\n";
        return 1;
    }
}
