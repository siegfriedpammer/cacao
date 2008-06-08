/* src/vm/optimizing/escape.h

   Copyright (C) 2008
   CACAOVM - Verein zu Foerderung der freien virtuellen Machine CACAO

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

#ifndef _VM_JIT_OPTIMIZING_ESCAPE_H
#define _VM_JIT_OPTIMIZING_ESCAPE_H

#include "vm/jit/jit.h"
#include "vmcore/method.h"

typedef enum {
	ESCAPE_UNKNOWN,
	ESCAPE_NONE,
	ESCAPE_METHOD,
	ESCAPE_GLOBAL_THROUGH_METHOD,
	ESCAPE_GLOBAL
} escape_state_t;

void escape_analysis_perform(jitdata *jd);

void escape_analysis_escape_check(void *vp);

void bc_escape_analysis_perform(methodinfo *m);

#endif
