/* src/native/jni.hpp - JNI types and data structures

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


/* jni.h ***********************************************************************

   ATTENTION: We include this file before we actually define our own
   jni.h.  We do this because otherwise we can get into unresolvable
   circular header dependencies.

   GNU Classpath's headers define:

   #define __CLASSPATH_JNI_MD_H__
   #define _CLASSPATH_JNI_H

   and jni.h uses:

   _CLASSPATH_VM_JNI_TYPES_DEFINED
   
   OpenJDK's headers define:

   #define _JAVASOFT_JNI_MD_H_
   #define _JAVASOFT_JNI_H_

   and jni.h uses:

   JNI_TYPES_ALREADY_DEFINED_IN_JNI_MD_H

   CLASSPATH_JNI_MD_H and CLASSPATH_JNI_H are defined in config.h.

*******************************************************************************/

#include "config.h"

/* We include both headers with the absolute path so we can be sure
   that the preprocessor does not take another header.  Furthermore we
   include jni_md.h before jni.h as the latter includes the former. */

#include INCLUDE_JNI_MD_H
#include INCLUDE_JNI_H

#ifndef _JNI_HPP
#define _JNI_HPP

#include <stdbool.h>
#include <stdint.h>

#include "vm/global.h"


#ifdef __cplusplus
extern "C" {
#endif

/* CACAO related stuff ********************************************************/

extern const struct JNIInvokeInterface_ _Jv_JNIInvokeInterface;
extern struct JNINativeInterface_ _Jv_JNINativeInterface;


/* hashtable_global_ref_entry *************************************************/

typedef struct hashtable_global_ref_entry hashtable_global_ref_entry;

struct hashtable_global_ref_entry {
	java_object_t              *o;      /* object pointer of global ref       */
	int32_t                     refs;   /* references of the current pointer  */
	hashtable_global_ref_entry *hashlink; /* link for external chaining       */
};


/* function prototypes ********************************************************/

bool jni_init(void);
bool jni_version_check(int version);

#ifdef __cplusplus
}
#endif

#endif // _JNI_HPP


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
