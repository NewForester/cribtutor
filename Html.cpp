//----------------------------------------------------------------------------//
//
// Implementation file for the Html namespace class of the cribtutor program.
//
// The cribtutor program uses html markup in cribsheets to generate quizzes.
// The program needs some means of recognising and processing this markup.
//
// The Html namespace provides these.  It consists of a simple parser, the
// data structures that support this and a few symbolic constants.
//
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
//
// https://github.com/NewForester/cribtutor
// Copyright (C) 2016, NewForester
// Released under the terms of the GNU GPL v2
//
//----------------------------------------------------------------------------//

#include "Html.h"

#include <cctype>

#include <fstream>
#include <map>
#include <iostream>

using namespace std;

//----------------------------------------------------------------------------//
//
// See Html.h for a description of the class interface and data structures.
//
// The html namespace offers two external routines, one to parse the html in
// a cribsheet file and another to print out the resultant parsed structure.
//
// Each of these simply calls an internal routine.  Both these routines have
// a recursive design to handle the nesting of html elements.
//
// To keep the print routine simple, static (peri-construction time) state
// variables are set by the parser to indicate when to print blank lines etc.
//
// To keep the main parser routine simple, ugly details are delegated to
// a suite of subroutines of which most have a single call site.
//
//----------------------------------------------------------------------------//

namespace       Html
{
    // the main routine of the html parser

    static  void    parseElement (ifstream& input, Element &element, const bool htmlBlockElement = true);

    // the routines that actually read from the input.

    static  string& readToNextTag  (ifstream& input, string &content);
    static  string& readTag        (ifstream& input, string &tag, string &content, const string &endTag);
    static  string& readComment    (ifstream& input, string &comment, const string& content);
    inline  void    swallowNewLines(ifstream& input, bool condition);

    // the two second level routines that do some real work

    static  Element*    newElement (ifstream& input, const Element& parent, const string &tag, string& content);
    static  void    addSubelement (ifstream& input, Element& parent, Element& element, string& content, const bool htmlBlockElement);

    // helper routines with a single call site in parseElement()

    inline  bool    unexpectedCloseTag  (const string &tag, string& content);
    inline  bool    unexpectedNestedTag (const string &tag, const string &previous, string& content);

    inline  bool    checkImageElement (string &tag);
    inline  void    stripAttributes   (string &tag);
    inline  bool    openAndCloseTag   (string &tag);

    // the two third level routines that do some real work

    static  void    tidyContent (string& content, const string &tag, const bool startOfElement, const bool endOfElement);
    static  void    checkForPairedTerms (Element &element);

    // helper routines with a single call site in parseElement(), newElement() or addSubelement()

    inline  bool    endOfSentence (const string& content);
    inline  bool    startOfSentence (const Element& parent, const string& content);
    inline  bool    extraNewLine (const Element& parent, const Element &element);

    inline  bool    lineBeforeSubElement (const Element& parent, Element& element, const bool htmlBlockElement);
    inline  bool    lineAfterSubElement (const Element& element);

    //------

    // recursive routine to print an element and its nested sub elements

    static  void    printElement (ostream &stream, const Element& element, string indent = "");

    // html is hierarchy that requires a recursive print routine

    extern  ostream& operator<< (ostream &stream, const Element& element)
    {
        return (printElement (stream, element), stream);
    }

    // for debug of printElement() only

  /* global */ bool verbose = false;

    //------

    // encapsulate the substitution of html escape sequences

    namespace   Escapes
    {
        static  string& replace (string &statement);
    };
};

//----------------------------------------------------------------------------//
//
// The main routine of the html parser.
//
// The parser is procedural and handles nested elements by recursion.
//
// To keep the main routine short and clear, much of the ugly details are in
// helper routines, many with a single call site.
//
// The parser handles three distinct cases:
//      - html comments (that may comment out other html elements)
//      - open and close tags (with, by definition, no nested elements)
//      - open tags and close tags, possibly separated by nested elements
//
//----------------------------------------------------------------------------//

