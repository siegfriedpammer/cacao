/* vm/resolve.h - resolving classes/interfaces/fields/methods

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

   $Id: resolve.h 3460 2005-10-20 09:34:16Z edwin $

*/


#ifndef _RESOLVE_H
#define _RESOLVE_H

/* forward declarations *******************************************************/

typedef struct unresolved_class unresolved_class;
typedef struct unresolved_field unresolved_field;
typedef struct unresolved_method unresolved_method;
typedef struct unresolved_subtype_set unresolved_subtype_set;


#include "vm/global.h"
#include "vm/references.h"
#include "vm/jit/jit.h"


/* constants ******************************************************************/

#define RESOLVE_STATIC    0x0001  /* ref to static fields/methods             */
#define RESOLVE_PUTFIELD  0x0002  /* field ref inside a PUT{FIELD,STATIC}...  */
#define RESOLVE_SPECIAL   0x0004  /* method ref inside INVOKESPECIAL          */


/* enums **********************************************************************/

typedef enum {
	resolveLazy,
	resolveEager
} resolve_mode_t;


typedef enum {
	resolveLinkageError,
	resolveIllegalAccessError
} resolve_err_t;


/* structs ********************************************************************/

struct unresolved_subtype_set {
	classref_or_classinfo *subtyperefs;     /* NULL terminated list */
};

struct unresolved_class {
	constant_classref      *classref;
	methodinfo		       *referermethod;
	unresolved_subtype_set  subtypeconstraints;
};

/* XXX unify heads of unresolved_field and unresolved_method? */

struct unresolved_field {
	constant_FMIref *fieldref;
	methodinfo      *referermethod;
	s4               flags;
	
	unresolved_subtype_set  instancetypes;
	unresolved_subtype_set  valueconstraints;
};

struct unresolved_method {
	constant_FMIref *methodref;
	methodinfo      *referermethod;
	s4               flags;
	
	unresolved_subtype_set  instancetypes;
	unresolved_subtype_set *paramconstraints;
};

#define SUBTYPESET_IS_EMPTY(stset) \
	((stset).subtyperefs == NULL)

#define UNRESOLVED_SUBTYPE_SET_EMTPY(stset) \
	do { (stset).subtyperefs = NULL; } while(0)

/* function prototypes ********************************************************/

bool resolve_class_from_name(classinfo* referer,methodinfo *refmethod,
			  			utf *classname,
			  			resolve_mode_t mode,
						bool checkaccess,
						bool link,
			  			classinfo **result);

bool resolve_classref(methodinfo *refmethod,
				 constant_classref *ref,
				 resolve_mode_t mode,
				 bool checkaccess,
			     bool link,
				 classinfo **result);

bool resolve_classref_or_classinfo(methodinfo *refmethod,
							  classref_or_classinfo cls,
							  resolve_mode_t mode,
							  bool checkaccess,
							  bool link,
							  classinfo **result);

bool resolve_class_from_typedesc(typedesc *d,bool checkaccess,bool link,classinfo **result);

bool resolve_class(unresolved_class *ref,
			  resolve_mode_t mode,
			  bool checkaccess,
			  classinfo **result);

bool resolve_field(unresolved_field *ref,
			  resolve_mode_t mode,
			  fieldinfo **result);

bool resolve_method(unresolved_method *ref,
			  resolve_mode_t mode,
			   methodinfo **result);

classinfo * resolve_classref_eager(constant_classref *ref);
classinfo * resolve_classref_eager_nonabstract(constant_classref *ref);
classinfo * resolve_class_eager(unresolved_class *ref);
fieldinfo * resolve_field_eager(unresolved_field *ref);
methodinfo * resolve_method_eager(unresolved_method *ref);

bool resolve_and_check_subtype_set(classinfo *referer,methodinfo *refmethod,
							  unresolved_subtype_set *ref,
							  classref_or_classinfo type,
							  bool reversed,
							  resolve_mode_t mode,
							  resolve_err_t error,
							  bool *checked);

unresolved_class * create_unresolved_class(methodinfo *refmethod,
						constant_classref *classref,
						typeinfo *valuetype);

unresolved_field * create_unresolved_field(classinfo *referer,methodinfo *refmethod,
						instruction *iptr);

bool constrain_unresolved_field(unresolved_field *ref,
						   classinfo *referer,methodinfo *refmethod,
						   instruction *iptr,
						   stackelement *stack);

unresolved_method * create_unresolved_method(classinfo *referer,methodinfo *refmethod,
						 instruction *iptr);

bool constrain_unresolved_method(unresolved_method *ref,
						 	classinfo *referer,methodinfo *refmethod,
						 	instruction *iptr,
						 	stackelement *stack);

void unresolved_class_free(unresolved_class *ref);
void unresolved_field_free(unresolved_field *ref);
void unresolved_method_free(unresolved_method *ref);

void unresolved_class_debug_dump(unresolved_class *ref,FILE *file);
void unresolved_field_debug_dump(unresolved_field *ref,FILE *file);
void unresolved_method_debug_dump(unresolved_method *ref,FILE *file);
void unresolved_subtype_set_debug_dump(unresolved_subtype_set *stset,FILE *file);
	
#endif /* _RESOLVE_H */

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

