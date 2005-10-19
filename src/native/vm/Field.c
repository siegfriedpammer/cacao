/* src/native/vm/Field.c - java/lang/reflect/Field

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

   Authors: Roman Obermaiser

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: Field.c 3444 2005-10-19 11:27:42Z twisti $

*/


#include <assert.h>

#include "config.h"
#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_reflect_Field.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/loader.h"
#include "vm/resolve.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/utf8.h"
#include "vm/jit/stacktrace.h"


/* cacao_get_field_address *****************************************************

   XXX

*******************************************************************************/

static void *cacao_get_field_address(classinfo *c, fieldinfo *f,
									 java_lang_Object *o)
{
	CHECKFIELDACCESS(this,f,c,return 0);

	if (f->flags & ACC_STATIC) {
		/* initialize class if required */

		if (!initialize_class(c))
			return NULL;

		/* return value address */

		return &(f->value);

	} else {
		/* obj is required for not-static fields */

		if (o == NULL) {
			*exceptionptr = new_nullpointerexception();
			return NULL;
		}
	
		if (builtin_instanceof((java_objectheader *) o, c))
			return (void *) ((ptrint) o + f->offset);
	}

	/* exception path */

	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);

	return NULL;
}


/* XXX FIXE SET NATIVES */

#if (defined(__ALPHA__) || defined(__I386__))
/*this = java_lang_reflect_Field, fi=fieldinfo, c=declaredClass (classinfo)*/
#define CHECKFIELDACCESS(this,fi,c,doret)	\
	/*log_text("Checking access rights");*/	\
	if (!(getField(this,jboolean,getFieldID_critical(env,this->header.vftbl->class,"flag","Z")))) {	\
		int throwAccess=0;	\
		struct methodinfo *callingMethod;	\
	\
		if ((fi->flags & ACC_PUBLIC)==0) {	\
			callingMethod=cacao_callingMethod();	\
	\
			if ((fi->flags & ACC_PRIVATE)!=0) {	\
				if (c!=callingMethod->class) {	\
					throwAccess=1;	\
				}	\
			} else {	\
				if ((fi->flags & ACC_PROTECTED)!=0) {	\
					if (!builtin_isanysubclass(callingMethod->class, c)) {	\
						throwAccess=1;	\
					}	\
				} else {	\
					/* default visibility*/	\
					if (c->packagename!=callingMethod->class->packagename) {	\
						throwAccess=1;	\
					}	\
				}	\
			}	\
		}	\
		if (throwAccess) {	\
			*exceptionptr=0;	\
			*exceptionptr = new_exception(string_java_lang_IllegalAccessException);	\
			doret;	\
		}	\
	}	
#else
#define CHECKFIELDACCESS(this,fi,c,doret) \
    (c) = (c) /* prevent compiler warning */
#endif


