// Ikaros 3.0

#include "ikaros.h"

#include <array>
#include <fstream>
#include <random>
#include <system_error>

#if __has_include(<CommonCrypto/CommonDigest.h>) && __has_include(<CommonCrypto/CommonHMAC.h>)
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#define IKAROS_HAVE_COMMONCRYPTO 1
#endif

#if IKAROS_HAVE_OPENSSL
#include <openssl/evp.h>
#include <openssl/hmac.h>
#endif

using namespace ikaros;
using namespace std::chrono;

namespace ikaros
{
    namespace
    {

        std::filesystem::path
        temporary_save_path(const std::filesystem::path & target_path)
        {
            static std::atomic<unsigned long long> serial{0};
            const auto timestamp =
                std::chrono::steady_clock::now().time_since_epoch().count();

            for(int attempt = 0; attempt < 100; attempt++)
            {
                const auto id = serial.fetch_add(1, std::memory_order_relaxed);
                const std::string temporary_name =
                    "." + target_path.filename().string() + ".ikaros-save-" +
                    std::to_string(timestamp) + "-" + std::to_string(id) +
                    ".tmp";
                const std::filesystem::path temporary_path =
                    target_path.parent_path() / temporary_name;
                std::error_code error;
                const bool exists = std::filesystem::exists(temporary_path, error);
                if(error)
                    throw std::system_error(
                        error, "Could not inspect temporary save file \"" +
                                   temporary_path.string() + "\"");
                if(!exists)
                    return temporary_path;
            }

            throw std::runtime_error(
                "Could not create a unique temporary save filename for \"" +
                target_path.string() + "\"");
        }


        void
        replace_file_atomically(const std::filesystem::path & source,
                                const std::filesystem::path & target)
        {
            std::error_code error;
            std::filesystem::rename(source, target, error);
            if(error)
                throw std::system_error(
                    error, "Could not atomically replace model \"" +
                               target.string() + "\"");
        }
        bool constant_time_equals(const std::string & a, const std::string & b)
        {
            const size_t max_length = std::max(a.size(), b.size());
            unsigned char diff = static_cast<unsigned char>(a.size() ^ b.size());
            for(size_t i = 0; i < max_length; ++i)
            {
                const unsigned char ac = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
                const unsigned char bc = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
                diff |= static_cast<unsigned char>(ac ^ bc);
            }
            return diff == 0;
        }

        std::string extract_cookie_value(const std::string & cookie_header, const std::string & cookie_name)
        {
            for(const auto & cookie : split(cookie_header, ";"))
            {
                std::string trimmed_cookie = trim(cookie);
                auto separator = trimmed_cookie.find('=');
                if(separator == std::string::npos)
                    continue;

                std::string key = trim(trimmed_cookie.substr(0, separator));
                if(key != cookie_name)
                    continue;

                return trim(trimmed_cookie.substr(separator + 1));
            }
            return "";
        }

        std::string hex_encode(const unsigned char * data, size_t size)
        {
            static constexpr char hex[] = "0123456789abcdef";
            std::string encoded;
            encoded.reserve(size * 2);
            for(size_t i = 0; i < size; ++i)
            {
                encoded.push_back(hex[(data[i] >> 4) & 0x0F]);
                encoded.push_back(hex[data[i] & 0x0F]);
            }
            return encoded;
        }

        std::string random_hex_string(size_t byte_count)
        {
            std::random_device rd;
            std::uniform_int_distribution<int> dist(0, 255);
            std::vector<unsigned char> bytes(byte_count);
            for(unsigned char & byte : bytes)
                byte = static_cast<unsigned char>(dist(rd));
            return hex_encode(bytes.data(), bytes.size());
        }

        long long unix_time_seconds()
        {
            return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        }

#if IKAROS_HAVE_COMMONCRYPTO
        std::string hmac_sha256_hex(const std::string & key, const std::string & message)
        {
            unsigned char digest[CC_SHA256_DIGEST_LENGTH];
            CCHmac(kCCHmacAlgSHA256,
                   key.data(),
                   key.size(),
                   message.data(),
                   message.size(),
                   digest);
            return hex_encode(digest, sizeof(digest));
        }
#elif IKAROS_HAVE_OPENSSL
        std::string hmac_sha256_hex(const std::string & key, const std::string & message)
        {
            unsigned char digest[EVP_MAX_MD_SIZE];
            unsigned int digest_size = 0;
            if(HMAC(EVP_sha256(),
                    key.data(),
                    static_cast<int>(key.size()),
                    reinterpret_cast<const unsigned char *>(message.data()),
                    message.size(),
                    digest,
                    &digest_size) == nullptr)
                throw exception("Could not calculate authentication HMAC.");
            return hex_encode(digest, digest_size);
        }
#endif



        std::string component_path_from_request(const Request & request)
        {
            std::string component_path;
            if(request.parameters.contains("module"))
                component_path = std::string(request.parameters["module"]);
            else if(request.parameters.contains("path"))
                component_path = std::string(request.parameters["path"]);
            else if(request.parameters.contains("component"))
                component_path = std::string(request.parameters["component"]);

            component_path = trim(component_path);
            if(!component_path.empty() && component_path[0] == '.')
                component_path = component_path.substr(1);
            return component_path;
        }



        std::string canonicalize_shape_aliases(const std::string & xml)
        {
            std::string out;
            out.reserve(xml.size());

            for(size_t i = 0; i < xml.size();)
            {
                if(i + 5 <= xml.size() && xml.compare(i, 5, ".size") == 0)
                {
                    const char next = (i + 5 < xml.size()) ? xml[i + 5] : '\0';
                    const bool is_alias = next == '[' || next == '\0'
                        || (!ascii_is_alnum(static_cast<unsigned char>(next)) && next != '_');
                    if(is_alias)
                    {
                        out += ".shape";
                        i += 5;
                        continue;
                    }
                }

                out.push_back(xml[i]);
                ++i;
            }

            return out;
        }

        std::string normalize_request_value_path(const std::string & path)
        {
            if(!path.empty() && path[0] == '.')
                return path.substr(1);
            return path;
        }

        bool request_path_matches_command(const std::string & uri, const std::string & command)
        {
            std::string path = "/" + command;
            return uri == path || uri.rfind(path + "?", 0) == 0 || uri.rfind(path + "/", 0) == 0;
        }

    }


    bool
    Kernel::AuthEnabled() const
    {
        return auth_enabled_;
    }

    bool
    Kernel::CheckPassword(const std::string & candidate) const
    {
        return auth_enabled_ && constant_time_equals(candidate, auth_password_);
    }

