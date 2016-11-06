# cribtutor
A simple program to run fill-in-the-blank quizzes from sets of crib-sheets.

---

The cribtutor program is pedagogic.

It generates fill-in-the-blanks quizzes from crib sheets.
The crib sheetsare expressed in a subset of _html_
but the program has only a simple command line interface, not a web interface.

---

How to build ...

---

Help ...

---

There are a number of unit tests in the subdirectory _test/_.
They are almost certainly not as self-evident as had been hoped.

Individual tests can be run from the command line with:

    ./cribtutor -t <test> [-p]

where <test> are the initial letters of the test file name.
Tests whose names end in _-p_ should be run with the _-p_ flag,
which simply prints. no questions asked.

The tests are used for regression purposes:

    test/regress <test>

will run the test comparing output with a reference file and,
when appropriate, taking input from a file instead of the terminal.
No output means success.

The command:

   make test

will ensure the program is built and runs all regression tests.

The tests were written in the spirit of TDD but they do not use a unit test framework.

One principle of TDD is that each test case should be atomic.
This would lead to many, many, trivial test, reference and input files.
Here test files contain _html_ block elements each of which may be considered a test case.

Another consequence of an atomic test case approach is that it does no catch combinatorial bugs:
if A pass, if B pass but if A and B fail.

The regression tests are brittle.
The program uses random shuffles:
add a new case to the test and the existing cases are affected.
Both input and reference files need to be updated to compensate.

---

Description of code ...

---

Copyright (C) 2012, 2016, NewForester, released under the terms of the GNU GPL v2.

---

Under construction.

    NewForester, November 2016
