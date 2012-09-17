//
//     Ikaros_XML.cc
//     XMLParser
//
//    Version 1.0.2
//
//    Copyright (C) 2001-2009  Christian Balkenius
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    See http://www.ikaros-project.org/ for more information.
//
//  Created 2006-03-05.
//

#include "IKAROS_XML.h"
#include "IKAROS_Utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



void
XMLNode::Print(FILE * f, int d)
{
    if (next != NULL)
        next->Print(f, d);
}



XMLElement *
XMLNode::GetElement(const char * element_name)
{
    if (next != NULL)
        return next->GetElement(element_name);
    else
        return NULL;
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
    if(prev != NULL)
        prev->next = next;
    else if(parent != NULL)
        ((XMLElement *)parent)->content = next;
    
    parent = NULL;
    prev = NULL;
    next = NULL;

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
    if (next != NULL)
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
    if (next != NULL)
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

    if (next != NULL)
        next->Print(f, d);
}



XMLElement::XMLElement(XMLNode * p, char * nm, XMLAttribute * a, bool e) :
        XMLNode()
{
    parent = p;
    name = nm;
    attributes = a;
    empty = e;
    content = NULL;
    next = NULL;
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
        if (attributes != NULL)
            attributes->Print(f, d);
        fprintf(f, "/>");
        if (next != NULL)
            next->Print(f, d);
        return;
    }

    fprintf(f, "<%s", name);
    if (attributes != NULL)
        attributes->Print(f, d);
    fprintf(f, ">");
    if(content != NULL)
        content->Print(f, d+1);
    fprintf(f, "</%s>", name);
    if (next != NULL)
        next->Print(f, d);
}



void
XMLElement::SetPrev(XMLNode * p)
{
    if(content)
        content->SetPrev(NULL);
    if(attributes)
        attributes->SetPrev(NULL);
    XMLNode::SetPrev(p);
}



const char *
XMLElement::GetAttribute(const char * attribute_name)
{
    for (XMLAttribute * a = attributes; a != NULL; a = (XMLAttribute *)(a->next))
        if (!strcmp(a->name, attribute_name))
            return a->value;
    return NULL;
}



void
XMLElement::SetAttribute(const char * attribute_name, const char * value)
{
    for (XMLAttribute * a = attributes; a != NULL; a = (XMLAttribute *)(a->next))
        if (!strcmp(a->name, attribute_name))
        {
			destroy_string(a->value);
			a->value = create_string(value);
			return;
		}
	
	attributes = new XMLAttribute(create_string(attribute_name), create_string(value), '\"', attributes);
	if(attributes->next != NULL)
		attributes->next->prev = attributes;
	attributes->parent = this;
}



// Finds the next element with the supplied name or any element if element_name == NULL
XMLElement *
XMLElement::GetElement(const char * element_name)
{
    if (IsElement(element_name))
        return this;
    else if (next != NULL)
        return GetNextElement(element_name);

    return NULL;
}


// Finds the first element with a particular name in the contents of the current element
XMLElement *
XMLElement::GetContentElement(const char * element_name)
{
    if (content != NULL)
        return content->GetElement(element_name);
    else
        return NULL;
}



XMLElement *
XMLElement::GetNextElement(const char * element_name)
{
	if(next)
		return next->GetElement(element_name);
	else
		return NULL;
	
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
    if (next != NULL)
        next->Print(f, d);
    return;
}



XMLDocument::XMLDocument(const char * filename, bool debug)
{
    debug_mode = debug;
    action = "";
    action_line = 0;

    f = fopen(filename, "rb");

    if (f==NULL)
    {
        printf("Could not open \"%s\".\n", filename);
        return;
    }

    line = 1;
    character = 1;

    try
    {
        prolog = Parse(NULL);
        prolog->SetPrev(NULL);
        
        // Find root

        XMLNode * xml_node = prolog;
        while(!xml_node->IsElement())
        {
            if(xml_node->next == NULL)
                throw "XML contains no root element";
            xml_node = xml_node->next;
        }
        xml = (XMLElement *)xml_node;

        // Disconnect prolog before root element
        
        if(xml->prev != NULL)
            xml->prev->next = NULL;
        xml->prev = NULL;
        
        if(prolog == xml)
            prolog = NULL;
            
        // Delete data after root (should never have been parsed)
        
        delete xml->next;
        xml->next = NULL;
        
    }
    catch (const char * msg)
    {
        printf("%s: ", filename);
        printf("%s at line %d, position %d", msg, line, character);
        if (action_line != 0)
            printf(" while %s at line %d\n", action, action_line);
        else
            printf("\n");
        exit(1);
    }

    fclose(f);
}



XMLDocument::~XMLDocument()
{
    delete xml;
    delete prolog;
}



