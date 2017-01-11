//----------------------------------------------------------------------------//
//
// Implementation file for the Dialogue namespace of the cribtutor program.
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

#include "Terms.h"

#include <cctype>

#include <algorithm>
#include <string>
#include <vector>

using namespace std;

//----------------------------------------------------------------------------//
//
// If the user were asked to fill in the blanks, in order, from left to right
// and their response had to be letter perfect, the routines in this namespace
// would be very simple but the program would also be frustrating to use.
//
// It isn't feasible to run the user's response through a spell checker but
// it is possible, without too much cost, to allow some leeway.  Half a dozen
// measures have been implemented to reduce the frustration level.  Most
// could be taken further but with diminishing returns and with increasing
// ambiguity.
//
// 1.  Compound Terms
//
// Simple terms are blanked with ____.  Compound terms comprise more than one
// word.  These are blank with a ____ sequence:  one ____ for each word.
//
// Element::contentMask is the mask, Terms::mask() and Terms::reset() set and
// clear the mask while Html::printElement() blanks terms by printing the mask
// instead of the term.
//
// 2.  Hyphenated Terms
//
// An example of simple, compound and hyphenated terms is: "microcomputer",
// "micro computer", "micro-computer".  The hyphenated term is treated in
// Terms::newTerm() very much like the compound term except that the content
// mask will be "____-____" instead of "____ ____".  splitIntoWords() allows
// a user response to include the hyphen but it is optional.
//
// Note the hyphen is the minus sign, not one a several Unicode hyphens.
//
// 3.  Unordered Lists
//
// Terms in an unordered list need not be entered in list order.  This gives
// rise to the typedefs described in Terms.h and much of the complexity of
// Terms::mask() and Terms::check().
//
// 4.  Paired Terms
//
// Pairs of terms adjacent except for the conjunctions " and ", " or " and "/"
// are treated as mini-lists of two unordered terms.
//
// Yes, conjunctions may be strung together into mini-lists of three or more
// terms and yes, mixing conjunctions is allowed but makes little sense.
//
// A single term containing "/" is also treated in Terms::mask() as a
// mini-list but it counts as only one choice.  splitIntoWords() allows a
// user response to include the slash but it is optional.
//
// Yes, this does mean the user can enter O/I instead of I/O.
//
// 5.   Ignore Punctuation in Responses
//
// The program prints 'ideal' responses with ',' and ';' as delimiters: the
// comma between unordered terms, the semi-colon between ordered (groups of)
// terms.  Spaces occur between the words of a compound and hyphenated terms.
//
// Punctuation is not needed in responses but its presence should not be a
// reason to fail a response.
//
// splitIntoWords() and friends drop trailing ';', ',' and '.' from the words
// in responses and treat '-' and '/' as well as space as separating words.
//
// What if a term legitimately includes a '-' or '/' ?  These characters are
// not treated as separating words when a word appears to start or end with
// a non-alphanumeric character.  Yes, this is only a partial solution.
//
// 6.   Ignore case at the start of a sentence
//
// English sentences begin with a capital letter but a term is a term and
// should not have to be entered differently simply because of its appears at
// the start of a sentence.
//
// Terms::mask() lowers the case of the first letter of a simple term when
// the term begins a sentence.  Terms::check() checks terms as is and with
// the case of the first letter lowered.
//
// There is the assumption that compound terms are supposed to be entered
// verbatim as very often each word is capitalised.
//
// 7.   Alternative Spellings
//
// English has notoriously eccentric spelling and, in some very common cases,
// different nations spell the same word differently.  Terms::adjustSpelling()
// is used to equivalence -ise/-ize, -ice/-ise and -our/-or spellings.
//
// 8.   Plurals
//
// In some sentences, either the singular term or the plural term make sense.
// Terms:fuzzyCompare() goes some way to equivalencing regular plurals with
// the singular.  Not perfect.
//
//----------------------------------------------------------------------------//

//----  not Term specific but used here

typedef deque< string > WordList;

static  void    splitIntoWords (const string& line, WordList& wordList);

//---- Avoid length error crashes

