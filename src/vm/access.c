/* vm/access.c - checking access rights

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

   Authors: Edwin Steiner

   Changes:

   $Id: access.c 2096 2005-03-27 18:57:00Z edwin $

*/

#include <assert.h>
#include "vm/access.h"
#include "vm/builtin.h"

/****************************************************************************/
/* DEBUG HELPERS                                                            */
/****************************************************************************/

#ifdef NDEBUG
#define ACCESS_DEBUG
#endif

#ifdef ACCESS_DEBUG
#define ACCESS_ASSERT(cond)  assert(cond)
#else
#define ACCESS_ASSERT(cond)
#endif

/****************************************************************************/
/* ACCESS CHECKS                                                            */
/****************************************************************************/

/* for documentation see access.h */
bool
is_accessible_class(classinfo *referer,classinfo *cls)
{
	ACCESS_ASSERT(referer);
	ACCESS_ASSERT(cls);

	/* XXX specially check access to array classes? (vmspec 5.3.3) */
	
	/* public classes are always accessible */
	if ((cls->flags & ACC_PUBLIC) != 0)
		return true;

	/* a class in the same package is always accessible */
	if (SAME_PACKAGE(referer,cls))
		return true;

	/* a non-public class in another package is not accessible */
	return false;
}

/* for documentation see access.h */
bool
is_accessible_member(classinfo *referer,classinfo *declarer,s4 memberflags)
{
	ACCESS_ASSERT(referer);
	ACCESS_ASSERT(declarer);
	
	/* public members are accessible */
	if ((memberflags & ACC_PUBLIC) != 0)
		return true;

	/* {declarer is not an interface} */

	/* private members are only accessible by the class itself */
	if ((memberflags & ACC_PRIVATE) != 0)
		return (referer == declarer);

	/* {the member is protected or package private} */

	/* protected and package private members are accessible in the same package */
	if (SAME_PACKAGE(referer,declarer))
		return true;

	/* package private members are not accessible outside the package */
	if ((memberflags & ACC_PROTECTED) == 0)
		return false;

	/* {the member is protected and declarer is in another package} */

	/* a necessary condition for access is that referer is a subclass of declarer */
	ACCESS_ASSERT(referer->linked && declarer->linked);
	if (builtin_isanysubclass(referer,declarer))
		return true;

	return false;
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

