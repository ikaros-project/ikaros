#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "ikaros.h"

using namespace ikaros;

static_assert(!std::is_copy_constructible_v<XMLNode>);
static_assert(!std::is_copy_assignable_v<XMLNode>);
static_assert(!std::is_move_constructible_v<XMLNode>);
static_assert(!std::is_move_assignable_v<XMLNode>);
static_assert(!std::is_copy_constructible_v<XMLElement>);
static_assert(!std::is_copy_assignable_v<XMLElement>);
static_assert(!std::is_copy_constructible_v<XMLDocument>);
static_assert(!std::is_copy_assignable_v<XMLDocument>);

namespace
{
    struct ParseFailure
    {
        std::string message;
        std::string path;
    };


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


    ParseFailure
    require_parse_failure(const std::filesystem::path & path, const std::string & message)
    {
        try
        {
            XMLDocument document(path.string().c_str());
        }
        catch(const ikaros::exception & e)
        {
            return {e.message(), e.path()};
        }
        catch(const std::exception & e)
        {
            return {e.what(), ""};
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

        require_parse_failure(files.write("duplicate-attribute.xml",
                                          "<root value=\"one\" value=\"two\"/>"),
                              "duplicate XML attribute was accepted");

        const std::filesystem::path disconnect = files.write(
            "disconnect.xml", "<root first=\"1\" second=\"2\"><one/><two/><three/></root>");
        XMLDocument disconnect_document(disconnect.string().c_str());
        XMLElement * root = disconnect_document.xml;
        XMLNode * one = root->content;
        XMLNode * two = one->next;
        XMLNode * three = two->next;
        {
            std::unique_ptr<XMLNode> detached(two->Disconnect());
            require(one->next == three && three->prev == one,
                    "disconnecting a middle node broke sibling links");
            require(detached->parent == nullptr && detached->prev == nullptr && detached->next == nullptr,
                    "detached middle node retained tree links");
        }
        {
            std::unique_ptr<XMLNode> detached(one->Disconnect());
            require(root->content == three && three->prev == nullptr,
                    "disconnecting the first content node did not update its parent");
        }
        XMLAttribute * first_attribute = root->attributes;
        XMLAttribute * second_attribute = static_cast<XMLAttribute *>(first_attribute->next);
        {
            std::unique_ptr<XMLNode> detached(first_attribute->Disconnect());
            require(root->attributes == second_attribute && second_attribute->prev == nullptr,
                    "disconnecting the first attribute did not update its parent");
        }

        const std::filesystem::path malformed = files.write(
            "malformed.xml", "<root><child></root>");
        for(int i = 0; i < 32; ++i)
            require_parse_failure(malformed, "malformed document was accepted");
        const ParseFailure direct_failure = require_parse_failure(
            malformed, "malformed document was accepted while checking diagnostics");
        require(direct_failure.path == std::filesystem::weakly_canonical(malformed).string(),
                "direct parse failure did not retain its filename");
        require(direct_failure.message.find("line 1") != std::string::npos,
                "direct parse failure did not retain its source location");

        const std::filesystem::path missing = malformed.parent_path() / "missing.xml";
        const ParseFailure missing_failure = require_parse_failure(
            missing, "missing XML document was accepted");
        require(missing_failure.path == std::filesystem::weakly_canonical(missing).string(),
                "file-open failure did not retain the requested filename");

        const std::filesystem::path broken_include = files.write("broken-include.xml", "<child>");
        const std::filesystem::path broken_parent = files.write(
            "broken-parent.xml", "<root><?include file=\"broken-include.xml\"?></root>");
        const ParseFailure included_failure = require_parse_failure(
            broken_parent, "malformed included document was accepted");
        require(included_failure.path == std::filesystem::weakly_canonical(broken_include).string(),
                "included parse failure did not retain the included filename");
        require(included_failure.message.find(
                    "Included from \"" + std::filesystem::weakly_canonical(broken_parent).string() + "\"") !=
                std::string::npos,
                "included parse failure did not retain its include chain");

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

        std::string nested;
        for(int i = 0; i < 257; ++i)
            nested += "<level>";
        for(int i = 0; i < 257; ++i)
            nested += "</level>";
        require_parse_failure(files.write("too-deep.xml", nested),
                              "excessively nested XML document was accepted");

        std::string too_many_nodes = "<root>";
        for(int i = 0; i < 8192; ++i)
            too_many_nodes += "<node/>";
        too_many_nodes += "</root>";
        require_parse_failure(files.write("too-many-nodes.xml", too_many_nodes),
                              "XML parser recursion limit was not enforced");

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
