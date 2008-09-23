/* src/vm/access.h - checking access rights

   Copyright (C) 1996-2005, 2006, 2007, 2008
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


#ifndef _ACCESS_H
#define _ACCESS_H

#include "config.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "vm/class.hpp"
#include "vm/field.hpp"
#include "vm/global.h"
#include "vm/method.h"


/* macros *********************************************************************/

#define SAME_PACKAGE(a,b)                                  \
			((a)->classloader == (b)->classloader &&       \
			 (a)->packagename == (b)->packagename)


/* function prototypes ********************************************************/

bool access_is_accessible_class(classinfo *referer, classinfo *cls);

bool access_is_accessible_member(classinfo *referer, classinfo *declarer,
								 int32_t memberflags);

#if defined(ENABLE_JAVASE)
bool access_check_field(fieldinfo *f, int callerdepth);
bool access_check_method(methodinfo *m, int callerdepth);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ACCESS_H */


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

