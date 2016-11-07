//----------------------------------------------------------------------------//
//
// The cribtutor program is pedagogic.
//
// It generates fill-in-the-blanks quizzes from crib sheets.  The crib sheets
// are expressed in a subset of html but the program has only a simple command
// line interface, not a web interface.
//
// This source file contains only the main routine, command line processing,
// the global variables set from the command line arguments and the top level
// routine cribSheetQuiz().
//
// main() processes a file that lists the crib sheets.  cribSheetQuiz()
// processes crib sheets one at a time.  It opens the crib sheet but delegates
// parsing to Html::parseCribSheet() and running the quiz to Quiz::run().
//
// See Html.h and Quiz.h for details.
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
#include "SectionNumber.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include <cstdlib>

using namespace std;

//----  program parameters

int     choices = 2;
bool    runQuiz = true;
string  cribSheetDirectory (".");
string  cribSheets ("cribsheets.txt");
string  beginsWith;

//----  forward declarations - first level routines

static  void    cribSheetQuiz (const string& pathName, int choices);

static  void    processArguments (int argc, char* argv[]);

//----  forward declarations - help routines

static  int     convertInteger (const char* param);

static  void    getCribsheetName (ifstream& sheets, string& pathName);

static  string  file (const string& pathName);

static  string  directory (const string& pathName);

static  string  normalise (const string& pathName);

//----  main

int     main (int argc, char* argv[])
{
    processArguments (argc, argv);

    // open the (external) list of crib sheets

    string      pathName (cribSheetDirectory + cribSheets);

    ifstream    sheets (pathName.c_str(), ios_base::in);

    if (!sheets)
    {
        cerr << "Not found: '" << pathName << "'" << endl;
        return (1);
    }

    // process the list of crib sheets

    bool    fastforward = !beginsWith.empty();

    do
    {
        // get next crib sheet pathname

        getCribsheetName(sheets, pathName);

        if (pathName.empty()) continue;

        // fast forward to the first crib sheet whose name begins with beginsWith

        if (fastforward)
            fastforward = (file(pathName).compare(0, beginsWith.size(), beginsWith) != 0);

        // process crib sheets one by obe

        if (!fastforward)
            cribSheetQuiz(cribSheetDirectory + pathName, choices);
    }
    while (sheets && (fastforward || runQuiz));

    if (fastforward)
    {
        cerr << "Skip to: '" << beginsWith << "' not found" << endl;
        return (1);
    }

    return (0);
}

//----------------------------------------------------------------------------//
//
// First level routines
//
//----------------------------------------------------------------------------//

//----  do a crib sheet based quiz

void    cribSheetQuiz (const string& pathName, int choices)
{
    // open the crib sheet

    ifstream    cribSheet (pathName.c_str(), ios_base::in);

    if (!cribSheet.good())
    {
         cerr << "Not found: '" << pathName << "'" << endl;
         return;
    }

    // read and parse the crib sheet

    Html::Element   html;

    Html::parseCribSheet(cribSheet, html);

    if (runQuiz)
    {
        // initialise section numbering and run the quiz

        SectionNumber   prefix (pathName);

        Quiz::run (prefix, html, choices);
    }
    else
    {
        // just print the parsed html

        cout << html << endl;
    }
}

//----  process parameters (sets globals)

void    processArguments (int argc, char* argv[])
{
    int     nn = 0;

    if (argc == 1)
    {
        runQuiz = false;
        cribSheetDirectory = "help";
    }

    for (int ii = 1; ii < argc; ++ii)
    {
        string  arg = argv[ii];

        // undocumented flag bypass

        if (arg[0] != '-' && nn < 4)
        {
            static  const char *    bypass [] = { "-d", "-s", "-c", "-f" };

            arg = bypass [nn++];

            ii--;
        }

        // normal options

        if (arg == "-d" || arg == "--directory")
        {
            if (argv[++ii] != 0)
                cribSheetDirectory = argv[ii];

            nn = 1;

            continue;
        }

        if (arg == "-s" || arg == "--skipto")
        {
            if (argv[++ii] != 0)
                beginsWith = argv[ii];

            nn = 2;

            continue;
        }

        if (arg == "-c" || arg == "--choices")
        {
            if (argv[++ii] != 0)
                choices = convertInteger(argv[ii]);

            nn = 3;

            continue;
        }

        if (arg == "-f" || arg == "--file")
        {
            if (argv[++ii] != 0)
                cribSheets = argv[ii];

            nn = 4;

            continue;
        }

        // test options

        if (arg == "-t" || arg == "--test")
        {
            cribSheetDirectory = "test";

            nn = 1;

            continue;
        }

        if (arg == "-p" || arg == "--parser")
        {
            runQuiz = false;

            continue;
        }

        if (arg == "-r" || arg == "--raw")
        {
            Html::verbose = true;

            continue;
        }

        // help options

        if (arg == "-h" || arg == "--help")
        {
            cribSheetDirectory = "help";

            nn = 1;

            continue;
        }

        if (ii != 0)
        {
            cerr << "Not recognised: '" << arg << "'" << endl;
            exit (1);
        }
    }

    // normalise the crib sheet path

    cribSheetDirectory = normalise (directory(argv[0]) + cribSheetDirectory + "/");
}

//----------------------------------------------------------------------------//
//
// Helper routines - nothing complex but currently hardwired for Unicies.
//
// KISS routines - no requirement for speed.
//
//----------------------------------------------------------------------------//

//---   convert the string representation to an integer

int     convertInteger (const char* param)
{
    istringstream   stream (param);

    int     result;
    stream >> result;

    return (result);
}

//---   read the next line from cribsheet.txt and strip comments and white space

void    getCribsheetName (ifstream& sheets, string& pathName)
{
    getline(sheets, pathName);

    size_t pos;

    // strip comments

    pos = pathName.find("#");
    if (pos != string::npos)
        pathName.erase(pos);

    // convert tabs to spaces

    for (pos = pathName.find("\t"); pos != string::npos; pos = pathName.find("\t"))
        pathName.replace(pos, 1, " ");

    // strip trailing space

    for (pos = pathName.length() - 1; pathName[pos] == ' '; --pos)
        pathName.erase(pos, 1);

    // strip leading space

    for (pos = 0; pathName[pos] == ' ';)
        pathName.erase(pos, 1);
}

//----  return the file part of a path name

string  file (const string& pathName)
{
    size_t  pos = pathName.rfind("/");

    if (pos != string::npos)
        pos += 1;
    else
        pos = 0;

    return (pathName.substr(pos));
}

//----  return the directory part of a path name

string  directory (const string& pathName)
{
    size_t  pos = pathName.rfind("/");

    if (pos != string::npos)
        return (pathName.substr(0, pos + 1));

    return (pathName);
}

//----  return a normalised a pathname (after joining relative pathname)

string  normalise (const string& pathName)
{
    string  normal (pathName);

    do
    {
        const int   pos = normal.find("\\");

        if (pos == string::npos) break;

        normal.replace(pos, 1, "/");
    }
    while (true);

    do
    {
        const int   pos = normal.find("//");

        if (pos == string::npos) break;

        normal.erase(pos, 1);
    }
    while (true);

    do
    {
        const int   pos = normal.rfind("/../");

        if (pos == string::npos || pos == 0) break;

        const int   bpos = normal.rfind("/", pos - 1);

        if (bpos == string::npos) break;

        normal.erase(bpos, pos + 3 - bpos);
    }
    while (true);

    do
    {
        const int   pos = normal.find("/./");

        if (pos == string::npos) break;

        normal.erase(pos, 2);

        break;
    }
    while (true);

    return (normal);
}

// EOF
