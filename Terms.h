# ifndef    _TERMS_H
# define    _TERMS_H

//----------------------------------------------------------------------------//
//
// Interface file for the Terms namespace of the cribtutor program.
//
// The cribtutor program generates quizzes.  A quiz requires prompting the
// user with questions and analysing their response.
//
// The Terms namespace handles the generation of questions and the analysis of
// responses in behalf of the Dialogue namespace.
//
// The Terms namespace has no knowledge of the user interface:  that is the
// responsibility of the Dialogue namespace.
//
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
//
// https://github.com/NewForester/cribtutor
// Copyright (C) 2016, NewForester
// Released under the terms of the GNU GPL v2
//
//----------------------------------------------------------------------------//

#include "Quiz.h"

#include <string>

using namespace std;

//----------------------------------------------------------------------------//
//
// Two typedefs appear in the interface that provide aliases for two typedefs
// defined in the Quiz namespace.
//
// A SourceTermList is the terms in a paragraph that may be blanked to yield
// a quiz question.  It is a sequence of iterators that refer to structures
// within the parse tree.
//
//----------------------------------------------------------------------------//

namespace       Terms
{
    typedef Quiz::ContentsIterator  SourceTermIterator;

    typedef Quiz::ContentsList      SourceTermList;
};

//----------------------------------------------------------------------------//
//
// Three typedefs appear in the interface that together model the blanked
// terms in a quiz question that the user is asked to fill in.
//
// In the first instance, a deque of masked terms would suffice.  However,
// when two or more terms from an unordered list are blanked, the user would
// have to fill in the blanks in the right order when there really isn't one.
//
// A deque of sets would solve this problem:  terms from unordered lists are
// stored in the same set and other terms each in a set of their own.
//
// However, some terms are compound:  they comprise more than one word.  There
// is no reason why compound terms cannot appear in unordered lists but the
// words in a compound term must be filled in the correct order.
//
// A snag arises when a unordered list comprises two or more compound terms
// that start with the same word.  A simple set (or map) is not enough.
//
// To solve these problems, the set is replaced with a multimap.  The keys are
// the first words of compound terms or the term itself for simple terms.  The
// values are deques that are empty for simple terms but for compound terms
// contain the remaining words in order.
//
// So a deque of maps of deques.  This is only a simple program.
//
//----------------------------------------------------------------------------//

namespace       Terms
{
    typedef pair< string, deque< string > >             CompoundTerm;

    typedef multimap< const string, deque< string> >    MaskedTermSet;

    typedef deque< MaskedTermSet >                      MaskedTermList;
};

//----------------------------------------------------------------------------//
//
// There are three interface routines:
//    - mask() generates the masked term list and sets content masks
//    - check() checks the user's response against the mask term list
//    - resets() the content masks
//
// The content mask is a data member of the Html::Element class.  When set,
// Html::printElement() prints the mask instead of the element's text content.
//
// Terms::mask() uses this to 'blank out' terms but this alters the parse tree.
// Terms::reset() clears the content masks, thus restoring the parse tree.
//
// Terms::mask() constructs a masked term list which is subsequently passed to
// Terms::check() to compare against the user response.
//
// For implementation details see Terms.cpp.
//
//----------------------------------------------------------------------------//

namespace   Terms
{
    extern  void    reset (SourceTermList& sourceTerms);

    extern  void    mask (MaskedTermList& maskedTerms, SourceTermList& sourceTerms, int termCount, const int choices);

    extern  bool    check (const MaskedTermList& maskedTerms, const string& response);
};

# endif  /* _TERMS_H */
