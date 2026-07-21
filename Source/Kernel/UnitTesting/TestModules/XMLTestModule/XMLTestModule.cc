#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

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


    void
    require_parse_failure(const std::filesystem::path & path, const std::string & message)
    {
        try
        {
            XMLDocument document(path.string().c_str());
        }
        catch(const std::exception &)
        {
            return;
        }
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

        const std::filesystem::path quoted = files.write(
            "quoted.xml", "<root single='one' double=\"two\"/>");
        XMLDocument quoted_document(quoted.string().c_str());
        require(std::string(quoted_document.xml->GetActualAttribute("single")) == "one",
                "single-quoted attribute was not parsed");
        require(std::string(quoted_document.xml->GetActualAttribute("double")) == "two",
                "double-quoted attribute changed while fixing single quotes");

        const std::filesystem::path entities = files.write(
            "entities.xml",
            "<root named=\"&amp;&lt;&gt;&quot;&apos;\" decimal=\"&#229;\" hex=\"&#x1F600;\"/>");
        XMLDocument entity_document(entities.string().c_str());
        require(std::string(entity_document.xml->GetActualAttribute("named")) == "&<>\"'",
                "named XML entities were decoded incorrectly");
        require(std::string(entity_document.xml->GetActualAttribute("decimal")) == "\xC3\xA5",
                "decimal XML entity was not encoded as UTF-8");
        require(std::string(entity_document.xml->GetActualAttribute("hex")) == "\xF0\x9F\x98\x80",
                "hexadecimal XML entity was not encoded as UTF-8");

        const std::vector<std::string> malformed_entities = {
            "&#0;", "&#xD800;", "&#12junk;", "&#x;", "&unknown;", "&amp",
        };
        for(size_t i = 0; i < malformed_entities.size(); ++i)
            require_parse_failure(
                files.write("malformed-entity-" + std::to_string(i) + ".xml",
                            "<root value=\"" + malformed_entities[i] + "\"/>"),
                "malformed XML entity was accepted");

        const std::filesystem::path malformed = files.write(
            "malformed.xml", "<root><child></root>");
        for(int i = 0; i < 32; ++i)
            require_parse_failure(malformed, "malformed document was accepted");

        files.write("included.xml", "<child value=\"included\"/>");
        const std::filesystem::path including = files.write(
            "including.xml", "<root><?include file=\"included.xml\"?></root>");
        dictionary parsed_include;
        parsed_include.load_xml(including.string());
        require(parsed_include["childs"][0]["value"].as_string() == "included",
                "included document was not transferred into the parent tree");

        const std::filesystem::path decorated = files.write(
            "decorated.xml", "<?setup value=\"yes\"?>\n<root/>\n<!-- done -->\n");
        XMLDocument decorated_document(decorated.string().c_str());
        require(decorated_document.xml->IsElement("root"),
                "valid top-level comments or processing instructions were rejected");
        require_parse_failure(files.write("multiple-roots.xml", "<first/><second/>"),
                              "document with multiple roots was accepted");
        require_parse_failure(files.write("leading-content.xml", "text<root/>"),
                              "non-whitespace content before the root was accepted");
        require_parse_failure(files.write("trailing-content.xml", "<root/>text"),
                              "non-whitespace content after the root was accepted");

        const std::filesystem::path recursive = files.write(
            "recursive.xml", "<root><?include file=\"recursive.xml\"?></root>");
        require_parse_failure(recursive, "recursive standalone include was accepted");

        for(int i = 0; i <= 33; ++i)
        {
            const std::string content = i == 33
                ? "<root/>"
                : "<root><?include file=\"depth-" + std::to_string(i + 1) + ".xml\"?></root>";
            files.write("depth-" + std::to_string(i) + ".xml", content);
        }
        require_parse_failure(files.write("depth-start.xml",
                              "<root><?include file=\"depth-0.xml\"?></root>"),
                              "excessively deep standalone include was accepted");

        std::cout << "XML TEST OK" << std::endl;
    }
};

INSTALL_CLASS(XMLTestModule)
