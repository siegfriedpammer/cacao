/* native/jni.h - JNI types and data structures

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

   Authors: Reinhard Grafl
            Roman Obermaisser

   Changes: Christian Thalinger

   $Id: jni.h 4913 2006-05-14 14:02:51Z edwin $

*/


#ifndef _JNI_H
#define _JNI_H

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"
#include "vm/method.h"


/* Include the JNI header from GNU Classpath **********************************/

#include CLASSPATH_JNI_H


/* _Jv_JNIEnv *****************************************************************/

typedef struct _Jv_JNIEnv _Jv_JNIEnv;

struct _Jv_JNIEnv {
	const struct JNINativeInterface *env;     /* This MUST be the first entry */
};


/* _Jv_JavaVM *****************************************************************/

typedef struct _Jv_JavaVM _Jv_JavaVM;

struct _Jv_JavaVM {
	const struct JNIInvokeInterface *functions;/* This MUST be the first entry*/
};


/* CACAO related stuff ********************************************************/

extern const struct JNIInvokeInterface _Jv_JNIInvokeInterface;
extern struct JNINativeInterface _Jv_JNINativeInterface;


/* local reference table ******************************************************/

#define LOCALREFTABLE_CAPACITY    16

typedef struct localref_table localref_table;

/* localref_table **************************************************************

   ATTENTION: keep this structure a multiple of 8-bytes!!! This is
   essential for the native stub on 64-bit architectures.

*******************************************************************************/

struct localref_table {
	s4                 capacity;        /* table size                         */
	s4                 used;            /* currently used references          */
	s4                 localframes;     /* number of current frames           */
	s4                 PADDING;         /* 8-byte padding                     */
	localref_table    *prev;            /* link to prev table (LocalFrame)    */
	java_objectheader *refs[LOCALREFTABLE_CAPACITY]; /* references            */
};

#if defined(USE_THREADS)
#define LOCALREFTABLE    (THREADOBJECT->_localref_table)
#else
extern localref_table *_no_threads_localref_table;

#define LOCALREFTABLE    (_no_threads_localref_table)
#endif


/* hashtable_global_ref_entry *************************************************/

typedef struct hashtable_global_ref_entry hashtable_global_ref_entry;

struct hashtable_global_ref_entry {
	java_objectheader          *o;      /* object pointer of global ref       */
	s4                          refs;   /* references of the current pointer  */
	hashtable_global_ref_entry *hashlink; /* link for external chaining       */
};


/* function prototypes ********************************************************/

/* initialize JNI subsystem */
bool jni_init(void);

java_objectheader *_Jv_jni_invokeNative(methodinfo *m, java_objectheader *o,
										java_objectarray *params);

#endif /* _JNI_H */


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
