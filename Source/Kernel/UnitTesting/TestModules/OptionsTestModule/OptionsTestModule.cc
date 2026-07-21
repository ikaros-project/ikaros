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


    class ScopedCurrentPath
    {
    public:
        explicit ScopedCurrentPath(const std::filesystem::path & path)
            : old_path_(std::filesystem::current_path())
        {
            std::filesystem::current_path(path);
        }

        ~ScopedCurrentPath()
        {
            std::error_code error;
            std::filesystem::current_path(old_path_, error);
        }

        ScopedCurrentPath(const ScopedCurrentPath &) = delete;
        ScopedCurrentPath & operator=(const ScopedCurrentPath &) = delete;

    private:
        std::filesystem::path old_path_;
    };


    void
    require(bool condition, const std::string & message)
    {
        if(!condition)
            throw exception("OptionsTestModule: " + message);
    }


    template<typename Function>
    void
    require_registration_failure(Function function, const std::string & expected_message,
                                 const std::string & message)
    {
        bool rejected = false;
        try
        {
            function();
        }
        catch(const std::invalid_argument & e)
        {
            rejected = std::string(e.what()).find(expected_message) != std::string::npos;
        }
        require(rejected, message);
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
        const std::filesystem::path dashed_model = files.write("-model.ikg", "<group/>");
        const std::filesystem::path assignment_model = files.write("model=assignment.ikg", "<group/>");

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

#if !defined(_WIN32)
        const std::filesystem::path non_executable =
            files.write("non-executable/ikaros", "not executable");
        std::filesystem::permissions(non_executable,
                                     std::filesystem::perms::owner_exec |
                                     std::filesystem::perms::group_exec |
                                     std::filesystem::perms::others_exec,
                                     std::filesystem::perm_options::remove);

        bool non_executable_direct_path_rejected = false;
        try
        {
            options direct_non_executable = configured_options();
            parse(direct_non_executable, {non_executable.string()});
        }
        catch(const std::exception & e)
        {
            non_executable_direct_path_rejected =
                std::string(e.what()).find("Could not resolve the Ikaros executable path") !=
                std::string::npos;
        }
        require(non_executable_direct_path_rejected,
                "non-executable direct path was accepted as the executable");

        {
            const std::string search_path = non_executable.parent_path().string() + ":" +
                                            executable.parent_path().string();
            ScopedPath path(search_path);
            options after_non_executable = configured_options();
            parse(after_non_executable, {executable.filename().string()});
            require(after_non_executable.ikaros_root == std::filesystem::canonical(root).string(),
                    "PATH lookup did not skip a non-executable file");
        }
#endif

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

        bool empty_required_assignment_rejected = false;
        try
        {
            options empty_required_assignment = configured_options();
            parse(empty_required_assignment, {executable.string(), "webui_port="});
        }
        catch(const std::exception & e)
        {
            empty_required_assignment_rejected =
                std::string(e.what()).find("requires a non-empty value") != std::string::npos;
        }
        require(empty_required_assignment_rejected,
                "empty assignment to a required option was accepted");

        bool empty_separated_required_value_rejected = false;
        try
        {
            options empty_separated_required_value = configured_options();
            parse(empty_separated_required_value, {executable.string(), "-w", ""});
        }
        catch(const std::exception & e)
        {
            empty_separated_required_value_rejected =
                std::string(e.what()).find("requires a non-empty value") != std::string::npos;
        }
        require(empty_separated_required_value_rejected,
                "separated empty required option value was accepted");

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

        options dash_prefixed_required_value = configured_options();
        parse(dash_prefixed_required_value,
              {executable.string(), "-a", "-banana"});
        require(dash_prefixed_required_value.get("auth_password") == "-banana",
                "dash-prefixed required value was mistaken for an invalid Boolean option");

        bool attached_following_option_rejected = false;
        try
        {
            options missing_value_before_attached_option = configured_options();
            parse(missing_value_before_attached_option,
                  {executable.string(), "-a", "-w9000"});
        }
        catch(const std::exception & e)
        {
            attached_following_option_rejected =
                std::string(e.what()).find("\"-a\" requires a value") != std::string::npos;
        }
        require(attached_following_option_rejected,
                "required option consumed a following attached option as its value");

        options negative_required_value = configured_options();
        parse(negative_required_value, {executable.string(), "-s", "-1"});
        require(negative_required_value.get_long("stop") == -1 &&
                negative_required_value.is_explicitly_set("stop"),
                "negative required option value was mistaken for another option");

        require_registration_failure(
            []()
            {
                options malformed;
                malformed.add_option("", "empty_short", "invalid");
            },
            "exactly one character", "empty option short name was accepted");
        require_registration_failure(
            []()
            {
                options malformed;
                malformed.add_option("ab", "long_short", "invalid");
            },
            "exactly one character", "multi-character option short name was accepted");
        require_registration_failure(
            []()
            {
                options malformed;
                malformed.add_option("x", "", "invalid");
            },
            "must not be empty", "empty option full name was accepted");

        options duplicate_short;
        duplicate_short.add_option("x", "first", "first option");
        require_registration_failure(
            [&duplicate_short]()
            {
                duplicate_short.add_option("x", "second", "second option");
            },
            "already registered", "duplicate option short name was accepted");
        require(duplicate_short.full.at("x") == "first" &&
                !duplicate_short.description.count("second"),
                "failed duplicate short-name registration changed existing state");

        options duplicate_full;
        duplicate_full.add_option("x", "same", "first option");
        require_registration_failure(
            [&duplicate_full]()
            {
                duplicate_full.add_option("y", "same", "second option");
            },
            "already registered", "duplicate option full name was accepted");
        require(!duplicate_full.full.count("y"),
                "failed duplicate full-name registration changed existing state");

        require_registration_failure(
            []()
            {
                options contradictory;
                contradictory.add_option("x", "contradictory", "invalid", true, "", true);
            },
            "cannot both require", "contradictory option value modes were accepted");
        require_registration_failure(
            []()
            {
                options invalid_default;
                invalid_default.add_option("x", "flag", "invalid", false, "perhaps");
            },
            "Invalid Boolean default", "invalid Boolean option default was accepted");

        options boolean_defaults;
        boolean_defaults.add_option("x", "enabled", "enabled by default", false, "YES");
        boolean_defaults.add_option("y", "disabled", "disabled by default", false, "off");
        require(boolean_defaults.get("enabled") == "true" &&
                boolean_defaults.is_set("enabled") &&
                boolean_defaults.get("disabled") == "false" &&
                !boolean_defaults.is_set("disabled"),
                "valid Boolean option defaults were not normalized");

        std::vector<std::string> many_arguments = {executable.string()};
        for(int i = 0; i < 100; ++i)
            many_arguments.push_back("override_" + std::to_string(i) + "=" + std::to_string(i));
        options many_overrides = configured_options();
        parse(many_overrides, many_arguments);
        require(many_overrides.explicitly_set.size() == 100 &&
                many_overrides.get("override_0") == "0" &&
                many_overrides.get("override_99") == "99",
                "more than 64 command-line arguments were not parsed correctly");

        bool directory_rejected = false;
        try
        {
            options directory_model = configured_options();
            parse(directory_model, {executable.string(), files.path().string()});
        }
        catch(const std::exception & e)
        {
            directory_rejected = std::string(e.what()).find("not a regular file") !=
                                 std::string::npos;
        }
        require(directory_rejected, "directory was accepted as a model file");

        {
            ScopedCurrentPath current_path(files.path());

            options dashed_filename = configured_options();
            parse(dashed_filename,
                  {executable.string(), "--", dashed_model.filename().string()});
            require(std::filesystem::equivalent(dashed_filename.full_path(), dashed_model),
                    "end-of-options marker did not allow a dashed model filename");

            options assignment_filename = configured_options();
            parse(assignment_filename,
                  {executable.string(), "--", assignment_model.filename().string()});
            require(std::filesystem::equivalent(assignment_filename.full_path(), assignment_model) &&
                    assignment_filename.explicitly_set.empty(),
                    "end-of-options marker did not preserve an assignment-like model filename");

            options optional_value_before_marker = configured_options();
            parse(optional_value_before_marker,
                  {executable.string(), "-W", state.string(), "--",
                   dashed_model.filename().string()});
            require(optional_value_before_marker.get("save_state") == state.string() &&
                    std::filesystem::equivalent(optional_value_before_marker.full_path(), dashed_model),
                    "optional-value lookahead failed across the end-of-options marker");
        }

        bool marker_rejected_as_required_value = false;
        try
        {
            options missing_value_at_marker = configured_options();
            parse(missing_value_at_marker,
                  {executable.string(), "-w", "--", model.string()});
        }
        catch(const std::exception & e)
        {
            marker_rejected_as_required_value =
                std::string(e.what()).find("\"-w\" requires a value") != std::string::npos;
        }
        require(marker_rejected_as_required_value,
                "end-of-options marker was consumed as a required option value");

        options reusable = configured_options();
        parse(reusable,
              {executable.string(), "-w", "9001", "old_override=old", model.string()});
        parse(reusable, {executable.string(), "-h", second_model.string()});
        require(reusable.get("webui_port") == "8000" &&
                !reusable.is_explicitly_set("webui_port") &&
                reusable.get("old_override").empty() &&
                reusable.is_set("help") &&
                reusable.filenames.size() == 1 &&
                reusable.full_path() == second_model.string(),
                "successful parser reuse retained state from the previous parse");

        options transactional = configured_options();
        parse(transactional,
              {executable.string(), "-w", "9001", "kept=value", model.string()});
        bool failed_reparse = false;
        try
        {
            parse(transactional,
                  {executable.string(), "-w", "9002", "discarded=value",
                   second_model.string(), model.string()});
        }
        catch(const std::exception &)
        {
            failed_reparse = true;
        }
        require(failed_reparse &&
                transactional.get("webui_port") == "9001" &&
                transactional.get("kept") == "value" &&
                transactional.get("discarded").empty() &&
                transactional.filenames.size() == 1 &&
                transactional.full_path() == model.string(),
                "failed parser reuse did not preserve the previous valid state");

        options pristine_failure = configured_options();
        bool initial_parse_failed = false;
        try
        {
            parse(pristine_failure,
                  {executable.string(), "-w", "9002", model.string(), second_model.string()});
        }
        catch(const std::exception &)
        {
            initial_parse_failed = true;
        }
        require(initial_parse_failed &&
                pristine_failure.get("webui_port") == "8000" &&
                pristine_failure.explicitly_set.empty() &&
                pristine_failure.filenames.empty() &&
                pristine_failure.full_path().empty() &&
                pristine_failure.ikaros_root.empty(),
                "failed initial parse left partial state behind");

        std::cout << "OPTIONS TEST OK" << std::endl;
    }
};

INSTALL_CLASS(OptionsTestModule)
