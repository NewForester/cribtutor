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
#include <sstream>

using namespace std;

//----------------------------------------------------------------------------//
//
// The massage module extends the Html namespace with additional nested
// namespaces.  The nested namespaces serve to distinguish different massage
// requirements for different circumstances.  At present there is only one.
//
// The nested namespace Markdown contains routines to massage the html
// generated by the pandoc translator from Markdown wiki mark-up, specifically,
// that used on GitHub.
//
//----------------------------------------------------------------------------//

namespace   Html
{
    typedef ElementContents::iterator   ContentsIterator;

    namespace   Markdown
    {
        void    massageAsisLists (Element& element);
        void    massageCodeElements (Element& parent);

        void    fixupListShuffles (Element& listElement);
        void    fixupCodeSpans (Element& parent);

        // helper routines

        bool    termsInCodeSpan (string &text, const char delim);
    };
};

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
    Markdown::massageAsisLists (element);
    Markdown::massageCodeElements (element);
}

//----------------------------------------------------------------------------//
//
// The cribtutor program expects lists and asis blocks to be enclosed within
// paragraphs so that they appear as part of a quiz question rather than as
// a separate questions.
//
// Markdown allows paragraphs within list items and it does not seem to be
// possible in Markdown to mark-up so that translators, such as pandoc, will
// generate lists and asis blocks within paragraphs.
//
// The massageLists() routine compensates by:
//
//   - enclosing list and asis elements in a (new) paragraph
//   - merging the paragraphs either side with the new paragraph
//   - unless they end/start with a comment
//
// This massaging yields the expected cribtutor form by default while allowing
// the merging to be turned off explicitly when required.
//
// This routine is processes ordered lists and is an appropriate place to call
// fixupListShuffles.
//
//----------------------------------------------------------------------------//