void    Html::parseCribSheet (ifstream &input, Element &element)
{
    parseElement(input, element);
}

//----  parse an html element

void    Html::parseElement (ifstream &input, Element &element, const bool htmlBlockElement)
{
    string content;

    string  endTag (element.tag);       // looking for this closing tag
    endTag.insert(1, "/");

    while (input.good())
    {
        readToNextTag (input, content);

        if (input.eof()) break;

        string  tag;

        readTag (input, tag, content, endTag);

        if (tag == endTag) break;

        if (tag.compare(0, Comment::beg.length(), Comment::beg) == 0)
        {
            string&     comment = readComment (input, tag, content);

            Element&  subelement = * newElement (input, element, Comment::beg, content);

            addSubelement(input, element, subelement, content, htmlBlockElement);

            tidyContent(comment, Comment::beg, true, true);

            if (!comment.empty())
                subelement.contents.push_back(ElementPart (comment));

            continue;
        }

        if (checkImageElement (tag))
        {
            content += tag;

            continue;
        }

        stripAttributes(tag);

        if (unexpectedCloseTag(tag, content)) continue;

        if (openAndCloseTag(tag))
        {
            Element&  subelement = * newElement (input, element, tag, content);

            addSubelement(input, element, subelement, content, htmlBlockElement);

            continue;
        }

        if (unexpectedNestedTag(tag, element.tag, content)) continue;

        // open tag followed by a separate close tag - process any nested elements
        {
            Element&  subelement = * newElement (input, element, tag, content);

            parseElement(input, subelement, htmlBlockElement && element.tag != Markup::para);

            addSubelement(input, element, subelement, content, htmlBlockElement);

            checkForPairedTerms(element);
        }
    }

    // element ends with some text

    if (!content.empty())
    {
        tidyContent(content, element.tag, element.contents.empty(), true);

        if (!content.empty())
        {
            element.endOfSentence = endOfSentence(content);

            element.contents.push_back(ElementPart (content));
        }
    }
}

//----------------------------------------------------------------------------//
//
// The routines that actually read from the input.
//
// All but the last have a single call site in the main parse routine.
// They are implemented to cope gracefully with an unexpected end of file.
//
// The last routine is a bit of hack.
//----------------------------------------------------------------------------//

//----  read text upto the next html tag - append to any old content

string& Html::readToNextTag (ifstream& input, string &content)
{
    string  buffer;

    getline(input, buffer, '<');

    if (!input.eof())
        input.putback('<');

    content += buffer;

    return (content);
}

//----  read an html tag

string& Html::readTag (ifstream& input, string &tag, string &content, const string &endTag)
{
    getline(input, tag, '>');

    if (input.eof())
        content += tag, tag = endTag;   // unexpected EOF
    else
        tag += '>';

    return (tag);
}

//----  read an html comment element and discard its tags

string& Html::readComment (ifstream& input, string &comment, const string& content)
{
    comment.erase(0,Comment::beg.length());

    while (comment.find(Comment::end) == string::npos)
    {
        string  buffer;

        getline(input, buffer, '>');
        if (input.eof())
            buffer += "--";
        buffer += '>';

        comment += buffer;
    }

    comment.erase(comment.length()-Comment::end.length(),string::npos);

    if (content.length() && isspace(content[content.length() - 1]))
        while (!input.eof() && isspace(input.peek()))
            input.get();

    return (comment);
}

//----  discard new lines from the input when condition is true

void    Html::swallowNewLines (ifstream& input, bool condition)
{
    if (condition)
        while (!input.eof() && input.peek() == '\n')
            input.get();
}

//----------------------------------------------------------------------------//
//
// The two second level routines that do some real work.
//
// newElement() creates a new element and addSubelement() adds the new element
// to its parent's contents.
//
// There are two routines because there are cases where, in between, parsing
// of nested elements is appropriate and other cases where it is not required.
//
// The side affects on the element contents and the calls to swallowNewLines()
// logically do not belong here but moving them out would obscure the code in
// parseElement() and raise 'to do or not to do' questions.
//
//----------------------------------------------------------------------------//

