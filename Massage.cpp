//----------------------------------------------------------------------------//
//
// Implementation file for the Massage module of the cribtutor program.
//
// The intermediary pass of the cribtutor parser permits the parse tree to be
// massaged.  This may be necessary when the cribsheet's html structure does
// not conform to expectations.
//
// This may arise from the use of html editors or of wiki mark-up with a
// translator.
//
// The purpose of using a separate pass and a separate module is to provide
// a space where necessary adaptations can be implemented without threatening
// the integrity rest of the program.
//
// See also Massage.h.
//
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
//
// https://github.com/NewForester/cribtutor
// Copyright (C) 2016, NewForester
// Released under the terms of the GNU GPL v2
//
//----------------------------------------------------------------------------//

#include "Massage.h"
#include <iostream>

using namespace std;

//----------------------------------------------------------------------------//
//
// Two helper routines to test:
// - whether an element start with a given subelement
// - whether an element ends with a given subelement
//
//----------------------------------------------------------------------------//

namespace       Html
{
    bool    startsWithSubElement (const Element* element, const string& tag);
    bool    endsWithSubElement (const Element* element, const string& tag);
};

//----------------------------------------------------------------------------//
//
// Massage the parse tree as required - the second pass of the parse operation.
//
//----------------------------------------------------------------------------//

void    Html::massageElement (Element& element)
{
    Markdown::massageLists (element);
}

//----------------------------------------------------------------------------//
//
// The cribtutor program expects lists to be within paragraphs so that a list
// appears within a question (and not as a separate question).
//
// Markdown allows paragraphs within list items and it does not seems to be
// possible in Markdown to mark-up so that translators such as pandoc will
// generate lists within paragraphs.
//
// The massageLists() routine compensates by:
//   - enclosing a list element found at the top level in a (new) paragraph
//   - merging the paragraphs either side with the new paragraph
//   - unless they end/start with a comment
// This massaging yields the expected cribtutor form by default while allowing
// the merging to be turned off explicitly when required.
//
// Here 'top level' means at the same level as headers and paragraphs.
//
//----------------------------------------------------------------------------//

void    Html::Markdown::massageLists (Element& element)
{
    ElementContents::iterator  it;

    for (it = element.contents.begin(); it != element.contents.end(); ++it)
    {
        Element*    subElement = it->subElement;

        if (subElement == 0)
            continue;

        const string&   tag = subElement->tag;

        if (subElement->tag != Markup::ulst && subElement->tag != Markup::olst)
            continue;

        // push the list down into a paragraph of its own

        Element*  newParagraph = (Element *) new Element (Markup::para);

        subElement->startOfSentence = true;

        newParagraph->contents.push_back(ElementPart (subElement));

        it->subElement = newParagraph;

        // is there a paragraph after that does not start with a comment ?

        if (it != element.contents.end())
        {
            Element*    after = (it + 1)->subElement;

            if (!startsWithSubElement(after, Comment::beg))
            {
                if (tag == Markup::ulst)
                    after->contents.begin()->text.insert(0," ");

                newParagraph->merge(*after);

                int     index = it - element.contents.begin();

                element.contents.erase(it + 1);

                it = element.contents.begin() + index;
            }
        }

        // is there a paragraph before that does not end with a comment ?

        if (it != element.contents.begin())
        {
            Element*    before = (it - 1)->subElement;

            if (!endsWithSubElement(before, Comment::beg))
            {
                newParagraph->contents.begin()->text += " ";

                before->merge(*newParagraph);

                int     index = it - element.contents.begin() - 1;

                element.contents.erase(it);

                it = element.contents.begin() + index;
            }
        }
    }
}

//----------------------------------------------------------------------------//
//
// Two helper routines to test:
// - whether an element start with a given subelement
// - whether an element ends with a given subelement
//
//----------------------------------------------------------------------------//

// test whether an element start with a given subelement

bool    Html::startsWithSubElement (const Element* element, const string& tag)
{
    if (!element || element->tag != Markup::para)
        return (true);

    size_t    parts = element->contents.size();

    if (parts == 0)
        return (false);

    const ElementPart&    firstPart = element->contents[0];

    if (firstPart.subElement && firstPart.subElement->tag == tag)
        if (firstPart.text.empty())
            return (true);

    return (false);
}

// test whether an element ends with a given subelement

bool    Html::endsWithSubElement (const Element* element, const string& tag)
{
    if (!element || element->tag != Markup::para)
        return (true);

    size_t    parts = element->contents.size();

    if (parts == 0)
        return (false);

    const ElementPart&    lastPart = element->contents[parts - 1];

    if (lastPart.subElement && lastPart.subElement->tag == tag)
        return (true);

    return (false);
}

// EOF
