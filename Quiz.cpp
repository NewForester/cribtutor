//----------------------------------------------------------------------------//
//
// Implementation file for the Quiz namespace of the cribtutor program.
//
// The cribtutor program generates quizzes from cribsheet files.  It does this
// by parsing the html markup in cribsheet files and then processing the data
// structure created by the parser.
//
// The Quiz namespace provides the processing of the data structure.  It uses
// the Dialogue namespace to handle interaction with the user.
//
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
//
// https://github.com/NewForester/cribtutor
// Copyright (C) 2016, NewForester
// Released under the terms of the GNU GPL v2
//
//----------------------------------------------------------------------------//

#include "Dialogue.h"
#include "Html.h"
#include "Quiz.h"
#include "SectionNumber.h"

#include <algorithm>
#include <deque>
#include <iostream>
#include <string>

using namespace std;


//----  internal routines and data

namespace       Quiz
{
    namespace   Process
    {
        // the three nested delegates

        static  bool    chapters (SectionNumber& prefix, const ContentsIterator& first, const ContentsIterator& last, const int choices);

        static  bool    sections (SectionNumber& prefix, const ContentsIterator& first, const ContentsIterator& last, const int choices, int &maxTermCount);

        static  bool    paragraphs (const ContentsIterator& first, const ContentsIterator& last, const int choices, int &maxTermCount);

        // two helper routines used by the delegates

        static  void    findHeaderTags (ContentsList& markers, const ContentsIterator& first, const ContentsIterator& last, const string& markerTag);

        static  void    findTermTags (ContentsList& terms, const ContentsIterator& first, const ContentsIterator& last, const string& termTag);

        // the shuffle routines used by paragraphs()

        static  void    shuffleParagraphs (const ContentsIterator& first, const ContentsIterator& last);

        static  void    shuffleOrderedLists (Html::Element& paragraph);
    };

    // the quiz header (if any) - a level above chapter headers

    static  string  tomeHeader;
};

//----------------------------------------------------------------------------//
//
// The quiz master.
//
// The quiz process is:
//    for each chapter ...
//      for each section ...
//        for each statement ...
//          generate a question and ask the user to fill in the blanks
//
// The quiz master delegates to Process::chapters(), which delegates to
// Process::sections(), which delegates to Process::paragraphs().
//
// There are two complications.
//
// The first is that the crib sheet might be just a crib sheet with headers
// and paragraphs at the top level or it might be a complete html document.
// In the second case, the program has to find the html elements of interest
// inside those it does not care about.
//
// The second is that the notion of chapters with sections requires only two
// levels of header:  <h1> and <h2>.  However, for large quizzes with many
// cribsheet files two levels is insufficient.
//
// The program supports an alternative where, typically, each cribsheet file
// represents a level above chapter (named here a tome).  Chapters and sections
// are introduced by <h2> and <h3> headers.
//
// The complication is printing the tome header only when it changes when that
// typically (but not necessarily) happens as processing moves on to the next
// crib sheet at a level above the quiz master.  Hence the use of static data
// at the namespace level.
//
//----------------------------------------------------------------------------//

//----  run quiz, chapter by chapter, section by section and paragraph by paragraph

void    Quiz::run (SectionNumber& prefix, Html::Element& quiz, int choices)
{
    ContentsIterator  it;

    // look in this quiz element for quiz questions

    for (it = quiz.contents.begin(); it != quiz.contents.end(); ++it)
    {
        if (it->subElement == 0)
            continue;

        const Html::Element&    element = *it->subElement;

        if (element.tag == Html::Markup::para)
        {
            // quiz questions found - process chapters/sections/paragraphs

            Process::chapters(prefix, quiz.contents.begin(), quiz.contents.end(), choices);

            return;
        }
        else if (element.tag == Html::Markup::hdr1)
        {
            // print new tome header if appropriate

            const string&   header = element.contents.front().text;

            if (header != tomeHeader)
            {
                tomeHeader = header;

                if (prefix.singleDigit())
                    ;
                else if (prefix.doubleDigit())
                    cout << prefix.quiz(header) << endl << endl;
                else if (Dialogue::skipYesNo("\n" + tomeHeader))
                    return;
            }
        }
    }

    // no quiz questions found - try all sub elements

    for (it = quiz.contents.begin(); it != quiz.contents.end(); ++it)
        if (it->subElement)
            run (prefix, *it->subElement, choices);
}