//----  parse sub-element that follows an open tag

Html::Element*    Html::newElement (ifstream& input, const Element& parent, const string &tag, string& content)
{
    swallowNewLines (input, tag == Markup::asis || tag == Markup::newl || tag == Comment::beg);

    tidyContent(content, parent.tag, parent.contents.empty(), false);

    Element&  element = * (Element *) new Element (tag);

    element.strictOrder     = (parent.strictOrder && tag != Markup::ulst);
    element.startOfSentence = startOfSentence(parent, content);
    element.extraNewLine    = extraNewLine(parent, element);

    return (&element);
}

//----  add the new sub-element to its parent's contents

void    Html::addSubelement (ifstream& input, Element& parent, Element& element, string& content, const bool htmlBlockElement)
{
    const bool  lineBefore = lineBeforeSubElement(parent, element, htmlBlockElement);
    const bool  lineAfter  = lineAfterSubElement(element);

    parent.endOfSentence   = element.endOfSentence;

    parent.contents.push_back(ElementPart (content, &element, lineBefore, lineAfter));

    content.clear();

    swallowNewLines (input, lineAfter);
}

//----------------------------------------------------------------------------//
//
// Helper routines with a single call site in parseElement().
//
// The first two routines check for two cases of a malformed cribsheet that
// commonly occur when the html in a cribsheet is authored in a text, rather
// than an html, editor.
//
// checkImageElement() checks for an <img ...> element and strips out the
// text from the alt="text" attribute.  The text is retained and the element
// discarded.
//
// stripAttributes() discards html attributes the program has no use for.
//
// openAndCloseTag() recognises html tags that open and close an element
// (such as <br> and <p />).  The parser needs to handle them differently.
//
//----------------------------------------------------------------------------//

//----  check for a close tag when an open tag is expected

bool    Html::unexpectedCloseTag (const string &tag, string& content)
{
    if (tag[1] == '/')
    {
        content += tag;     // treat tag as ordinary text for debugging purposes
        return (true);
    }

    return (false);
}

//----  check for open tag nested within an element with the same open tag

bool    Html::unexpectedNestedTag (const string &tag, const string &previous, string& content)
{
    if (tag == previous)
    {
        content += tag;     // treat tag as ordinary text for debugging purposes
        return (true);
    }

    return (false);
}

//----  check for <img alt="text">

bool    Html::checkImageElement (string &tag)
{
    if (tag.substr(0,5) != "<img ")
        return (false);

    const int   bpos = tag.find("alt=\"");

    if (bpos == string::npos)
        return (false);

    string  text = tag.substr(bpos + sizeof ("alt=\"") - 1, string::npos);

    const int   epos = text.find("\"");

    if (epos == string::npos)
        return (false);

    tag = text.substr(0,epos);

    return (true);
}

//----  remove html attributes from a tag

void    Html::stripAttributes (string &tag)
{
    const int   bpos = tag.find(" ");

    if (bpos != string::npos)
    {
        const int   epos = tag.find(" />");

        if (epos != string::npos)
            tag.erase(bpos, epos - bpos);
        else
            tag.erase(bpos, tag.length() - bpos - 1);
    }
}

//----  check for tags that open and close an element in one

bool    Html::openAndCloseTag (string &tag)
{
    const size_t    pos = tag.find(" />");

    if (pos != string::npos && pos == tag.length() - 3)
    {
        tag.erase(pos, 2);
        return (true);
    }

    return (tag == Markup::newl || tag == Markup::rule);
}

//----------------------------------------------------------------------------//
//
// The two third level routines that do some real work.
//
// tidyContent() is called in several places to tidy white space in an around
// the text content of elements (and comments).
//
// checkForPairedTerms() looks to see if two terms are paired by a conjunction.
// If so, the pair will be treated as mini unordered list.
//
//----------------------------------------------------------------------------//

//----  tidy the white space in the text content of html elements

