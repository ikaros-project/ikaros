// xml.cc Copyright (C) 2001-2024  Christian Balkenius

#include "xml.h"
#include "exceptions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cctype>
#include <algorithm>
#include <stdexcept>


static constexpr int max_xml_include_depth = 32;
using owned_c_string = std::unique_ptr<char[]>;


static char *
create_string(const char * c)
{
    if (c)
    {
        char * p = strcpy(new char [strlen(c)+1], c);
        return p;
    }
    else
        return nullptr;
}


static void
destroy_string(char * c)
{
    delete [] c;
}


static bool
is_xml_whitespace(const char * text)
{
    if(text == nullptr)
        return true;

    for(const char * c = text; *c != 0; ++c)
        if(*c != ' ' && *c != '\t' && *c != '\n' && *c != '\r')
            return false;
    return true;
}


static bool
is_valid_xml_character(unsigned long code)
{
    return code == 0x09 || code == 0x0A || code == 0x0D ||
           (code >= 0x20 && code <= 0xD7FF) ||
           (code >= 0xE000 && code <= 0xFFFD) ||
           (code >= 0x10000 && code <= 0x10FFFF);
}


static void
append_utf8(std::string & text, unsigned long code)
{
    if(code <= 0x7F)
        text += static_cast<char>(code);
    else if(code <= 0x7FF)
    {
        text += static_cast<char>(0xC0 | (code >> 6));
        text += static_cast<char>(0x80 | (code & 0x3F));
    }
    else if(code <= 0xFFFF)
    {
        text += static_cast<char>(0xE0 | (code >> 12));
        text += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        text += static_cast<char>(0x80 | (code & 0x3F));
    }
    else
    {
        text += static_cast<char>(0xF0 | (code >> 18));
        text += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
        text += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        text += static_cast<char>(0x80 | (code & 0x3F));
    }
}


static std::string
decode_xml_entities(const char * text)
{
    std::string decoded;
    if(!text)
        return decoded;

    for(size_t i = 0; text[i] != 0; ++i)
    {
        if(text[i] != '&')
        {
            decoded += text[i];
            continue;
        }

        size_t entity_end = i + 1;
        while(text[entity_end] != 0 && text[entity_end] != ';')
            ++entity_end;

        if(text[entity_end] != ';')
            throw std::runtime_error("Unterminated XML entity");

        std::string entity(text + i + 1, entity_end - i - 1);
        if(entity == "amp")
            decoded += '&';
        else if(entity == "lt")
            decoded += '<';
        else if(entity == "gt")
            decoded += '>';
        else if(entity == "quot")
            decoded += '"';
        else if(entity == "apos")
            decoded += '\'';
        else if(!entity.empty() && entity[0] == '#')
        {
            unsigned long base = 10;
            size_t number_pos = 1;
            if(entity.size() > 2 && (entity[1] == 'x' || entity[1] == 'X'))
            {
                base = 16;
                number_pos = 2;
            }

            if(number_pos == entity.size())
                throw std::runtime_error("Empty numeric XML entity");

            unsigned long code = 0;
            for(; number_pos < entity.size(); ++number_pos)
            {
                const char c = entity[number_pos];
                unsigned long digit;
                if(c >= '0' && c <= '9')
                    digit = static_cast<unsigned long>(c - '0');
                else if(base == 16 && c >= 'a' && c <= 'f')
                    digit = static_cast<unsigned long>(c - 'a' + 10);
                else if(base == 16 && c >= 'A' && c <= 'F')
                    digit = static_cast<unsigned long>(c - 'A' + 10);
                else
                    throw std::runtime_error("Invalid digit in numeric XML entity");

                if(code > (0x10FFFF - digit) / base)
                    throw std::runtime_error("Numeric XML entity is out of range");
                code = code * base + digit;
            }

            if(!is_valid_xml_character(code))
                throw std::runtime_error("Numeric XML entity is not a valid XML character");
            append_utf8(decoded, code);
        }
        else
            throw std::runtime_error("Unknown XML entity &" + entity + ";");

        i = entity_end;
    }

    return decoded;
}


