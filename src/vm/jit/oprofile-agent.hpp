/* src/vm/jit/oprofile-agent.hpp - oprofile agent implementation

   Copyright (C) 2008
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


#ifndef _OPROFILE_AGENT_HPP
#define _OPROFILE_AGENT_HPP

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "vm/method.h"

#include <opagent.h>

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class OprofileAgent
{
	static op_agent_t _handle;

public:
	static void initialize();

	static void newmethod(methodinfo *);

	static void close();
};

#else

/* Legacy C interface *********************************************************/

typedef struct OprofileAgent OprofileAgent;

void  OprofileAgent_initialize(void);
void  OprofileAgent_newmethod(methodinfo *);
void  OprofileAgent_close();

#endif

#endif /* _OPROFILE_AGENT_HPP */


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