void    Html::tidyContent (string& content, const string &tag, const bool startOfElement, const bool endOfElement)
{
    if (content.length() == 0)
        return;

    size_t pos;

    if (endOfElement && tag == Markup::item)
        content += " ";

    if (tag != Markup::asis)
    {
        // convert new lines to spaces

        for (pos = content.find("\n"); pos != string::npos; pos = content.find("\n"))
            content.replace(pos, 1, " ");

        // convert tabs to spaces

        for (pos = content.find("\t"); pos != string::npos; pos = content.find("\t"))
            content.replace(pos, 1, " ");

        // eliminate duplicate spaces

        for (pos = content.find("  "); pos != string::npos; pos = content.find("  "))
            content.replace(pos, 2, " ");

        // strip trailing space ?

        if (endOfElement)
        {
            pos = content.length() - 1;
            if (content[pos] == ' ')
                content.erase(pos, 1);
        }

        // strip leading space ?

        if (startOfElement || tag == Markup::olst || tag == Markup::none)
        {
            pos = 0;
            if (content[pos] == ' ')
                content.erase(pos, 1);
        }
    }
    else
    {
        pos = content.length() - 1;
        if (content[pos] == '\n')
            content.erase(pos, 1);
    }

    // convert escaped html characters

    Escapes::replace (content);
}

//----  check for blankable terms paired by a conjunction

void Html::checkForPairedTerms (Element &element)
{
    const size_t    count = element.contents.size();

    if (count < 2) return;

    const string&   text = element.contents[count - 1].text;

    static  const char*     joinText[] = {"/", " and ", " or "};

    for (int ii = 0; ii < 3; ++ii)
        if (text == joinText[ii])
        {
            ElementPart&    lhs = element.contents[count - 2];
            ElementPart&    rhs = element.contents[count - 1];

            if (lhs.subElement == 0 || rhs.subElement == 0) return;

            lhs.subElement->strictOrder = rhs.subElement->strictOrder = false;

            break;
        }
}

//----------------------------------------------------------------------------//
//
// Helper routines with a single call site.
//
// These routines are used to set 'state' flags in new Element or ElementPart
// objects.  All return bool.
//
// They become more complex as features are added.  Placing their logic in
// separate routines with return statements yields a cleaner, clearer
// expression than if the logic were in-line at the call site.
//
//----------------------------------------------------------------------------//

//----  Does content end a sentence ?

bool    Html::endOfSentence (const string& content)
{
    size_t len = content.length();

    if (len == 0 || content[len - 1] != '.') return (false);
    if (len == 1 || content[len - 2] != '.') return (true);
    if (len == 2 || content[len - 3] != '.') return (true);

    // not elipsis ...

    return (false);
}

//----  Will the next element start a sentence ?

bool    Html::startOfSentence (const Element& parent, const string& content)
{
    const int   len = content.length();

    switch (len)
    {
        case (0):
        {
            if (parent.tag == Markup::para)
                return (true);

            if (parent.tag == Markup::ulst || parent.tag == Markup::olst)
                return (false);

            break;
        }
        case (1):
        {
            if (content[0] != '/') return (false);

            const size_t    count = parent.contents.size();

            if (count == 0) return (false);

            const ElementPart&    part = parent.contents[count - 1];

            return (part.subElement->startOfSentence);
        }
        default:
            return (content[len - 2] == '.');
    }

    return (parent.startOfSentence);
}

//----  Will an element merit an extra new line when printed ?

bool    Html::extraNewLine (const Element& parent, const Element &element)
{
    if (parent.tag == Markup::olst && element.tag == Markup::item)
        return (true);

    return (element.tag == Markup::newl);
}

//----  Will a nested element merit an blank line before it when printed ?

bool   Html::lineBeforeSubElement (const Element& parent, Element& element, const bool htmlBlockElement)
{
    if (element.tag == Markup::para && element.contents.empty())
        return (element.extraNewLine = true, false);    // not for an empty paragraph

    if (element.tag == Markup::asis)
        return (true);      // yes for a code block
    if (element.tag == Markup::olst)
        return (true);      // yes for a ordered list

    if (element.tag == Comment::beg)
        return (false);     // not for a comment
    if (element.tag == Markup::rule)
        return (false);     // not for a header rule

    return (htmlBlockElement && parent.tag != Markup::para);
}