/*
 * Class:     java/lang/reflect/Field
 * Method:    get
 * Signature: (Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_java_lang_reflect_Field_get(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj)
{
	jfieldID target_fid;   /* the JNI-fieldid of the wrapping object */
	jfieldID fid;          /* the JNI-fieldid of the field containing the value */
	jobject o;		 /* the object for wrapping the primitive type */
	classinfo *c = (classinfo *) this->declaringClass;
	int st;

	/* get the fieldid of the field represented by this Field-object */
	fid=&((classinfo*)this->declaringClass)->fields[this->slot]; /*get field*/
	/*fid = class_findfield_by_name((classinfo *) this->declaringClass,javastring_toutf(this->name, false));*/
	st = (fid->flags & ACC_STATIC); /* true if field is static */


	CHECKFIELDACCESS(this,fid,c,return 0);

	/* The fieldid is used to retrieve the value, for primitive types a new 
	   object for wrapping the primitive type is created.  */
	if (st || obj)
		switch ((((classinfo *) this->declaringClass)->fields[this->slot]).descriptor->text[0]) {
		case 'I':
			/* create wrapping class */
			c = class_java_lang_Integer;
			o = builtin_new(c);
			/* get fieldid to store the value */
			target_fid = (*env)->GetFieldID(env, c, "value", "I");
			if (!target_fid)
				break;
				   
			if (st)
		        /* static field */
				SetIntField(env,o,target_fid, (*env)->GetStaticIntField(env, c, fid));
			else
				/* instance field */
				SetIntField(env,o,target_fid, (*env)->GetIntField(env,(jobject) obj, fid));

			/* return the wrapped object */		   
			return (java_lang_Object *) o;

		case 'J':
			c = class_java_lang_Long;
			o = builtin_new(c);
			target_fid = (*env)->GetFieldID(env, c, "value", "J");
			if (!target_fid)
				break;
				   
			if (st)
				SetLongField(env,o,target_fid, (*env)->GetStaticLongField(env, c, fid));
			else
				SetLongField(env,o,target_fid, (*env)->GetLongField(env,(jobject)  obj, fid));

			return (java_lang_Object *) o;

		case 'F':
			c = class_java_lang_Float;
			o = builtin_new(c);
			target_fid = (*env)->GetFieldID(env, c, "value", "F");
			if (!target_fid)
				break;
				   
			if (st)
				SetFloatField(env,o,target_fid, (*env)->GetStaticFloatField(env, c, fid));
			else
				SetFloatField(env,o,target_fid, (*env)->GetFloatField(env, (jobject) obj, fid));

			return (java_lang_Object *) o;

		case 'D':
			c = class_java_lang_Double;
			o = builtin_new(c);
			target_fid = (*env)->GetFieldID(env, c, "value", "D");
			if (!target_fid)
				break;
				   
			if (st)
				SetDoubleField(env,o,target_fid, (*env)->GetStaticDoubleField(env, c, fid));
			else
				SetDoubleField(env,o,target_fid, (*env)->GetDoubleField(env, (jobject) obj, fid));

			return (java_lang_Object *) o;

		case 'B':
			c = class_java_lang_Byte;
			o = builtin_new(c);
			target_fid = (*env)->GetFieldID(env, c, "value", "B");
			if (!target_fid)
				break;
				   
			if (st)
				SetByteField(env,o,target_fid, (*env)->GetStaticByteField(env, c, fid));
			else
				SetByteField(env,o,target_fid, (*env)->GetByteField(env, (jobject) obj, fid));

			return (java_lang_Object *) o;

		case 'C':
			c = class_java_lang_Character;
			o = builtin_new(c);
			target_fid = (*env)->GetFieldID(env, c, "value", "C");
			if (!target_fid)
				break;
				   
			if (st)
				SetCharField(env,o,target_fid, (*env)->GetStaticCharField(env, c, fid));
			else
				SetCharField(env,o,target_fid, (*env)->GetCharField(env, (jobject) obj, fid));

			return (java_lang_Object *) o;

		case 'S':
			c = class_java_lang_Short;
			o = builtin_new(c);
			target_fid = (*env)->GetFieldID(env, c, "value", "S");
			if (!target_fid)
				break;
				   
			if (st)
				SetShortField(env,o,target_fid, (*env)->GetStaticShortField(env, c, fid));
			else
				SetShortField(env,o,target_fid, (*env)->GetShortField(env, (jobject) obj, fid));

			return (java_lang_Object *) o;

		case 'Z':
			c = class_java_lang_Boolean;
			o = builtin_new(c);
			target_fid = (*env)->GetFieldID(env, c, "value", "Z");
			if (!target_fid)
				break;
				   
			if (st)
				SetBooleanField(env,o,target_fid, (*env)->GetStaticBooleanField(env, c, fid));
			else
				SetBooleanField(env,o,target_fid, (*env)->GetBooleanField(env, (jobject) obj, fid));

			return (java_lang_Object *) o;

		case '[':
		case 'L':
			if (st)
		        /* static field */
				return (java_lang_Object*) (*env)->GetStaticObjectField(env, c, fid);
			else
				/* instance field */
				return (java_lang_Object*) (*env)->GetObjectField(env, (jobject) obj, fid);
		}

	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);

	return NULL;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getBoolean
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getBoolean(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	if (f->descriptor == utf_Z)
		return (s4) *((s4 *) addr);

	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
	return 0;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getByte
 * Signature: (Ljava/lang/Object;)B
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getByte(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	if (f->descriptor == utf_B)
		return (s4) *((s4 *) addr);

	/* incompatible field type */

	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);

	return 0;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getChar
 * Signature: (Ljava/lang/Object;)C
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getChar(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	if (f->descriptor == utf_C)
		return (s4) *((s4 *) addr);

	/* incompatible field type */

	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);

	return 0;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getShort
 * Signature: (Ljava/lang/Object;)S
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getShort(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->descriptor->text[0]) {
	case 'B':
	case 'S':
		return (s4) *((s4 *) addr);
	default:
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getInt
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getInt(JNIEnv *env , java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->descriptor->text[0]) {
	case 'B':
	case 'C':
	case 'S':
	case 'I':
		return (s4) *((s4 *) addr);
	default:
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getLong
 * Signature: (Ljava/lang/Object;)J
 */
JNIEXPORT s8 JNICALL Java_java_lang_reflect_Field_getLong(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->descriptor->text[0]) {
	case 'B':
	case 'C':
	case 'S':
	case 'I':
		return (s8) *((s4 *) addr);
	case 'J':
		return (s8) *((s8 *) addr);
	default:
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getFloat
 * Signature: (Ljava/lang/Object;)F
 */
JNIEXPORT float JNICALL Java_java_lang_reflect_Field_getFloat(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->descriptor->text[0]) {
	case 'B':
	case 'C':
	case 'S':
	case 'I':
		return (float) *((s4 *) addr);
	case 'J':
		return (float) *((s8 *) addr);
	case 'F':
		return (float) *((float *) addr);
	default:
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getDouble
 * Signature: (Ljava/lang/Object;)D
 */
JNIEXPORT double JNICALL Java_java_lang_reflect_Field_getDouble(JNIEnv *env , java_lang_reflect_Field *this, java_lang_Object *o)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return 0;

	/* check the field type and return the value */

	switch (f->descriptor->text[0]) {
	case 'B':
	case 'C':
	case 'S':
	case 'I':
		return (double) *((s4 *) addr);
	case 'J':
		return (double) *((s8 *) addr);
	case 'F':
		return (double) *((float *) addr);
	case 'D':
		return (double) *((double *) addr);
	default:
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    set
 * Signature: (Ljava/lang/Object;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_set(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, java_lang_Object *value)
{
	classinfo *sc;                      /* source classinfo                   */
	fieldinfo *sf;                      /* source fieldinfo                   */
	classinfo *dc;                      /* destination classinfo              */
	fieldinfo *df;                      /* destination fieldinfo              */

	/* get the destination fieldinfo from the destination class */

	dc = (classinfo *) this->declaringClass;

	df = class_findfield_by_name(dc, javastring_toutf(this->name, false));

	CHECKFIELDACCESS(this, fid, dc, return);

	/* for non-static fields check the object for NPE */

	if (!(df->flags & ACC_STATIC) && (o == NULL)) {
		*exceptionptr = new_nullpointerexception();
		return;
	}

	if (value) {
		/* get the source classinfo from the object */

		sc = value->header.vftbl->class;

		/* The fieldid is used to set the new value, for primitive
		   types the value has to be retrieved from the wrapping
		   object */

		switch (dc->fields[this->slot].descriptor->text[0]) {
		case 'Z':
			/* check the wrapping class */
			if (sc != class_java_lang_Boolean)
				break;

			/* determine the field to read the value */
			if (!(sf = class_findfield(sc, utf_value, utf_Z)))
				break;
				   
			/* set the new value */
			if (df->flags & ACC_STATIC)
		        /* static field */
				(*env)->SetStaticBooleanField(env, dc, df, GetBooleanField(env, (jobject) value, sf));
			else
				/* instance field */
				(*env)->SetBooleanField(env, (jobject) o, df, GetBooleanField(env, (jobject) value, sf));

			return;

		case 'B':
			if (sc != class_java_lang_Byte)
				break;

			if (!(sf = class_findfield(sc, utf_value, utf_B)))
				break;

			if (df->flags & ACC_STATIC)
				(*env)->SetStaticByteField(env, dc, df, GetByteField(env, (jobject) value, sf));
			else
				(*env)->SetByteField(env, (jobject) o, df, GetByteField(env, (jobject) value, sf));

			return;

		case 'C':
			if (sc != class_java_lang_Character)
				break;

			if (!(sf = class_findfield(sc, utf_value, utf_C)))
				break;
				   
			if (df->flags & ACC_STATIC)
				(*env)->SetStaticCharField(env, dc, df, GetCharField(env, (jobject) value, sf));
			else
				(*env)->SetCharField(env, (jobject) o, df, GetCharField(env, (jobject) value, sf));

			return;

		case 'S': {
			s4 val;

			if ((sc != class_java_lang_Byte) &&
				(sc != class_java_lang_Short))
				break;

			/* get field only by name, it can be one of B, S */
			if (!(sf = class_findfield_by_name(sc, utf_value)))
				break;
				   
			switch (sf->descriptor->text[0]) {
			case 'B':
				val = (s4) GetByteField(env, (jobject) value, sf);
				break;
			case 'S':
				val = (s4) GetShortField(env, (jobject) value, sf);
				break;
			}

			if (df->flags & ACC_STATIC)
				(*env)->SetStaticShortField(env, dc, df, val);
			else
				(*env)->SetShortField(env, (jobject) o, df, val);

			return;
		}

		case 'I': {
			s4 val;

			if ((sc != class_java_lang_Byte) &&
				(sc != class_java_lang_Short) &&
				(sc != class_java_lang_Character) &&
				(sc != class_java_lang_Integer))
				break;

			/* get field only by name, it can be one of B, S, C, I */
			if (!(sf = class_findfield_by_name(sc, utf_value)))
				break;

			switch (sf->descriptor->text[0]) {
			case 'B':
				val = (s4) GetByteField(env, (jobject) value, sf);
				break;
			case 'C':
				val = (s4) GetCharField(env, (jobject) value, sf);
				break;
			case 'S':
				val = (s4) GetShortField(env, (jobject) value, sf);
				break;
			case 'I':
				val = GetIntField(env, (jobject) value, sf);
				break;
			}

			if (df->flags & ACC_STATIC)
				(*env)->SetStaticIntField(env, dc, df, val);
			else
				(*env)->SetIntField(env, (jobject) o, df, val);

			return;
		}

		case 'J': {
			s8 val;

			if ((sc != class_java_lang_Byte) &&
				(sc != class_java_lang_Short) &&
				(sc != class_java_lang_Character) &&
				(sc != class_java_lang_Integer) &&
				(sc != class_java_lang_Long))
				break;

			/* get field only by name, it can be one of B, S, C, I, J */
			if (!(sf = class_findfield_by_name(sc, utf_value)))
				break;

			switch (sf->descriptor->text[0]) {
			case 'B':
				val = (s8) GetByteField(env, (jobject) value, sf);
				break;
			case 'C':
				val = (s8) GetCharField(env, (jobject) value, sf);
				break;
			case 'S':
				val = (s8) GetShortField(env, (jobject) value, sf);
				break;
			case 'I':
				val = (s8) GetIntField(env, (jobject) value, sf);
				break;
			case 'J':
				val = GetLongField(env, (jobject) value, sf);
				break;
			}

			if (df->flags & ACC_STATIC)
				(*env)->SetStaticLongField(env, dc, df, val);
			else
				(*env)->SetLongField(env, (jobject) o, df, val);

			return;
		}

		case 'F': {
			float val;

			if ((sc != class_java_lang_Byte) &&
				(sc != class_java_lang_Short) &&
				(sc != class_java_lang_Character) &&
				(sc != class_java_lang_Integer) &&
				(sc != class_java_lang_Long) &&
				(sc != class_java_lang_Float))
				break;

			/* get field only by name, it can be one of B, S, C, I, J, F */
			if (!(sf = class_findfield_by_name(sc, utf_value)))
				break;

			switch (sf->descriptor->text[0]) {
			case 'B':
				val = (float) GetByteField(env, (jobject) value, sf);
				break;
			case 'C':
				val = (float) GetCharField(env, (jobject) value, sf);
				break;
			case 'S':
				val = (float) GetShortField(env, (jobject) value, sf);
				break;
			case 'I':
				val = (float) GetIntField(env, (jobject) value, sf);
				break;
			case 'J':
				val = (float) GetLongField(env, (jobject) value, sf);
				break;
			case 'F':
				val = GetFloatField(env, (jobject) value, sf);
				break;
			}

			if (df->flags & ACC_STATIC)
				(*env)->SetStaticFloatField(env, dc, df, val);
			else
				(*env)->SetFloatField(env, (jobject) o, df, val);

			return;
		}

		case 'D': {
			double val;

			if ((sc != class_java_lang_Byte) &&
				(sc != class_java_lang_Short) &&
				(sc != class_java_lang_Character) &&
				(sc != class_java_lang_Integer) &&
				(sc != class_java_lang_Long) &&
				(sc != class_java_lang_Float) &&
				(sc != class_java_lang_Double))
				break;

			/* get field only by name, it can be one of B, S, C, I, J, F, D */
			if (!(sf = class_findfield_by_name(sc, utf_value)))
				break;

			switch (sf->descriptor->text[0]) {
			case 'B':
				val = (double) GetByteField(env, (jobject) value, sf);
				break;
			case 'C':
				val = (double) GetCharField(env, (jobject) value, sf);
				break;
			case 'S':
				val = (double) GetShortField(env, (jobject) value, sf);
				break;
			case 'I':
				val = (double) GetIntField(env, (jobject) value, sf);
				break;
			case 'J':
				val = (double) GetLongField(env, (jobject) value, sf);
				break;
			case 'F':
				val = (double) GetFloatField(env, (jobject) value, sf);
				break;
			case 'D':
				val = GetDoubleField(env, (jobject) value, sf);
				break;
			}

			if (df->flags & ACC_STATIC)
				(*env)->SetStaticDoubleField(env, dc, df, val);
			else
				(*env)->SetDoubleField(env, (jobject) o, df, val);

			return;
		}

		case '[':
		case 'L':
			/* check if value is an instance of the destination class */

			/* XXX TODO */
/*  			if (!builtin_instanceof((java_objectheader *) value, df->class)) */
/*  				break; */

			if (df->flags & ACC_STATIC)
				(*env)->SetStaticObjectField(env, dc, df, (jobject) value);
			else
				(*env)->SetObjectField(env, (jobject) o, df, (jobject) value);

			return;
		}
	}

	/* raise exception */

	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setBoolean
 * Signature: (Ljava/lang/Object;Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, s4 value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return;

	/* check the field type and set the value */

	if (f->descriptor == utf_Z)
		*((s4 *) addr) = value;
	else
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setByte
 * Signature: (Ljava/lang/Object;B)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, s4 value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->descriptor->text[0]) {
	case 'B':
	case 'S':
	case 'I':
		*((s4 *) addr) = value;
		break;
	case 'J':
		*((s8 *) addr) = value;
		break;
	case 'F':
		*((float *) addr) = value;
		break;
	case 'D':
		*((double *) addr) = value;
		break;
	default:
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setChar
 * Signature: (Ljava/lang/Object;C)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, s4 value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->descriptor->text[0]) {
	case 'C':
	case 'I':
		*((s4 *) addr) = value;
		break;
	case 'J':
		*((s8 *) addr) = value;
		break;
	case 'F':
		*((float *) addr) = value;
		break;
	case 'D':
		*((double *) addr) = value;
		break;
	default:
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setShort
 * Signature: (Ljava/lang/Object;S)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, s4 value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->descriptor->text[0]) {
	case 'S':
	case 'I':
		*((s4 *) addr) = value;
		break;
	case 'J':
		*((s8 *) addr) = value;
		break;
	case 'F':
		*((float *) addr) = value;
		break;
	case 'D':
		*((double *) addr) = value;
		break;
	default:
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setInt
 * Signature: (Ljava/lang/Object;I)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, s4 value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->descriptor->text[0]) {
	case 'I':
		*((s4 *) addr) = value;
		break;
	case 'J':
		*((s8 *) addr) = value;
		break;
	case 'F':
		*((float *) addr) = value;
		break;
	case 'D':
		*((double *) addr) = value;
		break;
	default:
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setLong
 * Signature: (Ljava/lang/Object;J)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, s8 value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->descriptor->text[0]) {
	case 'J':
		*((s8 *) addr) = value;
		break;
	case 'F':
		*((float *) addr) = value;
		break;
	case 'D':
		*((double *) addr) = value;
		break;
	default:
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setFloat
 * Signature: (Ljava/lang/Object;F)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, float value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return;

	/* check the field type and set the value */

	switch (f->descriptor->text[0]) {
	case 'F':
		*((float *) addr) = value;
		break;
	case 'D':
		*((double *) addr) = value;
		break;
	default:
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
	}

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setDouble
 * Signature: (Ljava/lang/Object;D)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *o, double value)
{
	classinfo *c;
	fieldinfo *f;
	void      *addr;

	/* get the class and the field */

	c = (classinfo *) this->declaringClass;
	f = &c->fields[this->slot];

	/* get the address of the field with an internal helper */

	if ((addr = cacao_get_field_address(c, f, o)) == NULL)
		return;

	/* check the field type and set the value */

	if (f->descriptor == utf_D)
		*((double *) addr) = value;
	else
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);

	return;
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_reflect_Field_getType(JNIEnv *env, java_lang_reflect_Field *this)
{
	classinfo *c;
	typedesc  *desc;
	classinfo *ret;

	c = (classinfo *) this->declaringClass;
	desc = c->fields[this->slot].parseddesc;

	if (!desc)
		return NULL;

	if (!resolve_class_from_typedesc(desc, true, false, &ret))
		return NULL;
	
	if (!use_class_as_object(ret))
		return NULL;

	return (java_lang_Class *) ret;
}


/*
 * Class:     java_lang_reflect_Field
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getModifiers(JNIEnv *env, java_lang_reflect_Field *this)
{
	classinfo *c;

	c = (classinfo *) this->declaringClass;

	return c->fields[this->slot].flags;
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