inline  size_t  safeLength(size_t bpos, size_t epos)
{
    return ((epos == string::npos) ? epos : epos - bpos);
}

//---- Adjust end position to avoid null strings

inline  size_t  safeEndPosition(size_t epos, const string& text)
{
    if (epos != string::npos)
        if (text[epos + 1] == '\0')
            return (string::npos);

    return (epos);
}

//----  local routines

namespace   Terms
{
    static  CompoundTerm  newTerm (string &contentMask, const string& maskedTerm);

    static  MaskedTermSet::iterator fuzzyFind (MaskedTermSet& termSet, string& term);

    static  bool    fuzzyCompare (string lhs, string rhs);

    static  void    adjustSpelling (string& lhs, string& rhs, const string& alt1, const string& alt2);
};

//----  reset the blanking content mask for all source terms

void    Terms::reset (SourceTermList& sourceTerms)
{
    for (SourceTermList::iterator it = sourceTerms.begin(); it != sourceTerms.end(); ++it)
    {
        if ((*it)->subElement == 0)
            continue;

        Html::Element&  element = *(*it)->subElement;

        element.contentMask.clear();
    }
}

//----  create a list of masked terms and set the blanking content mask for each

int     Terms::mask (MaskedTermList& maskedTerms, SourceTermList& sourceTerms, int termCount, const int choices)
{
    maskedTerms.clear();

    // terms to be masked as chosen randomly

    vector <int>    shuffle (termCount);

    for (int ii = 0; ii < termCount; ++ii)
        shuffle [ii] = ii;

    random_shuffle (shuffle.begin(), shuffle.end());

    termCount = min(termCount, choices);

    sort (shuffle.begin(), shuffle.begin() + termCount);

    shuffle.resize(termCount);

    // mask the chosen terms

    MaskedTermSet   termSet;

    for (vector< int >::iterator it = shuffle.begin(); it != shuffle.end(); ++it)
    {
        SourceTermIterator&     sourceTerm = *(sourceTerms.begin() + it[0]);

        // get to the text of the source term safely

        if (sourceTerm->subElement == 0)
            continue;

        Html::Element&  element = *sourceTerm->subElement;

        if (element.contents.empty())
            continue;

        if (element.strictOrder)
        {
            // add outstanding set of terms to the list of masked terms

            if (!termSet.empty())
            {
                maskedTerms.push_back(termSet);

                termSet.clear();
            }
        }

        // allow for a lax case comparison of the initial letter of sentences.

        string          termText = element.contents.begin()->text;

        if (element.startOfSentence)
            if (termText.length() > 1)
            {
                size_t  pos = termText.find(" ");

                char    letter = termText[pos == string::npos ? 1 : pos + 1];

                if (letter == tolower(letter))
                    termText[0] = tolower(termText[0]);
            }

        //  add new term to current set of terms

        element.contentMask = "";

        size_t  bpos = 0;
        size_t  epos = safeEndPosition(termText.find_first_of("/"), termText);

        while (epos != string::npos)
        {
            // treat a/b/c and a mini-list

            termSet.insert(newTerm(element.contentMask, termText.substr(bpos, epos - bpos)));

            element.contentMask += "/";

            bpos = epos + 1;
            epos = safeEndPosition(termText.find_first_of("/", bpos), termText);

            ++termCount;
        }

        if (bpos != termText.length())
            termSet.insert(newTerm(element.contentMask, termText.substr(bpos)));

        if (element.strictOrder)
        {
            // add current set of terms to the list of masked terms

            maskedTerms.push_back(termSet);

            termSet.clear();
        }
    }

    // add any outstanding set of terms to the list of masked terms

    if (!termSet.empty())
        maskedTerms.push_back(termSet);

    return (termCount);
}

//----  construct a (compound) masked term and set the blanking content mask

