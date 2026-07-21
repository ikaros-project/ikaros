#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ikaros.h"

using namespace ikaros;

namespace
{
    class TemporaryOptionsDirectory
    {
    public:
        TemporaryOptionsDirectory()
        {
            const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
            path_ = std::filesystem::temp_directory_path() /
                    ("ikaros-options-tests-" + std::to_string(suffix));
            if(!std::filesystem::create_directory(path_))
                throw std::runtime_error("Could not create temporary options test directory");
        }

        ~TemporaryOptionsDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path_, error);
        }

        std::filesystem::path
        write(const std::filesystem::path & relative_path, const std::string & contents) const
        {
            const std::filesystem::path path = path_ / relative_path;
            std::filesystem::create_directories(path.parent_path());
            std::ofstream file(path);
            if(!file)
                throw std::runtime_error("Could not create temporary options test file");
            file << contents;
            if(!file)
                throw std::runtime_error("Could not write temporary options test file");
            return path;
        }

        const std::filesystem::path &
        path() const noexcept
        {
            return path_;
        }

    private:
        std::filesystem::path path_;
    };


    class ScopedPath
    {
    public:
        explicit ScopedPath(const std::string & value)
        {
            const char * current = std::getenv("PATH");
            if(current != nullptr)
            {
                had_value_ = true;
                old_value_ = current;
            }
            if(setenv("PATH", value.c_str(), 1) != 0)
                throw std::runtime_error("Could not set PATH for options test");
        }

        ~ScopedPath()
        {
            if(had_value_)
                setenv("PATH", old_value_.c_str(), 1);
            else
                unsetenv("PATH");
        }

        ScopedPath(const ScopedPath &) = delete;
        ScopedPath & operator=(const ScopedPath &) = delete;

    private:
        bool had_value_ = false;
        std::string old_value_;
    };


    void
    require(bool condition, const std::string & message)
    {
        if(!condition)
            throw exception("OptionsTestModule: " + message);
    }


    options
    configured_options()
    {
        options result;
        result.add_option("w", "webui_port", "WebUI port", true, "8000");
        result.add_option("a", "auth_password", "authentication password", true, "", false, true);
        result.add_option("W", "save_state", "save state", false, "", true);
        result.add_option("h", "help", "show help");
        return result;
    }


    void
    parse(options & result, std::vector<std::string> arguments)
    {
        std::vector<char *> argv;
        argv.reserve(arguments.size());
        for(std::string & argument : arguments)
            argv.push_back(argument.data());
        result.parse_args(static_cast<int>(argv.size()), argv.data());
    }
}


class OptionsTestModule : public Module
{
    void Init() override
    {
        TemporaryOptionsDirectory files;
        const std::filesystem::path root = files.path() / "root";
        const std::filesystem::path executable = files.write("root/Bin/ikaros", "test executable");
        std::filesystem::permissions(executable,
                                     std::filesystem::perms::owner_exec,
                                     std::filesystem::perm_options::add);
        const std::filesystem::path model = files.write("model.ikg", "<group/>");
        const std::filesystem::path second_model = files.write("second.ikg", "<group/>");
        const std::filesystem::path state = files.write("state.json", "{}");

        options direct = configured_options();
        parse(direct, {executable.string()});
        require(direct.ikaros_root == std::filesystem::canonical(root).string(),
                "direct executable path resolved the wrong Ikaros root");

        const std::filesystem::path link_directory = files.path() / "links";
        std::filesystem::create_directory(link_directory);
        const std::filesystem::path executable_link = link_directory / "ikaros";
        std::filesystem::create_symlink(executable, executable_link);
        options through_link = configured_options();
        parse(through_link, {executable_link.string()});
        require(through_link.ikaros_root == std::filesystem::canonical(root).string(),
                "symlinked executable path resolved the wrong Ikaros root");

        {
            ScopedPath path(executable.parent_path().string());
            options through_path = configured_options();
            parse(through_path, {executable.filename().string()});
            require(through_path.ikaros_root == std::filesystem::canonical(root).string(),
                    "bare executable name resolved the wrong Ikaros root through PATH");
        }

        options help = configured_options();
        parse(help, {executable.string(), "-aSECRET", "-w9000", "-h"});
        std::ostringstream help_output;
        help.print_help(help_output);
        require(help_output.str().find("SECRET") == std::string::npos,
                "help output exposed a sensitive parsed value");
        require(help_output.str().find("[8000]") != std::string::npos &&
                help_output.str().find("[9000]") == std::string::npos,
                "help output did not retain the registered default value");

        options value_before_model = configured_options();
        parse(value_before_model,
              {executable.string(), "-W", state.string(), model.string()});
        require(value_before_model.get("save_state") == state.string() &&
                value_before_model.full_path() == model.string(),
                "optional value before the model was parsed incorrectly");

        options value_after_model = configured_options();
        parse(value_after_model,
              {executable.string(), model.string(), "-W", state.string()});
        require(value_after_model.get("save_state") == state.string() &&
                value_after_model.full_path() == model.string(),
                "optional value after the model was parsed as another model");

        options bare_optional_value = configured_options();
        parse(bare_optional_value, {executable.string(), "-W", model.string()});
        require(bare_optional_value.get("save_state").empty() &&
                bare_optional_value.full_path() == model.string(),
                "bare optional value consumed the model filename");

        options bare_value_before_flag = configured_options();
        parse(bare_value_before_flag,
              {executable.string(), "-W", model.string(), "-h"});
        require(bare_value_before_flag.get("save_state").empty() &&
                bare_value_before_flag.full_path() == model.string() &&
                bare_value_before_flag.is_set("help"),
                "a trailing flag made a bare optional value consume the model filename");

        options value_before_required_option = configured_options();
        parse(value_before_required_option,
              {executable.string(), "-W", state.string(), "-w", "9000", model.string()});
        require(value_before_required_option.get("save_state") == state.string() &&
                value_before_required_option.get("webui_port") == "9000" &&
                value_before_required_option.full_path() == model.string(),
                "required option values confused optional-value lookahead");

        bool multiple_models_rejected = false;
        try
        {
            options multiple_models = configured_options();
            parse(multiple_models,
                  {executable.string(), model.string(), second_model.string()});
        }
        catch(const std::exception & e)
        {
            multiple_models_rejected = std::string(e.what()).find("Only one model file") != std::string::npos;
        }
        require(multiple_models_rejected, "multiple model files were not rejected clearly");

        std::cout << "OPTIONS TEST OK" << std::endl;
    }
};

INSTALL_CLASS(OptionsTestModule)
