/* src/vm/jit/asmhelper.h - code patching helper functions

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

   $Id: helper.h 2259 2005-04-11 09:45:52Z twisti $

*/


#ifndef _ASMHELPER_H
#define _ASMHELPER_H

#include "vm/class.h"
#include "vm/linker.h"
#include "vm/method.h"
#include "vm/resolve.h"


/* function prototypes ********************************************************/

/* code patching helper functions */

classinfo *helper_resolve_classinfo(constant_classref *cr);
s4         helper_resolve_classinfo_flags(constant_classref *cr);
vftbl_t   *helper_resolve_classinfo_vftbl(constant_classref *cr);
s4         helper_resolve_classinfo_index(constant_classref *cr);

methodinfo *helper_resolve_methodinfo(unresolved_method *um);
s4          helper_resolve_methodinfo_vftblindex(unresolved_method *um);
u1         *helper_resolve_methodinfo_stubroutine(unresolved_method *um);

void *helper_get_fieldinfo_value_address(unresolved_field *uf);
s4    helper_get_fieldinfo_offset(unresolved_field *uf);

#endif /* _ASMHELPER_H */


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