    std::string
    Kernel::CreateSessionToken()
    {
        if(!auth_enabled_ || auth_cookie_secret_.empty())
            return "";

        static constexpr long long auth_cookie_lifetime_seconds = 30LL * 24LL * 60LL * 60LL;
        const long long expires_at = unix_time_seconds() + auth_cookie_lifetime_seconds;
        const std::string payload =
            std::to_string(expires_at) + "." +
            random_hex_string(16) + "." +
            PasswordMarker();

#if IKAROS_HAVE_COMMONCRYPTO || IKAROS_HAVE_OPENSSL
        return payload + "." + hmac_sha256_hex(auth_cookie_secret_, payload);
#else
        return "";
#endif
    }

    std::string
    Kernel::PasswordMarker() const
    {
#if IKAROS_HAVE_COMMONCRYPTO || IKAROS_HAVE_OPENSSL
        if(auth_cookie_secret_.empty() || auth_password_.empty())
            return "";
        return hmac_sha256_hex(auth_cookie_secret_, "password:" + auth_password_).substr(0, 32);
#else
        return "";
#endif
    }

    bool
    Kernel::LoadOrCreateAuthCookieSecret()
    {
#if !IKAROS_HAVE_COMMONCRYPTO && !IKAROS_HAVE_OPENSSL
        return false;
#else
        std::lock_guard<std::mutex> lock(auth_mutex_);
        if(!auth_cookie_secret_.empty())
            return true;

        std::filesystem::path secret_path = std::filesystem::path(user_dir) / ".auth_cookie_secret";
        std::error_code ec;

        if(std::filesystem::exists(secret_path, ec) && !ec)
        {
            std::ifstream secret_file(secret_path);
            std::string secret;
            std::getline(secret_file, secret);
            secret = trim(secret);
            if(secret.size() >= 32)
            {
                auth_cookie_secret_ = secret;
                return true;
            }
        }

        auth_cookie_secret_ = random_hex_string(32);
        std::ofstream secret_file(secret_path, std::ios::trunc);
        if(!secret_file.is_open())
        {
            auth_cookie_secret_.clear();
            return false;
        }

        secret_file << auth_cookie_secret_ << '\n';
        secret_file.close();
        if(secret_file.fail())
        {
            auth_cookie_secret_.clear();
            return false;
        }

        std::filesystem::permissions(
            secret_path,
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
            std::filesystem::perm_options::replace,
            ec);

        return true;
#endif
    }

    bool
    Kernel::IsRequestAuthenticated() const
    {
        if(!auth_enabled_)
            return true;
        if(socket == nullptr || !socket->RequestHeader().contains_non_null("cookie"))
            return false;

        std::string cookie_value = extract_cookie_value(
            std::string(socket->RequestHeader()["cookie"]), "ikaros_session");
        if(cookie_value.empty())
            return false;

        const auto parts = split(cookie_value, ".");
        if(parts.size() != 4)
            return false;

        long long expires_at = 0;
        const char * expiration_begin = parts[0].data();
        const char * expiration_end = expiration_begin + parts[0].size();
        const auto expiration_result = std::from_chars(expiration_begin, expiration_end, expires_at);
        if(expiration_result.ec != std::errc() || expiration_result.ptr != expiration_end)
            return false;

        if(expires_at < unix_time_seconds())
            return false;

        const std::string payload = parts[0] + "." + parts[1] + "." + parts[2];
        if(!constant_time_equals(parts[2], PasswordMarker()))
            return false;

#if IKAROS_HAVE_COMMONCRYPTO || IKAROS_HAVE_OPENSSL
        std::lock_guard<std::mutex> lock(auth_mutex_);
        if(auth_cookie_secret_.empty())
            return false;
        return constant_time_equals(parts[3], hmac_sha256_hex(auth_cookie_secret_, payload));
#else
        return false;
#endif
    }

    bool
    Kernel::IsPublicRequest(const Request & request) const
    {
        std::string public_url = request.url;
        if(!public_url.empty() && public_url[0] == '/')
            public_url.erase(0, 1);

        if(request.command == "auth" || request.command == "login")
            return true;

        if(request.command.empty() || public_url == "index.html")
            return true;

        static const std::array<std::string, 8> public_prefixes = {
            "core/",
            "ui/",
            "widgets/",
            "js/",
            "Images/",
            "images/",
            "Models/",
            "models/"
        };

        for(const auto & prefix : public_prefixes)
            if(starts_with(public_url, prefix))
                return true;

        static const std::array<std::string, 10> public_files = {
            "index.html",
            "style.css",
            "defaults.css",
            "widget_style.css",
            "widget_defaults.css",
            "info.html",
            "profiling_window.html",
            "startup_steps_window.html",
            "error.glb",
            "old_style.css"
        };

        return std::find(public_files.begin(), public_files.end(), public_url) != public_files.end();
    }

    void
    Kernel::SendStringResponse(dictionary header, const std::string & body, const char * response)
    {
        header["Content-Length"] = std::to_string(body.size());
        socket->SendHTTPHeader(header, response);
        socket->Append(body);
    }


    void
    Kernel::DoNew(Request & request)
    {
        New();
        DoUpdate(request); 
    }


    void
    Kernel::DoOpen(Request & request)
    {
        if(!request.parameters.contains("file"))
        {
            Notify(msg_warning, "No file specified.");
            DoSendNetwork(request);
            return;
        }

        std::string file = request.parameters["file"];
        std::string where = request.parameters.contains("where") ? std::string(request.parameters["where"]) : "";
        auto & files =
            where == "system" ? system_files :
            where == "examples" ? examples_files :
            user_files;
        auto file_path = files.find(file);
        std::filesystem::path open_path;
        if(file_path == files.end())
        {
            const std::filesystem::path root =
                where == "system" ? std::filesystem::path(options_.ikaros_root) / "Source/Modules" :
                where == "examples" ? std::filesystem::path(options_.ikaros_root) / "Examples" :
                std::filesystem::path(user_dir);
            if(!SanitizePathUnderRoot(root, add_extension(file, ".ikg"), open_path))
            {
                Notify(msg_warning, "File \""+file+"\" could not be found.");
                DoSendNetwork(request);
                return;
            }
        }
        else
        {
            open_path = file_path->second;
        }

        bool should_start = false;
        try
        {
            dictionary requested_file_info;
            LoadXMLWithRestrictedIncludes(requested_file_info, open_path);
            should_start = requested_file_info.is_set("start") || requested_file_info.is_set("real_time");
        }
        catch(...)
        {
            // LoadFile will report the detailed error below.
        }

        try
        {
            Stop();
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            if(components.size() > 0)
                Clear();
            options_.path_ = open_path.string();
            LoadFile();
            if(should_start && info_.is_set("real_time"))
                Realtime();
            else if(should_start)
                Play();
            else
            {
                info_.erase("start");
                info_.erase("real_time");
                run_mode = run_mode_stop;
                timer.Pause();
                timer.SetPauseTime(0);
            }
        }
        catch(const setup_failed& e)
        {
            Notify(msg_warning, "Could not set up file \""+file+"\": "+e.message(), e.path());
        }
        catch(const load_failed& e)
        {
            Notify(msg_warning, "Could not load file \""+file+"\": "+e.message(), e.path());
            New();
        }
        catch(const std::exception& e)
        {
            Notify(msg_warning, "Could not open file \""+file+"\": "+std::string(e.what()));
            New();
        }

        DoSendNetwork(request);
    }


