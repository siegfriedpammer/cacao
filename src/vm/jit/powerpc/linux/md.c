/* src/vm/jit/powerpc/linux/md.c - machine dependent PowerPC Linux functions

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Christian Thalinger

   Changes:

   $Id: md.c 2816 2005-06-23 15:21:02Z twisti $

*/


#include <ucontext.h>

#include "vm/jit/powerpc/types.h"
#include "vm/jit/powerpc/linux/md-abi.h"

#include "vm/exceptions.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"


/* signal_handle_sigsegv *******************************************************

   NullPointerException signal handler for hardware null pointer check.

*******************************************************************************/

void signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t *_uc;
	mcontext_t *_mc;
	u4          instr;
	s4          reg;
	ptrint      addr;

 	_uc = (ucontext_t *) _p;
 	_mc = _uc->uc_mcontext.uc_regs;

	instr = *((u4 *) _mc->gregs[PT_NIP]);
	reg = (instr >> 16) & 0x1f;
	addr = _mc->gregs[reg];

	if (addr == 0) {
		_mc->gregs[REG_ITMP1_XPTR] = (ptrint) new_nullpointerexception();
		_mc->gregs[REG_ITMP2_XPC] = _mc->gregs[PT_NIP];
		_mc->gregs[PT_NIP] = (ptrint) asm_handle_exception;

	} else {
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Segmentation fault at 0x%08lx", addr);
	}		
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
