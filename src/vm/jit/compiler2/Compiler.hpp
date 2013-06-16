/* src/vm/jit/compiler2/Compiler.hpp - 2nd stage Just-In-Time compiler

   Copyright (C) 2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

/**
 * @file
 * Second stage compiler class.
 */

#ifndef _JIT_COMPILER2_COMPILER
#define _JIT_COMPILER2_COMPILER

#include "vm/method.hpp"

namespace cacao {
namespace jit {
/**
 * Second stage compiler namespace.
 * @ingroup compiler2
 *
 * All parts specific to the second stage compiler are contained in this
 * namespace.
 */
namespace compiler2 {

/**
 * @defgroup compiler2 Second Stage Compiler
 *
 * This module contains the second stage compiler. The second stage compiler
 * (compiler2) is an optimizing Just-In-Time (JIT) compiler for the CACAO
 * virtual machine. It is intended to compile and optimize frequently used
 * methods. The main design goals are code quality and usability in terms
 * of how difficult it is to create new compiler passes.
 *
 * # Intermediate Representations #
 *
 * The compiler centers around two different representation models for
 * the code. In the architecture independent part a SSA based graph
 * representation is used. The absence of a concrete scheduling makes most
 * optimization and analyse passes easier.
 *
 * For the architecture dependent part the graph representation is successively
 * transformed into a list of machine instructions. This form is more suitable
 * for low level tasks such as register allocation and code generation. The
 * following sections will go more details of both representations
 *
 * ## High level intermediate representation ##
 *
 * The main data structure of this representation is the Instruction class.
 * An Instruction can take an arbitrary number of input Values and produce
 * on output Value. The inputs are modeled as pointer to other Instructions.
 * The Instruction itself is the only representative of the value it produces.
 * There are no such things as variables or registers. Each value is defined
 * exactly once, at the Instruction that computes it. This is one requirement
 * for the SSA representation. Phi instruction (PHIInst) are used to join
 * values from different control flow paths. An Instruction with the pointers
 * to its operands form a so-called Use-Def chain. All Use-Def chains together
 * for a Data Flow Graph.
 *
 * Unfortunately this is not enough to comprise the
 * complete semantics of a method. Language constructs like conditionals and
 * loops require some notion of control flow. The traditional approach is to
 * introduce basic blocks. A basic block is a list instructions which one entry
 * at the first instruction and one exit at last instruction. There are no side
 * entries or exits. A basic block might have several preceding and several
 * succeeding basic blocs. The last instruction always changes the control flow
 * via a goto, jump, switch, return, etc. instruction.
 *
 * The basic block representation is more restrictive than necessary. Many
 * instructions are have no dependencies on basic block boundaries. They only
 * depend on their input operands. So in order to keep the freedom of the
 * Data Flow Graph a lightweight replacement model for basic blocks is used.
 * Control flow joins (which would mark the start of a basic block) are
 * modeled as instructions on its own, namely BeginInst. A BeginInst can
 * have several predecessors. Control flow changes (end of basic blocks) are
 * modeled as EndInst. A EndInst may have several successors. Every BeginIns
 * has exactly one EndInst. Such an instruction pair forms a frame for a
 * traditional basic block. To model the control flow dependencies of
 * instructions another type of input edges is introduces, so-called dependency
 * links. Most instruction do not have any dependency links although some
 * instructions need them to maintain the semantics, for example PHIInst.
 * A PHIInst joins values from different preceding control flow paths. The
 * operands of a PHIInst are in direct correspondence to the predecessors
 * of the BeginInst they are connected to.
 *
 * ## Low level intermediate representation ##
 *
 */

typedef u1 MachineCode;

MachineCode* compile(methodinfo* m);

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

#endif /* _JIT_COMPILER2_COMPILER */


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