void    Html::Markdown::massageAsisLists (Element& element)
{
    ElementContents::iterator  it;

    for (it = element.contents.begin(); it != element.contents.end(); ++it)
    {
        Element*    subElement = it->subElement;

        if (subElement == 0)
            continue;

        const string&   tag = subElement->tag;

        if (subElement->tag != Markup::asis)
            if (subElement->tag != Markup::ulst && subElement->tag != Markup::olst)
            {
                if (subElement->tag != Markup::para)
                    massageAsisLists (*subElement);
                continue;
            }

        if (subElement->tag == Markup::olst)
            fixupListShuffles (*subElement);

        // push the list down into a paragraph of its own

        Element*  newParagraph = (Element *) new Element (Markup::para);

        subElement->startOfSentence = true;

        newParagraph->contents.push_back(ElementPart (subElement));

        it->subElement = newParagraph;

        // is there a paragraph after that does not start with a comment ?

        if ((it + 1) != element.contents.end())
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
// For code blocks, the Markdown syntax is one indent level, which it drops.
// Cribtutor restores this dropped indent in Html::tidyText().
//
// For code blocks, Markdown generates <pre><code> ... </code></pre> but
// Html::tidyText() only knows about one tag, not two.
//
// The minimal disruption principle say not to change the calling sequence to
// Html::tidyText().  So, instead, this routine alters the <code> tag by
// concatenation to <pre><code> and Html::tidyText() is able to act on this
// to restore the indent in a safe and isolated manner.
//
// This routine is recursive, so by calling FixupCodeSpans() this fixup is
// propagated throughout the parse tree.
//
//----------------------------------------------------------------------------//

void    Html::Markdown::massageCodeElements (Element& parent)
{
    for (ContentsIterator it = parent.contents.begin(); it != parent.contents.end(); ++it)
    {
        if (it->subElement == 0)
            continue;

        Html::Element&    element = *it->subElement;

        if (element.tag != Markup::code || parent.tag != Markup::asis)
            massageCodeElements(element);
        else if (it == parent.contents.begin() && it->text.empty())
            const_cast <string&> (element.tag) = Markup::asis + Markup::code;
    }

    if (parent.tag == Markup::code && !parent.contents.empty())
        fixupCodeSpans(parent);
}

//----------------------------------------------------------------------------//
//
// Cribtutor supports the shuffling of (ordered) list elements but this is
// based on an specific html comment appearing in a specific place:  between
// the <ol> tag and the first <li> tag.
//
// There is no way to get this when generating html from Markdown mark-up so
// cribtutor accepts an alternative.  This alternative is massaged into the
// desired form by this routine.
//
// The Markdown syntax:
/*
       1. <!-- Shuffle List -->
          First list item.
       2. Second list item.
*/
// generates:
/*
       <ol>
       <li><!-- Shuffle List --> First list item.</li>
       <li>Second list item.</li>
       </ol>
*/
// which this routine massages into:
/*
       <ol><!-- Shuffle List -->
       <li> First list item.</li>
       <li>Second list item.</li>
       </ol>
 */
//
//----------------------------------------------------------------------------//

void    Html::Markdown::fixupListShuffles (Element& listElement)
{
    Element*    subElement;

    // is there a Shuffle List comment at the start of the first list item ?

    subElement = listElement.contents.front().subElement;

    if (!subElement || subElement->tag != Markup::item) return;

    Element&    itemElement = *subElement;

    if (itemElement.contents.empty()) return;

    subElement = itemElement.contents.front().subElement;

    if (!subElement || subElement->tag != Comment::beg) return;

    Element&    comment = *subElement;

    if (comment.contents.empty()) return;

    string& text = comment.contents.front().text;

    if (text.find("Shuffle List") == string::npos) return;

    // massage: move the comment from the first list item to the front of the list

    listElement.contents.push_front(itemElement.contents.front());

    itemElement.contents.pop_front();
}

//----------------------------------------------------------------------------//
//
// The cribtutor program has no use for <code> elements and ignores them.  It
// also renders apostrophe (tick) and grave (back tick) characters allowing
// for the quoting of words or phrases of significance without blanking them.
//
// Markdown converts back ticks to <code> elements.  This routine reverses
// this for inline (but not block) <code> elements.
//
// It also 'recognises' and 'converts' blankable terms within code spans as,
// some translators do not translate Markdown within code spans as they should.
//
// Markdown has support for nested back ticks.  This routine does not.
//
//----------------------------------------------------------------------------//

void    Html::Markdown::fixupCodeSpans (Element& parent)
{
    if (parent.contents.size() == 1 && parent.contents.begin()->subElement == 0)
    {
        string& text = parent.contents.begin()->text;

        if (termsInCodeSpan(text, '_') + termsInCodeSpan(text, '*'))
        {
            istringstream   codeSpan (text);

            parent.contents.pop_front();

            parseElement(codeSpan, parent);
        }
    }

    parent.contents.begin()->text.insert(0,"`");

    if (parent.contents.rbegin()->subElement)
        parent.contents.push_back(ElementPart());

    parent.contents.rbegin()->text.append("`");
}

//----------------------------------------------------------------------------//
//
// There seems to be no good reason why a term that merits Markdown back tick
// emphasis should not also be blankable.
//
// Markdown mark-up within back tick code spans should be honored.  GitHub
// appears to honour it but pandoc does not.  Cribtutor must compenstate.
//
// This routine converts code span text from Markdown syntax to html and
// return true if any conversion took place.  It is expected the caller will
// then parse the html.
//
//----------------------------------------------------------------------------//

bool    Html::Markdown::termsInCodeSpan (string &text, const char delim)
{
    bool    trueIfFound = false;

    size_t  bpos = text.find(delim);
    size_t  epos = string::npos;

    do
    {
        if (bpos == string::npos)
            break;

        epos = text.find(delim,bpos + 1);

        if (epos == string::npos)
            break;

        if (epos == bpos + 1 || text[epos + 1] == delim)
            continue;

        text.replace(epos,1,"</em>");
        text.replace(bpos,1,"<em>");

        trueIfFound = true;
    }
    while (bpos = text.find_first_of(delim,epos + 1));

    return (trueIfFound);
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
