# ifndef    _MASSAGE_H
# define    _MASSAGE_H

//----------------------------------------------------------------------------//
//
// Interface file for the Massage module of the cribtutor program.
//
// The cribtutor program uses html markup in cribsheets to generate quizzes.
// While it should parse any html document, it only recognises and handles
// the html is needs to.  The rest is simply ignored.
//
// This carries the assumption that it can recognise and handle the html it
// needs to.  As long as cribsheets were hand crafted by the author of the
// program, this assumption went unchallenged.
//
// However, as soon as html editors or translators are introduced, the
// assumption may be challenged.  What if a list is not within a paragraph ?
// The original program would ignore such a list.
//
// A number of potential approaches to the problem were considered.  Building
// a separate tool was one, altering the cribtutor program was another.
// Building a separate tool was rejected not because it appeared to be, in
// this case, more effort.  It was rejected because it was difficult to see
// how such an approach would help reduce the effort needed to solve problems
// of a similar nature in the future.
//
// The solution chosen is an intermediary pass in the html parser between
// reading and parsing the html cribsheet and annotating the parse tree.
// The intermediary pass may alter the parse in whatever way is necessary.
//
// The Message module encapsulates this intermediary pass.  Code to massage
// the parse tree is to be added here rather than adapting code elsewhere to
// handle 'exceptions' it was not designed to handle.
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

using namespace std;

//----------------------------------------------------------------------------//
//
// The massage module extends the Html namespace.  It provides a single entry
// point whereby extensions may be added with minimal disruption to the rest
// of the program.
//
// See Massage.cpp for further details.
//
//----------------------------------------------------------------------------//

namespace   Html
{
    void    massageElement (Element& element);
};

# endif  /* _MASSAGE_H */
