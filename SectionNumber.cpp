//----------------------------------------------------------------------------//
//
// Implementation file for the SectionNumber class of the cribtutor program.
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

#include "SectionNumber.h"

#include <sstream>
#include <string>

using namespace std;

//----------------------------------------------------------------------------//
//
// See SectionNumber.h for a description of the class interface and behaviour.
//
//----------------------------------------------------------------------------//

//----  make prefix from cribsheet pathname

string      SectionNumber::makePrefix (const string& pathName)
{
    string  prefix (pathName);

    // drop path from  pathname (if any)

    size_t  pos = prefix.rfind("/");

    if (pos != string::npos)
        prefix.erase(0, pos + 1);

    // anything before the the first underscore is the raw prefix

    pos = prefix.find("_");

    if (pos != string::npos)
        prefix.erase(pos, string::npos);
    else
        prefix.clear();

    // the prefix is assumed to be one or two digits

    switch (prefix.size())
    {
        default:
            // expand as necessary
        case (2):       // take prefix[1] as chapter number
        case (1):       // take prefix[0] as chapter number
        {
            int     last = prefix.size() - 1;
            istringstream   stream(prefix.substr(last, 1));
            stream >> chapterNumber;
            chapterNumber--;
            sectionNumber = 0;
            prefix.erase(last, 1);
            break;
        }
        case (0):
            break;
    }

    return (prefix);
}

//----  return header with prefix (if any)

string    SectionNumber::quiz (const string& header)
{
    ostringstream   result;

    if (!prefix.empty())
        result << prefix << ' ';

    result << header;

    return (result.str());
}

//----  return header with chapter number prefix

string    SectionNumber::chapter (const string& header)
{
    ostringstream   result;

    if (chapterNumber != noChapterNumber)
    {
        if (!prefix.empty())
            result << prefix << '.';

        result << ++chapterNumber << ' ';

        sectionNumber = 0;
    }
    else
    {
        result << endl;
    }

    result << header;

    return (result.str());
}

//----  return header with section number prefix

string    SectionNumber::section (const string& header)
{
    ostringstream   result;

    if (sectionNumber != noSectionNumber)
    {
        if (!prefix.empty())
            result << prefix << '.';

        result << chapterNumber << '.' << ++sectionNumber << ' ';
    }
    else
    {
        result << endl;
    }

    result << header;

    return (result.str());
}

// EOF