Terms::CompoundTerm  Terms::newTerm (string &contentMask, const string& term)
{
    // split off words one by one - using space or minus to separate them

    size_t  bpos = term.find_first_not_of(" ", 0);
    size_t  epos = safeEndPosition(term.find_first_of(" -", bpos + 1), term);

    // first or only word is map key

    string          termKey (term.substr(bpos, safeLength(bpos, epos)));
    deque <string>  termValue;

    contentMask += "____";

    while (epos != string::npos)
    {
        // second and subsequent words are the map value

        if (term[epos] == ' ')
            contentMask += " ____";
        else if (term[++epos] == '\0')
            break;
        else
            contentMask += "–____";     // not portable ?

        bpos = term.find_first_not_of(" ", epos);
        if (bpos == string::npos) break;

        epos = safeEndPosition(term.find_first_of(" -", bpos + 1), term);

        termValue.push_back(term.substr(bpos, safeLength(bpos, epos)));
    }

    return (CompoundTerm (termKey, termValue));
}

//----   check a response against the masked terms

bool    Terms::check (const MaskedTermList& maskedTerms, const string& response)
{
    // split the response into a list of words that represent terms

    WordList  wordList;

    splitIntoWords (response, wordList);

    // check the masked terms against the response (ordered)

    for (MaskedTermList::const_iterator it = maskedTerms.begin(); it != maskedTerms.end(); ++it)
    {
        MaskedTermSet   termSet = *it;

        // check a set of masked terms against the response

        while (!termSet.empty())
        {
            if (wordList.empty()) return (false);

            MaskedTermSet::iterator     termit = fuzzyFind (termSet, *wordList.begin());

            if (termit == termSet.end()) return (false);

            // a loop is needed to handle multimap with duplicate keys

            while (true)
            {
                WordList::iterator wordit = wordList.begin();

                // check a compound masked term word for word

                deque< string>::iterator it;

                for (it = termit->second.begin(); it != termit->second.end(); ++it)
                     if (++wordit == wordList.end() || !fuzzyCompare (*wordit, *it))
                         break;

                if (it == termit->second.end())
                    break;

                const string&   keyWord = termit->first;

                if (++termit == termSet.end() || termit->first != keyWord)
                    return (false);
            }

            // success - discard

            wordList.pop_front();

            while (!termit->second.empty())
            {
                wordList.pop_front();
                termit->second.pop_front();
            }

            termSet.erase(termit);
        }
    }

    // all words in the response should have been accounted for

    return (wordList.empty());
}

//---   find using a heuristic string compare to allow for alternative spellings and regular plurals

Terms::MaskedTermSet::iterator Terms::fuzzyFind (Terms::MaskedTermSet& termSet, string& term)
{
    Terms::MaskedTermSet::iterator     it;

    // check for an exact match first

    if ((it = termSet.find(term)) != termSet.end())
        return (it);

    // permit the case of the initial letter to differ

    term[0] = tolower(term[0]);

    if ((it = termSet.find(term)) != termSet.end())
        return (it);

    // if this is a simple term ...

    if (it = termSet.begin(), termSet.size() == 1)
        return (fuzzyCompare(it->first, term) ? it : termSet.end());

    // compound terms are a headache (I don't think a fuzzy comparator would work)

    if ((it = termSet.lower_bound(term)) != termSet.end() && fuzzyCompare(it->first, term))
        return (it);

    if ((it = termSet.upper_bound(term)) != termSet.end() && fuzzyCompare(it->first, term))
        return (it);

    if ((it = termSet.end(), --it) != termSet.end() && fuzzyCompare(it->first, term))
        return (it);

    return (termSet.end());
}

//---   heuristic string comparison to allow for alternative spellings and regular plurals

