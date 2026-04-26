#include "ikaros.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using namespace ikaros;

class OutputJSON: public Module
{
    matrix      input;
    parameter   filename;
    parameter   clear_file;
    parameter   JSONL;

    std::string output_filename;
    fs::path output_path;

    std::string ComputeOutputFilename()
    {
        std::string base = filename.c_str();
        std::string ext  = static_cast<bool>(JSONL) ? ".jsonl" : ".json";

        // If filename already ends with the right extension, keep it
        if (base.size() >= ext.size() &&
            base.compare(base.size() - ext.size(), ext.size(), ext) == 0)
            return base;

        // Remove any existing .json/.jsonl to avoid duplicates
        if (base.size() >= 6 && base.compare(base.size() - 6, 6, ".jsonl") == 0)
            base.erase(base.size() - 6);
        else if (base.size() >= 5 && base.compare(base.size() - 5, 5, ".json") == 0)
            base.erase(base.size() - 5);

        return base + ext;
    }
    
    // --- Helper: create JSON object for current tick
    std::string MakeFrameJSON()
    {
        std::ostringstream oss;
        oss << "{ \"ts\": \"" << escape_json_string(GetClockTimeString()) << "\", "
            << "\"tick\": " << GetTick() << ", "
            << "\"frame\": ";

            oss << input.json();

        oss << "}";
        return oss.str();
    }



    
    // Ensure a JSON array file exists with an initial "[\n]\n"
    void EnsureArrayFile(const fs::path& path)
    {
        std::error_code ec;
        if (!fs::exists(path, ec) || fs::file_size(path, ec) == 0) {
            std::ofstream init_out(path, std::ios::out | std::ios::trunc);
            init_out << "[\n]\n";  // 4 bytes total
        }
    }


    // Simplified: assume last char in file is always ']'
    void AppendObjectToArrayFile(const fs::path& path, const std::string& obj)
    {
        std::error_code ec;
        auto size = fs::file_size(path, ec);
        if (ec || size < 2) {
            EnsureArrayFile(path);
            size = fs::file_size(path, ec);
        }

        std::fstream io(path, std::ios::in | std::ios::out | std::ios::binary);
        if (!io.is_open()) {
            Notify(msg_warning, "JSONStreamer: Could not open '%s' for append.", filename.c_str());
            return;
        }

        // Overwrite the final ']'
        io.seekp(static_cast<std::streamoff>(size) - 2, std::ios::beg);

        // Insert a comma only if there is already at least one object.
        // Empty array is "[\n]\n" -> size == 4; first insert should NOT add a comma.
        const bool insertComma = (size > 4);
        if (insertComma)
            io << ",\n" << obj << "\n]";
        else
            io << obj << "\n]";

        io.close();
    }
public:
    void Init()
    {
        Bind(input,     "INPUT");
        Bind(filename,  "filename");
        Bind(clear_file,"clear_file");
        Bind(JSONL,     "JSONL");

        output_filename = ComputeOutputFilename();
        if(!kernel().SanitizeWritePath(output_filename, output_path))
            throw std::runtime_error("OutputJSON can only write files inside UserData.");

        // Delete existing file if requested
        if (clear_file) 
        {
            std::error_code ec;
            if (fs::exists(output_path, ec))
                    fs::remove(output_path, ec);
        }
        if (!JSONL)
            EnsureArrayFile(output_path);  // If not JSONL, make sure the array wrapper exists
    }



    void 
    Tick()
    {
        const std::string obj = MakeFrameJSON();

        if (static_cast<bool>(JSONL)) {
            std::ofstream file(output_path, std::ios::out | std::ios::app);
            if (!file.is_open()) {
                Notify(msg_warning, "JSONStreamer: Could not open '%s' for JSONL append.", output_filename.c_str());
                return;
            }
            file << obj << '\n';
            return;
        }

        EnsureArrayFile(output_path);
        AppendObjectToArrayFile(output_path, obj);
    }
};

INSTALL_CLASS(OutputJSON)
