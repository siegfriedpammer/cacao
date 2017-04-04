CC=gcc
CPP=g++
O=.o
E=
CPPFLAGS=-g -I ./ -I ../ -std=c++11
LDFLAGS=
YFLAGS=

all:	Matcher

Matcher:	Grammar.inc Matcher.cpp Matcher.hpp Instruction.hpp Type.hpp InstructionIDGen.inc InstructionTermGen.inc
	$(CPP) $(CPPFLAGS) -o Matcher$E Matcher.cpp

clean:
	rm -rf Matcher