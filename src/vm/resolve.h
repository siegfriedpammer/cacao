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

   $Id: resolve.h 2181 2005-04-01 16:53:33Z edwin $

*/


#ifndef _RESOLVE_H
#define _RESOLVE_H

/* forward declarations *******************************************************/

typedef struct unresolved_field unresolved_field;
typedef struct unresolved_method unresolved_method;
typedef struct unresolved_subtype_set unresolved_subtype_set;


#include "vm/global.h"
#include "vm/references.h"
#include "vm/jit/jit.h"


/* constants ******************************************************************/

#define RESOLVE_STATIC    0x0001
#define RESOLVE_PUTFIELD  0x0002


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

/* resolve_class ***************************************************************
 
   Resolve a symbolic class reference
  
   IN:
       referer..........the class containing the reference
       refmethod........the method from which resolution was triggered
                        (may be NULL if not applicable)
       classname........class name to resolve
       mode.............mode of resolution:
                            resolveLazy...only resolve if it does not
                                          require loading classes
                            resolveEager..load classes if necessary
  
   OUT:
       *result..........set to result of resolution, or to NULL if
                        the reference has not been resolved
                        In the case of an exception, *result is
                        guaranteed to be set to NULL.
  
   RETURN VALUE:
       true.............everything ok 
                        (*result may still be NULL for resolveLazy)
       false............an exception has been thrown

   NOTE:
       The returned class is *not* guaranteed to be linked!
	   (It is guaranteed to be loaded, though.)
   
*******************************************************************************/

bool
resolve_class(classinfo *referer,methodinfo *refmethod,
			  utf *classname,
			  resolve_mode_t mode,
			  classinfo **result);

/* resolve_classref_or_classinfo ***********************************************
 
   Resolve a symbolic class reference if necessary
  
   IN:
       refmethod........the method from which resolution was triggered
                        (may be NULL if not applicable)
       cls..............class reference or classinfo
       mode.............mode of resolution:
                            resolveLazy...only resolve if it does not
                                          require loading classes
                            resolveEager..load classes if necessary
	   link.............if true, guarantee that the returned class, if any,
	                    has been linked
  
   OUT:
       *result..........set to result of resolution, or to NULL if
                        the reference has not been resolved
                        In the case of an exception, *result is
                        guaranteed to be set to NULL.
  
   RETURN VALUE:
       true.............everything ok 
                        (*result may still be NULL for resolveLazy)
       false............an exception has been thrown
   
*******************************************************************************/

bool
resolve_classref_or_classinfo(methodinfo *refmethod,
							  classref_or_classinfo cls,
							  resolve_mode_t mode,
							  bool link,
							  classinfo **result);

/* resolve_field ***************************************************************
 
   Resolve an unresolved field reference
  
   IN:
       ref..............struct containing the reference
       mode.............mode of resolution:
                            resolveLazy...only resolve if it does not
                                          require loading classes
                            resolveEager..load classes if necessary
  
   OUT:
       *result..........set to the result of resolution, or to NULL if
                        the reference has not been resolved
                        In the case of an exception, *result is
                        guaranteed to be set to NULL.
  
   RETURN VALUE:
       true.............everything ok 
                        (*result may still be NULL for resolveLazy) XXX implement
       false............an exception has been thrown
   
*******************************************************************************/

bool
resolve_field(unresolved_field *ref,
			  resolve_mode_t mode,
			  fieldinfo **result);

/* resolve_method **************************************************************
 
   Resolve an unresolved method reference
  
   IN:
       ref..............struct containing the reference
       mode.............mode of resolution:
                            resolveLazy...only resolve if it does not
                                          require loading classes
                            resolveEager..load classes if necessary
  
   OUT:
       *result..........set to the result of resolution, or to NULL if
                        the reference has not been resolved
                        In the case of an exception, *result is
                        guaranteed to be set to NULL.
  
   RETURN VALUE:
       true.............everything ok 
                        (*result may still be NULL for resolveLazy) XXX implement
       false............an exception has been thrown
   
*******************************************************************************/

