#ifndef AUTOMATA_HPP_
#define AUTOMATA_HPP_ 1
#include "vm/jit/codegen-common.hpp"
#include "vm/jit/ir/instruction.hpp"

void automaton_initialize(basicblock *bptr);
void automaton_next(jitdata *jd, instruction *iptr);
void automaton_cleanup();

#endif