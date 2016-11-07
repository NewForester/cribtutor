//----------------------------------------------------------------------------//
//
// Implementation file for the Dialogue namespace of the cribtutor program.
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

#include "Dialogue.h"
#include "Terms.h"

#include <cctype>

#include <algorithm>
#include <iostream>
#include <string>

using namespace std;


//----------------------------------------------------------------------------//
//
// operator<< is overloaded for Terms::MaskedTermList.
//
// It is used to peek at the expected answer to a quiz question.
//
// Terms may separated by , or ;.  The comma is used to separate terms in an
// unordered list:  the user may enter such terms in any order.  The semi-colon
// is used to separate (groups of) terms than must be entered in order from
// left to right.  A compound term appears as is without punctuation characters.
//
// Thus:
//
//    organisations; European Union, United States; international;
//
// shows organisations must be entered first and international last but
// United States and European Union may be entered in either order.
//
//----------------------------------------------------------------------------//

namespace   Dialogue
{
    void    printWord (const string& word)
    {
        cout << " " << word;
    }

    void    printTerm (const Terms::CompoundTerm& term)
    {
        printWord(term.first);

        for_each(term.second.begin(), term.second.end(), printWord);

        cout << ",";
    }

    void    printTermSet (const Terms::MaskedTermSet& terms)
    {
        for_each(terms.begin(), terms.end(), printTerm);

        cout << "\b;";
    }

    void    printTermList (const Terms::MaskedTermList& terms)
    {
        for_each (terms.begin(), terms.end(), printTermSet);
    }

    // the masked terms is hierarchy that suits a for each approach

    ostream& operator<< (ostream &stream, const Terms::MaskedTermList& terms)
    {
        return (printTermList (terms), stream);
    }
};

//----  local routines

namespace       Dialogue
{
    static  bool    tryAgain (const Terms::MaskedTermList& maskedTerms, bool& goodResponse);
};

//----------------------------------------------------------------------------//
//
// Of the two interface routines Dialogue::yesNo() is the simple one.
//
// The parameters keep the prompt flexible but not the response, which is
// one of: yes, no and quit.  Quit exits the program immediately.  No is
// the default (user just presses return).
//
//----------------------------------------------------------------------------//

bool    Dialogue::yesNo (const string& header, const string& prompt)
{
    if (cin)
    {
        cout << header << endl;
        cout << "    " << prompt << " [yNq] ? ";

        string      response;
        getline(cin, response);

        cout << endl;

        if (cin)
        {
            const char  letter = (response.empty()) ? 'n' : tolower(response[0]);

            switch (letter)
            {
                case ('q'):
                    break;
                case ('y'):
                    return (true);
                default:
                    return (false);
            }
        }
    }

    exit(0);
}

//----------------------------------------------------------------------------//
//
// Dialogue::fillInTheBlanks() poses a fill in the blanks question to the user
// and solicits a response.
//
// If the response is incorrect, the user is invited to try again until either
// they get it right or give up.  See Dialogue::tryAgain() for details.
//
// The generation of the question and analysis of the response is delegated is
// delegated to the Terms namespace.
//
//----------------------------------------------------------------------------//

void    Dialogue::fillInTheBlanks (const Html::Element& element, Quiz::ContentsList& sourceTerms, const int choices, bool& goodResponse)
{
    int     termCount = sourceTerms.size();

    if (choices == 0 || 2 * termCount < choices)
    {
        // no (or relatively too few) terms - just print the paragraph

        cout << element << endl;
    }
    else
    {
        // ask the user to fill in the blanks until they get it right or give up

        Terms::MaskedTermList  maskedTerms;

        do
        {
            Terms::mask(maskedTerms, sourceTerms, termCount, choices);

            cout << element << endl;

            Html::Element*    lastSubelement = (element.contents.end() - 1)->subElement;

            if (lastSubelement)
                if (lastSubelement->tag == Html::Markup::asis || lastSubelement->tag == Html::Markup::olst)
                    cout << endl;

            Terms::reset(sourceTerms);
        }
        while (tryAgain(maskedTerms, goodResponse));
    }

    cout << endl;
}

//----------------------------------------------------------------------------//
//
// Dialogue::tryAgain() solicits a response to a fill in the blanks question
// and, if incorrect, asks the user if they would like to try again.
//
// The standard responses are yes, no and quit.  Quit exits the program.  No
// is the default (user just presses return).  The routine returns true
// if the response is yes.  In all other circumstances, it return false.
//
// A response of ? gives a peek at the expected answer.  The user does not
// get to try again.
//
// The user may simply try again without answering the prompt.
//
//----------------------------------------------------------------------------//

bool    Dialogue::tryAgain (const Terms::MaskedTermList& maskedTerms, bool& goodResponse)
{
    string  response;

    // ask nicely

    if (cin)
    {
        cout << "Fill in the blanks: ";
        getline(cin, response);
    }

    // empty response means skip - check otherwise

    while (!response.empty() && !Terms::check(maskedTerms, response))
    {
        if (response != "?" && response != "q")
        {
            if (cin)
            {
                cout << "    Oops ... try again [yNq?] ? ";
                getline(cin, response);
            }
        }

        if (response.length() <= 1)
        {
            if (!cin) break;

            goodResponse = false;

            const char  letter = (response.empty()) ? 'n' : tolower(response[0]);

            switch (letter)
            {
                case ('q'):
                    cout << endl;
                    exit(0);
                case ('?'):
                    cout << " >>" << maskedTerms << endl;
                case ('n'):
                    return (false);
                case ('y'):
                    return (true);
                default:
                    break;
            }
        }
        else
        {
            // assume the user has tried again and so check their response again
        }
    }

    // don't try again

    if (!cin)
    {
        cout << endl;
        exit(0);
    }

    return (false);
}

// EOF
