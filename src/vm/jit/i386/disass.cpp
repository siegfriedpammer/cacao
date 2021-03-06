/* src/vm/jit/i386/disass.cpp - wrapper functions for GNU binutils disassembler

   Copyright (C) 1996-2013
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

   Contact: cacao@cacaojvm.org

   Authors: Andreas  Krall
            Reinhard Grafl

   Changes: Christian Thalinger

*/


#include "config.h"

#include <assert.h>
#include <string.h>
#include <dis-asm.h>
#include <stdarg.h>

#include "vm/types.hpp"

#include "vm/global.hpp"
#include "vm/jit/disass.hpp"


/* disassinstr *****************************************************************

   Outputs a disassembler listing of one machine code instruction on
   'stdout'.

   code: instructions machine code

*******************************************************************************/

u1 *disassinstr(u1 *code)
{
	s4 seqlen;
	s4 i;

	if (!disass_initialized) {
		INIT_DISASSEMBLE_INFO(info, NULL, disass_printf);

		/* setting the struct members must be done after
		   INIT_DISASSEMBLE_INFO */

		info.mach             = bfd_mach_i386_i386;
		info.read_memory_func = &disass_buffer_read_memory;

#if HAVE_ONE_ARG_DISASM
		disass_func = print_insn_i386;
#else
		disass_func = disassembler(bfd_arch_i386, FALSE, bfd_mach_i386_i386, nullptr);
#endif

		disass_initialized = 1;
	}

	printf("0x%08x:   ", (s4) code);

	disass_len = 0;

	seqlen = disass_func((bfd_vma) (ptrint) code, &info);

	for (i = 0; i < seqlen; i++, code++) {
		printf("%02x ", *code);
	}

	for (; i < 8; i++) {
		printf("   ");
	}

	printf("   %s\n", disass_buf);

	return code;
}


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