void
XMLNode::Print(FILE * f, int d)
{
    if (next != nullptr)
        next->Print(f, d);
}



XMLElement *
XMLNode::GetElement(const char * element_name)
{
    if (next != nullptr)
        return next->GetElement(element_name);
    else
        return nullptr;
}



void
XMLNode::SetPrev(XMLNode * p)
{
    this->prev = p;
    if(this->next)
        this->next->SetPrev(this);
}



XMLNode *
XMLNode::Disconnect()
{
    if(prev != nullptr)
        prev->next = next;
    else if(parent != nullptr)
        ((XMLElement *)parent)->content = next;
    
    parent = nullptr;
    prev = nullptr;
    next = nullptr;

    return this;
}



XMLCharacterData::XMLCharacterData(char * s, bool cd, XMLNode * n) :
        XMLNode()
{
    data = s;
    cdata = cd;
    next = n;
}



XMLCharacterData::~XMLCharacterData()
{
    destroy_string(data);
}



void
XMLCharacterData::Print(FILE * f, int d)
{
    if (cdata) fprintf(f, "<![CDATA[");
    fprintf(f, "%s", data);
    if (cdata) fprintf(f, "]]>");
    if (next != nullptr)
        next->Print(f, d);
}



XMLComment::XMLComment(char * s, XMLNode * n) :
        XMLNode()
{
    data = s;
    next = n;
}



XMLComment::~XMLComment()
{
    destroy_string(data);
}



void
XMLComment::Print(FILE * f, int d)
{
    fprintf(f, "<!--%s-->", data);
    if (next != nullptr)
        next->Print(f, d);
}



XMLAttribute::XMLAttribute(char * nm, char * v, int q, XMLNode * n) :
        XMLNode()
{
    name = nm;
    value = v;
    quote = q;
    next = n;
}



XMLAttribute::~XMLAttribute()
{
    destroy_string(name);
    destroy_string(value);
}


void
XMLAttribute::Print(FILE * f, int d)
{
    fprintf(f, " %s=%c%s%c", name, quote, value, quote);

    if (next != nullptr)
        next->Print(f, d);
}



XMLElement::XMLElement(XMLNode * p, char * nm, XMLAttribute * a, bool e) :
        XMLNode()
{
    parent = p;
    name = nm;
    attributes = a;
    empty = e;
    content = nullptr;
    next = nullptr;
}



XMLElement::XMLElement(XMLNode * p, char * nm, XMLAttribute * a, bool e, XMLNode * c, XMLNode * n) :
        XMLNode()
{
    parent = p;
    name = nm;
    attributes = a;
    empty = e;
    content = c;
    next = n;
}



XMLElement::~XMLElement()
{
	destroy_string(name);
    delete attributes;
    delete content;
}



void
XMLElement::Print(FILE * f, int d)
{
    if(empty)
    {
        fprintf(f, "<%s", name);
        if (attributes != nullptr)
            attributes->Print(f, d);
        fprintf(f, "/>");
        if (next != nullptr)
            next->Print(f, d);
        return;
    }

    fprintf(f, "<%s", name);
    if (attributes != nullptr)
        attributes->Print(f, d);
    fprintf(f, ">");
    if(content != nullptr)
        content->Print(f, d+1);
    fprintf(f, "</%s>", name);
    if (next != nullptr)
        next->Print(f, d);
}



void
XMLElement::SetPrev(XMLNode * p)
{
    if(content)
        content->SetPrev(nullptr);
    if(attributes)
        attributes->SetPrev(nullptr);
    XMLNode::SetPrev(p);
}



