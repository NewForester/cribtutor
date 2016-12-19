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

using namespace std;

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
//   - merging the paragraphs either side with new paragraph
//   - unless this end/start with <p />
// This massaging yields the expected cribtutor form by default while allowing
// the merging to be turned off explicitly when required.
//
// It is acknowledged that 'top level' is inadequate.
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

        // is there a paragraph after >

        if (it != element.contents.end())
        {
            Element*    after = (it + 1)->subElement;

            if (after && after->tag == Markup::para)
            {
                string& text = after->contents.begin()->text;

                Escapes::replace(text);

                if (text.substr(0,5) == "<p />")
                {
                    text.erase(0,5);
                }
                else
                {
                    if (tag == Markup::ulst)
                        text.insert(0," ");

                    newParagraph->merge(*after);
                }
            }
        }

        // is there a paragraph before ?

        if (it != element.contents.begin())
        {
            Element*    before = (it - 1)->subElement;

            if (before && before->tag == Markup::para)
            {
                string& text = (before->contents.end() - 1)->text;

                Escapes::replace(text);

                if (text.substr(text.length() - 5) == "<p />")
                {
                    text.erase(text.length() - 5);
                }
                else
                {
                    newParagraph->contents.begin()->text += " ";

                    before->merge(*newParagraph);
                }
            }
        }
    }
}

// EOF
