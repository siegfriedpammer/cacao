/* src/vm/access.h - checking access rights

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

   $Id: access.h 2181 2005-04-01 16:53:33Z edwin $

*/

#ifndef _ACCESS_H
#define _ACCESS_H

#include "vm/references.h"


/* macros *********************************************************************/

#define SAME_PACKAGE(a,b)                                  \
			((a)->classloader == (b)->classloader &&       \
			 (a)->packagename == (b)->packagename)

/* function prototypes ********************************************************/

/* is_accessible_class *********************************************************
 
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

bool
is_accessible_class(classinfo *referer,classinfo *cls);

/* is_accessible_member ********************************************************
 
   Check if a field or method is accessible from a given class
  
   IN:
       referer..........the class containing the reference
       cls..............the class declaring the member
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

bool
is_accessible_member(classinfo *referer,classinfo *declarer,s4 memberflags);

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

