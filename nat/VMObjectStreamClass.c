/* nat/VMObjectStreamClass.c - java/io/ObjectStreamClass

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   Authors: Roman Obermaiser

   Changes: Joseph Wenninger

   $Id: VMObjectStreamClass.c 873 2004-01-11 20:59:29Z twisti $

*/


#include "jni.h"
#include "types.h"
#include "loader.h"
#include "toolbox/loging.h"
#include "java_lang_Class.h"


/*
 * Class:     java_io_VMObjectStreamClass
 * Method:    hasClassInitializer
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMObjectStreamClass_hasClassInitializer(JNIEnv *env, jclass clazz, java_lang_Class *par1)
{
	log_text("Java_java_io_VMOBjectStreamClass_hasClassInitializer");

	return (class_findmethodIndex((classinfo *) par1, clinit_name(), clinit_desc()) != -1);
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
 */
