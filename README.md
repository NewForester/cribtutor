# cribtutor
A simple program to run fill-in-the-blank quizzes from sets of crib-sheets.

---

The cribtutor program is pedagogic.

It generates fill-in-the-blanks quizzes from crib-sheets.
The crib-sheets are expressed in a subset of *html*
but the program has only a simple command line interface, not a web interface.

---

To build the program under Linux, enter:

    make

in the directory in which the repository has been cloned.
The result of the build is an executable named *cribtutor*.

For more information on how to invoke the program, enter:

    ./cribtutor

An example crib-sheet is provided.
To generate a quiz from this example, enter:

    ./cribtutor -d c++

---

Help can be invoked with:

    ./cribtutor --help

This allows the browsing several help topics:

  * command line options
  * how to use the program
  * how to write crib-sheets for the program
  * how to use Markdown to write crib-sheets for the program
  * the basic ideas behind the program

---

The program is written in C++ but is not particularly object oriented:
it is rather more recursive-procedural.

It makes use of STL containers, algorithms and strings
but defines no templates of its own.

It does not use any element of C++ 11 (or later).

The code itself is intended to be self-explanatory as to &apos;how&apos;.
The comments attempt to explain &apos;what&apos; and &apos;why&apos;.

---

There are a number of unit tests in the subdirectory *test/*.
They are almost certainly not as self-evident as had been hoped.

Individual tests can be run from the command line with:

    ./cribtutor -t <test> [-p]

where &lt;test&gt; are the initial letters of the test file name.
Tests whose names end in *-p* should be run with the *-p* flag,
which simply prints, no questions asked.

The tests are used for regression purposes:

    test/regress <test>

will run the test comparing output with a reference file and,
when appropriate, taking input from a file instead of the terminal.
No terminal output means success.

The command:

    make test

will ensure the program is built and runs all regression tests.

The tests were written in the spirit of TDD but they do not use a unit test framework.

One principle of TDD is that each test case should be atomic.
This would lead to many, many, trivial test, reference and input files.
Here test files contain *html* block elements each of which may be considered a test case.

Another consequence of an atomic test case approach is that it does no catch combinatorial bugs:
if A pass, if B pass but if A and B then fail.

The regression tests are brittle.
The program uses random shuffles:
add a new case to the test and the existing cases are affected.
Both input and reference files need to be updated to compensate.

Test coverage is incomplete.
They do not, for example, test the command line option/flag logic.

---

Release 1.0.0.  Program, help files and test files committed.
Example crib-sheet committed.

Copyright (C) 2016, NewForester, released under the terms of the GNU GPL v2.

---

Release 1.1.0 with support for html generated using *pandoc* from crib-sheets written using Markdown mark-up.

Copyright (C) 2017, NewForester, released under the terms of the GNU GPL v2.

---

No longer under development although bug fixes are likely.

NewForester, January 2017
