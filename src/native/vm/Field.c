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

   $Id: Field.c 2493 2005-05-21 14:59:14Z twisti $

*/


#include <assert.h>

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
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/resolve.h"
#include "vm/jit/stacktrace.h"

#undef DEBUG

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
	/*fid = class_findfield_approx((classinfo *) this->declaringClass,javastring_toutf(this->name, false));*/
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
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getBoolean(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj)
{
	jfieldID fid;
	char *utf_ptr;
	classinfo *c = (classinfo *) this->declaringClass;

#ifdef DEBUG
	/* check if the specified slot could be a valid field of this class*/
	if (this->slot>((classinfo*)this->declaringClass)->fieldscount) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: fieldscount mismatch");
#endif
	fid=&((classinfo*)this->declaringClass)->fields[this->slot]; /*get field*/
	CHECKFIELDACCESS(this,fid,c,return 0);
#ifdef DEBUG
	/* check if the field really has the same name and check if the type descriptor is not empty*/
	if (fid->name!=javastring_toutf(this->name,false)) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: field name mismatch");
	if (fid->descriptor->blength<1) {
		log_text("Type-Descriptor is empty");
		assert(0);
	}
#endif
	/* check if obj would be needed (not static field), but is 0)*/
	if ((!(fid->flags & ACC_STATIC)) && (obj==0)) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return 0;
	}
	
	/*if (!(fid->flags & ACC_PUBLIC)) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;

	} */

	if (fid->flags & ACC_STATIC) {
		/* initialize class if needed*/
		fprintf(stderr, "calling initialize_class %s\n",((classinfo*) this->declaringClass)->name->text);
		initialize_class((classinfo *) this->declaringClass);
		if (*exceptionptr) return 0;
		/*return value*/
                utf_ptr = fid->descriptor->text;
                switch (*utf_ptr) {
			case 'Z': return fid->value.i;
                        default:
                                *exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
                                return 0;
                }
	} else {
		if (!builtin_instanceof((java_objectheader*)obj,(classinfo*)this->declaringClass)) {
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
		utf_ptr = fid->descriptor->text;
		switch (*utf_ptr) {
			case 'Z':return getField(obj,jboolean,fid);
			default:
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getByte
 * Signature: (Ljava/lang/Object;)B
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getByte(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj)
{
	jfieldID fid;
	char *utf_ptr;
	classinfo *c = (classinfo *) this->declaringClass;

#ifdef DEBUG
	/* check if the specified slot could be a valid field of this class*/
	if (this->slot>((classinfo*)this->declaringClass)->fieldscount) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: fieldscount mismatch");
#endif
	fid=&((classinfo*)this->declaringClass)->fields[this->slot]; /*get field*/

	CHECKFIELDACCESS(this,fid,c,return 0);
#ifdef DEBUG
	/* check if the field really has the same name and check if the type descriptor is not empty*/
	if (fid->name!=javastring_toutf(this->name,false)) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: field name mismatch");
	if (fid->descriptor->blength<1) {
		log_text("Type-Descriptor is empty");
		assert(0);
	}
#endif
	/* check if obj would be needed (not static field), but is 0)*/
	if ((!(fid->flags & ACC_STATIC)) && (obj==0)) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return 0;
	}
	
	/* if (!(fid->flags & ACC_PUBLIC)) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;

	}*/

	if (fid->flags & ACC_STATIC) {
		/* initialize class if needed*/
		fprintf(stderr, "calling initialize_class %s\n",((classinfo*) this->declaringClass)->name->text);
		initialize_class((classinfo *) this->declaringClass);
		if (*exceptionptr) return 0;
		/*return value*/
                utf_ptr = fid->descriptor->text;
                switch (*utf_ptr) {
                        case 'B': return fid->value.i;
                        default:
                                *exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
                                return 0;
                }
	} else {
		if (!builtin_instanceof((java_objectheader*)obj,(classinfo*)this->declaringClass)) {
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
		utf_ptr = fid->descriptor->text;
		switch (*utf_ptr) {
			case 'B':return getField(obj,jbyte,fid);
			default:
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getChar
 * Signature: (Ljava/lang/Object;)C
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getChar(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj)
{
	jfieldID fid;
	char *utf_ptr;
	classinfo *c = (classinfo *) this->declaringClass;

#ifdef DEBUG
	/* check if the specified slot could be a valid field of this class*/
	if (this->slot>((classinfo*)this->declaringClass)->fieldscount) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: fieldscount mismatch");
#endif
	fid=&((classinfo*)this->declaringClass)->fields[this->slot]; /*get field*/
	CHECKFIELDACCESS(this,fid,c,return 0);

#ifdef DEBUG
	/* check if the field really has the same name and check if the type descriptor is not empty*/
	if (fid->name!=javastring_toutf(this->name,false)) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: field name mismatch");
	if (fid->descriptor->blength<1) {
		log_text("Type-Descriptor is empty");
		assert(0);
	}
#endif
	/* check if obj would be needed (not static field), but is 0)*/
	if ((!(fid->flags & ACC_STATIC)) && (obj==0)) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return 0;
	}
	
	/*if (!(fid->flags & ACC_PUBLIC)) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;

	}*/

	if (fid->flags & ACC_STATIC) {
		/* initialize class if needed*/
		fprintf(stderr, "calling initialize_class %s\n",((classinfo*) this->declaringClass)->name->text);
		initialize_class((classinfo *) this->declaringClass);
		if (*exceptionptr) return 0;
		/*return value*/
                utf_ptr = fid->descriptor->text;
                switch (*utf_ptr) { 
                        case 'C': return fid->value.i;
                        default:
                                *exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
                                return 0;
                }
	} else {
		if (!builtin_instanceof((java_objectheader*)obj,(classinfo*)this->declaringClass)) {
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
		utf_ptr = fid->descriptor->text;
		switch (*utf_ptr) {
			case 'C':return getField(obj,jchar,fid);
			default:
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getDouble
 * Signature: (Ljava/lang/Object;)D
 */
JNIEXPORT double JNICALL Java_java_lang_reflect_Field_getDouble(JNIEnv *env , java_lang_reflect_Field *this, java_lang_Object *obj)
{
	jfieldID fid;
	char *utf_ptr;
	classinfo *c = (classinfo *) this->declaringClass;

#ifdef DEBUG
	/* check if the specified slot could be a valid field of this class*/
	if (this->slot>((classinfo*)this->declaringClass)->fieldscount) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: fieldscount mismatch");
#endif
	fid=&((classinfo*)this->declaringClass)->fields[this->slot]; /*get field*/
	CHECKFIELDACCESS(this,fid,c,return 0);

#ifdef DEBUG
	/* check if the field really has the same name and check if the type descriptor is not empty*/
	if (fid->name!=javastring_toutf(this->name,false)) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: field name mismatch");
	if (fid->descriptor->blength<1) {
		log_text("Type-Descriptor is empty");
		assert(0);
	}
#endif
	/* check if obj would be needed (not static field), but is 0)*/
	if ((!(fid->flags & ACC_STATIC)) && (obj==0)) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return 0;
	}
	
	/*if (!(fid->flags & ACC_PUBLIC)) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;

	}*/

	if (fid->flags & ACC_STATIC) {
		/* initialize class if needed*/
		fprintf(stderr, "calling initialize_class %s\n",((classinfo*) this->declaringClass)->name->text);
		initialize_class((classinfo *) this->declaringClass);
		if (*exceptionptr) return 0;
		/*return value*/
                utf_ptr = fid->descriptor->text;
                switch (*utf_ptr) {
                        case 'B':
			case 'S': 
                        case 'C': 
			case 'I': return fid->value.i;
			case 'F': return fid->value.f;
			case 'D': return fid->value.d;
                        default:
                                *exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
                                return 0;
                }
	} else {
		if (!builtin_instanceof((java_objectheader*)obj,(classinfo*)this->declaringClass)) {
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
		utf_ptr = fid->descriptor->text;
		switch (*utf_ptr) {
			case 'B':return getField(obj,jbyte,fid);
			case 'S':return getField(obj,jshort,fid);
			case 'C':return getField(obj,jchar,fid);
			case 'I':return getField(obj,jint,fid);
			case 'D':return getField(obj,jdouble,fid);
			default:
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getFloat
 * Signature: (Ljava/lang/Object;)F
 */
JNIEXPORT float JNICALL Java_java_lang_reflect_Field_getFloat(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj)
{
	jfieldID fid;
	char *utf_ptr;
	classinfo *c = (classinfo *) this->declaringClass;

#ifdef DEBUG
	/* check if the specified slot could be a valid field of this class*/
	if (this->slot>((classinfo*)this->declaringClass)->fieldscount) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: fieldscount mismatch");
#endif
	fid=&((classinfo*)this->declaringClass)->fields[this->slot]; /*get field*/
	CHECKFIELDACCESS(this,fid,c,return 0);

#ifdef DEBUG
	/* check if the field really has the same name and check if the type descriptor is not empty*/
	if (fid->name!=javastring_toutf(this->name,false)) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: field name mismatch");
	if (fid->descriptor->blength<1) {
		log_text("Type-Descriptor is empty");
		assert(0);
	}
#endif
	/* check if obj would be needed (not static field), but is 0)*/
	if ((!(fid->flags & ACC_STATIC)) && (obj==0)) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return 0;
	}
	
	/*if (!(fid->flags & ACC_PUBLIC)) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;

	}*/

	if (fid->flags & ACC_STATIC) {
		/* initialize class if needed*/
		fprintf(stderr, "calling initialize_class %s\n",((classinfo*) this->declaringClass)->name->text);
		initialize_class((classinfo *) this->declaringClass);
		if (*exceptionptr) return 0;
		/*return value*/
                utf_ptr = fid->descriptor->text;
                switch (*utf_ptr) {
                        case 'B':
			case 'S': 
                        case 'C': 
			case 'I': return fid->value.i;
			case 'F': return fid->value.f;
                        default:
                                *exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
                                return 0;
                }
	} else {
		if (!builtin_instanceof((java_objectheader*)obj,(classinfo*)this->declaringClass)) {
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
		utf_ptr = fid->descriptor->text;
		switch (*utf_ptr) {
			case 'B':return getField(obj,jbyte,fid);
			case 'S':return getField(obj,jshort,fid);
			case 'C':return getField(obj,jchar,fid);
			case 'I':return getField(obj,jint,fid);
			case 'F':return getField(obj,jfloat,fid);
			default:
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getInt
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getInt(JNIEnv *env , java_lang_reflect_Field *this, java_lang_Object *obj)
{
	jfieldID fid;
	char *utf_ptr;
	classinfo *c = (classinfo *) this->declaringClass;

#ifdef DEBUG
	/* check if the specified slot could be a valid field of this class*/
	if (this->slot>((classinfo*)this->declaringClass)->fieldscount) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: fieldscount mismatch");
#endif
	fid=&((classinfo*)this->declaringClass)->fields[this->slot]; /*get field*/
	CHECKFIELDACCESS(this,fid,c,return 0);
#ifdef DEBUG
	/* check if the field really has the same name and check if the type descriptor is not empty*/
	if (fid->name!=javastring_toutf(this->name,false)) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: field name mismatch");
	if (fid->descriptor->blength<1) {
		log_text("Type-Descriptor is empty");
		assert(0);
	}
#endif
	/* check if obj would be needed (not static field), but is 0)*/
	if ((!(fid->flags & ACC_STATIC)) && (obj==0)) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return 0;
	}
	
	/*if (!(fid->flags & ACC_PUBLIC)) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;

	} */

	if (fid->flags & ACC_STATIC) {
		/* initialize class if needed*/
		fprintf(stderr, "calling initialize_class %s\n",((classinfo*) this->declaringClass)->name->text);
		initialize_class((classinfo *) this->declaringClass);
		if (*exceptionptr) return 0;
		/*return value*/
                utf_ptr = fid->descriptor->text;
                switch (*utf_ptr) {
                        case 'B':
			case 'S': 
                        case 'C': 
			case 'I': return fid->value.i;
                        default:
                                *exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
                                return 0;
                }
	} else {
		if (!builtin_instanceof((java_objectheader*)obj,(classinfo*)this->declaringClass)) {
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
		utf_ptr = fid->descriptor->text;
		switch (*utf_ptr) {
			case 'B':return getField(obj,jbyte,fid);
			case 'S':return getField(obj,jshort,fid);
			case 'C':return getField(obj,jchar,fid);
			case 'I':return getField(obj,jint,fid);
			default:
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getLong
 * Signature: (Ljava/lang/Object;)J
 */
JNIEXPORT s8 JNICALL Java_java_lang_reflect_Field_getLong(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj)
{
	jfieldID fid;
	char *utf_ptr;
	classinfo *c = (classinfo *) this->declaringClass;

#ifdef DEBUG
	/* check if the specified slot could be a valid field of this class*/
	if (this->slot>((classinfo*)this->declaringClass)->fieldscount) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: fieldscount mismatch");
#endif
	fid=&((classinfo*)this->declaringClass)->fields[this->slot]; /*get field*/
	CHECKFIELDACCESS(this,fid,c,return 0);

#ifdef DEBUG
	/* check if the field really has the same name and check if the type descriptor is not empty*/
	if (fid->name!=javastring_toutf(this->name,false)) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: field name mismatch");
	if (fid->descriptor->blength<1) {
		log_text("Type-Descriptor is empty");
		assert(0);
	}
#endif
	/* check if obj would be needed (not static field), but is 0)*/
	if ((!(fid->flags & ACC_STATIC)) && (obj==0)) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return 0;
	}
	
	/*if (!(fid->flags & ACC_PUBLIC)) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;

	}*/

	if (fid->flags & ACC_STATIC) {
		/* initialize class if needed*/
		fprintf(stderr, "calling initialize_class %s\n",((classinfo*) this->declaringClass)->name->text);
		initialize_class((classinfo *) this->declaringClass);
		if (*exceptionptr) return 0;
		/*return value*/
                utf_ptr = fid->descriptor->text;
                switch (*utf_ptr) {
                        case 'B':
			case 'S': 
                        case 'C': 
			case 'I': return fid->value.i;
			case 'J': return fid->value.l;
                        default:
                                *exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
                                return 0;
                }
	} else {
		if (!builtin_instanceof((java_objectheader*)obj,(classinfo*)this->declaringClass)) {
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
		utf_ptr = fid->descriptor->text;
		switch (*utf_ptr) {
			case 'B':return getField(obj,jbyte,fid);
			case 'S':return getField(obj,jshort,fid);
			case 'C':return getField(obj,jchar,fid);
			case 'I':return getField(obj,jint,fid);
			case 'J':return getField(obj,jlong,fid);
			default:
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    getShort
 * Signature: (Ljava/lang/Object;)S
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getShort(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj)
{
	jfieldID fid;
	char *utf_ptr;
	classinfo *c = (classinfo *) this->declaringClass;

#ifdef DEBUG
	/* check if the specified slot could be a valid field of this class*/
	if (this->slot>((classinfo*)this->declaringClass)->fieldscount) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: fieldscount mismatch");
#endif
	fid=&((classinfo*)this->declaringClass)->fields[this->slot]; /*get field*/
	CHECKFIELDACCESS(this,fid,c,return 0);

#ifdef DEBUG
	/* check if the field really has the same name and check if the type descriptor is not empty*/
	if (fid->name!=javastring_toutf(this->name,false)) throw_cacao_exception_exit(string_java_lang_IncompatibleClassChangeError,
                                                                          "declaring class: field name mismatch");
	if (fid->descriptor->blength<1) {
		log_text("Type-Descriptor is empty");
		assert(0);
	}
#endif
	/* check if obj would be needed (not static field), but is 0)*/
	if ((!(fid->flags & ACC_STATIC)) && (obj==0)) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return 0;
	}
	
	/*if (!(fid->flags & ACC_PUBLIC)) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;

	}*/

	if (fid->flags & ACC_STATIC) {
		/* initialize class if needed*/
		fprintf(stderr, "calling initialize_class %s\n",((classinfo*) this->declaringClass)->name->text);
		initialize_class((classinfo *) this->declaringClass);
		if (*exceptionptr) return 0;
		/*return value*/
                utf_ptr = fid->descriptor->text;
                switch (*utf_ptr) {
                        case 'B':
			case 'S': return fid->value.i;
                        default:
                                *exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
                                return 0;
                }
	} else {
		if (!builtin_instanceof((java_objectheader*)obj,(classinfo*)this->declaringClass)) {
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
		utf_ptr = fid->descriptor->text;
		switch (*utf_ptr) {
			case 'B':return getField(obj,jbyte,fid);
			case 'S':return getField(obj,jshort,fid);
			default:
				*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
				return 0;
		}
	}
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    set
 * Signature: (Ljava/lang/Object;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_set(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj, java_lang_Object *val)
{
	jfieldID source_fid;  /* the field containing the value to be written */
	jfieldID fid;         /* the field to be written */
	classinfo *c;
	int st;

	fid = class_findfield_approx((classinfo *) this->declaringClass,
								 javastring_toutf(this->name, false));
	st = (fid->flags & ACC_STATIC); /* true if the field is static */
	CHECKFIELDACCESS(this,fid,((classinfo *) this->declaringClass),return);

	if (val && (st || obj)) {

		c = val->header.vftbl->class;

		/* The fieldid is used to set the new value, for primitive types the value
		   has to be retrieved from the wrapping object */  
		switch ((((classinfo *) this->declaringClass)->fields[this->slot]).descriptor->text[0]) {
		case 'I':
			/* illegal argument specified */
			if (c != class_java_lang_Integer)
				break;
			/* determine the field to read the value */
			source_fid = (*env)->GetFieldID(env, c, "value", "I");
			if (!source_fid)
				break;
				   
			/* set the new value */
			if (st)
		        /* static field */
				(*env)->SetStaticIntField(env, c, fid, GetIntField(env, (jobject) val, source_fid));
			else
				/* instance field */
				(*env)->SetIntField(env, (jobject) obj, fid, GetIntField(env, (jobject) val, source_fid));

			return;

		case 'J':
			if (c != class_java_lang_Long)
				break;
			source_fid = (*env)->GetFieldID(env, c, "value", "J");
			if (!source_fid)
				break;
				   
			if (st)
				(*env)->SetStaticLongField(env, c, fid, GetLongField(env, (jobject) val, source_fid));
			else
				(*env)->SetLongField(env, (jobject) obj, fid, GetLongField(env, (jobject) val, source_fid));

			return;

		case 'F':
			if (c != class_java_lang_Float)
				break;
			source_fid = (*env)->GetFieldID(env, c, "value", "F");
			if (!source_fid)
				break;
				   
			if (st)
				(*env)->SetStaticFloatField(env, c, fid, GetFloatField(env, (jobject) val, source_fid));
			else
				(*env)->SetFloatField(env, (jobject) obj, fid, GetFloatField(env, (jobject) val, source_fid));

			return;

		case 'D':
			if (c != class_java_lang_Double)
				break;
			source_fid = (*env)->GetFieldID(env, c, "value", "D");
			if (!source_fid) break;
				   
			if (st)
				(*env)->SetStaticDoubleField(env, c, fid, GetDoubleField(env,(jobject) val,source_fid));
			else
				(*env)->SetDoubleField(env, (jobject) obj, fid, GetDoubleField(env,(jobject) val,source_fid));

			return;

		case 'B':
			if (c != class_java_lang_Byte)
				break;
			source_fid = (*env)->GetFieldID(env, c, "value", "B");
			if (!source_fid)
				break;
				   
			if (st)
				(*env)->SetStaticByteField(env, c, fid, GetByteField(env, (jobject) val, source_fid));
			else
				(*env)->SetByteField(env, (jobject) obj, fid, GetByteField(env, (jobject) val, source_fid));

			return;

		case 'C':
			if (c != class_java_lang_Character)
				break;
			source_fid = (*env)->GetFieldID(env, c, "value", "C");
			if (!source_fid)
				break;
				   
			if (st)
				(*env)->SetStaticCharField(env, c, fid, GetCharField(env, (jobject) val, source_fid));
			else
				(*env)->SetCharField(env, (jobject) obj, fid, GetCharField(env, (jobject) val, source_fid));

			return;

		case 'S':
			if (c != class_java_lang_Short)
				break;
			source_fid = (*env)->GetFieldID(env, c, "value", "S");
			if (!source_fid)
				break;
				   
			if (st)
				(*env)->SetStaticShortField(env, c, fid, GetShortField(env, (jobject) val, source_fid));
			else
				(*env)->SetShortField(env, (jobject) obj, fid, GetShortField(env, (jobject) val, source_fid));

			return;

		case 'Z':
			if (c != class_java_lang_Boolean)
				break;
			source_fid = (*env)->GetFieldID(env, c, "value", "Z");
			if (!source_fid)
				break;
				   
			if (st)
				(*env)->SetStaticBooleanField(env, c, fid, GetBooleanField(env, (jobject) val, source_fid));
			else
				(*env)->SetBooleanField(env, (jobject) obj, fid, GetBooleanField(env, (jobject) val, source_fid));

			return;

		case '[':
		case 'L':
			if (st)
				(*env)->SetStaticObjectField(env, c, fid, (jobject) val);
			else
				(*env)->SetObjectField(env, (jobject) obj, fid, (jobject) val);

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
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj, s4 val)
{
	jfieldID fid;

	if (this->declaringClass && obj) {
		fid = class_findfield_approx((classinfo *) this->declaringClass,
									 javastring_toutf(this->name, false));

		if (fid) {
			(*env)->SetBooleanField(env, (jobject) obj, fid, val);
			return;
		}
	}

	/* raise exception */
	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setByte
 * Signature: (Ljava/lang/Object;B)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj, s4 val)
{
	jfieldID fid;

	if (this->declaringClass && obj) {
		fid = class_findfield_approx((classinfo *) this->declaringClass,
									 javastring_toutf(this->name, false));

		if (fid) {
			(*env)->SetByteField(env, (jobject) obj, fid, val);
			return;
		}
	}

	/* raise exception */
	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setChar
 * Signature: (Ljava/lang/Object;C)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj, s4 val)
{
	jfieldID fid;

	if (this->declaringClass && obj) {
		fid = class_findfield_approx((classinfo *) this->declaringClass,
									 javastring_toutf(this->name, false));

		if (fid) {
			(*env)->SetCharField(env, (jobject) obj, fid, val);
			return;
		}
	}

	/* raise exception */
	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setDouble
 * Signature: (Ljava/lang/Object;D)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj, double val)
{
	jfieldID fid;

	if (this->declaringClass && obj) {
		fid = class_findfield_approx((classinfo *) this->declaringClass,
									 javastring_toutf(this->name, false));

		if (fid) {
			(*env)->SetDoubleField(env, (jobject) obj, fid, val);
			return;
		} 
	}

	/* raise exception */
	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setFloat
 * Signature: (Ljava/lang/Object;F)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj, float val)
{
	jfieldID fid;

	if (this->declaringClass && obj) {
		fid = class_findfield_approx((classinfo *) this->declaringClass,
									 javastring_toutf(this->name, false));

		if (fid) {
			(*env)->SetFloatField(env, (jobject) obj, fid, val);
			return;
		}
	}

	/* raise exception */
	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setInt
 * Signature: (Ljava/lang/Object;I)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj, s4 val)
{
	jfieldID fid;

	if (this->declaringClass && obj) {
		fid = class_findfield_approx((classinfo *) this->declaringClass,
									 javastring_toutf(this->name, false));

		if (fid) {
			(*env)->SetIntField(env, (jobject) obj, fid, val);
			return;
		}
	}

	/* raise exception */
	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setLong
 * Signature: (Ljava/lang/Object;J)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj, s8 val)
{
	jfieldID fid;

	if (this->declaringClass && obj) {
		fid = class_findfield_approx((classinfo *) this->declaringClass,
									 javastring_toutf(this->name, false));

		if (fid) {
			(*env)->SetLongField(env, (jobject) obj, fid, val);
			return;
		}
	}

	/* raise exception */
	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
}


/*
 * Class:     java/lang/reflect/Field
 * Method:    setShort
 * Signature: (Ljava/lang/Object;S)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort(JNIEnv *env, java_lang_reflect_Field *this, java_lang_Object *obj, s4 val)
{
	jfieldID fid;

	if (this->declaringClass && obj) {
		fid = class_findfield_approx((classinfo *) this->declaringClass,
									 javastring_toutf(this->name, false));

		if (fid) {
			(*env)->SetShortField(env, (jobject) obj, fid, val);
			return;
		}
	}

	/* raise exception */
	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
}


/*
 * Class:     java_lang_reflect_Field
 * Method:    getType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_reflect_Field_getType(JNIEnv *env, java_lang_reflect_Field *this)
{
	typedesc *desc = (((classinfo *) this->declaringClass)->fields[this->slot]).parseddesc;
	java_lang_Class *ret;
	if (!desc)
		return NULL;

	if (!resolve_class_from_typedesc(desc,false,(classinfo **)&ret))
		return NULL; /* exception */
	
	use_class_as_object((classinfo*)ret);
	return ret;
}


/*
 * Class:     java_lang_reflect_Field
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getModifiers(JNIEnv *env, java_lang_reflect_Field *this)
{
	return (((classinfo *) this->declaringClass)->fields[this->slot]).flags;
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