bool
resolve_method(unresolved_method *ref,
			  resolve_mode_t mode,
			   methodinfo **result);

/* resolve_and_check_subtype_set ***********************************************
 
   Resolve the references in the given set and test subtype relationships
  
   IN:
       referer..........the class containing the references
       refmethod........the method triggering the resolution
       ref..............a set of class/interface references
                        (may be empty)
       type.............the type to test against the set
       reversed.........if true, test if type is a subtype of
                        the set members, instead of the other
                        way round
       mode.............mode of resolution:
                            resolveLazy...only resolve if it does not
                                          require loading classes
                            resolveEager..load classes if necessary
       error............which type of exception to throw if
                        the test fails. May be:
                            resolveLinkageError, or
                            resolveIllegalAccessError

   OUT:
       *checked.........set to true if all checks were performed,
	                    otherwise set to false
	                    (This is guaranteed to be true if mode was
						resolveEager and no exception occured.)
						If checked == NULL, this parameter is not used.
  
   RETURN VALUE:
       true.............the check succeeded
       false............the check failed. An exception has been
                        thrown.
   
   NOTE:
       The references in the set are resolved first, so any
       exception which may occurr during resolution may
       be thrown by this function.
   
*******************************************************************************/

bool
resolve_and_check_subtype_set(classinfo *referer,methodinfo *refmethod,
							  unresolved_subtype_set *ref,
							  classref_or_classinfo type,
							  bool reversed,
							  resolve_mode_t mode,
							  resolve_err_t error,
							  bool *checked);

/* create_unresolved_field *****************************************************
 
   Create an unresolved_field struct for the given field access instruction
  
   IN:
       referer..........the class containing the reference
	   refmethod........the method triggering the resolution (if any)
	   iptr.............the {GET,PUT}{FIELD,STATIC}{,CONST} instruction
	   stack............the input stack of the instruction

   RETURN VALUE:
       a pointer to a new unresolved_field struct

*******************************************************************************/

unresolved_field *
create_unresolved_field(classinfo *referer,methodinfo *refmethod,
						instruction *iptr,
						stackelement *stack);

/* create_unresolved_method ****************************************************
 
   Create an unresolved_method struct for the given method invocation
  
   IN:
       referer..........the class containing the reference
	   refmethod........the method triggering the resolution (if any)
	   iptr.............the INVOKE* instruction
	   stack............the input stack of the instruction

   RETURN VALUE:
       a pointer to a new unresolved_method struct

*******************************************************************************/

unresolved_method *
create_unresolved_method(classinfo *referer,methodinfo *refmethod,
						 instruction *iptr,
						 stackelement *stack);

/* unresolved_field_free *******************************************************
 
   Free the memory used by an unresolved_field
  
   IN:
       ref..............the unresolved_field

*******************************************************************************/

void unresolved_field_free(unresolved_field *ref);

/* unresolved_method_free ******************************************************
 
   Free the memory used by an unresolved_method
  
   IN:
       ref..............the unresolved_method

*******************************************************************************/

void unresolved_method_free(unresolved_method *ref);

/* unresolved_field_debug_dump *************************************************
 
   Print debug info for unresolved_field to stream
  
   IN:
       ref..............the unresolved_field
	   file.............the stream

*******************************************************************************/

void unresolved_field_debug_dump(unresolved_field *ref,FILE *file);

/* unresolved_method_debug_dump ************************************************
 
   Print debug info for unresolved_method to stream
  
   IN:
       ref..............the unresolved_method
	   file.............the stream

*******************************************************************************/

void unresolved_method_debug_dump(unresolved_method *ref,FILE *file);

/* unresolved_subtype_set_debug_dump *******************************************
 
   Print debug info for unresolved_subtype_set to stream
  
   IN:
       stset............the unresolved_subtype_set
	   file.............the stream

*******************************************************************************/

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