const char *
XMLElement::GetAttribute(const char * attribute_name)       // Implement variables and inheritance here - not standard XML but implemented here for simpicity
                                                            // FIXME: move to kernel and always use kernel function exept for raw acces to XML
{
    for (XMLAttribute * a = attributes; a != nullptr; a = (XMLAttribute *)(a->next))
        if (!strcmp(a->name, attribute_name))
        {
            if(a->value[0] != '@')
            {
                return a->value;
            }
            else // Variables
            {
                const char * v = GetAttribute(&a->value[1]);
                if(v)
                    return v;
            }
        }

    // Inhertiance
    if(parent != nullptr && parent->IsElement())
    {
        return ((XMLElement *)(parent))->GetAttribute(attribute_name);
    }

    return nullptr;
}




const char *
XMLElement::GetActualAttribute(const char * attribute_name)
{
    for (XMLAttribute * a = attributes; a != nullptr; a = (XMLAttribute *)(a->next))
        if (!strcmp(a->name, attribute_name))
        {
            if(a->value[0] != '@')
            {
                return a->value;
            }
            else // Variables
            {
                const char * v = GetAttribute(&a->value[1]);
                if(v)
                    return v;
            }
        }

    return nullptr;
}


void
XMLElement::SetAttribute(const char * attribute_name, const char * value)
{
    for (XMLAttribute * a = attributes; a != nullptr; a = (XMLAttribute *)(a->next))
        if (!strcmp(a->name, attribute_name))
        {
			owned_c_string new_value(create_string(value));
			destroy_string(a->value);
			a->value = new_value.release();
			return;
		}

    owned_c_string new_name(create_string(attribute_name));
    owned_c_string new_value(create_string(value));
    std::unique_ptr<XMLAttribute> new_attribute(
        new XMLAttribute(new_name.get(), new_value.get(), '\"', attributes));
    new_name.release();
    new_value.release();
	attributes = new_attribute.release();
	if(attributes->next != nullptr)
		attributes->next->prev = attributes;
	attributes->parent = this;
}



// Finds the next element with the supplied name or any element if element_name == NULL
XMLElement *
XMLElement::GetElement(const char * element_name)
{
    if (IsElement(element_name))
        return this;
    else if (next != nullptr)
        return GetNextElement(element_name);

    return nullptr;
}


// Finds the first element with a particular name in the contents of the current element
XMLElement *
XMLElement::GetContentElement(const char * element_name)
{
    if (content != nullptr)
        return content->GetElement(element_name);
    else
        return nullptr;
}



XMLElement *
XMLElement::GetNextElement(const char * element_name)
{
	if(next)
		return next->GetElement(element_name);
	else
		return nullptr;
	
/*    
	for (XMLNode * xml = next; xml != NULL; xml = xml->next)
        if (xml->IsElement(element_name))
            return (XMLElement *)(xml);

    return NULL;
*/
}



XMLElement *
XMLElement::GetParentElement()
{
    return (XMLElement *)parent;
}


XMLProcessingInstruction::XMLProcessingInstruction(char * nm, char * c, XMLNode * n) :
    XMLNode()
{
    name = nm;
    content = c;
    next = n;
}



XMLProcessingInstruction::~XMLProcessingInstruction()
{
	destroy_string(name);
	destroy_string(content);
}



void
XMLProcessingInstruction::Print(FILE * f, int d)
{
    fprintf(f, "<?%s", name);
    fprintf(f, "%s", content);
    fprintf(f, "?>");
    if (next != nullptr)
        next->Print(f, d);
    return;
}

 

XMLDocument::XMLDocument(const char * filename, bool included)
    : XMLDocument(filename, included, std::vector<std::filesystem::path>())
{
}


XMLDocument::XMLDocument(const char * filename, bool included, const std::vector<std::filesystem::path> & include_roots)
    : XMLDocument(filename, included, include_roots, std::vector<std::filesystem::path>(), 0)
{
}


