/* src/native/vm/VMMethod.c - jdwp->jvmti interface

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

Authors: Samuel Vinson
         Martin Platter
         

Changes: 


$Id: $

*/

#include "toolbox/logging.h"
#include "native/jni.h"
#include "native/include/gnu_classpath_jdwp_VMMethod.h"
#include "vm/stringlocal.h"
#include "toolbox/logging.h"

/*
 * Class:     gnu/classpath/jdwp/VMMethod
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_gnu_classpath_jdwp_VMMethod_getName(JNIEnv *env, struct gnu_classpath_jdwp_VMMethod* this)
{
	classinfo  *c;
	methodinfo *m;

	c = (classinfo *)this->_class;
	m = &(c->methods[this->_methodId]);
	log_message_utf("Method_getName %s", m->name);
	return javastring_new(m->name);
}


/*
 * Class:     gnu/classpath/jdwp/VMMethod
 * Method:    getSignature
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_gnu_classpath_jdwp_VMMethod_getSignature(JNIEnv *env, struct gnu_classpath_jdwp_VMMethod* this)
{
	log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}


/*
 * Class:     gnu/classpath/jdwp/VMMethod
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_gnu_classpath_jdwp_VMMethod_getModifiers(JNIEnv *env, struct gnu_classpath_jdwp_VMMethod* this)
{
	classinfo  *c;
	methodinfo *m;

	c = (classinfo *) this->_class;
	m = &(c->methods[this->_methodId]);

	return m->flags;
}


/*
 * Class:     gnu/classpath/jdwp/VMMethod
 * Method:    getLineTable
 * Signature: ()Lgnu/classpath/jdwp/util/LineTable;
 */
JNIEXPORT struct gnu_classpath_jdwp_util_LineTable* JNICALL Java_gnu_classpath_jdwp_VMMethod_getLineTable(JNIEnv *env, struct gnu_classpath_jdwp_VMMethod* this)
{
	log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}


/*
 * Class:     gnu/classpath/jdwp/VMMethod
 * Method:    getVariableTable
 * Signature: ()Lgnu/classpath/jdwp/util/VariableTable;
 */
JNIEXPORT struct gnu_classpath_jdwp_util_VariableTable* JNICALL Java_gnu_classpath_jdwp_VMMethod_getVariableTable(JNIEnv *env, struct gnu_classpath_jdwp_VMMethod* this)
{
	log_text ("JVMTI-Call: IMPLEMENT ME!!!");
	return 0;
}
