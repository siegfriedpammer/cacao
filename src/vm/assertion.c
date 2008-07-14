/* src/vm/assertion.c - assertion options

   Copyright (C) 2007, 2008
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

#include <stdint.h>
#include <errno.h>

#include "mm/memory.h"

#include "toolbox/list.h"

#include "vm/assertion.h"
#include "vm/global.h"
#include "vm/vm.hpp"

#include "vmcore/system.h"


/* -ea/-da options ************************************************************/

list_t  *list_assertion_names     = (list_t *)NULL;
int32_t  assertion_class_count    = 0;
int32_t  assertion_package_count  = 0;
bool     assertion_user_enabled   = false;
bool     assertion_system_enabled = false;


/* assertion_ea_da *************************************************************

   Handle -ea:/-enableassertions: and -da:/-disableassertions: options.

*******************************************************************************/

void assertion_ea_da(const char *name, bool enabled)
{
	bool              package;
	size_t            len;
	char             *buf;
	assertion_name_t *item;
	int32_t           i;

	if (name == NULL) {
		assertion_user_enabled = enabled;
		return;
	}

	package = false;
	len     = system_strlen(name);

	if (name[len - 1] == '/') {
		return;
	}

	buf = system_strdup(name);

	if (buf == NULL) {
		vm_abort("assertion_ea_da: strdup failed: %s", strerror(errno));
	}

	if ((len > 2) && (strcmp(name + (len - 3), "...") == 0)) {
		package = true;
		assertion_package_count += 1;
#if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
		buf[len - 2] = '\0';
		buf[len - 3] = '/';
#else
		buf[len - 3] = '\0';
#endif
	}
	else {
		assertion_class_count += 1;
	}

	len = system_strlen(buf);

	for (i = 0; i < len; i++) {
#if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
		if (buf[i] == '.') {
			buf[i] = '/';
		}
#else
		if (buf[i] == '/') {
			buf[i] = '.';
		}
#endif
	}

	item          = NEW(assertion_name_t);
	item->name    = buf;
	item->enabled = enabled;
	item->package = package;

	if (list_assertion_names == NULL) {
		list_assertion_names = list_create(OFFSET(assertion_name_t, linkage));
	}

	list_add_last(list_assertion_names, item);
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
 * vim:noexpandtab:sw=4:ts=4:
 */
