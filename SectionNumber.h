# ifndef    _SECTIONNUMBER_H
# define    _SECTIONNUMBER_H

//----------------------------------------------------------------------------//
//
// Interface file for the SectionNumber class of the cribtutor program.
//
// A typical cribtutor cribsheet partitions source material with html headers.
// These html headers are not numbered.
//
// The SectionNumber class provides 'chapter' and 'section' numbering derived
// from a numeric prefix to cribsheet file names.
//
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
//
// https://github.com/NewForester/cribtutor
// Copyright (C) 2016, NewForester
// Released under the terms of the GNU GPL v2
//
//----------------------------------------------------------------------------//

#include <string>

using namespace std;

//----------------------------------------------------------------------------//
//
// The SectionNumber class is used to prepend chapter and section numbers to
// the text found between html header tags based on an optional prefix on
// cribsheet file names.  There are three cases.
//
// 1) Default: there are no chapter and section numbers.
//    - quiz() simply returns header
//    - chapter() simply returns header
//    - section() simply returns header
//
// 2) If the cribsheet name begins n_, n is taken to be the chapter number.
//    - quiz() prepends nothing to header
//    - chapter() prepends "n " to header
//    - section() prepends "n.m " to header
//
// 3) If the cribsheet name begins Nn_, N is taken to be the immutable prefix
//    and n the chapter number.
//    - quiz() prepends "N " to header
//    - chapter() prepends "N.n " to header
//    - section() prepends "N.n.m " to header
//
// Caveat: both N and n are single digits.
//
// For cases 2) and 3), the initial value of m is 1 and increments with each
// subsequent call to section().
//
// For cases 2) and 3), the initial value of n is n and increments with each
// subsequent call to chapter() and m is reset to 1.
//
//----------------------------------------------------------------------------//

class   SectionNumber
{
    enum {noChapterNumber = -2, noSectionNumber = -2};

public:
    explicit SectionNumber (const string& pathName) :
        chapterNumber (noChapterNumber),
        sectionNumber (noSectionNumber),
        prefix (makePrefix(pathName)) {}

public:
    bool    singleDigit (void) const    { return (prefix.empty() && chapterNumber != noChapterNumber); }
    bool    doubleDigit (void) const    { return (!prefix.empty() && chapterNumber != noChapterNumber); }

    string  quiz    (const string& header);
    string  chapter (const string& header);
    string  section (const string& header);

    void    repeatChapter (void)        { if (sectionNumber != noSectionNumber) sectionNumber = 0; }

private:
    string  makePrefix (const string& pathName);

private:
    int     chapterNumber;
    int     sectionNumber;
    const string    prefix;
};

# endif  /* _SECTIONNUMBER_H */
