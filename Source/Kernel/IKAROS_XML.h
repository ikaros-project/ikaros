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


#include <cstdio>
#include <cstring>

class XMLElement;

class XMLNode
{
public:
    XMLNode *	parent;
    XMLNode *	prev;
    XMLNode *	next;
    void *		aux;		// used for extensions

    XMLNode() : parent(NULL), prev(NULL), next(NULL), aux(NULL)
    {};
    virtual ~XMLNode()
    {
        delete next;
    };

    virtual void	Print(FILE * f, int d);

    virtual XMLElement * GetElement(const char * element_name=NULL);
    virtual bool IsElement(const char * e=NULL)
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

    virtual void	Print(FILE * f, int d);
};



class XMLComment : public XMLNode
{
public:
	char * data;

    XMLComment(char * s, XMLNode * n);
    virtual ~XMLComment();

    virtual void	Print(FILE * f, int d);
};



class XMLAttribute : public XMLNode
{
public:
	char *	name;
	char *	value;
    char 	quote;

    XMLAttribute(char * nm, char * v, int q, XMLNode * n);
    virtual ~XMLAttribute();

    virtual void	Print(FILE * f, int d);
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

    virtual void	Print(FILE * f, int d);

	virtual bool IsElement(const char * e=NULL) // Is this an element with a particular name or any element (if NULL)
    {
        return !e || !strcmp(e, name);
    }
	
    virtual void SetPrev(XMLNode * p);
    
    const char *	GetAttribute(const char *);			
	void			SetAttribute(const char * a, const char * v);

    virtual XMLElement * GetElement(const char * element_name=NULL); // NULL indicates any element
    virtual XMLElement * GetContentElement(const char * element_name=NULL);
    virtual XMLElement * GetNextElement(const char * element_name=NULL);
    virtual XMLElement * GetParentElement();
};



class XMLProcessingInstruction : public XMLNode
{
public:
	char * name;
	char * content;

    XMLProcessingInstruction(char * nm, char * c, XMLNode * n);
    virtual ~XMLProcessingInstruction();

    virtual void	Print(FILE * f, int d);

    virtual bool IsElement(const char * e=NULL)
    {
        return false;
    }
};



const int max_buffer = 64000;

class XMLDocument
{
public:
    FILE *	f;
    int		line;
    int		character;
    bool	debug_mode;
    char	buffer[max_buffer];
    int		pos;

    const char *	action;
    int				action_line;

    char 	errbuf[1024];

    XMLElement * xml;
    XMLNode * prolog;

    XMLDocument(const char * filename, bool debug = false);
    ~XMLDocument();

    bool		Match(const char c, bool skip=true);
    bool		Match(const char * s, bool skip=true);

    void		Skip(const char * t, int n);
    void		SkipWhitespace(const char *t);

    char *		Push(const char * t, int n);
    char *		PushUntil(const char *t, const char * s0, const char * s1=NULL);
    char *		PushName(const char *t);

    void		SetAction(const char *);

    XMLNode *	ParseXMLDeclaration(XMLNode * parent);
    XMLNode *	ParseProcessingInstruction(XMLNode * parent);
    XMLNode *	ParseComment(XMLNode * parent);
    XMLNode *	ParseDoctype(XMLNode * parent);
    XMLNode *	ParseCDATA(XMLNode * parent);
    XMLNode *	ParseCharacterData(XMLNode * parent);
    XMLAttribute *	ParseAttribute(const char * element_name, bool & empty);
    XMLNode *	ParseElement(XMLNode * parent);
    XMLNode *	Parse(XMLNode * parent);

    void		Print(FILE * f);
};
