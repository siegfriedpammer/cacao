/* src/vm/linker.hpp - class linker header

   Copyright (C) 1996-2013
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


#ifndef LINKER_HPP_
#define LINKER_HPP_ 1

#include "config.h"
#include "vm/types.hpp"

#include "threads/mutex.hpp"

#include "vm/class.hpp"
#include "vm/references.h"
#include "vm/vftbl.hpp"

#ifdef __cplusplus

/* arraydescriptor *************************************************************

   For every array class an arraydescriptor is allocated which
   describes the array class. The arraydescriptor is referenced from
   the vftbl of the array class.

*******************************************************************************/

struct arraydescriptor {
	vftbl_t *componentvftbl; /* vftbl of the component type, NULL for primit. */
	vftbl_t *elementvftbl;   /* vftbl of the element type, NULL for primitive */
	s2       arraytype;      /* ARRAYTYPE_* constant                          */
	s2       dimension;      /* dimension of the array (always >= 1)          */
	s4       dataoffset;     /* offset of the array data from object pointer  */
	s4       componentsize;  /* size of a component in bytes                  */
	s2       elementtype;    /* ARRAYTYPE_* constant                          */
};


/* global variables ***********************************************************/

/* This lock must be taken while renumbering classes or while atomically      */
/* accessing classes.                                                         */

#if USES_NEW_SUBTYPE

#define LOCK_CLASSRENUMBER_LOCK   /* nothing */
#define UNLOCK_CLASSRENUMBER_LOCK /* nothing */

#else
extern Mutex *linker_classrenumber_lock;

#define LOCK_CLASSRENUMBER_LOCK   linker_classrenumber_lock->lock()
#define UNLOCK_CLASSRENUMBER_LOCK linker_classrenumber_lock->unlock()

#endif


/* function prototypes ********************************************************/

void       linker_preinit(void);
void       linker_init(void);

void linker_create_string_later(java_object_t **a, Utf8String u);
void linker_initialize_deferred_strings();

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

// TODO: remove 'extern "C"', this is used in an inline function in 
//       class.hpp (class_is_array) and thus visible to all machine code emitters
classinfo *link_class(classinfo *c);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // LINKER_HPP_


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
