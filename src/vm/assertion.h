/* src/vm/assertion.h - assertion options

   Copyright (C) 2007
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


#ifndef _ASSERTION_H
#define _ASSERTION_H

#include "config.h"
#include "vm/global.h"
#include "toolbox/list.h"

#include <stdint.h>

typedef struct assertion_name_t assertion_name_t;

struct assertion_name_t {
	char      *name;
	bool       enabled;
	bool       package;
	listnode_t linkage;
};

/* -ea/-esa/-da/-dsa options **************************************************/

extern list_t  *list_assertion_names;
extern int32_t  assertion_class_count;
extern int32_t  assertion_package_count;
extern bool     assertion_user_enabled;
extern bool     assertion_system_enabled;

/* function prototypes ********************************************************/

void assertion_ea_da(const char *name, bool enabled);

#endif /* _ASSERTION_H */


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
 * vim:noexpandtab:sw=4:ts=4:
 */