XMLDocument::XMLDocument(const char * filename, bool included, const std::vector<std::filesystem::path> & include_roots, const std::vector<std::filesystem::path> & include_stack, int include_depth)
{
    include_depth_ = include_depth;

    std::error_code ec;
    filename_ = std::filesystem::weakly_canonical(std::filesystem::path(filename), ec);
    if(ec)
        filename_ = std::filesystem::absolute(std::filesystem::path(filename), ec);
    if(ec)
        filename_ = std::filesystem::path(filename);

    base_dir_ = filename_.parent_path();

    for(const auto & root : include_roots)
    {
        if(root.empty())
            continue;
        std::filesystem::path resolved_root = std::filesystem::weakly_canonical(root, ec);
        if(!ec)
            include_roots_.push_back(resolved_root);
        ec.clear();
    }

    include_stack_ = include_stack;
    if(include_depth_ > max_xml_include_depth)
        throw std::runtime_error("Maximum XML include depth exceeded");
    if(std::find(include_stack_.begin(), include_stack_.end(), filename_) != include_stack_.end())
        throw std::runtime_error("Recursive XML include");
    include_stack_.push_back(filename_);

    std::unique_ptr<FILE, decltype(&fclose)> input(fopen(filename, "rb"), &fclose);
    if(input == nullptr)
    {
        printf("XML: Could not open \"%s\".\n", filename);
        throw std::runtime_error("File not found");
    }
    f = input.get();
    
    // allocate buffer
    
    buffer_size = initial_buffer_size;
    buffer_storage_ = std::make_unique<char[]>(buffer_size);
    buffer = buffer_storage_.get();

    try
    {
        std::unique_ptr<XMLNode> parsed(Parse(nullptr));
			if (parsed == nullptr)
				throw "File is empty";
        parsed->SetPrev(nullptr);

        XMLElement * root = nullptr;
        for(XMLNode * node = parsed.get(); node != nullptr; node = node->next)
        {
            if(node->IsElement())
            {
                if(root != nullptr && !included)
                    throw "XML document contains more than one root element";
                if(root == nullptr)
                    root = static_cast<XMLElement *>(node);
                continue;
            }

            XMLCharacterData * character_data = dynamic_cast<XMLCharacterData *>(node);
            if(!included && character_data != nullptr &&
               (character_data->cdata || !is_xml_whitespace(character_data->data)))
                throw "Non-whitespace content is not allowed outside the root element";
        }

        if(root == nullptr)
            throw "XML contains no root element";

        // Disconnect prolog before root element
        
        if(root->prev != nullptr)
            root->prev->next = nullptr;
        root->prev = nullptr;
        
        if(parsed.get() == root)
            parsed.release();

        std::unique_ptr<XMLElement> parsed_root(root);
            
        // Delete data after root (should never have been parsed)
        
        if(!included)
        {
            delete parsed_root->next;
            parsed_root->next = nullptr;
        }

        prolog_storage_ = std::move(parsed);
        prolog = prolog_storage_.get();
        xml_storage_ = std::move(parsed_root);
        xml = xml_storage_.get();
    }
    catch (const char * msg)
    {
        printf("%s: ", filename);
        printf("%s at line %d, position %d", msg, line, character);
        if (action_line != 0)
            printf(" while %s at line %d\n", action, action_line);
        else
            printf("\n");

        if (action_line != 0)
        {
            throw ikaros::exception(std::string(msg)+ " at line "+std::to_string(line)+" position "+std::to_string(character)+" while "+std::string(action)+" at line "+std::to_string(action_line));
        }
        else
        {
            throw ikaros::exception(std::string(msg)+ " at line "+std::to_string(line)+" position "+std::to_string(character));
        }
        // exit(1);
    }
    catch (const std::exception & e)
    {
        printf("%s: %s at line %d, position %d", filename, e.what(), line, character);
        if (action_line != 0)
            printf(" while %s at line %d\n", action, action_line);
        else
            printf("\n");

        if (action_line != 0)
            throw ikaros::exception(std::string(e.what())+ " at line "+std::to_string(line)+" position "+std::to_string(character)+" while "+std::string(action)+" at line "+std::to_string(action_line));
        else
            throw ikaros::exception(std::string(e.what())+ " at line "+std::to_string(line)+" position "+std::to_string(character));
    }

    f = nullptr;
}



