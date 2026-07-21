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
        result.add_option("b", "batch_mode", "batch mode");
        result.add_option("h", "help", "show help");
        result.add_option("s", "stop", "stop tick", true, "-1");
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

        options false_booleans = configured_options();
        parse(false_booleans,
              {executable.string(), "batch_mode=false", "help=off"});
        require(!false_booleans.is_set("batch_mode") &&
                !false_booleans.is_set("help") &&
                false_booleans.is_explicitly_set("batch_mode") &&
                false_booleans.is_explicitly_set("help"),
                "false Boolean assignments enabled command-line flags");

        options true_boolean = configured_options();
        parse(true_boolean, {executable.string(), "batch_mode=yes"});
        require(true_boolean.is_set("batch_mode"),
                "recognized true Boolean assignment did not enable its flag");

        bool invalid_boolean_rejected = false;
        try
        {
            options invalid_boolean = configured_options();
            parse(invalid_boolean, {executable.string(), "batch_mode=perhaps"});
        }
        catch(const std::exception & e)
        {
            invalid_boolean_rejected = std::string(e.what()).find("Invalid Boolean value") !=
                                       std::string::npos;
        }
        require(invalid_boolean_rejected, "invalid Boolean assignment was accepted");

        options assignments = configured_options();
        parse(assignments,
              {executable.string(), "empty=", "double_quoted=\"two words\"",
               "single_quoted='three words'"});
        require(assignments.get("empty").empty() &&
                assignments.is_explicitly_set("empty"),
                "empty assignment was not stored safely");
        require(assignments.get("double_quoted") == "two words" &&
                assignments.get("single_quoted") == "three words",
                "quoted assignment values were not unwrapped consistently");

        bool mismatched_quotes_rejected = false;
        try
        {
            options mismatched_quotes = configured_options();
            parse(mismatched_quotes, {executable.string(), "value=\"unterminated"});
        }
        catch(const std::exception & e)
        {
            mismatched_quotes_rejected = std::string(e.what()).find("Mismatched quotes") !=
                                         std::string::npos;
        }
        require(mismatched_quotes_rejected, "mismatched assignment quotes were accepted");

        options valid_integer = configured_options();
        parse(valid_integer, {executable.string(), "-w", " 9000 "});
        require(valid_integer.get_long("webui_port") == 9000,
                "valid integer option was not parsed completely");

        bool trailing_integer_text_rejected = false;
        try
        {
            options trailing_integer_text = configured_options();
            parse(trailing_integer_text, {executable.string(), "-w", "9000junk"});
            trailing_integer_text.get_long("webui_port");
        }
        catch(const std::invalid_argument &)
        {
            trailing_integer_text_rejected = true;
        }
        require(trailing_integer_text_rejected,
                "integer option with trailing text was accepted");

        bool oversized_integer_rejected = false;
        try
        {
            options oversized_integer = configured_options();
            parse(oversized_integer,
                  {executable.string(), "-w", "999999999999999999999999999999"});
            oversized_integer.get_long("webui_port");
        }
        catch(const std::invalid_argument &)
        {
            oversized_integer_rejected = true;
        }
        require(oversized_integer_rejected, "overflowing integer option was accepted");

        bool port_range_checked = false;
        try
        {
            options invalid_port = configured_options();
            parse(invalid_port, {executable.string(), "-w", "65536"});
            invalid_port.get_long("webui_port", 0, 65535);
        }
        catch(const std::out_of_range &)
        {
            port_range_checked = true;
        }
        require(port_range_checked, "out-of-range WebUI port was accepted");

        bool following_option_rejected = false;
        try
        {
            options missing_required_value = configured_options();
            parse(missing_required_value, {executable.string(), "-w", "-h"});
        }
        catch(const std::exception & e)
        {
            following_option_rejected = std::string(e.what()).find("\"-w\" requires a value") !=
                                        std::string::npos;
        }
        require(following_option_rejected,
                "required option consumed the following option as its value");

        options negative_required_value = configured_options();
        parse(negative_required_value, {executable.string(), "-s", "-1"});
        require(negative_required_value.get_long("stop") == -1 &&
                negative_required_value.is_explicitly_set("stop"),
                "negative required option value was mistaken for another option");

        std::cout << "OPTIONS TEST OK" << std::endl;
    }
};

INSTALL_CLASS(OptionsTestModule)