bool
XMLDocument::Match(const char c, bool skip)
{
    long p = ftell(f);
    if (feof(f)) return false;		// REPORT ERROR ********
    bool r = (fgetc(f) == c);
    fseek(f, p, SEEK_SET);
    if (skip) Skip("##", 1);
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
    if (pos >= max_buffer) throw "Buffer overflow";
    for (int i=0; i<n; i++)
    {
        char c= fgetc(f);
        if (feof(f)) return NULL; // throw "Unexpected end of file (2)";

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
    return (pos > 0 ? buffer : NULL);
}



char *
XMLDocument::PushUntil(const char * t, const char * s0 , const char * s1)
{
    pos = 0;
    while (!feof(f))
    {
        if (	(s0 != NULL && Match(s0, false)) ||
                (s1 != NULL && Match(s1, false))
           )
        {
            buffer[pos] = 0;
            return buffer;
        }
        else
            Push(t, 1);
    }

    buffer[pos] = 0;
    return (pos > 0 ? buffer : NULL);
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
            return (pos > 0 ? buffer : NULL);	// test needed???
        }
    }
    return NULL; // internal error throw
}



void
XMLDocument::SetAction(const char * s)
{
    action = s;
    action_line = line;
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
XMLDocument::ParseProcessingInstruction(XMLNode * parent)
{
    SetAction("parsing procesing instruction");
    PushName("N<");
    char * name = create_string(buffer);

    if (pos == 0) throw "Valid character for processing instruction name expected";

    PushUntil("PI", "?>");
    char * content = create_string(buffer);

    Skip("PI", 2);

    return new XMLProcessingInstruction(name, content, Parse(parent));
}



XMLNode *
XMLDocument::ParseComment(XMLNode * parent)
{
    SetAction("parsing comment");
    char * text = create_string(PushUntil("CM", "-->"));
    Skip("CO", 3);
    return new XMLComment(text, Parse(parent));
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
    char * text = create_string(PushUntil("CD", "]]>"));
    PushUntil("CD", "]]>");
    Skip("CD", 3);
    return new XMLCharacterData(text, true, Parse(parent));
}



XMLNode *
XMLDocument::ParseCharacterData(XMLNode * parent)
{
    SetAction("parsing character data");
    char * s = create_string(PushUntil("DA", "<"));
    if(s)
        return new XMLCharacterData(s, false, Parse(parent));
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
        return NULL;
    }

    if (Match(">"))
    {
        empty = false;
        return NULL;
    }

    if (Match("<"))
    {
        sprintf(errbuf, "Start tag <%s> not terminated", element_name);
        throw errbuf;
    }

    char * name = create_string(PushName("AT"));
    if (name == NULL) throw "Attribute not found";

    // Check that attribute does not already exist ***

    SkipWhitespace("WS");
    if (!Match("=")) throw "'=' expected";
    SkipWhitespace("WS");

    const char  * q = NULL;
    if (Match('"'))
        q = "\"";
    else if (Match('\''))
        q = "'";
    else
        throw "Quoted attribute value expected";

    PushUntil("AV", "<", q);
    if (Match('<')) throw "< not allowed in attribute value";
    Match(q);

    char * value = create_string(buffer);
    return new XMLAttribute(name, value, q[0], ParseAttribute(element_name, empty));
}



XMLNode *
XMLDocument::ParseElement(XMLNode * parent)
{
    SetAction("parsing element");
    int start_line = line;
    PushName("N<");
    char * name = create_string(buffer);

    if (pos == 0) throw "Valid character for tag name expected";

    bool empty = false;
    XMLAttribute * attributes = ParseAttribute(name, empty);

    if (empty) return new XMLElement(parent, name, attributes, true, NULL, Parse(parent));

    XMLElement * e = new XMLElement(parent, name, attributes, false);
    XMLNode * content = Parse(e);
    e->content = content;

    if (!Match("</"))
    {
        sprintf(errbuf, "End tag </%s> expected", name);
        throw errbuf;
    }

    PushName("N>");

    if (strcmp(name, buffer))
    {
        sprintf(errbuf, "Start tag <%s> at line %d does not match end tag </%s>", name, start_line, buffer);
        throw errbuf;
    }

    PushUntil("ET", ">");
    Skip("ET", 1);

    e->next =  Parse(parent);
    return e;
}



XMLNode *
XMLDocument::Parse(XMLNode * parent)
{
    if (feof(f))
        return NULL;

    if (Match("<?xml"))
        return ParseXMLDeclaration(parent);

    else if (Match("<?"))
        return ParseProcessingInstruction(parent);

    else if (Match("<!--"))
        return ParseComment(parent);

    else if (Match("<!DOCTYPE"))
        return ParseDoctype(parent);

    else if (Match("<![CDATA["))
        return ParseCDATA(parent);

    else if (Match("</", false))
        return NULL;

    else if (Match("<"))
        return ParseElement(parent);

    else
        return ParseCharacterData(parent);

    return NULL;
}



void
XMLDocument::Print(FILE * file)
{
    fprintf(file, "<?xml\n\tversion=\"1.0\"\n\tencoding=\"UTF-8\"\n\tstandalone=\"yes\"\n?>\n\n");
    xml->Print(file, 0);
}


