//
//     Ikaros_XML.h
//     XMLParser
//
//    Version 1.0.1
//
//    Copyright (C) 2001-2008  Christian Balkenius
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
//	Created 2006-03-05.
//

#pragma once

inline constexpr bool XML_DEBUG = false;

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>


class XMLElement;

class XMLNode
{
public:
    XMLNode *	parent;
    XMLNode *	prev;
    XMLNode *	next;
    void *		aux;		// used for extensions

    XMLNode() : parent(nullptr), prev(nullptr), next(nullptr), aux(nullptr)
    {};
    virtual ~XMLNode()
    {
        delete next;
    };

    virtual void	Print(FILE * f, int d);

    virtual XMLElement * GetElement(const char * element_name=nullptr);
    virtual bool IsElement(const char * =nullptr)
    {
        return false;
    }
    
    virtual void      SetPrev(XMLNode * p); // recursively set prev links
    virtual XMLNode * Disconnect();   // disconnect from XML tree
};



class XMLCharacterData : public XMLNode
{
public:
	char * data;
    bool cdata;

    XMLCharacterData(char * s, bool cd, XMLNode * n);
    virtual ~XMLCharacterData();

    void	Print(FILE * f, int d) override;
};



class XMLComment : public XMLNode
{
public:
	char * data;

    XMLComment(char * s, XMLNode * n);
    virtual ~XMLComment();

    void	Print(FILE * f, int d) override;
};



class XMLAttribute : public XMLNode
{
public:
	char *	name;
	char *	value;
    char 	quote;

    XMLAttribute(char * nm, char * v, int q, XMLNode * n);
    virtual ~XMLAttribute();

    void    RemoveDuplicates();

    void	Print(FILE * f, int d) override;
};



class XMLElement : public XMLNode
{
public:
    XMLNode * content;
    XMLAttribute * attributes;

	char *	name;
    bool	empty;

    XMLElement(XMLNode * p, char * nm, XMLAttribute * a, bool e);
    XMLElement(XMLNode * p, char * nm, XMLAttribute * a, bool e, XMLNode * c, XMLNode * n);
    virtual ~XMLElement();

    void	Print(FILE * f, int d) override;

	bool IsElement(const char * e=nullptr) override // Is this an element with a particular name or any element (if nullptr)
    {
        return !e || !strcmp(e, name);
    }
	
    void SetPrev(XMLNode * p) override;
    
    const char *	GetAttribute(const char *);

    std::string operator[](const std::string a)
    {
        const char * v = GetActualAttribute( a.c_str());
        if(v)
            return std::string(v);
        else
            return "";

    };


    const char *    GetActualAttribute(const char *);    // without inheritence but with variables - temporary
	void			SetAttribute(const char * a, const char * v);

    XMLElement * GetElement(const char * element_name=nullptr) override; // nullptr indicates any element
    virtual XMLElement * GetContentElement(const char * element_name=nullptr);
    virtual XMLElement * GetNextElement(const char * element_name=nullptr);
    virtual XMLElement * GetParentElement();
};



class XMLProcessingInstruction : public XMLNode
{
public:
	char * name;
	char * content;

    XMLProcessingInstruction(char * nm, char * c, XMLNode * n);
    virtual ~XMLProcessingInstruction();

    void	Print(FILE * f, int d) override;

    bool IsElement(const char * =nullptr) override
    {
        return false;
    }
};



inline constexpr int initial_buffer_size = 16768;

class XMLDocument
{
public:
    FILE *	f = nullptr;
    int		line = 1;
    int		character = 1;
    bool	debug_mode = XML_DEBUG;
    char	* buffer = nullptr;
    long    buffer_size = 0;
    int		pos = 0;

    const char *	action = "";
    int				action_line = 0;

    char 	errbuf[1024]{};

    XMLElement * xml = nullptr;
    XMLNode * prolog = nullptr;
    std::filesystem::path filename_;
    std::filesystem::path base_dir_;
    std::vector<std::filesystem::path> include_roots_;
    std::vector<std::filesystem::path> include_stack_;
    int include_depth_ = 0;

    XMLDocument(const char * filename, bool included = false);
    XMLDocument(const char * filename, bool included, const std::vector<std::filesystem::path> & include_roots);
    XMLDocument(const char * filename, bool included, const std::vector<std::filesystem::path> & include_roots, const std::vector<std::filesystem::path> & include_stack, int include_depth);
    ~XMLDocument();

    XMLElement * ReleaseXML() noexcept;

    bool		Match(const char c, bool skip=true);
    bool		Match(const char * s, bool skip=true);

    void		Skip(const char * t, int n);
    void		SkipWhitespace(const char *t);

    char *		Push(const char * t, int n);
    char *		PushUntil(const char *t, const char * s0, const char * s1=nullptr);
    char *		PushName(const char *t);

    void		SetAction(const char *);
    bool        PathIsUnderRoot(const std::filesystem::path & root, const std::filesystem::path & path) const;
    std::filesystem::path ResolveIncludedFilename(const std::string & filename);

    XMLNode *	ParseXMLDeclaration(XMLNode * parent);
    XMLNode *   ParseIncludedFile(XMLNode * parent); // non standard
    XMLNode *	ParseProcessingInstruction(XMLNode * parent);
    XMLNode *	ParseComment(XMLNode * parent);
    XMLNode *	ParseDoctype(XMLNode * parent);
    XMLNode *	ParseCDATA(XMLNode * parent);
    XMLNode *	ParseCharacterData(XMLNode * parent);
    XMLAttribute *	ParseAttribute(const char * element_name, bool & empty);
    XMLNode *	ParseElement(XMLNode * parent);
    XMLNode *	Parse(XMLNode * parent);

    void		Print(FILE * f);

private:
    std::unique_ptr<char[]> buffer_storage_;
    std::unique_ptr<XMLElement> xml_storage_;
    std::unique_ptr<XMLNode> prolog_storage_;
    int parse_depth_ = 0;
    int element_depth_ = 0;
};
