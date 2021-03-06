/* src/vm/jit/alpha/disass.cpp - primitive disassembler for Alpha machine code

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

*/


#include "config.h"

#include <string.h>
#include <dis-asm.h>
#include <cstdio>

#include "vm/types.hpp"

#include "vm/global.hpp"

#include "vm/jit/abi.hpp"
#include "vm/jit/disass.hpp"


/* disassinstr *****************************************************************

   Outputs a disassembler listing of one machine code instruction on
   'stdout'.

   code: pointer to instructions machine code

*******************************************************************************/

u1 *disassinstr(u1 *code)
{
    s4 seqlen;
    s4 i;

    if (!disass_initialized) {
        INIT_DISASSEMBLE_INFO(info, NULL, disass_printf);

        /* setting the struct members must be done after
         * INIT_DISASSEMBLE_INFO */
        info.mach             = bfd_mach_aarch64;
        info.read_memory_func = &disass_buffer_read_memory;

#if HAVE_ONE_ARG_DISASM
        disass_func = print_insn_aarch64;
#else
        disass_func = disassembler(bfd_arch_aarch64, FALSE, bfd_mach_aarch64, nullptr);
#endif

        disass_initialized = true;
    }

    printf("0x%016lx:   ", (s8) code);
    
    disass_len = 0;

    seqlen = disass_func((bfd_vma) code, &info);

    for (i = 0; i < seqlen; i++, code++) {
        printf("%02x ", *code);
    }

    for (; i < 10; i++) {
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