XMLDocument::~XMLDocument() = default;


XMLElement *
XMLDocument::ReleaseXML() noexcept
{
    xml = nullptr;
    return xml_storage_.release();
}



bool
XMLDocument::Match(const char c, bool skip)
{
    long p = ftell(f);
    if (feof(f)) return false;		// REPORT ERROR ********
    bool r = (fgetc(f) == c);
    fseek(f, p, SEEK_SET);
    if(skip && r)
        Skip("##", 1);
    return r;
}



bool
XMLDocument::Match(const char * s, bool skip)
{
    long p = ftell(f);
    int i = 0;
    for (; s[i]; i++)
    {
        if (feof(f)) break;			// REPORT ERROR ********
        if (fgetc(f) != s[i]) break;
    }
    fseek(f, p, SEEK_SET);

    if (skip && !s[i]) Skip("##", i);

    return !s[i];
}



void
XMLDocument::Skip(const char * t, int n)
{
    for (int i=0; i<n; i++)
    {
        char c= fgetc(f);

        if (c == 0xA)
        {
            line++;
            character=1;
            if (debug_mode) printf("%s %03d:%02d\t\t\t0xA\n\n", t, line, character);
        }

        else if (c == 0x9)
        {
            if (debug_mode) printf("%s %03d:%02d\t\t\t\t0x9\t\n", t, line, character);
        }

        else
        {
            if (debug_mode) printf("%s %03d:%02d\t\t\t%c\n", t, line, character, c);
            character++;
        }
    }
}



void
XMLDocument::SkipWhitespace(const char *t)
{
    while (!feof(f))
    {
        char c= fgetc(f);
        ungetc(c, f);	// Peek
        switch (c)
        {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            Skip(t, 1);
            break;
        default:
            return;
        }
    }

    throw "Unexpected end of file (1)";
}



char *
XMLDocument::Push(const char * t, int n)
{
    while (pos + n + 1 > buffer_size)
    {
        // grow buffer size

        std::unique_ptr<char[]> new_buffer = std::make_unique<char[]>(buffer_size + 16768);
        memcpy(new_buffer.get(), buffer, buffer_size);
        buffer_size += 16768;
        buffer_storage_ = std::move(new_buffer);
        buffer = buffer_storage_.get();
    }

    for (int i=0; i<n; i++)
    {
        char c= fgetc(f);
        if (feof(f)) return nullptr; // throw "Unexpected end of file (2)";

        if (c == 0xA)
        {
            line++;
            character=1;
            if (debug_mode) printf("%s %03d:%02d [%d]\t\t0xA\n", t, line, character, pos);
        }

        else if (c == 0x9)
        {
            if (debug_mode) printf("%s %03d:%02d [%d]\t\t0x9\n", t, line, character, pos);
            character++;
        }

        else
        {
            if (debug_mode) printf("%s %03d:%02d [%d]\t\t%c\n", t, line, character, pos, c);
            character++;
        }

        buffer[pos++] = c;		// Check bounds later***
    }
    return (pos > 0 ? buffer : nullptr);
}



char *
XMLDocument::PushUntil(const char * t, const char * s0 , const char * s1)
{
    pos = 0;
    while (!feof(f))
    {
        if (	(s0 != nullptr && Match(s0, false)) ||
                (s1 != nullptr && Match(s1, false))
           )
        {
            buffer[pos] = 0;
            return buffer;
        }
        else
            Push(t, 1);
    }

    buffer[pos] = 0;
    return (pos > 0 ? buffer : nullptr);
}