    void
    Kernel::DoSave(Request & request) // Save data received from WebUI
    {
        dictionary d;
        try
        {
            if(!request.HasJsonBody())
                throw exception("Save request must include a JSON body.");
            if(!request.json_body.is_dictionary())
                throw exception("Save request body must be a JSON object.");
            d = dictionary(request.json_body).copy();
        }
        catch(const std::exception & e)
        {
            Notify(msg_warning, "While handling WebUI Save request: Could not parse JSON body: " +
                   std::string(e.what()));
            DoSendError("400 Bad Request", "Save request body is not valid JSON.");
            return;
        }

        // Sanitize file name

        if(!d.contains_non_null("filename") || std::string(d["filename"]).empty())
        {
            DoSendError("400 Bad Request", "Save request must include a filename.");
            return;
        }

        try
        {
            std::filesystem::path path = add_extension(std::string(d["filename"]), ".ikg");
            std::filesystem::path filename = path.filename();
            if(filename.empty() || filename == "." || filename == ".." || filename.stem().empty())
            {
                DoSendError("400 Bad Request", "Save request filename is invalid.");
                return;
            }
            std::filesystem::path target_path = std::filesystem::path(user_dir) / filename;

            d.erase("filename");
            std::string data = canonicalize_shape_aliases(d.xml("group", {"module/parameters","module/inputs","module/outputs","module/states", "module/authors","module/descriptions", "group/views", "module.description"}));
            std::error_code ec;
            std::filesystem::create_directories(target_path.parent_path(), ec);
            if(ec)
            {
                DoSendError("500 Internal Server Error", "Could not create save directory \"" + target_path.parent_path().string() + "\".");
                return;
            }

            const std::filesystem::path temporary_path =
                temporary_save_path(target_path);
            try
            {
                std::ofstream file(temporary_path);
                if(!file)
                    throw exception(
                        "Could not create temporary save file \"" +
                        temporary_path.string() + "\".");
                file << data;
                file.close();
                if(!file)
                    throw exception(
                        "Could not finish writing temporary save file \"" +
                        temporary_path.string() + "\".");
                replace_file_atomically(temporary_path, target_path);
            }
            catch(...)
            {
                std::error_code cleanup_error;
                std::filesystem::remove(temporary_path, cleanup_error);
                throw;
            }

            options_.path_ = target_path.string();
            needs_reload = true;

            d["filename"] = filename.stem().string();
            info_ = d;
            if(automatic_reload_suppressed_until_save.exchange(false, std::memory_order_acq_rel))
                run_mode = run_mode_restart;

            std::cout << "Saved file \"" << target_path.string() << "\".\n";
            std::string response = "{\n";
            response += "\t\"ok\": true,\n";
            response += "\t\"filename\": " + value(filename.stem().string()).json() + "\n";
            response += "}\n";
            dictionary header({
                {"Session-Id", std::to_string(session_id)},
                {"Package-Type", "save"},
                {"Content-Type", "application/json"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            SendStringResponse(header, response);
        }
        catch(const std::exception & e)
        {
            DoSendError("500 Internal Server Error", "Could not save file: " + std::string(e.what()));
        }
    }


    void
    Kernel::DoSaveState(Request & request)
    {
        try
        {
            std::string filename = ResolveStateFilenameFromRequest(request, "save_state");
            std::string component_path = component_path_from_request(request);
            {
                std::lock_guard<std::recursive_mutex> lock(kernelLock);
                if(components.empty())
                    throw exception("No network is loaded.");
                WaitForAsyncComponents(false);
                SaveState(filename, component_path);
            }

            std::string response = "{\n";
            response += "\t\"ok\": true,\n";
            response += "\t\"filename\": " + value(filename).json() + ",\n";
            response += "\t\"module\": " + value(component_path).json() + "\n";
            response += "}\n";
            dictionary header({
                {"Session-Id", std::to_string(session_id)},
                {"Package-Type", "savestate"},
                {"Content-Type", "application/json"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            SendStringResponse(header, response);
        }
        catch(const std::exception & e)
        {
            DoSendError("500 Internal Server Error", "Could not save state: " + std::string(e.what()));
        }
    }


    void
    Kernel::DoLoadState(Request & request)
    {
        try
        {
            std::string filename = ResolveStateFilenameFromRequest(request, "load_state");
            std::string component_path = component_path_from_request(request);
            {
                std::lock_guard<std::recursive_mutex> lock(kernelLock);
                if(components.empty())
                    throw exception("No network is loaded.");
                WaitForAsyncComponents(false);
                LoadState(filename, component_path);
                BuildUISnapshot();
            }

            std::string response = "{\n";
            response += "\t\"ok\": true,\n";
            response += "\t\"filename\": " + value(filename).json() + ",\n";
            response += "\t\"module\": " + value(component_path).json() + "\n";
            response += "}\n";
            dictionary header({
                {"Session-Id", std::to_string(session_id)},
                {"Package-Type", "loadstate"},
                {"Content-Type", "application/json"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            SendStringResponse(header, response);
        }
        catch(const std::exception & e)
        {
            DoSendError("500 Internal Server Error", "Could not load state: " + std::string(e.what()));
        }
    }


    void
    Kernel::DoResetState(Request & request)
    {
        try
        {
            std::string component_path = component_path_from_request(request);

            {
                std::lock_guard<std::recursive_mutex> lock(kernelLock);
                if(components.empty())
                    throw exception("No network is loaded.");
                WaitForAsyncComponents(false);
                ResetState(component_path);
                BuildUISnapshot();
            }

            std::string response = "{\n";
            response += "\t\"ok\": true,\n";
            response += "\t\"module\": " + value(component_path).json() + "\n";
            response += "}\n";
            dictionary header({
                {"Session-Id", std::to_string(session_id)},
                {"Package-Type", "resetstate"},
                {"Content-Type", "application/json"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            SendStringResponse(header, response);
        }
        catch(const std::exception & e)
        {
            DoSendError("500 Internal Server Error", "Could not reset state: " + std::string(e.what()));
        }
    }


    void
    Kernel::DoQuit(Request & request)
    {
        Notify(msg_print, "quit");
        Stop();
        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            run_mode = run_mode_quit;
        }
        DoUpdate(request);
    }


    void
    Kernel::DoStop(Request & request)
    {
        // td::cout << "Kernel::DoStop" << std::endl;
        Notify(msg_print, "stop");
        Stop();
        DoUpdate(request);
    }


    bool
    Kernel::SanitizeProjectPath(const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const
    {
        if(candidate_path.empty())
            return false;

        std::error_code ec;
        std::filesystem::path project_root = std::filesystem::weakly_canonical(options_.ikaros_root, ec);
        if(ec)
            return false;

        std::filesystem::path resolved_path = candidate_path.is_absolute() ? candidate_path : project_root / candidate_path;
        resolved_path = std::filesystem::weakly_canonical(resolved_path, ec);
        if(ec)
            return false;

        auto root_it = project_root.begin();
        auto root_end = project_root.end();
        auto path_it = resolved_path.begin();
        auto path_end = resolved_path.end();

        for(; root_it != root_end && path_it != path_end; ++root_it, ++path_it)
            if(*root_it != *path_it)
                return false;

        if(root_it != root_end)
            return false;

        sanitized_path = resolved_path;
        return true;
    }


    bool
    Kernel::SanitizePathUnderRoot(const std::filesystem::path & root, const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const
    {
        if(root.empty() || candidate_path.empty() || candidate_path.is_absolute())
            return false;

        std::error_code ec;
        std::filesystem::path resolved_root = std::filesystem::weakly_canonical(root, ec);
        if(ec)
            return false;

        std::filesystem::path resolved_path = std::filesystem::weakly_canonical(resolved_root / candidate_path, ec);
        if(ec)
            return false;

        auto root_it = resolved_root.begin();
        auto root_end = resolved_root.end();
        auto path_it = resolved_path.begin();
        auto path_end = resolved_path.end();

        for(; root_it != root_end && path_it != path_end; ++root_it, ++path_it)
            if(*root_it != *path_it)
                return false;

        if(root_it != root_end)
            return false;

        sanitized_path = resolved_path;
        return true;
    }


    bool
    Kernel::SanitizeImportPath(const std::filesystem::path & candidate_path, std::filesystem::path & sanitized_path) const
    {
        if(candidate_path.empty())
            return false;

        std::error_code ec;
        std::filesystem::path project_root = std::filesystem::weakly_canonical(options_.ikaros_root, ec);
        if(ec)
            return false;

        std::filesystem::path user_root = std::filesystem::weakly_canonical(user_dir, ec);
        if(ec)
            return false;

        std::filesystem::path base_path = candidate_path;
        if(candidate_path.is_relative())
        {
            std::filesystem::path current_network = options_.full_path();
            if(!current_network.empty())
                base_path = current_network.parent_path() / candidate_path;
            else
                base_path = std::filesystem::current_path() / candidate_path;
        }

        std::filesystem::path resolved_path = std::filesystem::weakly_canonical(base_path, ec);
        if(ec)
            return false;

        auto is_within_root = [](const std::filesystem::path & root, const std::filesystem::path & path)
        {
            auto root_it = root.begin();
            auto root_end = root.end();
            auto path_it = path.begin();
            auto path_end = path.end();

            for(; root_it != root_end && path_it != path_end; ++root_it, ++path_it)
                if(*root_it != *path_it)
                    return false;

            return root_it == root_end;
        };

        if(!is_within_root(project_root, resolved_path) && !is_within_root(user_root, resolved_path))
            return false;

        sanitized_path = resolved_path;
        return true;
    }


    void
    Kernel::LoadXMLWithRestrictedIncludes(dictionary & d, const std::filesystem::path & filename) const
    {
        std::vector<std::filesystem::path> include_roots;
        include_roots.push_back(options_.ikaros_root);
        include_roots.push_back(user_dir);

        std::error_code ec;
        std::filesystem::path resolved_file = std::filesystem::weakly_canonical(filename, ec);
        if(!ec && !resolved_file.parent_path().empty())
            include_roots.push_back(resolved_file.parent_path());

        d.load_xml(filename.string(), include_roots);
    }


    Kernel::SendFileResult
    Kernel::SendFileIfSafe(const std::filesystem::path & root, const std::string & file)
    {
        std::filesystem::path sanitized_path;
        if(!SanitizeProjectPath(root / file, sanitized_path))
            return SendFileResult::forbidden;

        std::error_code ec;
        if(!std::filesystem::is_regular_file(sanitized_path, ec) || ec)
            return SendFileResult::not_found;

        return socket->SendFile(sanitized_path) ? SendFileResult::sent : SendFileResult::not_found;
    }


    Kernel::SendFileResult
    Kernel::SendPublicWebUIFileIfSafe(const std::filesystem::path & root, const std::string & file)
    {
        std::filesystem::path sanitized_path;
        if(!SanitizePathUnderRoot(root, std::filesystem::path(file), sanitized_path))
            return SendFileResult::forbidden;

        std::error_code ec;
        if(!std::filesystem::is_regular_file(sanitized_path, ec) || ec)
            return SendFileResult::not_found;

        return socket->SendFile(sanitized_path) ? SendFileResult::sent : SendFileResult::not_found;
    }


    void
    Kernel::DoSendFile(std::string file)
    {
        if(file.empty())
            return;

        if(file[0] == '/')
            file = file.erase(0,1); // Remove initial slash

        // if(socket->SendFile(file, ikc_dir))  // Check IKC-directory first to allow files to be overriden
        //    return;

        //std::cout << "Sending file: " << file << std::endl;

        bool forbidden = false;
        auto try_send_file = [this, &file, &forbidden](const std::filesystem::path & root)
        {
            SendFileResult result = SendFileIfSafe(root, file);
            if(result == SendFileResult::forbidden)
                forbidden = true;
            return result == SendFileResult::sent;
        };

        if(try_send_file(user_dir))   // Look in user directory
            return;

        if(try_send_file(webui_dir))   // Now look in WebUI directory
            return;

        if(try_send_file(std::filesystem::path(webui_dir) / "Images"))   // Now look in WebUI/Images directory
            return;

        if(try_send_file(std::filesystem::path(webui_dir) / ".."))   // Now look in Source directory
            return;

        if(forbidden)
            DoSendError("403 Forbidden", "403 Forbidden\n");
        else
            DoSendError("404 Not Found", "404 Not Found\n");

    

        /*
 
        
        file = "error." + rcut(file, ".");
        if(socket->SendFile("error." + rcut(file, "."), webui_dir)) // Try to send error file
            return;

        DoSendError();
        */
    }


    void
    Kernel::DoSendPublicWebUIFile(std::string file)
    {
        if(file.empty() || file == "/")
            file = "index.html";

        if(file[0] == '/')
            file = file.erase(0,1);

        bool forbidden = false;
        auto try_send_public_file = [this, &forbidden](const std::filesystem::path & root, const std::string & requested_file)
        {
            SendFileResult result = SendPublicWebUIFileIfSafe(root, requested_file);
            if(result == SendFileResult::forbidden)
                forbidden = true;
            return result == SendFileResult::sent;
        };

        if(try_send_public_file(webui_dir, file))
            return;

        if(starts_with(file, "images/"))
        {
            std::string rewritten = "Images/" + file.substr(7);
            if(try_send_public_file(webui_dir, rewritten))
                return;
        }
        else if(starts_with(file, "models/"))
        {
            std::string rewritten = "Models/" + file.substr(7);
            if(try_send_public_file(webui_dir, rewritten))
                return;
        }

        if(forbidden)
            DoSendError("403 Forbidden", "403 Forbidden\n");
        else
            DoSendError("404 Not Found", "404 Not Found\n");
    }


    void
    Kernel::DoSendNetwork(Request & request)
    {
        std::string s = json();
        {
            std::lock_guard<std::mutex> lock(ui_client_mutex);
            ui_client_states[request.client_id].last_seen_time = steady_clock::now();
        }

        std::string log_json = ConsumeLogForClient(request.client_id);
        if(s.size() > 0 && s.back() == '}')
        {
            s.pop_back();
            s += log_json;
            s += "\n}";
        }

        //std::cout << s << std::endl;

        dictionary rtheader({
            {"Session-Id", std::to_string(session_id)},
            {"Package-Type", "network"},
            {"Content-Type", "application/json"},
            {"Content-Length", std::to_string(s.size())}
        });
        SendStringResponse(rtheader, s);
    }


    void
    Kernel::DoPause(Request & request)
    {
        Notify(msg_print, "pause");
        try
        {
            Pause();
        }
        catch(const exception& e)
        {
            Notify(msg_warning, e.what(), e.path());
        }
        DoSendData(request);
    }



    void
    Kernel::DoStep(Request & request)
    {
        Notify(msg_print, "step");
        bool tick_succeeded = false;
        try
        {
            Pause();
            run_mode = run_mode_pause;
            tick_succeeded = Tick();
            if(tick_succeeded)
                BuildUISnapshot();
            timer.SetPauseTime(GetTime()+tick_duration);
        }
        catch(const fatal_runtime_error & e)
        {
            needs_reload = true;
            Notify(msg_fatal_error, e.what(), e.path());
            Stop();
        }
        catch(const exception& e)
        {
            Notify(msg_warning, e.what(), e.path());
        }
        DoSendData(request, tick_succeeded);
    }



    void
    Kernel::DoRealtime(Request & request)
    {
        Notify(msg_print, "realtime");
        try
        {
            Realtime();
        }
        catch(const exception& e)
        {
             Notify(msg_warning, e.what(), e.path());
        }
        DoSendData(request);
    }


    void
    Kernel::DoPlay(Request & request)
    {
        Notify(msg_print, "play");
        try
        {
            Play();

        }
        catch(const exception& e)
        {
            Notify(msg_warning, e.what(), e.path());
        }
        DoSendData(request);
    }



    void
    Kernel::DoData(Request & request)
    {
        dictionary header({
            {"Content-Type", "text/plain"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string key = normalize_request_value_path(request.component_path);

        std::string body;
        if(!buffers.count(key) || state_buffers.count(key))
        {
            body = "Buffer \""+request.component_path+"\" can not be found";
            SendStringResponse(header, body);
            return;
        }
        if(ValueOwnedByRunningAsyncComponent(key))
        {
            body = "Buffer \"" + request.component_path + "\" is currently being updated asynchronously";
            SendStringResponse(header, body);
            return;
        }
        if(buffers[key].rank() > 2)
        {
            body = "Rank of matrix != 2. Cannot be displayed as CSV";
            SendStringResponse(header, body);
            return;
        }
        SendStringResponse(header, buffers[key].csv());
    }



    void
    Kernel::DoJSON(Request & request)
    {
        std::string key = normalize_request_value_path(request.component_path);
        std::string format = rtail(key, ":");

        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string body;

        auto send_json_response = [&](const std::string & json_value, const std::vector<int> & shape)
        {
            body = "{\"path\": ";
            body += value(request.component_path).json();
            body += ", \"shape\": [";

            std::string sep;
            for(int dim : shape)
            {
                body += sep;
                body += std::to_string(dim);
                sep = ", ";
            }

            body += "], \"value\": ";
            body += json_value;
            body += "}";
        };

        if(buffers.count(key) && !state_buffers.count(key))
        {
            if(ValueOwnedByRunningAsyncComponent(key))
            {
                dictionary error;
                error["error"] = "Value \"" + request.component_path + "\" is currently being updated asynchronously";
                SendStringResponse(header, error.json());
                return;
            }
            send_json_response(format == "metadata" ? buffers[key].metadata_json() : buffers[key].json(), buffers[key].shape());
            SendStringResponse(header, body);
            return;
        }

        if(parameters.count(key))
        {
            parameter & parameter_value = parameters[key];
            if(parameter_value.get_type() == matrix_type)
            {
                const matrix & matrix_value = parameter_value.matrix_ref();
                send_json_response(format == "metadata" ? matrix_value.metadata_json() : parameter_value.json(), matrix_value.shape());
            }
            else
                send_json_response(parameter_value.json(), {});
            SendStringResponse(header, body);
            return;
        }

        std::string component_path = peek_rhead(key, ".");
        std::string attribute = peek_rtail(key, ".");

        if(components.count(component_path))
        {
            if(components[component_path]->IsAsyncRunning())
            {
                dictionary error;
                error["error"] = "Value \"" + request.component_path + "\" is currently being updated asynchronously";
                SendStringResponse(header, error.json());
                return;
            }
            std::string json_data = components[component_path]->json(attribute);
            if(!json_data.empty())
            {
                send_json_response(json_data, {});
                SendStringResponse(header, body);
                return;
            }
        }

        dictionary error;
        error["error"] = "Value \"" + request.component_path + "\" can not be found";
        SendStringResponse(header, error.json());
    }



    void
    Kernel::DoCSV(Request & request)
    {
        dictionary header({
            {"Content-Type", "text/csv; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string key = normalize_request_value_path(request.component_path);

        if(!buffers.count(key) || state_buffers.count(key))
        {
            SendStringResponse(header, "Buffer \""+request.component_path+"\" can not be found");
            return;
        }
        if(ValueOwnedByRunningAsyncComponent(key))
        {
            SendStringResponse(header, "Buffer \"" + request.component_path + "\" is currently being updated asynchronously");
            return;
        }
        if(buffers[key].rank() > 2)
        {
            SendStringResponse(header, "Rank of matrix != 2. Cannot be displayed as CSV");
            return;
        }
        SendStringResponse(header, buffers[key].csv());
    }



    void
    Kernel::DoImage(Request & request)
    {
        std::string key = normalize_request_value_path(request.component_path);

        matrix * image = nullptr;

        if(buffers.count(key) && !state_buffers.count(key))
        {
            if(ValueOwnedByRunningAsyncComponent(key))
            {
                dictionary header({
                    {"Content-Type", "text/plain"},
                    {"Cache-Control", "no-cache, no-store"},
                    {"Pragma", "no-cache"}
                });
                SendStringResponse(header, "Matrix \"" + request.component_path + "\" is currently being updated asynchronously");
                return;
            }
            image = &buffers[key];
        }
        else if(parameters.count(key) && parameters[key].get_type() == matrix_type)
        {
            image = &parameters[key].matrix_ref();
        }

        if(!image)
        {
            dictionary header({
                {"Content-Type", "text/plain"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            SendStringResponse(header, "Matrix \"" + request.component_path + "\" can not be found");
            return;
        }

        jpeg_data jpeg;

        if(image->rank() == 2)
            jpeg = create_gray_jpeg(*image, 0, 1, 90);
        else if(image->rank() == 3 && image->size(0) == 3)
            jpeg = create_color_jpeg(*image, 90);

        if(jpeg.empty())
        {
            dictionary header({
                {"Content-Type", "text/plain"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"}
            });
            SendStringResponse(header, "Matrix \"" + request.component_path + "\" can not be converted to JPEG");
            return;
        }

        dictionary header({
            {"Content-Type", "image/jpeg"},
            {"Content-Length", std::to_string(jpeg.size())},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        socket->SendHTTPHeader(header);
        socket->Flush();
        if(jpeg.size() > static_cast<std::size_t>(std::numeric_limits<long>::max()))
            throw std::length_error("JPEG data is too large for the socket API.");
        socket->SendData(reinterpret_cast<const char *>(jpeg.data()),
                         static_cast<long>(jpeg.size()));
    }



    void
    Kernel::DoProfiling(Request & request)
    {
        bool active = true;
        if(request.parameters.contains("active"))
            active = request.parameters.is_set("active");
        SetProfilingClientActive(request.client_id, active);

        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        SendStringResponse(header, GetProfilingJSON());
    }


    void
    Kernel::DoStartupSteps(Request &)
    {
        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        SendStringResponse(header, GetStartupStepsJSON());
    }



    void
    Kernel::DoCommand(Request & request)
    {
        std::string key;
        try
        {
            request.MergeJsonBodyIntoParameters();

            key = normalize_request_value_path(request.component_path);

            std::lock_guard<std::recursive_mutex> lock(kernelLock);

            if(!components.count(key))
            {
                Notify(msg_warning, "Component '"+request.component_path+"' could not be found.");
            }
            else if(!request.parameters.contains("command"))
            {
                Notify(msg_warning, "No command specified for  '"+request.component_path+"'.");
            }
            else if(components.at(key)->IsAsyncRunning())
            {
                std::string command_name = request.parameters["command"];
                components.at(key)->QueueDeferredCommand(command_name, request.parameters);
                Notify(msg_print, "Queued command \"" + command_name + "\" for asynchronous component \"" + key + "\".", key);
            }
            else
            {
                components.at(key)->Command(request.parameters["command"], request.parameters);
            }
        }
        catch(const exception & e)
        {
            Notify(msg_warning, "While handling WebUI command for module \"" + key + "\": " + e.message(),
                   e.path().empty() ? key : e.path());
        }
        catch(const std::exception & e)
        {
            Notify(msg_warning, "While handling WebUI command for module \"" + key + "\": " + e.what(), key);
        }
        DoSendData(request);
    }



    void
    Kernel::DoControl(Request & request)
    {
        std::string key;
        try
        {
            request.MergeJsonBodyIntoParameters();

            key = normalize_request_value_path(request.component_path);

            std::lock_guard<std::recursive_mutex> lock(kernelLock);

            if(!parameters.count(key))
            {
                Notify(msg_warning, "Parameter '"+request.component_path+"' could not be found.");
            }
            else
            {
                if(Component * component = ComponentForValuePath(key); component != nullptr && component->IsAsyncRunning())
                {
                    Component::DeferredParameterChange change;
                    change.parameter_path = key;
                    change.value = "1";
                    if(request.parameters.contains("value"))
                        change.value = std::string(request.parameters["value"]);

                    parameter & p = parameters.at(key);
                    if(p.get_type() == matrix_type)
                    {
                        change.is_matrix_cell = true;
                        if(request.parameters.contains("x"))
                            change.x = request.parameters["x"];
                        if(request.parameters.contains("y"))
                            change.y = request.parameters["y"];
                    }

                    component->QueueDeferredParameterChange(change);
                    Notify(msg_print, "Queued parameter change for asynchronous component \"" + component->path_ + "\".", component->path_);
                    DoSendData(request);
                    return;
                }

                parameter & p = parameters.at(key);
                if(p.get_type() == matrix_type)
                {
                    int x = 0;
                    int y = 0;
                    double value = 1;

                    if(request.parameters.contains("x"))
                        x = request.parameters["x"];

                    if(request.parameters.contains("y"))
                        y = request.parameters["y"];
                    
                    if(request.parameters.contains("value"))
                        value = request.parameters["value"];

                    matrix & matrix_value = p.matrix_ref();
                    if(matrix_value.rank() == 1)
                        matrix_value(x)= value;
                    else if(matrix_value.rank() == 2)
                        matrix_value(y,x)= value; // Is this correct?
                    else
                        throw exception("Higher-dimensional matrix parameters are not supported by /control.");
                }
                else
                {
                    SetParameter(key, std::string(request.parameters["value"]));
                }
            }
        }
        catch(const exception & e)
        {
            Notify(msg_warning, "While handling WebUI control for \"" + key + "\": " + e.message(),
                   e.path().empty() ? key : e.path());
        }
        catch(const std::exception & e)
        {
            Notify(msg_warning, "While handling WebUI control for \"" + key + "\": " + e.what(), key);
        }
        DoSendData(request);
    }


    void
    Kernel::DoUpdate(Request & request)
    {
        long current_session_id = 0;
        {
            std::lock_guard<std::mutex> lock(ui_snapshot_mutex);
            if(current_ui_snapshot)
                current_session_id = current_ui_snapshot->session_id;
        }
        if(current_session_id == 0)
        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            current_session_id = session_id;
        }

        if(request.session_id != current_session_id)
        {
            std::lock_guard<std::recursive_mutex> lock(kernelLock);
            DoSendNetwork(request);
        }
        else
            DoSendData(request, true, true);
    }


    void
    Kernel::DoNetwork(Request & request)
    {
        DoSendNetwork(request);
    }


    void
    Kernel::DoSendClasses(Request &)
    {
        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string body = "{\"classes\":[\n\t\"";
        std::string s = "";
        for(auto & [name, component_class]: classes)
        {
            if(component_class.info_.is_set("internal"))
                continue;

            body += s;
            body += escape_json_string(name);
            s = "\",\n\t\"";
        }
        body += "\"\n]\n}\n";
        SendStringResponse(header, body);
    }



    void
    Kernel::DoSendClassInfo(Request &)
    {
        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string body = "{\n";
        std::string s = "";
        for(auto & [name, component_class]: classes)
        {
            if(component_class.info_.is_set("internal"))
                continue;

            body += s;
            body += "\""+escape_json_string(name)+"\": ";
            dictionary class_info = component_class.info_.copy();
            class_info["path"] = std::filesystem::path(component_class.path).parent_path().string();
            body += class_info.json();
            s = ",\n\t";
        }
        body += "\n}\n";
        SendStringResponse(header, body);
    }



    void
    Kernel::DoSendClassReadMe(Request & request)
    {
        dictionary header({
            {"Content-Type", "text/plain; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string body;

        if(!request.parameters.contains("class"))
        {
            body = "No class selected.";
            SendStringResponse(header, body);
            return;
        }

        std::string class_name = request.parameters["class"];
        if(!classes.count(class_name))
        {
            body = "Class not found: " + class_name;
            SendStringResponse(header, body);
            return;
        }

        std::filesystem::path class_path = classes[class_name].path;
        if(class_path.empty())
        {
            body = "No class path available for: " + class_name;
            SendStringResponse(header, body);
            return;
        }

        std::filesystem::path readme_path = class_path.parent_path() / "ReadMe.md";
        std::ifstream readme_file(readme_path);
        if(!readme_file.is_open())
        {
            body = "No ReadMe.md found for: " + class_name;
            SendStringResponse(header, body);
            return;
        }

        body.assign((std::istreambuf_iterator<char>(readme_file)), std::istreambuf_iterator<char>());
        SendStringResponse(header, body);
    }



    void
    Kernel::DoSendFileList(Request &)
    {
        // Scan for files

        system_files.clear();
        examples_files.clear();
        user_files.clear();
        user_state_files.clear();
        ScanFiles(options_.ikaros_root+"/Source/Modules");
        ScanFiles(options_.ikaros_root+"/Examples", false, true);
        ScanFiles(user_dir, false);

        // Send result

        dictionary header({
            {"Content-Type", "application/json; charset=utf-8"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"}
        });
        std::string sep;
        std::string body = "{\"system_files\":[\n\t\"";
        for(auto & [name, path]: system_files)
        {
            (void)path;
            body += sep;
            body += escape_json_string(name);
            sep = "\",\n\t\"";
        }
        body += "\"\n],\n";
    
        sep = "";
        body += "\"examples_files\":[\n\t\"";
        for(auto & [name, path]: examples_files)
        {
            (void)path;
            body += sep;
            body += escape_json_string(name);
            sep = "\",\n\t\"";
        }
        body += "\"\n],\n";

        sep = "";
        body += "\"user_files\":[\n\t\"";
        for(auto & [name, path]: user_files)
        {
            (void)path;
            body += sep;
            body += escape_json_string(name);
            sep = "\",\n\t\"";
        }
        body += "\"\n],\n";

        sep = "";
        body += "\"user_state_files\":[\n\t\"";
        for(auto & [name, path]: user_state_files)
        {
            (void)path;
            body += sep;
            body += escape_json_string(name);
            sep = "\",\n\t\"";
        }
        body += "\"\n]\n";
        body += "}\n";
        SendStringResponse(header, body);
    }


    void
    Kernel::DoSendError(const std::string & status, const std::string & message)
    {
    dictionary header({
        {"Content-Type", "text/plain"},
    });
    SendStringResponse(header, message, status.c_str());
    }


    void
    Kernel::DoUnauthorized()
    {
        dictionary header({
            {"Content-Type", "text/plain"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"},
            {"Expires", "0"}
        });
        SendStringResponse(header, "401 Unauthorized\n", "401 Unauthorized");
    }


    void
    Kernel::DoAuthStatus()
    {
        dictionary header({
            {"Content-Type", "application/json"},
            {"Cache-Control", "no-cache, no-store"},
            {"Pragma", "no-cache"},
            {"Expires", "0"}
        });
        std::string body =
            "{\n"
            "\t\"enabled\": " + std::string(auth_enabled_ ? "true" : "false") + ",\n"
            "\t\"authenticated\": " + std::string((!auth_enabled_ || IsRequestAuthenticated()) ? "true" : "false") + "\n"
            "}\n";
        SendStringResponse(header, body);
    }


    void
    Kernel::DoLogin(Request & request)
    {
        if(!auth_enabled_)
        {
            DoAuthStatus();
            return;
        }

        try
        {
            if(!request.HasJsonBody())
                throw exception("Login request must include a JSON body.");
            if(!request.json_body.is_dictionary())
                throw exception("Login request body must be a JSON object.");

            dictionary login_request(request.json_body);
            std::string candidate_password =
                login_request.contains_non_null("password") ? std::string(login_request["password"]) : "";
            if(!CheckPassword(candidate_password))
            {
                DoUnauthorized();
                return;
            }

            std::string token = CreateSessionToken();
            if(token.empty())
            {
                DoUnauthorized();
                return;
            }

            dictionary header({
                {"Content-Type", "application/json"},
                {"Set-Cookie", "ikaros_session=" + token + "; Path=/; Max-Age=2592000; HttpOnly; SameSite=Strict"},
                {"Cache-Control", "no-cache, no-store"},
                {"Pragma", "no-cache"},
                {"Expires", "0"}
            });
            SendStringResponse(header, "{\n\t\"authenticated\": true\n}\n");
        }
        catch(const std::exception &)
        {
            DoUnauthorized();
        }
    }


    void
    Kernel::HandleHTTPRequest()
    {
        long sid = 0;
        const dictionary & request_header = socket->RequestHeader();
        if(request_header.contains_non_null("session-id"))
            sid = atol(std::string(request_header["session-id"]).c_str());

        long cid = 0;
        if(request_header.contains_non_null("client-id"))
            cid = atol(std::string(request_header["client-id"]).c_str());

        std::string content_type;
        if(request_header.contains_non_null("content-type"))
            content_type = std::string(request_header["content-type"]);

        std::optional<Request> parsed_request;
        try
        {
            parsed_request.emplace(std::string(request_header["uri"]), sid,
                                   socket->RequestBody(), content_type, cid);
        }
        catch(const std::exception & e)
        {
            DoSendError("400 Bad Request", e.what());
            return;
        }
        Request & request = *parsed_request;

        if(request.parameters.contains("proxy"))
            request.component_path = std::string(request.parameters["proxy"]);

        if(request == "auth")
        {
            DoAuthStatus();
            return;
        }
        else if(request == "login")
        {
            DoLogin(request);
            return;
        }
        else if(auth_enabled_ && !IsRequestAuthenticated())
        {
            if(IsPublicRequest(request))
            {
                DoSendPublicWebUIFile(request.url);
                return;
            }

            DoUnauthorized();
            return;
        }

        using RequestHandler = void (Kernel::*)(Request &);
        static const std::unordered_map<std::string, RequestHandler> authenticated_routes = {
            {"network", &Kernel::DoNetwork},
            {"update", &Kernel::DoUpdate},
            {"quit", &Kernel::DoQuit},
            {"stop", &Kernel::DoStop},
            {"pause", &Kernel::DoPause},
            {"step", &Kernel::DoStep},
            {"play", &Kernel::DoPlay},
            {"realtime", &Kernel::DoRealtime},
            {"new", &Kernel::DoNew},
            {"open", &Kernel::DoOpen},
            {"save", &Kernel::DoSave},
            {"savestate", &Kernel::DoSaveState},
            {"save_state", &Kernel::DoSaveState},
            {"loadstate", &Kernel::DoLoadState},
            {"load_state", &Kernel::DoLoadState},
            {"resetstate", &Kernel::DoResetState},
            {"reset_state", &Kernel::DoResetState},
            {"classes", &Kernel::DoSendClasses},
            {"classinfo", &Kernel::DoSendClassInfo},
            {"classreadme", &Kernel::DoSendClassReadMe},
            {"files", &Kernel::DoSendFileList},
            {"data", &Kernel::DoData},
            {"json", &Kernel::DoJSON},
            {"csv", &Kernel::DoCSV},
            {"image", &Kernel::DoImage},
            {"profiling", &Kernel::DoProfiling},
            {"startupsteps", &Kernel::DoStartupSteps},
            {"command", &Kernel::DoCommand},
            {"control", &Kernel::DoControl},
        };

        if(request.command.empty())
        {
            DoSendFile("index.html");
            return;
        }

        auto route = authenticated_routes.find(request.command);
        if(route != authenticated_routes.end())
        {
            (this->*route->second)(request);
            return;
        }

        DoSendFile(request.url);
    }

    void
    Kernel::HandleHTTPThread()
    {
        while(!shutdown)
        {
            try
            {
                if(socket != nullptr && socket->QueueRequest(true))
                {
                    ServerSocket::QueuedRequest queued_request;
                    if(!socket->PopRequest(queued_request))
                        continue;

                    std::string method = queued_request.header.contains_non_null("method") ? std::string(queued_request.header["method"]) : "";
                    std::string uri = queued_request.header.contains_non_null("uri") ? std::string(queued_request.header["uri"]) : "";
                    socket->ActivateRequest(std::move(queued_request));
                    bool is_update_request = uri.find("/update") != std::string::npos;
                    bool waits_before_locking =
                        request_path_matches_command(uri, "quit") ||
                        request_path_matches_command(uri, "stop") ||
                        request_path_matches_command(uri, "new") ||
                        request_path_matches_command(uri, "open");

                    if((is_update_request || waits_before_locking) && (method == "GET" || method == "PUT"))
                    {
                        HandleHTTPRequest();
                    }
                    else
                    {
                        std::lock_guard<std::recursive_mutex> lock(kernelLock); // Lock the mutex to ensure thread safety
                        const dictionary & request_header = socket->RequestHeader();
                        if(request_header.contains_non_null("method") && std::string(request_header["method"]) == "GET")
                        {
                            HandleHTTPRequest();
                        }
                        else if(request_header.contains_non_null("method") && std::string(request_header["method"]) == "PUT") // JSON Data
                        {
                            HandleHTTPRequest();
                        }
                        socket->Flush();
                        socket->FinishActiveRequest();
                        continue;
                    }
                    socket->Flush();
                    socket->FinishActiveRequest();
                }
            }
            catch(const std::exception & e)
            {
                if(shutdown.load(std::memory_order_acquire))
                    break;
                Notify(msg_warning, "While handling HTTP request: " + std::string(e.what()));
                if(socket != nullptr)
                    socket->Close();
            }
        }
    }


    void *
    Kernel::StartHTTPThread(Kernel * k)
    {
    k->HandleHTTPThread();
    return nullptr;
    }

    void
    Kernel::InitSocket(long port)
    {
        try
        {
            if(port < 0 || port > 65535)
                throw std::invalid_argument("Server port must be between 0 and 65535");
            shutdown.store(false, std::memory_order_release);
            socket = std::make_unique<ServerSocket>(static_cast<int>(port), GetOption("bind_address"));
        }
        catch (const exception& e)
        {
            throw socket_startup_error("Ikaros is unable to start the webserver on port " + std::to_string(port) + ": " + e.message(), e.path());
        }
        catch (const std::exception& e)
        {
            throw socket_startup_error("Ikaros is unable to start the webserver on port " + std::to_string(port) + ": " + std::string(e.what()));
        }

        httpThread = std::thread(Kernel::StartHTTPThread, this);
    }


    void
    Kernel::StopHTTPServer()
    {
        shutdown.store(true, std::memory_order_release);

        if(socket != nullptr)
            socket->StopListening();

        if(httpThread.joinable())
        {
            httpThread.join();
        }

        socket.reset();
    }

}; // namespace ikaros
