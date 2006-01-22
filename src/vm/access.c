/* vm/access.c - checking access rights

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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

   Authors: Edwin Steiner

   Changes:

   $Id: access.c 4357 2006-01-22 23:33:38Z twisti $

*/

#include <assert.h>

#include "config.h"
#include "vm/types.h"

#include "vm/access.h"
#include "vm/builtin.h"
#include "vm/class.h"


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

/* access_is_accessible_class **************************************************
 
   Check if a class is accessible from another class
  
   IN:
       referer..........the class containing the reference
       cls..............the result of resolving the reference
  
   RETURN VALUE:
       true.............access permitted
       false............access denied
   
   NOTE:
       This function performs the checks listed in section 5.4.4.
	   "Access Control" of "The Java(TM) Virtual Machine Specification,
	   Second Edition".

*******************************************************************************/

bool access_is_accessible_class(classinfo *referer, classinfo *cls)
{
	ACCESS_ASSERT(referer);
	ACCESS_ASSERT(cls);

	/* public classes are always accessible */
	if (cls->flags & ACC_PUBLIC)
		return true;

	/* a class in the same package is always accessible */
	if (SAME_PACKAGE(referer, cls))
		return true;

	/* a non-public class in another package is not accessible */
	return false;
}




/* access_is_accessible_member *************************************************
 
   Check if a field or method is accessible from a given class
  
   IN:
       referer..........the class containing the reference
       declarer.........the class declaring the member
       memberflags......the access flags of the member
  
   RETURN VALUE:
       true.............access permitted
       false............access denied

   NOTE:
       This function only performs the checks listed in section 5.4.4.
	   "Access Control" of "The Java(TM) Virtual Machine Specification,
	   Second Edition".

	   In particular a special condition for protected access with is
	   part of the verification process according to the spec is not
	   checked in this function.
   
*******************************************************************************/

bool access_is_accessible_member(classinfo *referer, classinfo *declarer,
								 s4 memberflags)
{
	ACCESS_ASSERT(referer);
	ACCESS_ASSERT(declarer);
	
	/* public members are accessible */
	if (memberflags & ACC_PUBLIC)
		return true;

	/* {declarer is not an interface} */

	/* private members are only accessible by the class itself */
	if (memberflags & ACC_PRIVATE)
		return (referer == declarer);

	/* {the member is protected or package private} */

	/* protected and package private members are accessible in the same package */
	if (SAME_PACKAGE(referer,declarer))
		return true;

	/* package private members are not accessible outside the package */
	if (!(memberflags & ACC_PROTECTED))
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

