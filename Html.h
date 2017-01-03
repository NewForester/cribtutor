# ifndef    _HTML_H
# define    _HTML_H

//----------------------------------------------------------------------------//
//
// Interface file for the Html namespace of the cribtutor program.
//
// The cribtutor program uses html markup in cribsheets to generate quizzes.
// The program needs some means of recognising and processing this markup.
//
// The Html namespace provides these.  It consists of a simple parser, data
// structures to described the parser output and a few symbolic constants.
//
// CAVEAT:
//
// The parser is not a full blown parser but one necessary and sufficient for
// the purposes of the cribtutor program.  It may be enhanced or even
// replaced in the future.
//
// It should accept full html documents but this is not guaranteed.  It does
// recognise tags and content but attributes (with a single one exception)
// are discarded.
//
// A limited number of tags are handled by the program.  The rest are ignored.
//
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
//
// https://github.com/NewForester/cribtutor
// Copyright (C) 2016, NewForester
// Released under the terms of the GNU GPL v2
//
//----------------------------------------------------------------------------//

#include <deque>
#include <string>

using namespace std;

//----------------------------------------------------------------------------//
//
// The Html::Markup and Html::Comment namespaces defines symbolic names for
// the limited number of html tags recognised by the cribtutor program.
//
//----------------------------------------------------------------------------//

namespace       Html
{
    namespace Markup
    {
        static string  none ("<>");
        static string  hdr1 ("<h1>");
        static string  hdr2 ("<h2>");
        static string  hdr3 ("<h3>");
        static string  para ("<p>");
        static string  term ("<em>");
        static string  asis ("<pre>");
        static string  code ("<code>");
        static string  ulst ("<ul>");
        static string  olst ("<ol>");
        static string  item ("<li>");
        static string  newl ("<br>");
        static string  rule ("<hr>");
        static string  link ("<a>");
    };

    namespace Comment
    {
        static string  beg ("<!--");
        static string  end ("-->");
    };
};

//----------------------------------------------------------------------------//
//
// A cribsheet is an html document (fragment).  It is considered to consist
// of nested html elements and text content.
//
// Two structures are used to represent this:  Element and ElementPart.  The
// contents of an element are represented by a sequence of element parts that
// each comprise text (optional) and a subelement (also optional).
//
// See Html.cpp for details of their use.
//
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
//
// The Element structure represents an html element with a tag and content.
// The content is optional and is represented by a sequence of ElementParts.
//
// An element has a reference count used to prevent premature destruction
// during STL copy operations.
//
// An element carries state set during the annotation of the parse tree.  All
// but the contentMask do not change once set.  Of these, all but strictOrder
// are only used by printElement().
//
// The contentMask is used by printElement() to blank out terms.  It is set
// and reset by routines in the Dialogue namespace.
//
// The strictOrder flag enables the Dialogue namespace to distinguish terms
// that must be entered in order relative to adjacent terms from terms in
// unordered (mini) lists that may be entered in any order relative to other
// terms in the list.
//
// The other data members are flags that control when printElement() prints
// new lines.  Essentially corner cases are turned into state.
//
//----------------------------------------------------------------------------//

namespace       Html
{
    struct      ElementPart;

    typedef     deque< Html::ElementPart >  ElementContents;

    // a representation of an html element

    struct      Element
    {
        const string    tag;
        ElementContents     contents;

        int         referenceCount;
        bool        strictOrder;
        bool        endOfSentence;
        bool        startOfSentence;
        bool        extraNewLine;
        string      contentMask;

        Element (string tag = Html::Markup::none) :
            tag (tag),
            referenceCount (0),
            strictOrder (true),
            endOfSentence (false),
            startOfSentence (false),
            extraNewLine (false)
            {}

        Element&    merge (Html::Element& rhs)
        {
            while (rhs.contents.size())
            {
                this->contents.push_back(rhs.contents.front());
                rhs.contents.pop_front();
            }

            const_cast <string&> (rhs.tag) = Html::Markup::none;

            return (*this);
        }
    };
};

//----------------------------------------------------------------------------//
//
// The ElementPart structure comprises a text string and an html subelement.
// Either may be absent but not both.
//
// The subelement is modelled by a pointer to an Element structure thus
// modelling the nested html element structure by indirect recursion.
//
// ElementPart is provided with the four for use with STL containers:
// default constructor, destructor, copy constructor and assignment operator.
// The reference count in the subelement Element structure is used to ensure
// destruction of subelement only when appropriate (not during STL copies).
//
// The other data members are flags that control when printElement() prints
// new lines.  Essentially corner cases are turned into state.
//
//----------------------------------------------------------------------------//

namespace       Html
{
    // a representation of part of an html element comprising a tag and a subelement

    struct      ElementPart
    {
        string    text;

        bool      lineBeforeSubElement;
        bool      lineAfterSubElement;

        Element*  subElement;

        ElementPart (const string& text = "", Element* sub = 0) :
        text (text),
        lineBeforeSubElement (false),
        lineAfterSubElement (false),
        subElement (sub)
        {
            if (subElement)
                subElement->referenceCount++;
        }

        ElementPart (Element* sub) :
        lineBeforeSubElement (false),
        lineAfterSubElement (false),
        subElement (sub)
        {
            if (subElement)
                subElement->referenceCount++;
        }

       ~ElementPart ()
        {
            if (subElement && --subElement->referenceCount == 0)
                delete subElement;
        }

        ElementPart (const ElementPart& rhs)
        {
            *this = rhs;
        }

        ElementPart &   operator= (const ElementPart& rhs)
        {
            text = rhs.text;
            subElement = rhs.subElement;

            if (subElement)
                subElement->referenceCount++;

            lineBeforeSubElement = rhs.lineBeforeSubElement;
            lineAfterSubElement = rhs.lineAfterSubElement;

            return (*this);
        }
    };
};

//----------------------------------------------------------------------------//
//
// There are but two interface routines, operator<< for the Element class and
// Html::parseCribSheet().  The latter is passed:
//    - input - file handle open at the beginning of the cribsheet file
//    - element - is the empty top level Element for the new parse tree
//
// The operator<< for the Element class may be used to print out any part of
// parsed html cribsheet for debug purposes but its normal run time use is to
// print statements (i.e. html paragraphs) with one or more terms blanked out.
//
// parseElement(), the html parser, is exposed for use by the Massage module,
// which is part of the Html namespace.
//
// For implementation details see Html.cpp.
//
//----------------------------------------------------------------------------//

namespace       Html
{
    extern  void    parseCribSheet (ifstream &input, Html::Element &element);

    extern  void    parseElement (istream& input, Element &element);

    extern  bool    verbose;    // debug only

    extern  ostream&    operator<< (ostream &stream, const Element& element);
};

# endif  /* _HTML_H */
