#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "ikaros.h"

using namespace ikaros;

namespace
{
    class TemporaryXMLDirectory
    {
    public:
        TemporaryXMLDirectory()
        {
            const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
            path_ = std::filesystem::temp_directory_path() /
                    ("ikaros-xml-tests-" + std::to_string(suffix));
            if(!std::filesystem::create_directory(path_))
                throw std::runtime_error("Could not create temporary XML test directory");
        }

        ~TemporaryXMLDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path_, error);
        }

        std::filesystem::path
        write(const std::string & name, const std::string & contents) const
        {
            const std::filesystem::path path = path_ / name;
            std::ofstream file(path);
            if(!file)
                throw std::runtime_error("Could not create temporary XML test file");
            file << contents;
            if(!file)
                throw std::runtime_error("Could not write temporary XML test file");
            return path;
        }

    private:
        std::filesystem::path path_;
    };


    void
    require(bool condition, const std::string & message)
    {
        if(!condition)
            throw exception("XMLTestModule: " + message);
    }
}


class XMLTestModule : public Module
{
    void Init() override
    {
        TemporaryXMLDirectory files;
        const std::filesystem::path simple = files.write(
            "simple.xml", "<root value=\"ok\"><child value=\"one\"/></root>");

        for(int i = 0; i < 32; ++i)
        {
            dictionary parsed;
            parsed.load_xml(simple.string());
            require(parsed["value"].as_string() == "ok", "valid document changed during repeated parsing");
            require(parsed["childs"][0]["value"].as_string() == "one",
                    "child element changed during repeated parsing");
        }

        const std::filesystem::path malformed = files.write(
            "malformed.xml", "<root><child></root>");
        for(int i = 0; i < 32; ++i)
        {
            bool rejected = false;
            try
            {
                XMLDocument document(malformed.string().c_str());
            }
            catch(const std::exception &)
            {
                rejected = true;
            }
            require(rejected, "malformed document was accepted");
        }

        files.write("included.xml", "<child value=\"included\"/>");
        const std::filesystem::path including = files.write(
            "including.xml", "<root><?include file=\"included.xml\"?></root>");
        dictionary parsed_include;
        parsed_include.load_xml(including.string());
        require(parsed_include["childs"][0]["value"].as_string() == "included",
                "included document was not transferred into the parent tree");

        std::cout << "XML TEST OK" << std::endl;
    }
};

INSTALL_CLASS(XMLTestModule)
