/* nat/Constructor.c - java/lang/reflect/Constructor

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

   $Id: Constructor.c 1344 2004-07-21 17:12:53Z twisti $

*/


#include <string.h>
#include "jni.h"
#include "builtin.h"
#include "exceptions.h"
#include "loader.h"
#include "native.h"
#include "tables.h"
#include "asmpart.h"
#include "options.h"
#include "toolbox/logging.h"
#include "java_lang_Object.h"
#include "java_lang_Class.h"
#include "java_lang_reflect_Constructor.h"


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    newInstance
 * Signature: ([Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_Constructor_constructNative(JNIEnv *env, java_lang_reflect_Constructor *this, java_objectarray *parameters, java_lang_Class *clazz, s4 par3)
{

#if defined(__GNUC__)
#warning fix me for parameters float/double and long long  parameters
#endif

	methodinfo *m;
	java_objectheader *o;

	/* find initializer */

	if (!parameters) {
		if (this->parameterTypes->header.size != 0) {
			*exceptionptr =
				new_exception_message(string_java_lang_IllegalArgumentException,
									  "wrong number of arguments");
			return 0;
		}

	} else {
		if (this->parameterTypes->header.size != parameters->header.size) {
			*exceptionptr =
				new_exception_message(string_java_lang_IllegalArgumentException,
									  "wrong number of arguments");
			return 0;
		}
	}

	if (this->slot >= ((classinfo *) clazz)->methodscount) {
		log_text("illegal index in methods table");
		return 0;
	}

	o = builtin_new((classinfo *) clazz);         /*          create object */
	if (!o) {
		log_text("Objet instance could not be created");
		return NULL;
	}
        
	/*	log_text("o!=NULL\n");*/


	m = &((classinfo *)clazz)->methods[this->slot];
	if (!((m->name == utf_new_char("<init>"))))
		/* && 
		   (m->descriptor == create_methodsig(this->parameterTypes,"V"))))*/
		{
			if (verbose) {
				char logtext[MAXLOGTEXT];
				sprintf(logtext, "Warning: class has no instance-initializer of specified type: ");
				utf_sprint(logtext + strlen(logtext), ((classinfo *) clazz)->name);
				log_text(logtext);
				log_plain_utf( create_methodsig(this->parameterTypes,"V"));
				log_plain("\n");
				class_showconstantpool((classinfo *) clazz);
			}
#if defined(__GNUC__)
#warning throw an exception here, although this should never happen
#endif
			return (java_lang_Object *) o;
		}

	/*	log_text("calling initializer");*/
	/* call initializer */
#if 0
	switch (this->parameterTypes->header.size) {
	case 0: exceptionptr=asm_calljavamethod (m, o, NULL, NULL, NULL);
		break;
	case 1: exceptionptr=asm_calljavamethod (m, o, parameters->data[0], NULL, NULL);
		break;
	case 2: exceptionptr=asm_calljavamethod (m, o, parameters->data[0], parameters->data[1], NULL);
		break;
	case 3: exceptionptr=asm_calljavamethod (m, o, parameters->data[0], parameters->data[1], 
											 parameters->data[2]);
	break;
	default:
		log_text("Not supported number of arguments in Java_java_lang_reflect_Constructor");
	}
#endif

	(void) jni_method_invokeNativeHelper(env, m ,o, parameters); 

	return (java_lang_Object *) o;
}


/*
 * Class:     java_lang_reflect_Constructor
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Constructor_getModifiers(JNIEnv *env, java_lang_reflect_Constructor *this)
{
	classinfo *c = (classinfo *) (this->clazz);

	if ((this->slot < 0) || (this->slot >= c->methodscount))
		panic("error illegal slot for constructor in class (getModifiers)");

	return (c->methods[this->slot]).flags & (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED);
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