bool    Terms::fuzzyCompare (string lhs, string rhs)
{
    // adust terms for common alternative spellings

    adjustSpelling (lhs, rhs, "isation", "ization");
    adjustSpelling (lhs, rhs, "ise", "ize");
    adjustSpelling (lhs, rhs, "ice", "ise");
    adjustSpelling (lhs, rhs, "our", "or");

    // ensure lhs is the shorter (singular) term

    int     lhslen = lhs.length();
    int     rhslen = rhs.length();

    if (lhslen > rhslen)
    {
        swap(lhs, rhs);
        swap(lhslen, rhslen);
    }

    // be prepared to tamper with the final character of the singular term

    char&   lhsend = lhs[lhslen -1];

    switch (rhslen - lhslen)
    {
        case (0):
            // same length - either equal or irregluar plural like woman <-> women

            if (lhs == rhs) return (true);

            adjustSpelling (lhs, rhs, "man", "men");

            if (lhs == rhs) return (true);

            adjustSpelling (lhs, rhs, "sis", "ses");
            adjustSpelling (lhs, rhs, "cis", "ces");
            adjustSpelling (lhs, rhs, "xis", "xes");

            if (lhs == rhs) return (true);

            break;

        case (1):
            // one character shorter so possibly regular weak plural

            if (lhs + "s" == rhs) return (true);

            // perhaps latin e.g.  focus <-> foci

            if (lhsend == 'i')
                lhsend = 'u';

            if (lhs + "s" == rhs) return (true);

            break;

        case (2):
            // two characters shorter so something like potato <-> potatoes

            if (lhs + "es" == rhs) return (true);

            // perhaps half <-> halves or country <-> countries

            if (lhsend == 'f')
                lhsend = 'v';
            else if (lhsend == 'y')
                lhsend = 'i';

            if (lhs + "es" == rhs) return (true);

            break;
    }

    return (false);
}

//---   adjust spelling (e.g practice and practise so they may compare equal)

void    Terms::adjustSpelling (string& lhs, string& rhs, const string& alt1, const string& alt2)
{
    int     pos;

    if ((pos = lhs.find(alt1)) != string::npos && rhs.rfind(alt2) == pos)
        rhs.replace(pos, alt2.length(), alt1);

    if ((pos = lhs.find(alt2)) != string::npos && rhs.rfind(alt1) == pos)
        rhs.replace(pos, alt1.length(), alt2);
}

//----------------------------------------------------------------------------//
//
// splitIntoWords() was not meant to be Term specific so that it could be used
// in other contexts.  It is now not used in other contexts and is no longer
// general purpose.
//
// The code that is not general purpose is confined to the two inline routines:
// dropTrailingPuncutation() and checkNextTokenIsWord ().
//
// The first inline routine permits user responses of words with trailing comma
// (or semi-colon or full stop).
//
// The second inline routine is mindful of this when it checks the 'first' and
// 'last' characters of the 'next' word.  If both characters are alphanumeric,
// splitInWords() will consider - and / as splitting the 'next' word into two.
//
// The net effect of this is to permit user responses to optionally include the
// hyphen ('-') in hyphenated terms and the slash ('/') between paired terms.
//
// Yes, this does mean that the hyphen and slash may be used in responses
// between any two words, not just those where it is reasonable.
//
//----------------------------------------------------------------------------//

//--- drop trailing ',' ';' or '.' from a word

inline  void    dropTrailingPuncutation (string& word)
{
    const size_t    len = word.length();

    if (len > 0)
    {
        const char& end = word[len - 1];

        if (end == ';' || end == ',' || end == '.')
            word.erase(len - 1);
    }
}

//--- check token starts and ends with alphanumeric characters

inline  bool    checkNextTokenIsWord (const string& line, const size_t bpos, const size_t epos)
{
    if (!isalnum(line[bpos])) return (false);

    const size_t    pos = (epos != string::npos) ? epos - 1 : line.length() - 1;

    if (pos - bpos < 2) return (false);

    const char& end = line[pos];

    if (isalnum(end)) return (true);

    if (end == ';' || end == ',' || end == '.')
        return (isalnum(line[pos - 1]));

    return (false);
}

//--- split a 'line' into 'words'

void    splitIntoWords (const string& line, WordList& wordList)
{
    if (line.length() == 0)
        return;

    size_t  bpos = 0, epos = 0;

    do
    {
        // find start of next word

        bpos = line.find_first_not_of(" ", epos);

        if (bpos == string::npos)
            break;

        // find end of current word

        epos = line.find_first_of(" ", bpos);

        if (checkNextTokenIsWord(line, bpos, epos))
            epos = line.find_first_of(" -/", bpos);

        // extract current word from line and add to list

        string  word = line.substr(bpos, safeLength(bpos, epos));

        dropTrailingPuncutation(word);

        wordList.push_back(word);

        if (epos != string::npos)
            ++epos;
    }
    while (bpos != string::npos);
}

// EOF
