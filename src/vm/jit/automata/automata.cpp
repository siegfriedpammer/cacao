#include <stdio.h>

#include "automata.hpp"

void automaton_initialize(basicblock *bptr)
{
    printf("initialize: yayyayayayyy\n");
}

void automaton_next(jitdata *jd, instruction *iptr)
{
    printf("next: %d\n", iptr->opc);
}

void automaton_cleanup()
{
    printf("cleanup: yayyayayayyy\n");
}