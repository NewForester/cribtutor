##----------------------------------------------------------------------------##
##
## This is the makefile for the
##    https://github.com/NewForester/cribtutor project
##    Copyright (C) 2016, NewForester
##    Released under the terms of the GNU GPL v2
##
##----------------------------------------------------------------------------##
##
## make - will build the program
## make test - will also run the regression tests
##
## make clean - will remove the object files
## make clobber - will also remove the executable
##
##----------------------------------------------------------------------------##

all:	cribtutor

OBJS=cribtutor.o Dialogue.o Html.o Quiz.o SectionNumber.o Terms.o

cribtutor.o:		Quiz.h SectionNumber.h Html.h
Quiz.o:			Quiz.h SectionNumber.h Dialogue.h Html.h
Dialogue.o:		Dialogue.h Terms.h Quiz.h Html.h
SectionNumber.o:	SectionNumber.h
Terms.o:		Terms.h Quiz.h
Html.o:			Html.h

cribtutor:	$(OBJS)
	g++ $^ -o $@;

clean:
	rm -f $(OBJS);

clobber:	clean
	rm -f cribtutor

.PHONY:	test
test:	cribtutor
	test/regress

.PHONY:	help
help:
	@sed -e "/^##/!d" -e "s/^##//" -e "s/^ //" makefile;

# EOF