char *
XMLDocument::PushName(const char * t)
{
    pos = 0;
    for (int i=0;; i++)
    {
        char c= fgetc(f);
        ungetc(c, f); // Peek
        if (feof(f)) throw "Unexpected end of file (3)";

        if (	('a' <= c && c <= 'z') ||
                ('A' <= c && c <= 'Z') ||
                c == '_' ||
                c == ':' ||
                (i != 0 && c == '-') ||
                (i != 0 && c == '.') ||
                (i != 0 && '0' <= c && c <= '9')
           )
            Push(t, 1);
        else
        {
            buffer[pos] = 0;
            return (pos > 0 ? buffer : nullptr);	// test needed???
        }
    }
    return nullptr; // internal error throw
}



void
XMLDocument::SetAction(const char * s)
{
    action = s;
    action_line = line;
}


bool
XMLDocument::PathIsUnderRoot(const std::filesystem::path & root, const std::filesystem::path & path) const
{
    auto root_it = root.begin();
    auto root_end = root.end();
    auto path_it = path.begin();
    auto path_end = path.end();

    for(; root_it != root_end && path_it != path_end; ++root_it, ++path_it)
        if(*root_it != *path_it)
            return false;

    return root_it == root_end;
}


std::filesystem::path
XMLDocument::ResolveIncludedFilename(const std::string & filename)
{
    if(filename.empty())
        throw std::runtime_error("XML include filename is empty");
    if(include_depth_ >= max_xml_include_depth)
        throw std::runtime_error("Maximum XML include depth exceeded");

    std::filesystem::path include_path(filename);
    if(include_path.is_relative())
        include_path = base_dir_ / include_path;

    std::error_code ec;
    include_path = std::filesystem::weakly_canonical(include_path, ec);
    if(ec)
        throw std::runtime_error("Could not resolve XML include \"" + filename + "\"");

    for(const auto & root : include_roots_)
        if(PathIsUnderRoot(root, include_path))
            return include_path;

    throw std::runtime_error("XML include \"" + filename + "\" is outside the allowed include roots");
}



XMLNode *
XMLDocument::ParseXMLDeclaration(XMLNode * parent)
{
    SetAction("parsing XML declaration");
    PushUntil("XD", "?>");
    Skip("XD", 2);
    SkipWhitespace("WS");	// TEST
    return Parse(parent);	// Ignore this, return next element
}



XMLNode *
XMLDocument::ParseIncludedFile(XMLNode * parent)
{
    SetAction("parsing included file");
//    PushName("N<");
//    char * name = create_string(buffer);

//    if (pos == 0) throw "Valid character for processing instruction name expected";

    SkipWhitespace("WS");
    if(!Match("file"))
            throw "'file' expected";
    SkipWhitespace("WS");
    if(!Match("="))
            throw "'=' expected";
    SkipWhitespace("WS");
    if(!Match("\""))
            throw "'\"' expected";
    PushUntil("FL", "\"");
    char * filename = create_string(buffer);
    std::string include_filename = filename ? filename : "";
    destroy_string(filename);

    PushUntil("IN", "?>");
    Skip("IN", 2);

    if(include_filename.empty())
        throw std::runtime_error("XML include filename is empty");
    if(include_depth_ >= max_xml_include_depth)
        throw std::runtime_error("Maximum XML include depth exceeded");

    std::filesystem::path resolved_filename;
    if(include_roots_.empty())
    {
        resolved_filename = include_filename;
        if(resolved_filename.is_relative())
            resolved_filename = base_dir_ / resolved_filename;
    }
    else
        resolved_filename = ResolveIncludedFilename(include_filename);

    std::unique_ptr<XMLDocument> xml_doc = std::make_unique<XMLDocument>(
        resolved_filename.string().c_str(), true, include_roots_, include_stack_, include_depth_ + 1);

    std::unique_ptr<XMLNode> included_xml(xml_doc->ReleaseXML());
    XMLNode * final_node = included_xml.get();
    while(final_node->next)
        final_node = final_node->next;

    std::unique_ptr<XMLNode> remainder(Parse(parent));
    final_node->next = remainder.release();

    return included_xml.release();
}



