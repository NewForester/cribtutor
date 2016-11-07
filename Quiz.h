# ifndef    _QUIZ_H
# define    _QUIZ_H

//----------------------------------------------------------------------------//
//
// Interface file for the Quiz namespace of the cribtutor program.
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

#include "Html.h"

#include <deque>
#include <map>

using namespace std;

class   SectionNumber;

//----------------------------------------------------------------------------//
//
// There is but one interface routine: Quiz::run().  It is passed:
//    - prefix - used to number chapters and sections
//    - quiz - the top level element of the parsed html cribsheet contents
//    - choices - (from the command line) the number of terms to blank
//
// For implementation details see Quiz.cpp.
//
// Two typedefs appear in the interface because they are part of the interface
// to the Dialogue namespace.
//
// A ContentsList may be the sequence of paragraphs in a section, sections in
// a chapter, chapters in a cribsheet but also the terms in a paragraph that
// may be blanked out to form a quiz question.  It is a sequence of iterators
// that refer to structures with the parsed html cribsheet.
//
//----------------------------------------------------------------------------//

namespace       Quiz
{
    typedef Html::ElementContents::iterator   ContentsIterator;

    typedef deque< ContentsIterator >   ContentsList;

    // only one external routine - called from main

    extern  void    run (SectionNumber& prefix, Html::Element& quiz, int choices);
};

# endif  /* _QUIZ_H */
