# ifndef    _DIALOGUE_H
# define    _DIALOGUE_H

//----------------------------------------------------------------------------//
//
// Interface file for the Dialogue namespace of the cribtutor program.
//
// The cribtutor program generates quizzes.  A quiz requires prompting the
// user with questions and analysing their response.
//
// The Dialogue namespace handles the prompt/response interface but delegates
// the generation of questions and the analysis of responses to the Terms
// namespace.
//
// The prompt/response interface it deliberately very simple.  The program's
// design allows for its replacement with a more sophisticated interface.
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
#include "Quiz.h"

#include <string>

using namespace std;

//----------------------------------------------------------------------------//
//
// Dialogue::yesNo() is used by the Quiz namespace to skip and repeat the
// chapters and sections of a quiz.
//
// For implementation details see Dialogue.cpp.
//
//----------------------------------------------------------------------------//

namespace   Dialogue
{
    extern  bool    yesNo (const string& header, const string& prompt);

    inline  bool    skipYesNo (const string& header)    {return (yesNo (header, "Skip"));}

    inline  bool    repeatYesNo (const string& header)  {return (yesNo (header, "Repeat"));}
};

//----------------------------------------------------------------------------//
//
// Dialogue::fillInTheBlanks() is used by the Quiz namespace to pose fill in
// the blanks style quiz questions to the user.
//
// It takes as parameters:
//    - element - is the statement with terms to be blanked
//    - sourceTerms - is the list of terms that may be blanked
//    - choices - the number of terms to be blanked
//    - goodResponse - out - set to false if the user gets it wrong
//
// For implementation details see Dialogue.cpp.
//
//----------------------------------------------------------------------------//

namespace   Dialogue
{
    extern  void    fillInTheBlanks (const Html::Element& element, Quiz::ContentsList& sourceTerms, const int choices, bool& goodResponse);
};

# endif  /* _DIALOGUE_H */