XMLNode *
XMLDocument::ParseProcessingInstruction(XMLNode * parent)
{
    SetAction("parsing procesing instruction");
    PushName("N<");
    owned_c_string name(create_string(buffer));

    if (pos == 0) throw "Valid character for processing instruction name expected";

    PushUntil("PI", "?>");
    owned_c_string content(create_string(buffer));

    Skip("PI", 2);

    std::unique_ptr<XMLNode> next(Parse(parent));
    std::unique_ptr<XMLProcessingInstruction> instruction(
        new XMLProcessingInstruction(name.get(), content.get(), next.get()));
    name.release();
    content.release();
    next.release();
    return instruction.release();
}



XMLNode *
XMLDocument::ParseComment(XMLNode * parent)
{
    SetAction("parsing comment");
    owned_c_string text(create_string(PushUntil("CM", "-->")));
    Skip("CO", 3);
    std::unique_ptr<XMLNode> next(Parse(parent));
    std::unique_ptr<XMLComment> comment(new XMLComment(text.get(), next.get()));
    text.release();
    next.release();
    return comment.release();
}



XMLNode *
XMLDocument::ParseDoctype(XMLNode * parent) // The DOCTYPE will be removed
{
    SetAction("parsing doctype");
    int c = 1;
    while (c >0)
    {
        if (feof(f)) throw "Unexpected end of file (4)";
        PushUntil("DT", "<", ">");
        if (Match("<"))
            c++;
        else if (Match(">"))
            c--;
    }
    return Parse(parent);
}



XMLNode *
XMLDocument::ParseCDATA(XMLNode * parent) // Return character data with CDATA tag
{
    SetAction("parsing CDATA");
    owned_c_string text(create_string(PushUntil("CD", "]]>")));
    PushUntil("CD", "]]>");
    Skip("CD", 3);
    std::unique_ptr<XMLNode> next(Parse(parent));
    std::unique_ptr<XMLCharacterData> node(new XMLCharacterData(text.get(), true, next.get()));
    text.release();
    next.release();
    return node.release();
}



XMLNode *
XMLDocument::ParseCharacterData(XMLNode * parent)
{
    SetAction("parsing character data");
    owned_c_string text(create_string(PushUntil("DA", "<")));
    if(text)
    {
        std::unique_ptr<XMLNode> next(Parse(parent));
        std::unique_ptr<XMLCharacterData> node(new XMLCharacterData(text.get(), false, next.get()));
        text.release();
        next.release();
        return node.release();
    }
    else
        return Parse(parent); // ** experimental **
}



XMLAttribute *
XMLDocument::ParseAttribute(const char * element_name, bool & empty)
{
    SetAction("parsing attributes");
    SkipWhitespace("WS");

    if (Match("/"))	// empty element
    {
        SkipWhitespace("WS");
        if (!Match(">"))
            throw "'>' expected after '/'";
        empty = true;
        return nullptr;
    }

    if (Match(">"))
    {
        empty = false;
        return nullptr;
    }

    if (Match("<"))
    {
        snprintf(errbuf, 1024, "Start tag <%s> not terminated", element_name);
        throw errbuf;
    }

    owned_c_string name(create_string(PushName("AT")));
    if (name == nullptr) throw "Attribute not found";

    // Check that attribute does not already exist ***

    SkipWhitespace("WS");
    if (!Match("=")) throw "'=' expected";
    SkipWhitespace("WS");

    const char  * q = nullptr;
    if (Match('"'))
        q = "\"";
    else if (Match('\''))
        q = "'";
    else
        throw "Quoted attribute value expected";

    PushUntil("AV", "<", q);
    if (Match('<')) throw "< not allowed in attribute value";
    if(!Match(q))
        throw "Closing quote expected for attribute value";

    owned_c_string value(create_string(buffer));
    
    std::string decoded_value = decode_xml_entities(value.get());
    owned_c_string decoded(create_string(decoded_value.c_str()));
    std::unique_ptr<XMLAttribute> next(ParseAttribute(element_name, empty));
    std::unique_ptr<XMLAttribute> attribute(
        new XMLAttribute(name.get(), decoded.get(), q[0], next.get()));
    name.release();
    decoded.release();
    next.release();
    return attribute.release();
}



