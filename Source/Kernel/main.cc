// Ikaros 3.0

#include <atomic>
#include <charconv>
#include <cstdlib>

#include "ikaros.h"

using namespace ikaros;

extern std::atomic<bool> global_terminate;

namespace ikaros
{
    class KernelMainAccess
    {
    public:
        static void SetDirectories(Kernel & kernel, std::string webui_dir, std::string user_dir)
        {
            kernel.webui_dir = std::move(webui_dir);
            kernel.user_dir = std::move(user_dir);
        }

        static const std::string & UserDirectory(const Kernel & kernel)
        {
            return kernel.user_dir;
        }

        static void ResetProcessControl(Kernel & kernel)
        {
            kernel.notify_stop_requested = false;
            kernel.process_exit_code = 0;
        }

        static bool ShouldKeepRunning(const Kernel & kernel)
        {
            return kernel.run_mode.load() != run_mode_quit && !global_terminate.load();
        }

        static int ProcessExitCode(const Kernel & kernel)
        {
            return kernel.process_exit_code.load();
        }

        static void SetRunMode(Kernel & kernel, int mode)
        {
            kernel.run_mode = mode;
        }

        static bool NeedsReload(const Kernel & kernel)
        {
            return kernel.needs_reload;
        }

        static void SetNeedsReload(Kernel & kernel, bool needs_reload)
        {
            kernel.needs_reload = needs_reload;
        }

        static dictionary & ModelInfo(Kernel & kernel)
        {
            return kernel.info_;
        }
    };
}

