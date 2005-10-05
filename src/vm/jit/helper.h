/* src/vm/jit/helper.h - code patching helper functions

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

   Authors: Christian Thalinger

   Changes:

   $Id: helper.h 3353 2005-10-05 13:30:10Z edwin $

*/


#ifndef _HELPER_H
#define _HELPER_H

#include "vm/class.h"
#include "vm/linker.h"
#include "vm/method.h"
#include "vm/resolve.h"


/* function prototypes ********************************************************/

classinfo  *helper_resolve_classinfo(constant_classref *cr);
classinfo  *helper_resolve_classinfo_nonabstract(constant_classref *cr);
methodinfo *helper_resolve_methodinfo(unresolved_method *um);
fieldinfo  *helper_resolve_fieldinfo(unresolved_field *uf);
java_objectheader *helper_fillin_stacktrace(java_objectheader*);
java_objectheader *helper_fillin_stacktrace_always(java_objectheader*);

#endif /* _HELPER_H */


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
