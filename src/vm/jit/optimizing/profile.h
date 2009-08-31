/* src/vm/jit/optimizing/profile.h - runtime profiling

   Copyright (C) 1996-2005, 2006, 2008
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


#ifndef _PROFILE_H
#define _PROFILE_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "vm/types.h"

#include "vm/global.h"


/* CPU cycle counting macros **************************************************/

#if defined(ENABLE_PROFILING) && defined(__X86_64__)

#define PROFILE_CYCLE_START \
    do { \
        if (JITDATA_HAS_FLAG_INSTRUMENT(jd)) { \
            M_PUSH(RAX); \
            M_PUSH(RDX); \
            \
            M_MOV_IMM(code, REG_ITMP3); \
            M_RDTSC; \
            M_ISUB_MEMBASE(RAX, REG_ITMP3, OFFSET(codeinfo, cycles)); \
            M_ISBB_MEMBASE(RDX, REG_ITMP3, OFFSET(codeinfo, cycles) + 4); \
            \
            M_POP(RDX); \
            M_POP(RAX); \
        } \
    } while (0)

#define PROFILE_CYCLE_STOP \
    do { \
        if (JITDATA_HAS_FLAG_INSTRUMENT(jd)) { \
            M_PUSH(RAX); \
            M_PUSH(RDX); \
            \
            M_MOV_IMM(code, REG_ITMP3); \
            M_RDTSC; \
            M_IADD_MEMBASE(RAX, REG_ITMP3, OFFSET(codeinfo, cycles)); \
            M_IADC_MEMBASE(RDX, REG_ITMP3, OFFSET(codeinfo, cycles) + 4); \
            \
            M_POP(RDX); \
            M_POP(RAX); \
        } \
    } while (0)

#else

#define PROFILE_CYCLE_START
#define PROFILE_CYCLE_STOP

#endif


/* function prototypes ********************************************************/

bool profile_init(void);
bool profile_start_thread(void);

#if !defined(NDEBUG)
void profile_printstats(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _PROFILE_H */


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