namespace
{
    long
    ParseWebUIPort(const std::string & value)
    {
        const std::string text = trim(value);
        long result = 0;
        const char * begin = text.data();
        const char * end = begin + text.size();
        bool valid_sign = true;
        if(begin != end && *begin == '+')
        {
            ++begin;
            valid_sign = begin != end && *begin != '+' && *begin != '-';
        }
        const auto conversion = std::from_chars(begin, end, result);
        if(text.empty() || !valid_sign || conversion.ec != std::errc() || conversion.ptr != end ||
           result < 0 || result > 65535)
            throw std::invalid_argument("Invalid WebUI port \"" + value +
                                        "\". Expected an integer between 0 and 65535.");
        return result;
    }


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
        o.add_option("d", "tick_duration", "duration of each tick", true);
        o.add_option("i", "info", "print model info");
        o.add_option("r", "real_time", "run in real-time mode; also implies S");
        o.add_option("S", "start", " start-up automatically without waiting for commands from WebUI");
        o.add_option("s", "stop", "stop Ikaros after this tick", true, "-1");
        o.add_option("p", "python_executable", "default Python interpreter for python-backed classes", true);
        o.add_option("P", "print-tick-interval", "print the current tick every N ticks", true);
        o.add_option("t", "threads", "number of worker threads for the kernel thread pool", true);
        o.add_option("u", "user_data", "alternative directory for user data files", true);
        o.add_option("w", "webui_port", "port for ikaros WebUI", true, "8000");
        o.add_option("B", "bind_address", "bind WebUI/API server to a specific IPv4 address, for example 127.0.0.1", true);
        o.add_option("a", "auth_password", "enable optional WebUI/API authentication using the provided password",
                     true, "", false, true);
        o.add_option("A", "agent", "set the agent identifier included in remote session logging", true);
        o.add_option("H", "hide_toolbar", "hide the WebUI top toolbar and breadcrumbs on startup");
        o.add_option("L", "load_state", "load persistent state after model setup; bare -L uses the model name with .state extension", false, "", true);
        o.add_option("W", "save_state", "save persistent state when the model stops; bare -W uses the model name with .state extension", false, "", true);
        o.add_option("h", "help", "list command line options");
    }

    void InitializeKernelPaths(Kernel & k, const options & o)
    {
        KernelMainAccess::SetDirectories(k, o.ikaros_root+"/Source/WebUI/",
                                         ResolveUserDirectory(o));
        k.ScanClasses(o.ikaros_root+"/Source/Modules");
        k.ScanClasses(o.ikaros_root+"/Source/Kernel/UnitTesting/TestModules");
        k.ScanClasses(o.ikaros_root+"/Source/UserModules");

        std::filesystem::current_path(KernelMainAccess::UserDirectory(k));
    }

    void PrintStartupBanner()
    {
#if DEBUG
        std::cout << "Ikaros 3.0 Starting (Debug)\n\n";
#else
        std::cout << "Ikaros 3.0 Starting\n\n";
#endif
    }


    void ReportStartupError(const exception & e) noexcept
    {
        try
        {
            std::cerr << "Ikaros error: " << e.what();
            const std::string path = e.path();
            if(!path.empty())
                std::cerr << " (" << path << ")";
            std::cerr << '\n';
        }
        catch(...)
        {
        }
    }


    void ReportStartupError(const std::exception & e) noexcept
    {
        try
        {
            std::cerr << e.what() << '\n';
        }
        catch(...)
        {
        }
    }


    void ReportUnknownStartupError() noexcept
    {
        try
        {
            std::cerr << "Ikaros: Internal Error\n";
        }
        catch(...)
        {
        }
    }


    class MainLoopController
    {
    public:
        MainLoopController(Kernel & kernel, options & opts)
            : k(kernel), o(opts)
        {
            k.SetOptions(o);
            KernelMainAccess::ResetProcessControl(k);
            k.LogProcessStart();
        }

        bool ShouldKeepRunning() const
        {
            return KernelMainAccess::ShouldKeepRunning(k);
        }

        int Finish()
        {
            return Shutdown(KernelMainAccess::ProcessExitCode(k), true);
        }

        int FailFast(int code)
        {
            return Shutdown(code, false);
        }

        int FailFast(const exception & e)
        {
            try
            {
                LogLoopError("Ikaros error: " + e.message(), e.path());
            }
            catch(...)
            {
                ReportStartupError(e);
            }
            return FailFast(1);
        }

        int FailFast(const std::exception & e)
        {
            try
            {
                LogLoopError("Standard exception: " + std::string(e.what()));
            }
            catch(...)
            {
                ReportStartupError(e);
            }
            return FailFast(1);
        }

        int FailFastUnknown()
        {
            try
            {
                LogLoopError("Unknown exception.");
            }
            catch(...)
            {
                ReportUnknownStartupError();
            }
            return FailFast(1);
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
            SetUpModelIfNeeded();
            model_stop_pending = true;
            ApplyAutoStartFlags();

            if(ShouldQuitEmptyBatchModel())
            {
                KernelMainAccess::SetRunMode(k, run_mode_quit);
                return;
            }

            StartRequestedRunMode();
            if(k.Run())
                model_stop_pending = false;

            if(k.GetOptionFilename().empty() && o.is_set("batch_mode"))
                KernelMainAccess::SetRunMode(k, run_mode_quit);
        }

    private:
        Kernel & k;
        options & o;
        bool socket_initialized = false;
        bool model_setup_pending = false;
        bool model_stop_pending = false;
        bool shutdown_started = false;

        int Shutdown(int code, bool print_banner)
        {
            if(shutdown_started)
                return code;

            shutdown_started = true;
            ShutdownHttp();
            if(!StopModelIfNeeded() && code == 0)
                code = 1;
            if(code == 0)
                code = KernelMainAccess::ProcessExitCode(k);
            LogProcessExit();
            if(print_banner)
                std::cout << "\nIkaros 3.0 Ended\n";
            return code;
        }

        void ShutdownHttp()
        {
            socket_initialized = false;
            try
            {
                k.StopHTTPServer();
            }
            catch(const exception & e)
            {
                LogLoopError("Failed to stop the WebUI server: " + e.message(), e.path());
            }
            catch(const std::exception & e)
            {
                LogLoopError("Failed to stop the WebUI server: " + std::string(e.what()));
            }
            catch(...)
            {
                LogLoopError("Failed to stop the WebUI server: Unknown error.");
            }
        }

        bool StopModelIfNeeded()
        {
            if(!model_stop_pending)
                return true;

            model_stop_pending = false;
            try
            {
                k.Stop();
                return true;
            }
            catch(const exception & e)
            {
                LogLoopError("Failed to stop the model: " + e.message(), e.path());
            }
            catch(const std::exception & e)
            {
                LogLoopError("Failed to stop the model: " + std::string(e.what()));
            }
            catch(...)
            {
                LogLoopError("Failed to stop the model: Unknown error.");
            }
            return false;
        }

        void LogProcessExit()
        {
            try
            {
                k.LogProcessExit();
            }
            catch(const std::exception & e)
            {
                std::cerr << "Failed to log process exit: " << e.what() << '\n';
            }
            catch(...)
            {
                std::cerr << "Failed to log process exit: Unknown error.\n";
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

            if(!StopModelIfNeeded())
            {
                KernelMainAccess::SetRunMode(k, run_mode_stop);
                KernelMainAccess::SetNeedsReload(k, true);
            }

            return -1;
        }

        void LoadModelIfNeeded()
        {
            if(k.GetOptionFilename().empty())
                k.New();
            else if(KernelMainAccess::NeedsReload(k) && !k.AutomaticReloadSuppressed())
            {
                try
                {
                    k.LoadFileConfiguration();
                    model_setup_pending = true;
                }
                catch(...)
                {
                    if(!o.is_set("batch_mode"))
                        k.SuppressAutomaticReloadUntilSave();
                    throw;
                }
            }
        }

        void SetUpModelIfNeeded()
        {
            if(!model_setup_pending)
                return;

            try
            {
                k.SetUpLoadedFile();
                model_setup_pending = false;
            }
            catch(...)
            {
                model_setup_pending = false;
                if(!o.is_set("batch_mode"))
                    k.SuppressAutomaticReloadUntilSave();
                throw;
            }
        }

        void EnsureSocketStarted()
        {
            bool should_start_socket =
                !o.is_set("batch_mode")
                || o.is_explicitly_set("webui_port")
                || KernelMainAccess::ModelInfo(k).contains("webui_port");

            if(!should_start_socket || socket_initialized)
                return;

            const std::string fallback_port_value = o.get("webui_port");
            long port = ParseWebUIPort(fallback_port_value);
            if(KernelMainAccess::ModelInfo(k).contains("webui_port"))
            {
                const std::string model_port_value = std::string(KernelMainAccess::ModelInfo(k)["webui_port"]);
                try
                {
                    port = ParseWebUIPort(model_port_value);
                }
                catch(const std::invalid_argument & e)
                {
                    if(o.is_set("batch_mode"))
                        throw;
                    k.Notify(msg_warning, std::string(e.what()) +
                             " Using WebUI port " + std::to_string(port) + " instead.");
                }
            }
            k.InitSocket(port);
            socket_initialized = true;
        }

        void ApplyAutoStartFlags()
        {
            if(o.is_set("batch_mode"))
                KernelMainAccess::ModelInfo(k)["start"] = true;

            if(KernelMainAccess::ModelInfo(k).is_set("real_time"))
                KernelMainAccess::ModelInfo(k)["start"] = true;
        }

        bool ShouldQuitEmptyBatchModel() const
        {
            return
                k.GetOptionFilename().empty()
                && o.is_set("batch_mode")
                && KernelMainAccess::ModelInfo(k).contains("stop")
                && long(KernelMainAccess::ModelInfo(k)["stop"]) == 0;
        }

        void StartRequestedRunMode()
        {
            if(k.AutomaticReloadSuppressed() && KernelMainAccess::NeedsReload(k))
                return;

            if(!KernelMainAccess::ModelInfo(k).is_set("start"))
                return;

            if(KernelMainAccess::ModelInfo(k).is_set("real_time"))
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

        try
        {
            while(session.ShouldKeepRunning())
            {
                int exit_code = session.RunProtected([&]() { session.RunIteration(); });
                if(exit_code >= 0)
                    return exit_code;
            }

            return session.Finish();
        }
        catch(const exception & e)
        {
            return session.FailFast(e);
        }
        catch(const std::exception & e)
        {
            return session.FailFast(e);
        }
        catch(...)
        {
            return session.FailFastUnknown();
        }
    }
    catch(const exception & e)
    {
        ReportStartupError(e);
        return 1;
    }
    catch(const std::exception & e)
    {
        ReportStartupError(e);
        return 1;
    }
    catch(...)
    {
        ReportUnknownStartupError();
        return 1;
    }
}
