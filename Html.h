# ifndef    _HTML_H
# define    _HTML_H

//----------------------------------------------------------------------------//
//
// Interface file for the Html namespace class of the cribtutor program.
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
// recognise tags and content but attributes are discarded.
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
        static string  ulst ("<ul>");
        static string  olst ("<ol>");
        static string  item ("<li>");
        static string  newl ("<br>");
        static string  rule ("<hr>");
    };

    namespace Comment
    {
        static string  beg ("<!--");
        static string  end ("-->");
    };
};

//----------------------------------------------------------------------------//
//
// The parser uses two structures, Element and ElementPart, to represent an
// cribsheet.  A cribsheet consists of nested html tags and text content.
//
// The Element structure represent an html element with a tag and content.
// The content is optional and is represented by a sequence of ElementParts.
//
// An element part comprises optional text content and an html subelement.
// Either may be absent but not both.
//
// Since html elements may contain other html elements, the structure is
// recursive and thus involves pointers.  For use with STL containers,
// the ElementPart class is provided with the four: default constructor,
// destructor, copy constructor and assignment operator.  A reference
// count is used to avoid premature deletion of subelements.  This may be
// replaced with an auto_ptr in the near future.
//
// In addition to the reference count, the Element structure has two members
// that carry state of interest to the Dialogue namespace:
//    - contentMask - used to blank out terms.
//    - strictOrder - if true blanks must be filled in strictly in order
//
// The other data members are flags that control when the html print routine
// prints blank lines.  Essentially corner cases are turned into state.
//
// See Html.cpp for details of their use.
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
    };

    // a representation of part of an html element comprising a tag and a subelement

    struct      ElementPart
    {
        string    text;

        bool      lineBeforeSubElement;
        bool      lineAfterSubElement;

        Element*  subElement;

        ElementPart (const string& text, Element* sub = 0, bool lineBeforeSubElement = false, bool lineAfterSubElement = false) :
            text (text),
            lineBeforeSubElement (lineBeforeSubElement),
            lineAfterSubElement (lineAfterSubElement),
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
// For implementation details see Html.cpp.
//
//----------------------------------------------------------------------------//

namespace       Html
{
    extern  void    parseCribSheet (ifstream &input, Html::Element &element);

    extern  bool    verbose;    // debug only

    extern  ostream&    operator<< (ostream &stream, const Element& element);
};

# endif  /* _HTML_H */