void
XMLAttribute::RemoveDuplicates()
{
    for (XMLAttribute * a = this; a != nullptr; a = (XMLAttribute *)(a->next))
    {
        XMLAttribute * b = static_cast<XMLAttribute *>(a->next);
        while(b != nullptr)
        {
            XMLAttribute * next = static_cast<XMLAttribute *>(b->next);
            if(!strcmp(a->name, b->name) && a != b)
            {
                printf("WARNING: Redefined attribute. Will use %s = \"%s\".\n", a->name, a->value);

                if(b->prev != nullptr)
                    b->prev->next = b->next;
                if(b->next != nullptr)
                    b->next->prev = b->prev;
                b->prev = nullptr;
                b->next = nullptr;
                delete b;
            }
            b = next;
        }
    }
}



XMLNode *
XMLDocument::ParseElement(XMLNode * parent)
{
    SetAction("parsing element");
    int start_line = line;
    PushName("N<");
    owned_c_string name(create_string(buffer));

    if (pos == 0) throw "Valid character for tag name expected";

    bool empty = false;
    std::unique_ptr<XMLAttribute> attributes(ParseAttribute(name.get(), empty));

    if(attributes)
    {
        attributes->SetPrev(nullptr);
        attributes->RemoveDuplicates();
    }
    if(empty)
    {
        std::unique_ptr<XMLNode> next(Parse(parent));
        std::unique_ptr<XMLElement> element(
            new XMLElement(parent, name.get(), attributes.get(), true, nullptr, next.get()));
        name.release();
        attributes.release();
        next.release();
        return element.release();
    }

    std::unique_ptr<XMLElement> element(new XMLElement(parent, name.get(), attributes.get(), false));
    name.release();
    attributes.release();
    std::unique_ptr<XMLNode> content(Parse(element.get()));
    element->content = content.release();

    if (!Match("</"))
    {
        snprintf(errbuf, 1024, "End tag </%s> expected", element->name);
        throw errbuf;
    }

    PushName("N>");

    if (strcmp(element->name, buffer))
    {
        snprintf(errbuf, 1024, "Start tag <%s> at line %d does not match end tag </%s>",
                 element->name, start_line, buffer);
        throw errbuf;
    }

    PushUntil("ET", ">");
    Skip("ET", 1);

    std::unique_ptr<XMLNode> next(Parse(parent));
    element->next = next.release();
    return element.release();
}



XMLNode *
XMLDocument::Parse(XMLNode * parent)
{
    if (feof(f))
        return nullptr;

    if (Match("<?xml"))
        return ParseXMLDeclaration(parent);

    else if (Match("<?include"))
        return ParseIncludedFile(parent);

    else if (Match("<?"))
        return ParseProcessingInstruction(parent);

    else if (Match("<!--"))
        return ParseComment(parent);

    else if (Match("<!DOCTYPE"))
        return ParseDoctype(parent);

    else if (Match("<![CDATA["))
        return ParseCDATA(parent);

    else if (Match("</", false))
        return nullptr;

    else if (Match("<"))
        return ParseElement(parent);

    else
        return ParseCharacterData(parent);

    return nullptr;
}



void
XMLDocument::Print(FILE * file)
{
    fprintf(file, "<?xml\n\tversion=\"1.0\"\n\tencoding=\"UTF-8\"\n\tstandalone=\"yes\"\n?>\n\n");
    xml->Print(file, 0);
}