//----------------------------------------------------------------------------//
//
// The three nested delegates.
//
// These embody the notion of for each chapter, for each section, and for each
// paragraph.  They handle skip/repeat.
//
// Their complication is tracking good/bad responses on a chapter/section
// basis so the number of choices may be incremented (or not) when the user
// chooses to repeat a chapter/section (and not offering the user the choice
// when that serves no purpose).
//
//----------------------------------------------------------------------------//

//--- process chapters one by one with skip and repeat

bool    Quiz::Process::chapters (SectionNumber& prefix, const ContentsIterator& first, const ContentsIterator& last, const int choices)
{
    // make a list of chapters

    ContentsList     chapters;

    findHeaderTags (chapters, first, last, prefix.singleDigit() ? Html::Markup::hdr1 : Html::Markup::hdr2);

    // process paragraphs before first chapter

    int     dummy = 0;

    bool    allResponsesGood = Process::paragraphs (first, *chapters.begin(), choices, dummy);

    // process chapters

    for (ContentsList::iterator chapter = chapters.begin(); chapter != chapters.end() - 1; ++chapter)
    {
        const Html::Element&    chapterElement = *(*chapter)->subElement;

        string  chapterHeader = prefix.chapter(chapterElement.contents.front().text);

        if (prefix.singleDigit())
            tomeHeader = chapterElement.contents.front().text;

        if (Dialogue::skipYesNo(chapterHeader)) continue;

        int     termCount = 0;
        int     chapterChoices = choices;
        bool    chapterGood = true;

        do
        {
            chapterGood = Process::sections (prefix, *chapter + 1, *(chapter + 1), chapterChoices, termCount);

            if (chapterGood && ++chapterChoices > termCount)
                break;

            prefix.repeatChapter();
        }
        while (choices != 0 && Dialogue::repeatYesNo(chapterHeader));

        allResponsesGood &= chapterGood;
    }

    return (allResponsesGood);
}

//--- process sections one by one with skip and repeat

bool    Quiz::Process::sections (SectionNumber& prefix, const ContentsIterator& first, const ContentsIterator& last, const int choices, int &maxTermCount)
{
    // make a list of sections

    ContentsList     sections;

    findHeaderTags (sections, first, last, prefix.singleDigit() ? Html::Markup::hdr2 : Html::Markup::hdr3);

    // process paragraphs before first section

    bool    allResponsesGood = Process::paragraphs (first, *sections.begin(), choices, maxTermCount);

    // process sections

    for (ContentsList::iterator section = sections.begin(); section != sections.end() - 1; ++section)
    {
        const Html::Element&    sectionElement = *(*section)->subElement;

        string  sectionHeader = prefix.section(sectionElement.contents.front().text);

        if (Dialogue::skipYesNo(sectionHeader)) continue;

        int     termCount = 0;
        int     sectionChoices = choices;
        bool    sectionGood = true;

        do
        {
            sectionGood = Process::paragraphs (*section + 1, *(section + 1), sectionChoices, termCount);

            if (sectionGood && ++sectionChoices > termCount)
                break;
        }
        while (choices != 0 && Dialogue::repeatYesNo(sectionHeader));

        maxTermCount = max(maxTermCount,termCount);

        allResponsesGood &= sectionGood;
    }

    return (allResponsesGood);
}

//--- process paragraphs asking the user to fill in the blanks