//----  Will a nested element merit an blank line after it when printed ?

bool   Html::lineAfterSubElement (const Element& element)
{
    return (element.tag == Markup::asis || element.tag == Markup::olst);
}

//----------------------------------------------------------------------------//
//
// The html print operation is implemented in a single recursive routine:
//    - printElement() prints an elements and, by recursion, its subelements.
//
// The format is appropriate for the cribtutor program.  It inserts only blank
// lines.  It does not attempt to honour inline formatting such as italics.
//
// The routine prints blanked terms using element.contentMask.  This mask is
// set and cleared elsewhere.
//
// The verbose option is for debugging the parser.
//
//----------------------------------------------------------------------------//

void  Html::printElement (ostream &stream, const Element& element, string indent)
{
    if (verbose)
    {
        stream << indent << element.tag << endl;

        indent += "  ";
    }

    bool    lineBetween = false;
    bool    lineAfter = false;

    deque< ElementPart >::const_iterator  it;

    for (it = element.contents.begin(); it != element.contents.end(); ++it)
    {
        if (!it->text.empty())
        {
            if (lineAfter || (lineBetween && element.tag == Markup::none))
                stream << "\n\n";

            if (!element.contentMask.empty())
                stream << indent << element.contentMask;
            else
                stream << indent << it->text;

            lineAfter = false;
            lineBetween = true;
        }

        if (it->subElement)
        {
            const Element&  subElement = *it->subElement;

            if (subElement.tag == Comment::beg && !verbose)
                continue;

            if (lineAfter || lineBetween && it->lineBeforeSubElement)
                stream << "\n\n";

            if (element.tag == Markup::olst && subElement.tag == Markup::item)
                if (it->text.empty())
                    stream << "  ";

            printElement (stream, subElement, indent);

            if (subElement.extraNewLine)
            {
                deque< ElementPart >::const_iterator  jit;

                for (jit = it + 1; jit != element.contents.end(); ++jit)
                    if (!jit->subElement || jit->subElement->tag != Comment::beg) break;

                if (jit != element.contents.end())
                {
                    stream << "\n";

                    if (element.tag == Markup::olst && subElement.tag == Markup::item)
                        if (subElement.endOfSentence)
                            stream << "\n";
                }
            }

            lineAfter = it->lineAfterSubElement;
            lineBetween = true;
        }
    }

    if (verbose)
    {
        string  endTag;

        if (element.tag == Comment::beg)
            endTag = Comment::end;
        else
            endTag.assign(element.tag).insert(1, "/");

        stream << endl << indent.substr(2) << endTag << endl;
    }
}

//----------------------------------------------------------------------------//
//
// One minor task the parser performs while reading a cribsheet is replace
// html escape sequences with the special characters they represent.
//
// The implementation gets the job done.  Other, possibly more efficient, ways
// of doing this exist.
//
// Note the first time initialisation of the local (static) map.
//
//----------------------------------------------------------------------------//

string& Html::Escapes::replace (string& statement)
{
    static  map< string, string, greater<string> >   escapes;

    if (escapes.empty())
    {
        escapes.insert(pair<string, string> ("&lt;",   "<"));
        escapes.insert(pair<string, string> ("&gt;",   ">"));
        escapes.insert(pair<string, string> ("&amp;",  "&"));
        escapes.insert(pair<string, string> ("&apos;", "'"));
        escapes.insert(pair<string, string> ("&quot;", "\""));
    }

    if (statement.find("&") != string::npos &&
        statement.find(";") != string::npos)
    {
        map< string, string, greater<string> >::const_iterator   it;

        for (it = escapes.begin(); it != escapes.end(); ++it)
        {
            size_t  pos = string::npos;

            while ((pos = statement.find(it->first)) != string::npos)
                statement.replace(pos, it->first.size(), it->second);
        }
    }

    return (statement);
}

// EOF