bool    Quiz::Process::paragraphs (const ContentsIterator& first, const ContentsIterator& last, const int choices, int &maxTermCount)
{
    bool    allResponsesGood = true;

    if (choices > 0)
        shuffleParagraphs (first, last);

    // for each paragraph

    for (ContentsIterator it = first; it != last; ++it)
    {
        if (it->subElement == 0) continue;

        Html::Element&    paragraph = *it->subElement;

        if (paragraph.tag != Html::Markup::para) continue;

        if (choices > 0)
            shuffleOrderedLists (paragraph);

        // make a list of terms that may be blanked

        ContentsList     sourceTerms;

        findTermTags (sourceTerms, paragraph.contents.begin(), paragraph.contents.end(), Html::Markup::term);

        maxTermCount = max(maxTermCount,int(sourceTerms.size()));

        // ask user to fill in the blanks

        Dialogue::fillInTheBlanks (paragraph, sourceTerms, choices, allResponsesGood);
    }

    return (allResponsesGood);
}

//----------------------------------------------------------------------------//
//
// Two helper routines used by the three nested delegates.
//
// The delegates embody the notions of for each chapter, section or paragraph.
// These helper routines build the lists of chapters/sections and paragraphs.
//
//----------------------------------------------------------------------------//

//----  build a list of header tags from a (non-recursive) list (of elements)

void    Quiz::Process::findHeaderTags (ContentsList& markers, const ContentsIterator& first, const ContentsIterator& last, const string& tag)
{
    for (ContentsIterator it = first; it != last; ++it)
    {
        if (it->subElement == 0)
            continue;

        const Html::Element&    subElement = *it->subElement;

        if (subElement.tag == tag)
            markers.push_back(it);
    }

    markers.push_back(last);
}

//----  build a list of (maskable) term tags from a (recursive) list (of elements)

void    Quiz::Process::findTermTags (ContentsList& terms, const ContentsIterator& first, const ContentsIterator& last, const string& tag)
{
    for (ContentsIterator it = first; it != last; ++it)
    {
        if (it->subElement == 0)
            continue;

        Html::Element&    subElement = *it->subElement;

        if (subElement.tag == tag)
            terms.push_back(it);
        else
            findTermTags(terms, subElement.contents.begin(), subElement.contents.end(), tag);
    }
}

//----------------------------------------------------------------------------//
//
// The shuffle routines used by paragraphs().
//
// The first of these shuffles the order of paragraphs and thus the order in
// which questions are posed in order to make the quiz less predictable.
//
// The shuffling is turned on and off within a section by the presence of
// html comments that read "Shuffle On" and "Shuffle Off".
//
// The second routine shuffles the order of items within an ordered list, also
// in order make the quiz less predictable.
//
// This shuffling of list items is triggered by the presence of an html comment
// at the head of the list before the first item that reads "Shuffle List".
//
//----------------------------------------------------------------------------//

//----  shuffle paragraphs as directed by comments

void    Quiz::Process::shuffleParagraphs (const ContentsIterator& first, const ContentsIterator& last)
{
    ContentsIterator beginShuffle = last;

    for (ContentsIterator it = first; it != last; ++it)
    {
        if (it->subElement == 0) continue;

        const Html::Element&    element = *it->subElement;

        if (element.tag == Html::Comment::beg && !element.contents.empty())
        {
            const string& comment =  element.contents.front().text;

            if (comment == "Shuffle On")
            {
                beginShuffle = it;
            }
            else if (comment == "Shuffle Off")
            {
                if (beginShuffle != last)
                    random_shuffle(beginShuffle + 1, it - 1);

                beginShuffle = last;
            }
        }
    }

    if (beginShuffle != last)
        random_shuffle(beginShuffle + 1, last);
}

//----  shuffle items in an ordered list as directed by comments

void    Quiz::Process::shuffleOrderedLists (Html::Element& paragraph)
{
    ContentsIterator  it;

    for (it = paragraph.contents.begin(); it != paragraph.contents.end(); ++it)
    {
        // check for an ordered list

        if (it->subElement == 0) continue;

        Html::Element&  list = *it->subElement;

        if (list.tag != Html::Markup::olst) continue;

        // check the first subelement is a comment

        const ContentsIterator  it = list.contents.begin();

        if (it->subElement == 0) continue;

        const Html::Element&  element = *it->subElement;

        if (element.tag != Html::Comment::beg || element.contents.empty()) continue;

        // shuffle list contents if so directed

        if (element.contents.front().text != "Shuffle List") continue;

        random_shuffle (it + 1, list.contents.end());
    }
}

// EOF
