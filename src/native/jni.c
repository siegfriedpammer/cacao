/* src/native/jni.c - implementation of the Java Native Interface functions

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

   Authors: Rainhard Grafl
            Roman Obermaisser

   Changes: Joseph Wenninger
            Martin Platter
            Christian Thalinger

   $Id: jni.c 3065 2005-07-19 11:52:21Z twisti $

*/


#include <assert.h>
#include <string.h>

#include "config.h"
#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_Byte.h"
#include "native/include/java_lang_Character.h"
#include "native/include/java_lang_Short.h"
#include "native/include/java_lang_Integer.h"
#include "native/include/java_lang_Boolean.h"
#include "native/include/java_lang_Long.h"
#include "native/include/java_lang_Float.h"
#include "native/include/java_lang_Double.h"
#include "native/include/java_lang_Throwable.h"
#include "native/include/java_lang_reflect_Method.h"
#include "native/include/java_lang_reflect_Constructor.h"
#include "native/include/java_lang_reflect_Field.h"

#include "native/include/java_lang_Class.h" /* for java_lang_VMClass.h */
#include "native/include/java_lang_VMClass.h"
#include "native/include/java_lang_VMClassLoader.h"
#include "native/jvmti/jvmti.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
# endif
#endif

#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/resolve.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/statistics.h"


/* XXX TWISTI hack: define it extern so they can be found in this file */

extern const struct JNIInvokeInterface JNI_JavaVMTable;
extern struct JNINativeInterface JNI_JNIEnvTable;

/* pointers to VM and the environment needed by GetJavaVM and GetEnv */

static JavaVM ptr_jvm = (JavaVM) &JNI_JavaVMTable;
void* ptr_env = (void*) &JNI_JNIEnvTable;


#define PTR_TO_ITEM(ptr)   ((u8)(size_t)(ptr))

/* global reference table */
static jobject *global_ref_table;
static bool initrunning=false;

/* jmethodID and jclass caching variables for NewGlobalRef and DeleteGlobalRef*/
static jmethodID getmid = NULL;
static jmethodID putmid = NULL;
static jclass intclass = NULL;
static jmethodID intvalue = NULL;
static jmethodID newint = NULL;
static jclass ihmclass = NULL;
static jmethodID removemid = NULL;

#define JWCLINITDEBUG(x)


/********************* accessing instance-fields **********************************/

#define setField(obj,typ,var,val) *((typ*) ((long int) obj + (long int) var->offset))=val;  
#define getField(obj,typ,var)     *((typ*) ((long int) obj + (long int) var->offset))
#define setfield_critical(clazz,obj,name,sig,jdatatype,val) \
    setField(obj, jdatatype, getFieldID_critical(env,clazz,name,sig), val);


static void fill_callblock_from_vargs(void *obj, methoddesc *descr,
									  jni_callblock blk[], va_list data,
									  s4 rettype)
{
	typedesc *paramtypes;
	s4        i;

	paramtypes = descr->paramtypes;

	/* if method is non-static fill first block and skip `this' pointer */

	i = 0;

	if (obj) {
		/* the `this' pointer */
		blk[0].itemtype = TYPE_ADR;
		blk[0].item = PTR_TO_ITEM(obj);

		paramtypes++;
		i++;
	} 

	for (; i < descr->paramcount; i++, paramtypes++) {
		switch (paramtypes->decltype) {
		/* primitive types */
		case PRIMITIVETYPE_BYTE:
		case PRIMITIVETYPE_CHAR:
		case PRIMITIVETYPE_SHORT: 
		case PRIMITIVETYPE_BOOLEAN: 
			blk[i].itemtype = TYPE_INT;
			blk[i].item = (s8) va_arg(data, s4);
			break;

		case PRIMITIVETYPE_INT:
			blk[i].itemtype = TYPE_INT;
			blk[i].item = (s8) va_arg(data, s4);
			break;

		case PRIMITIVETYPE_LONG:
			blk[i].itemtype = TYPE_LNG;
			blk[i].item = (s8) va_arg(data, s8);
			break;

		case PRIMITIVETYPE_FLOAT:
			blk[i].itemtype = TYPE_FLT;
#if defined(__ALPHA__)
			/* this keeps the assembler function much simpler */

			*((jdouble *) (&blk[i].item)) = (jdouble) va_arg(data, jdouble);
#else
			*((jfloat *) (&blk[i].item)) = (jfloat) va_arg(data, jdouble);
#endif
			break;

		case PRIMITIVETYPE_DOUBLE:
			blk[i].itemtype = TYPE_DBL;
			*((jdouble *) (&blk[i].item)) = (jdouble) va_arg(data, jdouble);
			break;

		case TYPE_ADR: 
			blk[i].itemtype = TYPE_ADR;
			blk[i].item = PTR_TO_ITEM(va_arg(data, void*));
			break;
		}
	}

	/* The standard doesn't say anything about return value checking, but it  */
	/* appears to be useful.                                                  */

	if (rettype != descr->returntype.decltype)
		log_text("\n====\nWarning call*Method called for function with wrong return type\n====");
}


/* XXX it could be considered if we should do typechecking here in the future */

static bool fill_callblock_from_objectarray(void *obj, methoddesc *descr,
											jni_callblock blk[],
											java_objectarray *params)
{
    jobject    param;
	s4         paramcount;
	typedesc  *paramtypes;
	classinfo *c;
    s4         i;
	s4         j;

	paramcount = descr->paramcount;
	paramtypes = descr->paramtypes;

	/* if method is non-static fill first block and skip `this' pointer */

	i = 0;

	if (obj) {
		/* this pointer */
		blk[0].itemtype = TYPE_ADR;
		blk[0].item = PTR_TO_ITEM(obj);

		paramtypes++;
		paramcount--;
		i++;
	}

	for (j = 0; j < paramcount; i++, j++, paramtypes++) {
		switch (paramtypes->type) {
		/* primitive types */
		case TYPE_INT:
		case TYPE_LONG:
		case TYPE_FLOAT:
		case TYPE_DOUBLE:
			param = params->data[j];
			if (!param)
				goto illegal_arg;

			/* internally used data type */
			blk[i].itemtype = paramtypes->type;

			/* convert the value according to its declared type */

			c = param->vftbl->class;

			switch (paramtypes->decltype) {
			case PRIMITIVETYPE_BOOLEAN:
				if (c == primitivetype_table[paramtypes->decltype].class_wrap)
					blk[i].item = (s8) ((java_lang_Boolean *) param)->value;
				else
					goto illegal_arg;
				break;

			case PRIMITIVETYPE_BYTE:
				if (c == primitivetype_table[paramtypes->decltype].class_wrap)
					blk[i].item = (s8) ((java_lang_Byte *) param)->value;
				else
					goto illegal_arg;
				break;

			case PRIMITIVETYPE_CHAR:
				if (c == primitivetype_table[paramtypes->decltype].class_wrap)
					blk[i].item = (s8) ((java_lang_Character *) param)->value;
				else
					goto illegal_arg;
				break;

			case PRIMITIVETYPE_SHORT:
				if (c == primitivetype_table[paramtypes->decltype].class_wrap)
					blk[i].item = (s8) ((java_lang_Short *) param)->value;
				else if (c == primitivetype_table[PRIMITIVETYPE_BYTE].class_wrap)
					blk[i].item = (s8) ((java_lang_Byte *) param)->value;
				else
					goto illegal_arg;
				break;

			case PRIMITIVETYPE_INT:
				if (c == primitivetype_table[paramtypes->decltype].class_wrap)
					blk[i].item = (s8) ((java_lang_Integer *) param)->value;
				else if (c == primitivetype_table[PRIMITIVETYPE_SHORT].class_wrap)
					blk[i].item = (s8) ((java_lang_Short *) param)->value;
				else if (c == primitivetype_table[PRIMITIVETYPE_BYTE].class_wrap)
					blk[i].item = (s8) ((java_lang_Byte *) param)->value;
				else
					goto illegal_arg;
				break;

			case PRIMITIVETYPE_LONG:
				if (c == primitivetype_table[paramtypes->decltype].class_wrap)
					blk[i].item = (s8) ((java_lang_Long *) param)->value;
				else if (c == primitivetype_table[PRIMITIVETYPE_INT].class_wrap)
					blk[i].item = (s8) ((java_lang_Integer *) param)->value;
				else if (c == primitivetype_table[PRIMITIVETYPE_SHORT].class_wrap)
					blk[i].item = (s8) ((java_lang_Short *) param)->value;
				else if (c == primitivetype_table[PRIMITIVETYPE_BYTE].class_wrap)
					blk[i].item = (s8) ((java_lang_Byte *) param)->value;
				else
					goto illegal_arg;
				break;

			case PRIMITIVETYPE_FLOAT:
				if (c == primitivetype_table[paramtypes->decltype].class_wrap)
					*((jfloat *) (&blk[i].item)) = (jfloat) ((java_lang_Float *) param)->value;
				else
					goto illegal_arg;
				break;

			case PRIMITIVETYPE_DOUBLE:
				if (c == primitivetype_table[paramtypes->decltype].class_wrap)
					*((jdouble *) (&blk[i].item)) = (jdouble) ((java_lang_Float *) param)->value;
				else if (c == primitivetype_table[PRIMITIVETYPE_FLOAT].class_wrap)
					*((jfloat *) (&blk[i].item)) = (jfloat) ((java_lang_Float *) param)->value;
				else
					goto illegal_arg;
				break;

			default:
				goto illegal_arg;
			} /* end declared type switch */
			break;
		
			case TYPE_ADDRESS:
				if (!resolve_class_from_typedesc(paramtypes, true, true, &c))
					return false;

				if (params->data[j] != 0) {
					if (paramtypes->arraydim > 0) {
						if (!builtin_arrayinstanceof(params->data[j], c->vftbl))
							goto illegal_arg;

					} else {
						if (!builtin_instanceof(params->data[j], c))
							goto illegal_arg;
					}
				}
				blk[i].itemtype = TYPE_ADR;
				blk[i].item = PTR_TO_ITEM(params->data[j]);
				break;			

			default:
				goto illegal_arg;
		} /* end param type switch */

	} /* end param loop */

/*  	if (rettype) */
/*  		*rettype = descr->returntype.decltype; */

	return true;

illegal_arg:
	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
	return false;
}


static jmethodID get_virtual(jobject obj, jmethodID methodID)
{
	if (obj->vftbl->class == methodID->class)
		return methodID;

	return class_resolvemethod(obj->vftbl->class, methodID->name,
							   methodID->descriptor);
}


static jmethodID get_nonvirtual(jclass clazz, jmethodID methodID)
{
	if (clazz == methodID->class)
		return methodID;

	/* class_resolvemethod -> classfindmethod? (JOWENN) */
	return class_resolvemethod(clazz, methodID->name, methodID->descriptor);
}


static jobject callObjectMethod(jobject obj, jmethodID methodID, va_list args)
{ 	
	int argcount;
	jni_callblock *blk;
	jobject ret;



	if (methodID == 0) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError); 
		return 0;
	}

	argcount = methodID->parseddesc->paramcount;

	if (!( ((methodID->flags & ACC_STATIC) && (obj == 0)) ||
		((!(methodID->flags & ACC_STATIC)) && (obj != 0)) )) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

	if (obj && !builtin_instanceof(obj, methodID->class)) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

#ifdef arglimit

	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. CallObjectMethod does not support that");
		return 0;
	}
#endif

	blk = MNEW(jni_callblock, /*4 */argcount+2);

	fill_callblock_from_vargs(obj, methodID->parseddesc, blk, args, TYPE_ADR);
	/*      printf("parameter: obj: %p",blk[0].item); */
	STATS(jnicallXmethodnvokation();)
	ret = asm_calljavafunction2(methodID,
								argcount + 1,
								(argcount + 1) * sizeof(jni_callblock),
								blk);
	MFREE(blk, jni_callblock, argcount + 1);
	/*      printf("(CallObjectMethodV)-->%p\n",ret); */

	return ret;
}


/*
  core function for integer class methods (bool, byte, short, integer)
  This is basically needed for i386
*/
static jint callIntegerMethod(jobject obj, jmethodID methodID, int retType, va_list args)
{
	int argcount;
	jni_callblock *blk;
	jint ret;

	STATS(jniinvokation();)

        /*
        log_text("JNI-Call: CallObjectMethodV");
        utf_display(methodID->name);
        utf_display(methodID->descriptor);
        printf("\nParmaeter count: %d\n",argcount);
        utf_display(obj->vftbl->class->name);
        printf("\n");
        */
	if (methodID == 0) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError); 
		return 0;
	}
        
	argcount = methodID->parseddesc->paramcount;

	if (!( ((methodID->flags & ACC_STATIC) && (obj == 0)) ||
		((!(methodID->flags & ACC_STATIC)) && (obj != 0)) )) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

	if (obj && !builtin_instanceof(obj, methodID->class)) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

#ifdef arglimit
	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. CallIntegerMethod does not support that");
		return 0;
	}
#endif

	blk = MNEW(jni_callblock, /*4 */ argcount+2);

	fill_callblock_from_vargs(obj, methodID->parseddesc, blk, args, retType);

	/*      printf("parameter: obj: %p",blk[0].item); */
	STATS(jnicallXmethodnvokation();)
	ret = asm_calljavafunction2int(methodID,
								   argcount + 1,
								   (argcount + 1) * sizeof(jni_callblock),
								   blk);

	MFREE(blk, jni_callblock, argcount + 1);
	/*      printf("(CallObjectMethodV)-->%p\n",ret); */

	return ret;
}


/*core function for long class functions*/
static jlong callLongMethod(jobject obj, jmethodID methodID, va_list args)
{
	int argcount;
	jni_callblock *blk;
	jlong ret;

	STATS(jniinvokation();)

/*	
        log_text("JNI-Call: CallObjectMethodV");
        utf_display(methodID->name);
        utf_display(methodID->descriptor);
        printf("\nParmaeter count: %d\n",argcount);
        utf_display(obj->vftbl->class->name);
        printf("\n");
*/      
	if (methodID == 0) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError); 
		return 0;
	}

	argcount = methodID->parseddesc->paramcount;

	if (!( ((methodID->flags & ACC_STATIC) && (obj == 0)) ||
		   ((!(methodID->flags & ACC_STATIC)) && (obj!=0)) )) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

	if (obj && !builtin_instanceof(obj,methodID->class)) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

#ifdef arglimit
	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. CallObjectMethod does not support that");
		return 0;
	}
#endif

	blk = MNEW(jni_callblock,/* 4 */argcount+2);

	fill_callblock_from_vargs(obj, methodID->parseddesc, blk, args, TYPE_LNG);

	/*      printf("parameter: obj: %p",blk[0].item); */
	STATS(jnicallXmethodnvokation();)
	ret = asm_calljavafunction2long(methodID,
									argcount + 1,
									(argcount + 1) * sizeof(jni_callblock),
									blk);

	MFREE(blk, jni_callblock, argcount + 1);
	/*      printf("(CallObjectMethodV)-->%p\n",ret); */

	return ret;
}


/*core function for float class methods (float,double)*/
static jdouble callFloatMethod(jobject obj, jmethodID methodID, va_list args,int retType)
{
	int argcount = methodID->parseddesc->paramcount;
	jni_callblock *blk;
	jdouble ret;

	STATS(jniinvokation();)

        /*
        log_text("JNI-Call: CallObjectMethodV");
        utf_display(methodID->name);
        utf_display(methodID->descriptor);
        printf("\nParmaeter count: %d\n",argcount);
        utf_display(obj->vftbl->class->name);
        printf("\n");
        */

#ifdef arglimit
	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. CallObjectMethod does not support that");
		return 0;
	}
#endif

	blk = MNEW(jni_callblock, /*4 */ argcount+2);

	fill_callblock_from_vargs(obj, methodID->parseddesc, blk, args, retType);

	/*      printf("parameter: obj: %p",blk[0].item); */
	STATS(jnicallXmethodnvokation();)
	ret = asm_calljavafunction2double(methodID,
									  argcount + 1,
									  (argcount + 1) * sizeof(jni_callblock),
									  blk);

	MFREE(blk, jni_callblock, argcount + 1);
	/*      printf("(CallObjectMethodV)-->%p\n",ret); */

	return ret;
}


/*************************** function: jclass_findfield ****************************
	
	searches for field with specified name and type in a 'classinfo'-structur
	if no such field is found NULL is returned 

************************************************************************************/

static fieldinfo *jclass_findfield (classinfo *c, utf *name, utf *desc)
{
	s4 i;
	STATS(jniinvokation();)

/*	printf(" FieldCount: %d\n",c->fieldscount);
	utf_display(c->name); */
		for (i = 0; i < c->fieldscount; i++) {
/*		utf_display(c->fields[i].name);
		printf("\n");
		utf_display(c->fields[i].descriptor);
		printf("\n");*/
		if ((c->fields[i].name == name) && (c->fields[i].descriptor == desc))
			return &(c->fields[i]);
		}

	if (c->super.cls) return jclass_findfield(c->super.cls,name,desc);

	return NULL;
}


/* GetVersion ******************************************************************

   Returns the major version number in the higher 16 bits and the
   minor version number in the lower 16 bits.

*******************************************************************************/

jint GetVersion(JNIEnv *env)
{
	STATS(jniinvokation();)

	/* just say we support JNI 1.4 */

	return JNI_VERSION_1_4;
}


/* Class Operations ***********************************************************/

/* DefineClass *****************************************************************

   Loads a class from a buffer of raw class data. The buffer
   containing the raw class data is not referenced by the VM after the
   DefineClass call returns, and it may be discarded if desired.

*******************************************************************************/

jclass DefineClass(JNIEnv *env, const char *name, jobject loader,
				   const jbyte *buf, jsize bufLen)
{
	java_lang_ClassLoader *cl;
	java_lang_String      *s;
	java_bytearray        *ba;
	jclass                 c;

	STATS(jniinvokation();)

	cl = (java_lang_ClassLoader *) loader;
	s = javastring_new_char(name);
	ba = (java_bytearray *) buf;

	c = (jclass) Java_java_lang_VMClassLoader_defineClass(env, NULL, cl, s, ba,
														  0, bufLen, NULL);

	return c;
}


/* FindClass *******************************************************************

   This function loads a locally-defined class. It searches the
   directories and zip files specified by the CLASSPATH environment
   variable for the class with the specified name.

*******************************************************************************/

jclass FindClass(JNIEnv *env, const char *name)
{
	utf               *u;
	classinfo         *c;
	java_objectheader *cl;

	STATS(jniinvokation();)

	u = utf_new_char_classname((char *) name);

	/* check stacktrace for classloader, if one found use it, otherwise use */
	/* the system classloader */

#if defined(__I386__) || defined(__X86_64__) || defined(__ALPHA__)
	cl = cacao_currentClassLoader();
#else
	cl = NULL;
#endif

	if (!(c = load_class_from_classloader(u, cl)))
		return NULL;

	if (!link_class(c))
		return NULL;

	use_class_as_object(c);

  	return c;
}
  

/* FromReflectedMethod *********************************************************

   Converts java.lang.reflect.Method or java.lang.reflect.Constructor
   object to a method ID.
  
*******************************************************************************/
  
jmethodID FromReflectedMethod(JNIEnv *env, jobject method)
{
	methodinfo *mi;
	classinfo  *c;
	s4          slot;

	STATS(jniinvokation();)

	if (method == NULL)
		return NULL;
	
	if (builtin_instanceof(method, class_java_lang_reflect_Method)) {
		java_lang_reflect_Method *rm;

		rm = (java_lang_reflect_Method *) method;
		c = (classinfo *) (rm->declaringClass);
		slot = rm->slot;

	} else if (builtin_instanceof(method, class_java_lang_reflect_Constructor)) {
		java_lang_reflect_Constructor *rc;

		rc = (java_lang_reflect_Constructor *) method;
		c = (classinfo *) (rc->clazz);
		slot = rc->slot;

	} else
		return NULL;

	if ((slot < 0) || (slot >= c->methodscount)) {
		/* this usually means a severe internal cacao error or somebody
		   tempered around with the reflected method */
		log_text("error illegal slot for method in class(FromReflectedMethod)");
		assert(0);
	}

	mi = &(c->methods[slot]);

	return mi;
}


/* GetSuperclass ***************************************************************

   If clazz represents any class other than the class Object, then
   this function returns the object that represents the superclass of
   the class specified by clazz.

*******************************************************************************/
 
jclass GetSuperclass(JNIEnv *env, jclass sub)
{
	classinfo *c;
	STATS(jniinvokation();)

	c = ((classinfo *) sub)->super.cls;

	if (!c)
		return NULL;

	use_class_as_object(c);

	return c;
}
  
 
/* IsAssignableFrom ************************************************************

   Determines whether an object of sub can be safely cast to sup.

*******************************************************************************/

jboolean IsAssignableFrom(JNIEnv *env, jclass sub, jclass sup)
{
	STATS(jniinvokation();)
	return Java_java_lang_VMClass_isAssignableFrom(env,
												   NULL,
												   (java_lang_Class *) sup,
												   (java_lang_Class *) sub);
}


/***** converts a field ID derived from cls to a java.lang.reflect.Field object ***/

jobject ToReflectedField(JNIEnv* env, jclass cls, jfieldID fieldID, jboolean isStatic)
{
	log_text("JNI-Call: ToReflectedField: IMPLEMENT ME!!!");
	STATS(jniinvokation();)
	return NULL;
}


/* Throw ***********************************************************************

   Causes a java.lang.Throwable object to be thrown.

*******************************************************************************/

jint Throw(JNIEnv *env, jthrowable obj)
{
	*exceptionptr = (java_objectheader *) obj;
	STATS(jniinvokation();)

	return JNI_OK;
}


/* ThrowNew ********************************************************************

   Constructs an exception object from the specified class with the
   message specified by message and causes that exception to be
   thrown.

*******************************************************************************/

jint ThrowNew(JNIEnv* env, jclass clazz, const char *msg) 
{
	java_lang_Throwable *o;
	java_lang_String    *s;
	STATS(jniinvokation();)

	s = (java_lang_String *) javastring_new_char(msg);

  	/* instantiate exception object */

	o = (java_lang_Throwable *) native_new_and_init_string((classinfo *) clazz,
														   s);

	if (!o)
		return -1;

	*exceptionptr = (java_objectheader *) o;

	return 0;
}


/* ExceptionOccurred ***********************************************************

   Determines if an exception is being thrown. The exception stays
   being thrown until either the native code calls ExceptionClear(),
   or the Java code handles the exception.

*******************************************************************************/

jthrowable ExceptionOccurred(JNIEnv *env)
{
	STATS(jniinvokation();)
	return (jthrowable) *exceptionptr;
}


/* ExceptionDescribe ***********************************************************

   Prints an exception and a backtrace of the stack to a system
   error-reporting channel, such as stderr. This is a convenience
   routine provided for debugging.

*******************************************************************************/

void ExceptionDescribe(JNIEnv *env)
{
	java_objectheader *e;
	methodinfo        *m;
	STATS(jniinvokation();)

	e = *exceptionptr;

	if (e) {
		/* clear exception, because we are calling jit code again */

		*exceptionptr = NULL;

		/* get printStackTrace method from exception class */

		m = class_resolveclassmethod(e->vftbl->class,
									 utf_printStackTrace,
									 utf_void__void,
									 NULL,
									 true);

		if (!m)
			/* XXX what should we do? */
			return;

		/* print the stacktrace */

		asm_calljavafunction(m, e, NULL, NULL, NULL);
	}
}


/* ExceptionClear **************************************************************

   Clears any exception that is currently being thrown. If no
   exception is currently being thrown, this routine has no effect.

*******************************************************************************/

void ExceptionClear(JNIEnv *env)
{
	STATS(jniinvokation();)
	*exceptionptr = NULL;
}


/* FatalError ******************************************************************

   Raises a fatal error and does not expect the VM to recover. This
   function does not return.

*******************************************************************************/

void FatalError(JNIEnv *env, const char *msg)
{
	STATS(jniinvokation();)
	throw_cacao_exception_exit(string_java_lang_InternalError, msg);
}


/******************* creates a new local reference frame **************************/ 

jint PushLocalFrame(JNIEnv* env, jint capacity)
{
	log_text("JNI-Call: PushLocalFrame: IMPLEMENT ME!");
	STATS(jniinvokation();)

	return 0;
}

/**************** Pops off the current local reference frame **********************/

jobject PopLocalFrame(JNIEnv* env, jobject result)
{
	log_text("JNI-Call: PopLocalFrame: IMPLEMENT ME!");
	STATS(jniinvokation();)

	return NULL;
}


/* DeleteLocalRef **************************************************************

   Deletes the local reference pointed to by localRef.

*******************************************************************************/

void DeleteLocalRef(JNIEnv *env, jobject localRef)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: DeleteLocalRef: IMPLEMENT ME!");
}


/* IsSameObject ****************************************************************

   Tests whether two references refer to the same Java object.

*******************************************************************************/

jboolean IsSameObject(JNIEnv *env, jobject ref1, jobject ref2)
{
	STATS(jniinvokation();)
	return (ref1 == ref2);
}


/* NewLocalRef *****************************************************************

   Creates a new local reference that refers to the same object as ref.

*******************************************************************************/

jobject NewLocalRef(JNIEnv *env, jobject ref)
{
	log_text("JNI-Call: NewLocalRef: IMPLEMENT ME!");
	STATS(jniinvokation();)
	return ref;
}

/*********************************************************************************** 

	Ensures that at least a given number of local references can 
	be created in the current thread

 **********************************************************************************/   

jint EnsureLocalCapacity (JNIEnv* env, jint capacity)
{
	STATS(jniinvokation();)
	return 0; /* return 0 on success */
}


/* AllocObject *****************************************************************

   Allocates a new Java object without invoking any of the
   constructors for the object. Returns a reference to the object.

*******************************************************************************/

jobject AllocObject(JNIEnv *env, jclass clazz)
{
	java_objectheader *o;
	STATS(jniinvokation();)

	if ((clazz->flags & ACC_INTERFACE) || (clazz->flags & ACC_ABSTRACT)) {
		*exceptionptr =
			new_exception_utfmessage(string_java_lang_InstantiationException,
									 clazz->name);
		return NULL;
	}
		
	o = builtin_new(clazz);

	return o;
}


/* NewObject *******************************************************************

   Constructs a new Java object. The method ID indicates which
   constructor method to invoke. This ID must be obtained by calling
   GetMethodID() with <init> as the method name and void (V) as the
   return type.

*******************************************************************************/

jobject NewObject(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	java_objectheader *o;
	void* args[3];
	int argcount=methodID->parseddesc->paramcount;
	int i;
	va_list vaargs;
	STATS(jniinvokation();)

#ifdef arglimit
	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. NewObject does not support that");
		return 0;
	}
#endif

	/* create object */

	o = builtin_new(clazz);
	
	if (!o)
		return NULL;

	va_start(vaargs, methodID);
	for (i = 0; i < argcount; i++) {
		args[i] = va_arg(vaargs, void*);
	}
	va_end(vaargs);

	/* call constructor */

	asm_calljavafunction(methodID, o, args[0], args[1], args[2]);

	return o;
}


/*********************************************************************************** 

       Constructs a new Java object
       arguments that are to be passed to the constructor are placed in va_list args 

***********************************************************************************/

jobject NewObjectV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: NewObjectV");
	STATS(jniinvokation();)

	return NULL;
}


/*********************************************************************************** 

	Constructs a new Java object
	arguments that are to be passed to the constructor are placed in 
	args array of jvalues 

***********************************************************************************/

jobject NewObjectA(JNIEnv* env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: NewObjectA");
	STATS(jniinvokation();)

	return NULL;
}


/* GetObjectClass **************************************************************

 Returns the class of an object.

*******************************************************************************/

jclass GetObjectClass(JNIEnv *env, jobject obj)
{
	classinfo *c;
	STATS(jniinvokation();)
	
	if (!obj || !obj->vftbl)
		return NULL;

 	c = obj->vftbl->class;
	use_class_as_object(c);
	return c;
}


/* IsInstanceOf ****************************************************************

   Tests whether an object is an instance of a class.

*******************************************************************************/

jboolean IsInstanceOf(JNIEnv *env, jobject obj, jclass clazz)
{
	STATS(jniinvokation();)

	return Java_java_lang_VMClass_isInstance(env,
											 NULL,
											 (java_lang_Class *) clazz,
											 (java_lang_Object *) obj);
}


/***************** converts a java.lang.reflect.Field to a field ID ***************/
 
jfieldID FromReflectedField(JNIEnv* env, jobject field)
{
	java_lang_reflect_Field *f;
	classinfo *c;
	jfieldID fid;   /* the JNI-fieldid of the wrapping object */
	STATS(jniinvokation();)
	/*log_text("JNI-Call: FromReflectedField");*/

	f=(java_lang_reflect_Field *)field;
	if (f==0) return 0;
	c=(classinfo*)(f->declaringClass);
	if ( (f->slot<0) || (f->slot>=c->fieldscount)) {
		/*this usually means a severe internal cacao error or somebody
		tempered around with the reflected method*/
		log_text("error illegal slot for field in class(FromReflectedField)");
		assert(0);
	}
	fid=&(c->fields[f->slot]);
	return fid;
}


/**********************************************************************************

	converts a method ID to a java.lang.reflect.Method or 
	java.lang.reflect.Constructor object

**********************************************************************************/

jobject ToReflectedMethod(JNIEnv* env, jclass cls, jmethodID methodID, jboolean isStatic)
{
	log_text("JNI-Call: ToReflectedMethod");
	STATS(jniinvokation();)

	return NULL;
}


/* GetMethodID *****************************************************************

   returns the method ID for an instance method

*******************************************************************************/

jmethodID GetMethodID(JNIEnv* env, jclass clazz, const char *name, const char *sig)
{
	jmethodID m;
	STATS(jniinvokation();)

 	m = class_resolvemethod(clazz, 
							utf_new_char((char *) name), 
							utf_new_char((char *) sig));

	if (!m || (m->flags & ACC_STATIC)) {
		*exceptionptr =
			new_exception_message(string_java_lang_NoSuchMethodError, name);

		return NULL;
	}

	return m;
}


/******************** JNI-functions for calling instance methods ******************/

jobject CallObjectMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jobject ret;
	va_list vaargs;
	STATS(jniinvokation();)

/*	log_text("JNI-Call: CallObjectMethod");*/

	va_start(vaargs, methodID);
	ret = callObjectMethod(obj, methodID, vaargs);
	va_end(vaargs);

	return ret;
}


jobject CallObjectMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	return callObjectMethod(obj,methodID,args);
}


jobject CallObjectMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallObjectMethodA");
	STATS(jniinvokation();)

	return NULL;
}




jboolean CallBooleanMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jboolean ret;
	va_list vaargs;
	STATS(jniinvokation();)

/*	log_text("JNI-Call: CallBooleanMethod");*/

	va_start(vaargs,methodID);
	ret = (jboolean)callIntegerMethod(obj,get_virtual(obj,methodID),PRIMITIVETYPE_BOOLEAN,vaargs);
	va_end(vaargs);
	return ret;

}

jboolean CallBooleanMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)

	return (jboolean)callIntegerMethod(obj,get_virtual(obj,methodID),PRIMITIVETYPE_BOOLEAN,args);

}

jboolean CallBooleanMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallBooleanMethodA");

	return 0;
}

jbyte CallByteMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jbyte ret;
	va_list vaargs;
	STATS(jniinvokation();)

/*	log_text("JNI-Call: CallVyteMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj,get_virtual(obj,methodID),PRIMITIVETYPE_BYTE,vaargs);
	va_end(vaargs);
	return ret;

}

jbyte CallByteMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
/*	log_text("JNI-Call: CallByteMethodV");*/
	STATS(jniinvokation();)

	return callIntegerMethod(obj,methodID,PRIMITIVETYPE_BYTE,args);
}


jbyte CallByteMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallByteMethodA");
	STATS(jniinvokation();)

	return 0;
}


jchar CallCharMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jchar ret;
	va_list vaargs;
	STATS(jniinvokation();)

/*	log_text("JNI-Call: CallCharMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj, get_virtual(obj, methodID),PRIMITIVETYPE_CHAR, vaargs);
	va_end(vaargs);

	return ret;
}


jchar CallCharMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)

/*	log_text("JNI-Call: CallCharMethodV");*/
	return callIntegerMethod(obj,get_virtual(obj,methodID),PRIMITIVETYPE_CHAR,args);
}


jchar CallCharMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: CallCharMethodA");

	return 0;
}


jshort CallShortMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jshort ret;
	va_list vaargs;
	STATS(jniinvokation();)

/*	log_text("JNI-Call: CallShortMethod");*/

	va_start(vaargs, methodID);
	ret = callIntegerMethod(obj, get_virtual(obj, methodID),PRIMITIVETYPE_SHORT, vaargs);
	va_end(vaargs);

	return ret;
}


jshort CallShortMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	return callIntegerMethod(obj, get_virtual(obj, methodID),PRIMITIVETYPE_SHORT, args);
}


jshort CallShortMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallShortMethodA");

	return 0;
}



jint CallIntMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jint ret;
	va_list vaargs;
	STATS(jniinvokation();)

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj, get_virtual(obj, methodID),PRIMITIVETYPE_INT, vaargs);
	va_end(vaargs);

	return ret;
}


jint CallIntMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	return callIntegerMethod(obj, get_virtual(obj, methodID),PRIMITIVETYPE_INT, args);
}


jint CallIntMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallIntMethodA");

	return 0;
}



jlong CallLongMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jlong ret;
	va_list vaargs;
	STATS(jniinvokation();)
	
	va_start(vaargs,methodID);
	ret = callLongMethod(obj,get_virtual(obj, methodID),vaargs);
	va_end(vaargs);

	return ret;
}


jlong CallLongMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	return 	callLongMethod(obj,get_virtual(obj, methodID),args);
}


jlong CallLongMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallLongMethodA");

	return 0;
}



jfloat CallFloatMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jfloat ret;
	va_list vaargs;

	STATS(jniinvokation();)
/*	log_text("JNI-Call: CallFloatMethod");*/

	va_start(vaargs,methodID);
	ret = callFloatMethod(obj, get_virtual(obj, methodID), vaargs, PRIMITIVETYPE_FLOAT);
	va_end(vaargs);

	return ret;
}


jfloat CallFloatMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallFloatMethodV");
	return callFloatMethod(obj, get_virtual(obj, methodID), args, PRIMITIVETYPE_FLOAT);
}


jfloat CallFloatMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallFloatMethodA");

	return 0;
}



jdouble CallDoubleMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jdouble ret;
	va_list vaargs;
	STATS(jniinvokation();)

/*	log_text("JNI-Call: CallDoubleMethod");*/

	va_start(vaargs,methodID);
	ret = callFloatMethod(obj, get_virtual(obj, methodID), vaargs, PRIMITIVETYPE_DOUBLE);
	va_end(vaargs);

	return ret;
}


jdouble CallDoubleMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallDoubleMethodV");
	return callFloatMethod(obj, get_virtual(obj, methodID), args, PRIMITIVETYPE_DOUBLE);
}


jdouble CallDoubleMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallDoubleMethodA");
	return 0;
}



void CallVoidMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	va_list vaargs;
	STATS(jniinvokation();)

	va_start(vaargs,methodID);
	(void) callIntegerMethod(obj, get_virtual(obj, methodID),TYPE_VOID, vaargs);
	va_end(vaargs);
}


void CallVoidMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallVoidMethodV");
	STATS(jniinvokation();)
	(void)callIntegerMethod(obj,get_virtual(obj,methodID),TYPE_VOID,args);
}


void CallVoidMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallVoidMethodA");
}



jobject CallNonvirtualObjectMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualObjectMethod");

	return NULL;
}


jobject CallNonvirtualObjectMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualObjectMethodV");

	return NULL;
}


jobject CallNonvirtualObjectMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualObjectMethodA");

	return NULL;
}



jboolean CallNonvirtualBooleanMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jboolean ret;
	va_list vaargs;
	STATS(jniinvokation();)

/*	log_text("JNI-Call: CallNonvirtualBooleanMethod");*/

	va_start(vaargs,methodID);
	ret = (jboolean)callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_BOOLEAN,vaargs);
	va_end(vaargs);
	return ret;

}


jboolean CallNonvirtualBooleanMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
/*	log_text("JNI-Call: CallNonvirtualBooleanMethodV");*/
	return (jboolean)callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_BOOLEAN,args);
}


jboolean CallNonvirtualBooleanMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualBooleanMethodA");

	return 0;
}



jbyte CallNonvirtualByteMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jbyte ret;
	va_list vaargs;

	STATS(jniinvokation();)
/*	log_text("JNI-Call: CallNonvirutalByteMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_BYTE,vaargs);
	va_end(vaargs);
	return ret;
}


jbyte CallNonvirtualByteMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	/*log_text("JNI-Call: CallNonvirtualByteMethodV"); */
	return callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_BYTE,args);

}


jbyte CallNonvirtualByteMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualByteMethodA");

	return 0;
}



jchar CallNonvirtualCharMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jchar ret;
	va_list vaargs;

	STATS(jniinvokation();)
/*	log_text("JNI-Call: CallNonVirtualCharMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_CHAR,vaargs);
	va_end(vaargs);
	return ret;
}


jchar CallNonvirtualCharMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	/*log_text("JNI-Call: CallNonvirtualCharMethodV");*/
	return callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_CHAR,args);
}


jchar CallNonvirtualCharMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualCharMethodA");

	return 0;
}



jshort CallNonvirtualShortMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jshort ret;
	va_list vaargs;
	STATS(jniinvokation();)

	/*log_text("JNI-Call: CallNonvirtualShortMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_SHORT,vaargs);
	va_end(vaargs);
	return ret;
}


jshort CallNonvirtualShortMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	/*log_text("JNI-Call: CallNonvirtualShortMethodV");*/
	return callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_SHORT,args);
}


jshort CallNonvirtualShortMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualShortMethodA");

	return 0;
}



jint CallNonvirtualIntMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{

        jint ret;
        va_list vaargs;
	STATS(jniinvokation();)

	/*log_text("JNI-Call: CallNonvirtualIntMethod");*/

        va_start(vaargs,methodID);
        ret = callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_INT,vaargs);
        va_end(vaargs);
        return ret;
}


jint CallNonvirtualIntMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	/*log_text("JNI-Call: CallNonvirtualIntMethodV");*/
        return callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_INT,args);
}


jint CallNonvirtualIntMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualIntMethodA");

	return 0;
}



jlong CallNonvirtualLongMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualLongMethod");

	return 0;
}


jlong CallNonvirtualLongMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualLongMethodV");

	return 0;
}


jlong CallNonvirtualLongMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualLongMethodA");

	return 0;
}



jfloat CallNonvirtualFloatMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jfloat ret;
	va_list vaargs;
	STATS(jniinvokation();)

	/*log_text("JNI-Call: CallNonvirtualFloatMethod");*/


	va_start(vaargs,methodID);
	ret = callFloatMethod(obj,get_nonvirtual(clazz,methodID),vaargs,PRIMITIVETYPE_FLOAT);
	va_end(vaargs);
	return ret;

}


jfloat CallNonvirtualFloatMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualFloatMethodV");
	return callFloatMethod(obj,get_nonvirtual(clazz,methodID),args,PRIMITIVETYPE_FLOAT);
}


jfloat CallNonvirtualFloatMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualFloatMethodA");

	return 0;
}



jdouble CallNonvirtualDoubleMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jdouble ret;
	va_list vaargs;
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualDoubleMethod");

	va_start(vaargs,methodID);
	ret = callFloatMethod(obj,get_nonvirtual(clazz,methodID),vaargs,PRIMITIVETYPE_DOUBLE);
	va_end(vaargs);
	return ret;

}


jdouble CallNonvirtualDoubleMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
/*	log_text("JNI-Call: CallNonvirtualDoubleMethodV");*/
	return callFloatMethod(obj,get_nonvirtual(clazz,methodID),args,PRIMITIVETYPE_DOUBLE);
}


jdouble CallNonvirtualDoubleMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualDoubleMethodA");

	return 0;
}



void CallNonvirtualVoidMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
        va_list vaargs;
	STATS(jniinvokation();)

/*      log_text("JNI-Call: CallNonvirtualVoidMethod");*/

        va_start(vaargs,methodID);
        (void)callIntegerMethod(obj,get_nonvirtual(clazz,methodID),TYPE_VOID,vaargs);
        va_end(vaargs);

}


void CallNonvirtualVoidMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
/*	log_text("JNI-Call: CallNonvirtualVoidMethodV");*/
	STATS(jniinvokation();)

        (void)callIntegerMethod(obj,get_nonvirtual(clazz,methodID),TYPE_VOID,args);

}


void CallNonvirtualVoidMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallNonvirtualVoidMethodA");
}

/************************* JNI-functions for accessing fields ************************/

jfieldID GetFieldID (JNIEnv *env, jclass clazz, const char *name, const char *sig) 
{
	jfieldID f;

	STATS(jniinvokation();)

/*	log_text("========================= searching for:");
	log_text(name);
	log_text(sig);*/
	f = jclass_findfield(clazz,
			    utf_new_char ((char*) name), 
			    utf_new_char ((char*) sig)
		  	    ); 
	
	if (!f) { 
		/*utf_display(clazz->name);
		log_text(name);
		log_text(sig);*/
		*exceptionptr =	new_exception(string_java_lang_NoSuchFieldError);  
	}
	return f;
}

/*************************** retrieve fieldid, abort on error ************************/

jfieldID getFieldID_critical(JNIEnv *env, jclass clazz, char *name, char *sig)
{
    jfieldID id = GetFieldID(env, clazz, name, sig);
	STATS(jniinvokation();)

    if (!id) {
       log_text("class:");
       utf_display(clazz->name);
       log_text("\nfield:");
       log_text(name);
       log_text("sig:");
       log_text(sig);

       log_text("setfield_critical failed");
	   assert(0);
    }
    return id;
}

jobject GetObjectField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	/*
	jobject dbg,dretval,*dpretval;	
	long int dli1, dli2, dli3;

	printf("GetObjectField(1): thread: %s obj: %p name: %s desc: %s \n",GetStringUTFChars(env,
			 ((threadobject *) THREADOBJECT)->o
			 .thread->name,NULL)
			 ,obj,((fieldinfo*)fieldID)->name->text,(fieldID->descriptor)->text);

	dbg = getField(obj,jobject,fieldID);
	dli1 = (long int) obj;
	dli2 = (long int) fieldID->offset;
	dli3 = dli1+dli2;
	dpretval = (jobject*) dli3;
	dretval = *dpretval;
	jclass tmp;
 	jmethodID mid;
	jstring jstr;

	tmp = FindClass(env, "java/lang/Object");
	mid = GetMethodID(env,tmp,"toString","()Ljava/lang/String;");
	jstr = CallObjectMethod(env,dbg,mid);*/

/*	printf("GetObjectField(2): retval %p (obj: %#lx + offset: %#lx = %#lx (jobject*) %p (jobject) %p\n"
	,dbg, dli1, dli2, dli3,dpretval, dretval);*/


/*	return dbg;*/
	STATS(jniinvokation();)

	return getField(obj,jobject,fieldID);
}

jboolean GetBooleanField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	STATS(jniinvokation();)
	return getField(obj,jboolean,fieldID);
}


jbyte GetByteField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	STATS(jniinvokation();)
       	return getField(obj,jbyte,fieldID);
}


jchar GetCharField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	STATS(jniinvokation();)
	return getField(obj,jchar,fieldID);
}


jshort GetShortField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	STATS(jniinvokation();)
	return getField(obj,jshort,fieldID);
}


jint GetIntField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	STATS(jniinvokation();)
	return getField(obj,jint,fieldID);
}


jlong GetLongField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	STATS(jniinvokation();)
	return getField(obj,jlong,fieldID);
}


jfloat GetFloatField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	STATS(jniinvokation();)
	return getField(obj,jfloat,fieldID);
}


jdouble GetDoubleField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	STATS(jniinvokation();)
	return getField(obj,jdouble,fieldID);
}

void SetObjectField (JNIEnv *env, jobject obj, jfieldID fieldID, jobject val)
{
	STATS(jniinvokation();)
        setField(obj,jobject,fieldID,val);
}


void SetBooleanField (JNIEnv *env, jobject obj, jfieldID fieldID, jboolean val)
{
	STATS(jniinvokation();)
        setField(obj,jboolean,fieldID,val);
}


void SetByteField (JNIEnv *env, jobject obj, jfieldID fieldID, jbyte val)
{
	STATS(jniinvokation();)
        setField(obj,jbyte,fieldID,val);
}


void SetCharField (JNIEnv *env, jobject obj, jfieldID fieldID, jchar val)
{
	STATS(jniinvokation();)
        setField(obj,jchar,fieldID,val);
}


void SetShortField (JNIEnv *env, jobject obj, jfieldID fieldID, jshort val)
{
	STATS(jniinvokation();)
        setField(obj,jshort,fieldID,val);
}


void SetIntField (JNIEnv *env, jobject obj, jfieldID fieldID, jint val)
{
	STATS(jniinvokation();)
        setField(obj,jint,fieldID,val);
}


void SetLongField (JNIEnv *env, jobject obj, jfieldID fieldID, jlong val)
{
	STATS(jniinvokation();)
        setField(obj,jlong,fieldID,val);
}


void SetFloatField (JNIEnv *env, jobject obj, jfieldID fieldID, jfloat val)
{
	STATS(jniinvokation();)
        setField(obj,jfloat,fieldID,val);
}


void SetDoubleField (JNIEnv *env, jobject obj, jfieldID fieldID, jdouble val)
{
	STATS(jniinvokation();)
        setField(obj,jdouble,fieldID,val);
}


/**************** JNI-functions for calling static methods **********************/ 

jmethodID GetStaticMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig)
{
	jmethodID m;
	STATS(jniinvokation();)

 	m = class_resolvemethod(clazz,
							utf_new_char((char *) name),
							utf_new_char((char *) sig));

	if (!m || !(m->flags & ACC_STATIC)) {
		*exceptionptr =
			new_exception_message(string_java_lang_NoSuchMethodError, name);

		return NULL;
	}

	return m;
}


jobject CallStaticObjectMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jobject ret;
	va_list vaargs;
	STATS(jniinvokation();)

	/* log_text("JNI-Call: CallStaticObjectMethod");*/

	va_start(vaargs, methodID);
	ret = callObjectMethod(0, methodID, vaargs);
	va_end(vaargs);

	return ret;
}


jobject CallStaticObjectMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	/* log_text("JNI-Call: CallStaticObjectMethodV"); */
	
	return callObjectMethod(0,methodID,args);
}


jobject CallStaticObjectMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticObjectMethodA");

	return NULL;
}


jboolean CallStaticBooleanMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jboolean ret;
	va_list vaargs;
	STATS(jniinvokation();)

	va_start(vaargs, methodID);
	ret = (jboolean) callIntegerMethod(0, methodID, PRIMITIVETYPE_BOOLEAN, vaargs);
	va_end(vaargs);

	return ret;
}


jboolean CallStaticBooleanMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	return (jboolean) callIntegerMethod(0, methodID, PRIMITIVETYPE_BOOLEAN, args);
}


jboolean CallStaticBooleanMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticBooleanMethodA");

	return 0;
}


jbyte CallStaticByteMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jbyte ret;
	va_list vaargs;
	STATS(jniinvokation();)

	/*      log_text("JNI-Call: CallStaticByteMethod");*/

	va_start(vaargs, methodID);
	ret = (jbyte) callIntegerMethod(0, methodID, PRIMITIVETYPE_BYTE, vaargs);
	va_end(vaargs);

	return ret;
}


jbyte CallStaticByteMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	return (jbyte) callIntegerMethod(0, methodID, PRIMITIVETYPE_BYTE, args);
}


jbyte CallStaticByteMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticByteMethodA");

	return 0;
}


jchar CallStaticCharMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jchar ret;
	va_list vaargs;
	STATS(jniinvokation();)

	/*      log_text("JNI-Call: CallStaticByteMethod");*/

	va_start(vaargs, methodID);
	ret = (jchar) callIntegerMethod(0, methodID, PRIMITIVETYPE_CHAR, vaargs);
	va_end(vaargs);

	return ret;
}


jchar CallStaticCharMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	return (jchar) callIntegerMethod(0, methodID, PRIMITIVETYPE_CHAR, args);
}


jchar CallStaticCharMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticCharMethodA");

	return 0;
}



jshort CallStaticShortMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jshort ret;
	va_list vaargs;
	STATS(jniinvokation();)

	/*      log_text("JNI-Call: CallStaticByteMethod");*/

	va_start(vaargs, methodID);
	ret = (jshort) callIntegerMethod(0, methodID, PRIMITIVETYPE_SHORT, vaargs);
	va_end(vaargs);

	return ret;
}


jshort CallStaticShortMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	/*log_text("JNI-Call: CallStaticShortMethodV");*/
	return (jshort) callIntegerMethod(0, methodID, PRIMITIVETYPE_SHORT, args);
}


jshort CallStaticShortMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticShortMethodA");

	return 0;
}



jint CallStaticIntMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jint ret;
	va_list vaargs;
	STATS(jniinvokation();)

	/*      log_text("JNI-Call: CallStaticIntMethod");*/

	va_start(vaargs, methodID);
	ret = callIntegerMethod(0, methodID, PRIMITIVETYPE_INT, vaargs);
	va_end(vaargs);

	return ret;
}


jint CallStaticIntMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticIntMethodV");

	return callIntegerMethod(0, methodID, PRIMITIVETYPE_INT, args);
}


jint CallStaticIntMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticIntMethodA");

	return 0;
}



jlong CallStaticLongMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jlong ret;
	va_list vaargs;
	STATS(jniinvokation();)

	/*      log_text("JNI-Call: CallStaticLongMethod");*/

	va_start(vaargs, methodID);
	ret = callLongMethod(0, methodID, vaargs);
	va_end(vaargs);

	return ret;
}


jlong CallStaticLongMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticLongMethodV");
	
	return callLongMethod(0,methodID,args);
}


jlong CallStaticLongMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticLongMethodA");

	return 0;
}



jfloat CallStaticFloatMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jfloat ret;
	va_list vaargs;
	STATS(jniinvokation();)

	/*      log_text("JNI-Call: CallStaticLongMethod");*/

	va_start(vaargs, methodID);
	ret = callFloatMethod(0, methodID, vaargs, PRIMITIVETYPE_FLOAT);
	va_end(vaargs);

	return ret;
}


jfloat CallStaticFloatMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)

	return callFloatMethod(0, methodID, args, PRIMITIVETYPE_FLOAT);

}


jfloat CallStaticFloatMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticFloatMethodA");

	return 0;
}



jdouble CallStaticDoubleMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jdouble ret;
	va_list vaargs;
	STATS(jniinvokation();)

	/*      log_text("JNI-Call: CallStaticDoubleMethod");*/

	va_start(vaargs,methodID);
	ret = callFloatMethod(0, methodID, vaargs, PRIMITIVETYPE_DOUBLE);
	va_end(vaargs);

	return ret;
}


jdouble CallStaticDoubleMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticDoubleMethodV");

	return callFloatMethod(0, methodID, args, PRIMITIVETYPE_DOUBLE);
}


jdouble CallStaticDoubleMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticDoubleMethodA");

	return 0;
}


void CallStaticVoidMethod(JNIEnv *env, jclass cls, jmethodID methodID, ...)
{
	va_list vaargs;
	STATS(jniinvokation();)

	va_start(vaargs, methodID);
	(void) callIntegerMethod(0, methodID, TYPE_VOID, vaargs);
	va_end(vaargs);
}


void CallStaticVoidMethodV(JNIEnv *env, jclass cls, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticVoidMethodV");
	STATS(jniinvokation();)
	(void)callIntegerMethod(0, methodID, TYPE_VOID, args);
}


void CallStaticVoidMethodA(JNIEnv *env, jclass cls, jmethodID methodID, jvalue * args)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: CallStaticVoidMethodA");
}


/* Accessing Static Fields ****************************************************/

/* GetStaticFieldID ************************************************************

   Returns the field ID for a static field of a class. The field is
   specified by its name and signature. The GetStatic<type>Field and
   SetStatic<type>Field families of accessor functions use field IDs
   to retrieve static fields.

*******************************************************************************/

jfieldID GetStaticFieldID(JNIEnv *env, jclass clazz, const char *name, const char *sig)
{
	jfieldID f;
	STATS(jniinvokation();)

	f = jclass_findfield(clazz,
						 utf_new_char((char *) name),
						 utf_new_char((char *) sig)); 
	
	if (!f)
		*exceptionptr =	new_exception(string_java_lang_NoSuchFieldError);  

	return f;
}


/* GetStatic<type>Field ********************************************************

   This family of accessor routines returns the value of a static
   field of an object.

*******************************************************************************/

jobject GetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("GetStaticObjectField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return NULL;

	return fieldID->value.a;       
}


jboolean GetStaticBooleanField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("GetStaticBooleanField: calling initialize_class %s\n",clazz->name->text);)

	if (!initialize_class(clazz))
		return false;

	return fieldID->value.i;       
}


jbyte GetStaticByteField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("GetStaticByteField: calling initialize_class %s\n",clazz->name->text);)

	if (!initialize_class(clazz))
		return 0;

	return fieldID->value.i;       
}


jchar GetStaticCharField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("GetStaticCharField: calling initialize_class %s\n",clazz->name->text);)

	if (!initialize_class(clazz))
		return 0;

	return fieldID->value.i;       
}


jshort GetStaticShortField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("GetStaticShorttField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return 0;

	return fieldID->value.i;       
}


jint GetStaticIntField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("GetStaticIntField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return 0;

	return fieldID->value.i;       
}


jlong GetStaticLongField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("GetStaticLongField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return 0;

	return fieldID->value.l;
}


jfloat GetStaticFloatField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("GetStaticFloatField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return 0.0;

 	return fieldID->value.f;
}


jdouble GetStaticDoubleField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("GetStaticDoubleField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return 0.0;

	return fieldID->value.d;
}


/*  SetStatic<type>Field *******************************************************

	This family of accessor routines sets the value of a static field
	of an object.

*******************************************************************************/

void SetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID fieldID, jobject value)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("SetStaticObjectField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return;

	fieldID->value.a = value;
}


void SetStaticBooleanField(JNIEnv *env, jclass clazz, jfieldID fieldID, jboolean value)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("SetStaticBooleanField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return;

	fieldID->value.i = value;
}


void SetStaticByteField(JNIEnv *env, jclass clazz, jfieldID fieldID, jbyte value)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("SetStaticByteField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return;

	fieldID->value.i = value;
}


void SetStaticCharField(JNIEnv *env, jclass clazz, jfieldID fieldID, jchar value)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("SetStaticCharField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return;

	fieldID->value.i = value;
}


void SetStaticShortField(JNIEnv *env, jclass clazz, jfieldID fieldID, jshort value)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("SetStaticShortField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return;

	fieldID->value.i = value;
}


void SetStaticIntField(JNIEnv *env, jclass clazz, jfieldID fieldID, jint value)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("SetStaticIntField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return;

	fieldID->value.i = value;
}


void SetStaticLongField(JNIEnv *env, jclass clazz, jfieldID fieldID, jlong value)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("SetStaticLongField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return;

	fieldID->value.l = value;
}


void SetStaticFloatField(JNIEnv *env, jclass clazz, jfieldID fieldID, jfloat value)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("SetStaticFloatField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return;

	fieldID->value.f = value;
}


void SetStaticDoubleField(JNIEnv *env, jclass clazz, jfieldID fieldID, jdouble value)
{
	STATS(jniinvokation();)
	JWCLINITDEBUG(printf("SetStaticDoubleField: calling initialize_class %s\n",clazz->name->text);)
	if (!initialize_class(clazz))
		return;

	fieldID->value.d = value;
}


/* String Operations **********************************************************/

/* NewString *******************************************************************

   Create new java.lang.String object from an array of Unicode
   characters.

*******************************************************************************/

jstring NewString(JNIEnv *env, const jchar *buf, jsize len)
{
	java_lang_String *s;
	java_chararray   *a;
	u4                i;

	STATS(jniinvokation();)
	
	s = (java_lang_String *) builtin_new(class_java_lang_String);
	a = builtin_newarray_char(len);

	/* javastring or characterarray could not be created */
	if (!a || !s)
		return NULL;

	/* copy text */
	for (i = 0; i < len; i++)
		a->data[i] = buf[i];

	s->value = a;
	s->offset = 0;
	s->count = len;

	return (jstring) s;
}


static jchar emptyStringJ[]={0,0};

/* GetStringLength *************************************************************

   Returns the length (the count of Unicode characters) of a Java
   string.

*******************************************************************************/

jsize GetStringLength(JNIEnv *env, jstring str)
{
	return ((java_lang_String *) str)->count;
}


/********************  convertes javastring to u2-array ****************************/
	
u2 *javastring_tou2(jstring so) 
{
	java_lang_String *s;
	java_chararray   *a;
	u2               *stringbuffer;
	u4                i;

	STATS(jniinvokation();)
	
	s = (java_lang_String *) so;

	if (!s)
		return NULL;

	a = s->value;

	if (!a)
		return NULL;

	/* allocate memory */

	stringbuffer = MNEW(u2, s->count + 1);

	/* copy text */

	for (i = 0; i < s->count; i++)
		stringbuffer[i] = a->data[s->offset + i];
	
	/* terminate string */

	stringbuffer[i] = '\0';

	return stringbuffer;
}


/* GetStringChars **************************************************************

   Returns a pointer to the array of Unicode characters of the
   string. This pointer is valid until ReleaseStringchars() is called.

*******************************************************************************/

const jchar *GetStringChars(JNIEnv *env, jstring str, jboolean *isCopy)
{	
	jchar *jc;

	STATS(jniinvokation();)

	jc = javastring_tou2(str);

	if (jc)	{
		if (isCopy)
			*isCopy = JNI_TRUE;

		return jc;
	}

	if (isCopy)
		*isCopy = JNI_TRUE;

	return emptyStringJ;
}


/* ReleaseStringChars **********************************************************

   Informs the VM that the native code no longer needs access to
   chars. The chars argument is a pointer obtained from string using
   GetStringChars().

*******************************************************************************/

void ReleaseStringChars(JNIEnv *env, jstring str, const jchar *chars)
{
	STATS(jniinvokation();)

	if (chars == emptyStringJ)
		return;

	MFREE(((jchar *) chars), jchar, ((java_lang_String *) str)->count + 1);
}


/* NewStringUTF ****************************************************************

   Constructs a new java.lang.String object from an array of UTF-8 characters.

*******************************************************************************/

jstring NewStringUTF(JNIEnv *env, const char *bytes)
{
	STATS(jniinvokation();)
    return (jstring) javastring_new(utf_new_char(bytes));
}


/****************** returns the utf8 length in bytes of a string *******************/

jsize GetStringUTFLength (JNIEnv *env, jstring string)
{   
    java_lang_String *s = (java_lang_String*) string;
	STATS(jniinvokation();)

    return (jsize) u2_utflength(s->value->data, s->count); 
}


/* GetStringUTFChars ***********************************************************

   Returns a pointer to an array of UTF-8 characters of the
   string. This array is valid until it is released by
   ReleaseStringUTFChars().

*******************************************************************************/

const char *GetStringUTFChars(JNIEnv *env, jstring string, jboolean *isCopy)
{
	utf *u;
	STATS(jniinvokation();)

	if (!string)
		return "";

	if (isCopy)
		*isCopy = JNI_TRUE;
	
	u = javastring_toutf((java_lang_String *) string, false);

	if (u)
		return u->text;

	return "";
}


/* ReleaseStringUTFChars *******************************************************

   Informs the VM that the native code no longer needs access to
   utf. The utf argument is a pointer derived from string using
   GetStringUTFChars().

*******************************************************************************/

void ReleaseStringUTFChars(JNIEnv *env, jstring string, const char *utf)
{
	STATS(jniinvokation();)

    /* XXX we don't release utf chars right now, perhaps that should be done 
	   later. Since there is always one reference the garbage collector will
	   never get them */
}


/* Array Operations ***********************************************************/

/* GetArrayLength **************************************************************

   Returns the number of elements in the array.

*******************************************************************************/

jsize GetArrayLength(JNIEnv *env, jarray array)
{
	STATS(jniinvokation();)

	return array->size;
}


/* NewObjectArray **************************************************************

   Constructs a new array holding objects in class elementClass. All
   elements are initially set to initialElement.

*******************************************************************************/

jobjectArray NewObjectArray(JNIEnv *env, jsize length, jclass elementClass, jobject initialElement)
{
	java_objectarray *oa;
	s4 i;
	STATS(jniinvokation();)

	if (length < 0) {
		*exceptionptr = new_negativearraysizeexception();
		return NULL;
	}

    oa = builtin_anewarray(length, elementClass);

	if (!oa)
		return NULL;

	/* set all elements to initialElement */

	for (i = 0; i < length; i++)
		oa->data[i] = initialElement;

	return oa;
}


jobject GetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index)
{
    jobject j = NULL;
	STATS(jniinvokation();)

    if (index < array->header.size)	
		j = array->data[index];
    else
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);
    
    return j;
}


void SetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index, jobject val)
{
	STATS(jniinvokation();)
    if (index >= array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else {
		/* check if the class of value is a subclass of the element class of the array */
		if (!builtin_canstore((java_objectarray *) array, (java_objectheader *) val))
			*exceptionptr = new_exception(string_java_lang_ArrayStoreException);

		else
			array->data[index] = val;
    }
}



jbooleanArray NewBooleanArray(JNIEnv *env, jsize len)
{
	java_booleanarray *j;
	STATS(jniinvokation();)

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_boolean(len);

    return j;
}


jbyteArray NewByteArray(JNIEnv *env, jsize len)
{
	java_bytearray *j;
	STATS(jniinvokation();)

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_byte(len);

    return j;
}


jcharArray NewCharArray(JNIEnv *env, jsize len)
{
	java_chararray *j;
	STATS(jniinvokation();)

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_char(len);

    return j;
}


jshortArray NewShortArray(JNIEnv *env, jsize len)
{
	java_shortarray *j;
	STATS(jniinvokation();)

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_short(len);

    return j;
}


jintArray NewIntArray(JNIEnv *env, jsize len)
{
	java_intarray *j;
	STATS(jniinvokation();)

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_int(len);

    return j;
}


jlongArray NewLongArray(JNIEnv *env, jsize len)
{
	java_longarray *j;
	STATS(jniinvokation();)

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_long(len);

    return j;
}


jfloatArray NewFloatArray(JNIEnv *env, jsize len)
{
	java_floatarray *j;
	STATS(jniinvokation();)

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_float(len);

    return j;
}


jdoubleArray NewDoubleArray(JNIEnv *env, jsize len)
{
	java_doublearray *j;
	STATS(jniinvokation();)

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_double(len);

    return j;
}


/* Get<PrimitiveType>ArrayElements *********************************************

   A family of functions that returns the body of the primitive array.

*******************************************************************************/

jboolean *GetBooleanArrayElements(JNIEnv *env, jbooleanArray array,
								  jboolean *isCopy)
{
	STATS(jniinvokation();)

    if (isCopy)
		*isCopy = JNI_FALSE;

    return array->data;
}


jbyte *GetByteArrayElements(JNIEnv *env, jbyteArray array, jboolean *isCopy)
{
	STATS(jniinvokation();)

    if (isCopy)
		*isCopy = JNI_FALSE;

    return array->data;
}


jchar *GetCharArrayElements(JNIEnv *env, jcharArray array, jboolean *isCopy)
{
	STATS(jniinvokation();)

    if (isCopy)
		*isCopy = JNI_FALSE;

    return array->data;
}


jshort *GetShortArrayElements(JNIEnv *env, jshortArray array, jboolean *isCopy)
{
	STATS(jniinvokation();)

    if (isCopy)
		*isCopy = JNI_FALSE;

    return array->data;
}


jint *GetIntArrayElements(JNIEnv *env, jintArray array, jboolean *isCopy)
{
	STATS(jniinvokation();)

    if (isCopy)
		*isCopy = JNI_FALSE;

    return array->data;
}


jlong *GetLongArrayElements(JNIEnv *env, jlongArray array, jboolean *isCopy)
{
	STATS(jniinvokation();)

    if (isCopy)
		*isCopy = JNI_FALSE;

    return array->data;
}


jfloat *GetFloatArrayElements(JNIEnv *env, jfloatArray array, jboolean *isCopy)
{
	STATS(jniinvokation();)

    if (isCopy)
		*isCopy = JNI_FALSE;

    return array->data;
}


jdouble *GetDoubleArrayElements(JNIEnv *env, jdoubleArray array,
								jboolean *isCopy)
{
	STATS(jniinvokation();)

    if (isCopy)
		*isCopy = JNI_FALSE;

    return array->data;
}


/* Release<PrimitiveType>ArrayElements *****************************************

   A family of functions that informs the VM that the native code no
   longer needs access to elems. The elems argument is a pointer
   derived from array using the corresponding
   Get<PrimitiveType>ArrayElements() function. If necessary, this
   function copies back all changes made to elems to the original
   array.

*******************************************************************************/

void ReleaseBooleanArrayElements(JNIEnv *env, jbooleanArray array,
								 jboolean *elems, jint mode)
{
	STATS(jniinvokation();)

	if (elems != array->data) {
		switch (mode) {
		case JNI_COMMIT:
			MCOPY(array->data, elems, jboolean, array->header.size);
			break;
		case 0:
			MCOPY(array->data, elems, jboolean, array->header.size);
			/* XXX TWISTI how should it be freed? */
			break;
		case JNI_ABORT:
			/* XXX TWISTI how should it be freed? */
			break;
		}
	}
}


void ReleaseByteArrayElements(JNIEnv *env, jbyteArray array, jbyte *elems,
							  jint mode)
{
	STATS(jniinvokation();)

	if (elems != array->data) {
		switch (mode) {
		case JNI_COMMIT:
			MCOPY(array->data, elems, jboolean, array->header.size);
			break;
		case 0:
			MCOPY(array->data, elems, jboolean, array->header.size);
			/* XXX TWISTI how should it be freed? */
			break;
		case JNI_ABORT:
			/* XXX TWISTI how should it be freed? */
			break;
		}
	}
}


void ReleaseCharArrayElements(JNIEnv *env, jcharArray array, jchar *elems,
							  jint mode)
{
	STATS(jniinvokation();)

	if (elems != array->data) {
		switch (mode) {
		case JNI_COMMIT:
			MCOPY(array->data, elems, jboolean, array->header.size);
			break;
		case 0:
			MCOPY(array->data, elems, jboolean, array->header.size);
			/* XXX TWISTI how should it be freed? */
			break;
		case JNI_ABORT:
			/* XXX TWISTI how should it be freed? */
			break;
		}
	}
}


void ReleaseShortArrayElements(JNIEnv *env, jshortArray array, jshort *elems,
							   jint mode)
{
	STATS(jniinvokation();)

	if (elems != array->data) {
		switch (mode) {
		case JNI_COMMIT:
			MCOPY(array->data, elems, jboolean, array->header.size);
			break;
		case 0:
			MCOPY(array->data, elems, jboolean, array->header.size);
			/* XXX TWISTI how should it be freed? */
			break;
		case JNI_ABORT:
			/* XXX TWISTI how should it be freed? */
			break;
		}
	}
}


void ReleaseIntArrayElements(JNIEnv *env, jintArray array, jint *elems,
							 jint mode)
{
	STATS(jniinvokation();)

	if (elems != array->data) {
		switch (mode) {
		case JNI_COMMIT:
			MCOPY(array->data, elems, jboolean, array->header.size);
			break;
		case 0:
			MCOPY(array->data, elems, jboolean, array->header.size);
			/* XXX TWISTI how should it be freed? */
			break;
		case JNI_ABORT:
			/* XXX TWISTI how should it be freed? */
			break;
		}
	}
}


void ReleaseLongArrayElements(JNIEnv *env, jlongArray array, jlong *elems,
							  jint mode)
{
	STATS(jniinvokation();)

	if (elems != array->data) {
		switch (mode) {
		case JNI_COMMIT:
			MCOPY(array->data, elems, jboolean, array->header.size);
			break;
		case 0:
			MCOPY(array->data, elems, jboolean, array->header.size);
			/* XXX TWISTI how should it be freed? */
			break;
		case JNI_ABORT:
			/* XXX TWISTI how should it be freed? */
			break;
		}
	}
}


void ReleaseFloatArrayElements(JNIEnv *env, jfloatArray array, jfloat *elems,
							   jint mode)
{
	STATS(jniinvokation();)

	if (elems != array->data) {
		switch (mode) {
		case JNI_COMMIT:
			MCOPY(array->data, elems, jboolean, array->header.size);
			break;
		case 0:
			MCOPY(array->data, elems, jboolean, array->header.size);
			/* XXX TWISTI how should it be freed? */
			break;
		case JNI_ABORT:
			/* XXX TWISTI how should it be freed? */
			break;
		}
	}
}


void ReleaseDoubleArrayElements(JNIEnv *env, jdoubleArray array,
								jdouble *elems, jint mode)
{
	STATS(jniinvokation();)

	if (elems != array->data) {
		switch (mode) {
		case JNI_COMMIT:
			MCOPY(array->data, elems, jboolean, array->header.size);
			break;
		case 0:
			MCOPY(array->data, elems, jboolean, array->header.size);
			/* XXX TWISTI how should it be freed? */
			break;
		case JNI_ABORT:
			/* XXX TWISTI how should it be freed? */
			break;
		}
	}
}


/*  Get<PrimitiveType>ArrayRegion **********************************************

	A family of functions that copies a region of a primitive array
	into a buffer.

*******************************************************************************/

void GetBooleanArrayRegion(JNIEnv *env, jbooleanArray array, jsize start,
						   jsize len, jboolean *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(buf, &array->data[start], jboolean, len);
}


void GetByteArrayRegion(JNIEnv *env, jbyteArray array, jsize start, jsize len,
						jbyte *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size) 
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(buf, &array->data[start], jbyte, len);
}


void GetCharArrayRegion(JNIEnv *env, jcharArray array, jsize start, jsize len,
						jchar *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(buf, &array->data[start], jchar, len);
}


void GetShortArrayRegion(JNIEnv *env, jshortArray array, jsize start,
						 jsize len, jshort *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else	
		MCOPY(buf, &array->data[start], jshort, len);
}


void GetIntArrayRegion(JNIEnv *env, jintArray array, jsize start, jsize len,
					   jint *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(buf, &array->data[start], jint, len);
}


void GetLongArrayRegion(JNIEnv *env, jlongArray array, jsize start, jsize len,
						jlong *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(buf, &array->data[start], jlong, len);
}


void GetFloatArrayRegion(JNIEnv *env, jfloatArray array, jsize start,
						 jsize len, jfloat *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(buf, &array->data[start], jfloat, len);
}


void GetDoubleArrayRegion(JNIEnv *env, jdoubleArray array, jsize start,
						  jsize len, jdouble *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start+len>array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(buf, &array->data[start], jdouble, len);
}


/*  Set<PrimitiveType>ArrayRegion **********************************************

	A family of functions that copies back a region of a primitive
	array from a buffer.

*******************************************************************************/

void SetBooleanArrayRegion(JNIEnv *env, jbooleanArray array, jsize start,
						   jsize len, jboolean *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(&array->data[start], buf, jboolean, len);
}


void SetByteArrayRegion(JNIEnv *env, jbyteArray array, jsize start, jsize len,
						jbyte *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(&array->data[start], buf, jbyte, len);
}


void SetCharArrayRegion(JNIEnv *env, jcharArray array, jsize start, jsize len,
						jchar *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(&array->data[start], buf, jchar, len);

}


void SetShortArrayRegion(JNIEnv *env, jshortArray array, jsize start,
						 jsize len, jshort *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(&array->data[start], buf, jshort, len);
}


void SetIntArrayRegion(JNIEnv *env, jintArray array, jsize start, jsize len,
					   jint *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(&array->data[start], buf, jint, len);

}


void SetLongArrayRegion(JNIEnv* env, jlongArray array, jsize start, jsize len,
						jlong *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(&array->data[start], buf, jlong, len);

}


void SetFloatArrayRegion(JNIEnv *env, jfloatArray array, jsize start,
						 jsize len, jfloat *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(&array->data[start], buf, jfloat, len);

}


void SetDoubleArrayRegion(JNIEnv *env, jdoubleArray array, jsize start,
						  jsize len, jdouble *buf)
{
	STATS(jniinvokation();)

    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		MCOPY(&array->data[start], buf, jdouble, len);
}


/* Registering Native Methods *************************************************/

/* RegisterNatives *************************************************************

   Registers native methods with the class specified by the clazz
   argument. The methods parameter specifies an array of
   JNINativeMethod structures that contain the names, signatures, and
   function pointers of the native methods. The nMethods parameter
   specifies the number of native methods in the array.

*******************************************************************************/

jint RegisterNatives(JNIEnv *env, jclass clazz, const JNINativeMethod *methods,
					 jint nMethods)
{
	STATS(jniinvokation();)

    log_text("JNI-Call: RegisterNatives: IMPLEMENT ME!!!");

    return 0;
}


/* UnregisterNatives ***********************************************************

   Unregisters native methods of a class. The class goes back to the
   state before it was linked or registered with its native method
   functions.

   This function should not be used in normal native code. Instead, it
   provides special programs a way to reload and relink native
   libraries.

*******************************************************************************/

jint UnregisterNatives(JNIEnv *env, jclass clazz)
{
	STATS(jniinvokation();)

	/* XXX TWISTI hmm, maybe we should not support that (like kaffe) */

    log_text("JNI-Call: UnregisterNatives: IMPLEMENT ME!!!");

    return 0;
}


/* Monitor Operations *********************************************************/

/* MonitorEnter ****************************************************************

   Enters the monitor associated with the underlying Java object
   referred to by obj.

*******************************************************************************/

jint MonitorEnter(JNIEnv *env, jobject obj)
{
	STATS(jniinvokation();)
	if (!obj) {
		*exceptionptr = new_nullpointerexception();
		return JNI_ERR;
	}

#if defined(USE_THREADS)
	builtin_monitorenter(obj);
#endif

	return JNI_OK;
}


/* MonitorExit *****************************************************************

   The current thread must be the owner of the monitor associated with
   the underlying Java object referred to by obj. The thread
   decrements the counter indicating the number of times it has
   entered this monitor. If the value of the counter becomes zero, the
   current thread releases the monitor.

*******************************************************************************/

jint MonitorExit(JNIEnv *env, jobject obj)
{
	STATS(jniinvokation();)
	if (!obj) {
		*exceptionptr = new_nullpointerexception();
		return JNI_ERR;
	}

#if defined(USE_THREADS)
	builtin_monitorexit(obj);
#endif

	return JNI_OK;
}


/* JavaVM Interface ***********************************************************/

/* GetJavaVM *******************************************************************

   Returns the Java VM interface (used in the Invocation API)
   associated with the current thread. The result is placed at the
   location pointed to by the second argument, vm.

*******************************************************************************/

jint GetJavaVM(JNIEnv *env, JavaVM **vm)
{
	STATS(jniinvokation();)
    *vm = &ptr_jvm;

	return 0;
}


void GetStringRegion (JNIEnv* env, jstring str, jsize start, jsize len, jchar *buf)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: GetStringRegion");
}


void GetStringUTFRegion (JNIEnv* env, jstring str, jsize start, jsize len, char *buf)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: GetStringUTFRegion");
}


/* GetPrimitiveArrayCritical ***************************************************

   Obtain a direct pointer to array elements.

*******************************************************************************/

void *GetPrimitiveArrayCritical(JNIEnv *env, jarray array, jboolean *isCopy)
{
	java_objectheader *s;
	arraydescriptor   *desc;

	STATS(jniinvokation();)

	s = (java_objectheader *) array;
	desc = s->vftbl->arraydesc;

	if (!desc)
		return NULL;

	if (isCopy)
		*isCopy = JNI_FALSE;

	/* TODO add to global refs */

	return ((u1 *) s) + desc->dataoffset;
}


/* ReleasePrimitiveArrayCritical ***********************************************

   No specific documentation.

*******************************************************************************/

void ReleasePrimitiveArrayCritical(JNIEnv *env, jarray array, void *carray,
								   jint mode)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: ReleasePrimitiveArrayCritical: IMPLEMENT ME!!!");

	/* TODO remove from global refs */
}


/* GetStringCritical ***********************************************************

   The semantics of these two functions are similar to the existing
   Get/ReleaseStringChars functions.

*******************************************************************************/

const jchar *GetStringCritical(JNIEnv *env, jstring string, jboolean *isCopy)
{
	STATS(jniinvokation();)

	return GetStringChars(env, string, isCopy);
}


void ReleaseStringCritical(JNIEnv *env, jstring string, const jchar *cstring)
{
	STATS(jniinvokation();)

	ReleaseStringChars(env, string, cstring);
}


jweak NewWeakGlobalRef (JNIEnv* env, jobject obj)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: NewWeakGlobalRef");

	return obj;
}


void DeleteWeakGlobalRef (JNIEnv* env, jweak ref)
{
	STATS(jniinvokation();)
	log_text("JNI-Call: DeleteWeakGlobalRef");

	/* empty */
}


/** Creates a new global reference to the object referred to by the obj argument **/
    
jobject NewGlobalRef(JNIEnv* env, jobject lobj)
{
	jobject refcount;
	jint val;
	jobject newval;
	STATS(jniinvokation();)

	MonitorEnter(env, *global_ref_table);
	
	refcount = CallObjectMethod(env, *global_ref_table, getmid, lobj);
	val = (refcount == NULL) ? 0 : CallIntMethod(env, refcount, intvalue);
	newval = NewObject(env, intclass, newint, val + 1);

	if (newval != NULL) {
		CallObjectMethod(env, *global_ref_table, putmid, lobj, newval);
		MonitorExit(env, *global_ref_table);
		return lobj;

	} else {
		log_text("JNI-NewGlobalRef: unable to create new java.lang.Integer");
		MonitorExit(env, *global_ref_table);
		return NULL;
	}
}

/*************  Deletes the global reference pointed to by globalRef **************/

void DeleteGlobalRef(JNIEnv* env, jobject gref)
{
	jobject refcount;
	jint val;
	STATS(jniinvokation();)

	MonitorEnter(env, *global_ref_table);
	refcount = CallObjectMethod(env, *global_ref_table, getmid, gref);

	if (refcount == NULL) {
		log_text("JNI-DeleteGlobalRef: unable to find global reference");
		return;
	}

	val = CallIntMethod(env, refcount, intvalue);
	val--;

	if (val == 0) {
		CallObjectMethod(env, *global_ref_table, removemid,refcount);

	} else {
		jobject newval = NewObject(env, intclass, newint, val);

		if (newval != NULL) {
			CallObjectMethod(env,*global_ref_table, putmid,newval);

		} else {
			log_text("JNI-DeleteGlobalRef: unable to create new java.lang.Integer");
		}
	}

	MonitorExit(env,*global_ref_table);
}


/* ExceptionCheck **************************************************************

   Returns JNI_TRUE when there is a pending exception; otherwise,
   returns JNI_FALSE.

*******************************************************************************/

jboolean ExceptionCheck(JNIEnv *env)
{
	STATS(jniinvokation();)
	return *exceptionptr ? JNI_TRUE : JNI_FALSE;
}


/* New JNI 1.4 functions ******************************************************/

/* NewDirectByteBuffer *********************************************************

   Allocates and returns a direct java.nio.ByteBuffer referring to the
   block of memory starting at the memory address address and
   extending capacity bytes.

*******************************************************************************/

jobject NewDirectByteBuffer(JNIEnv *env, void *address, jlong capacity)
{
	STATS(jniinvokation();)
	log_text("NewDirectByteBuffer: IMPLEMENT ME!");

	return NULL;
}


/* GetDirectBufferAddress ******************************************************

   Fetches and returns the starting address of the memory region
   referenced by the given direct java.nio.Buffer.

*******************************************************************************/

void *GetDirectBufferAddress(JNIEnv *env, jobject buf)
{
	STATS(jniinvokation();)
	log_text("GetDirectBufferAddress: IMPLEMENT ME!");

	return NULL;
}


/* GetDirectBufferCapacity *****************************************************

   Fetches and returns the capacity in bytes of the memory region
   referenced by the given direct java.nio.Buffer.

*******************************************************************************/

jlong GetDirectBufferCapacity(JNIEnv* env, jobject buf)
{
	STATS(jniinvokation();)
	log_text("GetDirectBufferCapacity: IMPLEMENT ME!");

	return 0;
}


jint DestroyJavaVM(JavaVM *vm)
{
	STATS(jniinvokation();)
	log_text("DestroyJavaVM called");

	return 0;
}


/* AttachCurrentThread *********************************************************

   Attaches the current thread to a Java VM. Returns a JNI interface
   pointer in the JNIEnv argument.

   Trying to attach a thread that is already attached is a no-op.

   A native thread cannot be attached simultaneously to two Java VMs.

   When a thread is attached to the VM, the context class loader is
   the bootstrap loader.

*******************************************************************************/

jint AttachCurrentThread(JavaVM *vm, void **env, void *thr_args)
{
	STATS(jniinvokation();)

	log_text("AttachCurrentThread called");

#if !defined(HAVE___THREAD)
/*	cacao_thread_attach();*/
#else
	#error "No idea how to implement that. Perhaps Stefan knows"
#endif

	*env = &ptr_env;

	return 0;
}


jint DetachCurrentThread(JavaVM *vm)
{
	STATS(jniinvokation();)
	log_text("DetachCurrentThread called");

	return 0;
}


/* GetEnv **********************************************************************

   If the current thread is not attached to the VM, sets *env to NULL,
   and returns JNI_EDETACHED. If the specified version is not
   supported, sets *env to NULL, and returns JNI_EVERSION. Otherwise,
   sets *env to the appropriate interface, and returns JNI_OK.

*******************************************************************************/

jint GetEnv(JavaVM *vm, void **env, jint version)
{
	STATS(jniinvokation();)

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	if (thread_getself() == NULL) {
		*env = NULL;
		return JNI_EDETACHED;
	}
#endif

	if ((version == JNI_VERSION_1_1) || (version == JNI_VERSION_1_2) ||
		(version == JNI_VERSION_1_4)) {
		*env = &ptr_env;
		return JNI_OK;
	}

	if (version == JVMTI_VERSION_1_0) {
		*env = (void*)new_jvmtienv();
		if (env != NULL) return JNI_OK;
	}
	
	*env = NULL;
	return JNI_EVERSION;
}



jint AttachCurrentThreadAsDaemon(JavaVM *vm, void **par1, void *par2)
{
	STATS(jniinvokation();)
	log_text("AttachCurrentThreadAsDaemon called");

	return 0;
}

/************* JNI Initialization ****************************************************/

jobject jni_init1(JNIEnv* env, jobject lobj) {
#if defined(USE_THREADS)
	while (initrunning) {yieldThread();} /* wait until init is done */
#endif
	if (global_ref_table == NULL) {
		jni_init();
	} 
#if defined(USE_THREADS)
	else {
		/* wait until jni_init is done */
		MonitorEnter(env, *global_ref_table) ;
		MonitorExit(env, *global_ref_table);
	}
#endif
	return NewGlobalRef(env, lobj); 
}
void jni_init2(JNIEnv* env, jobject gref) {
	log_text("DeleteGlobalref called before NewGlobalref");
#if defined(USE_THREADS)
	while (initrunning) {yieldThread();} /* wait until init is done */
#endif
	if (global_ref_table == NULL) {
		jni_init();
	} 
#if defined(USE_THREADS)
	else {
		/* wait until jni_init is done */
		MonitorEnter(env, *global_ref_table) ;
		MonitorExit(env, *global_ref_table);
	}
#endif
	DeleteGlobalRef(env, gref); 
}


void jni_init(void)
{
	jmethodID mid;

	initrunning = true;

	/* initalize global reference table */
	ihmclass = FindClass(NULL, "java/util/IdentityHashMap");
	
	if (ihmclass == NULL) {
		log_text("JNI-Init: unable to find java.util.IdentityHashMap");
	}

	mid = GetMethodID(NULL, ihmclass, "<init>","()V");
	if (mid == NULL) {
		log_text("JNI-Init: unable to find constructor in java.util.IdentityHashMap");
	}
	
	global_ref_table = (jobject*)heap_allocate(sizeof(jobject),true,NULL);

	*global_ref_table = NewObject(NULL,ihmclass,mid);

	if (*global_ref_table == NULL) {
		log_text("JNI-Init: unable to create new global_ref_table");
	}

	initrunning = false;

	getmid = GetMethodID(NULL, ihmclass, "get","(Ljava/lang/Object;)Ljava/lang/Object;");
	if (mid == NULL) {
		log_text("JNI-Init: unable to find method \"get\" in java.util.IdentityHashMap");
	}

	getmid = GetMethodID(NULL ,ihmclass, "get","(Ljava/lang/Object;)Ljava/lang/Object;");
	if (getmid == NULL) {
		log_text("JNI-Init: unable to find method \"get\" in java.util.IdentityHashMap");
	}

	putmid = GetMethodID(NULL, ihmclass, "put","(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	if (putmid == NULL) {
		log_text("JNI-Init: unable to find method \"put\" in java.util.IdentityHashMap");
	}

	intclass = FindClass(NULL, "java/lang/Integer");
	if (intclass == NULL) {
		log_text("JNI-Init: unable to find java.lang.Integer");
	}

	newint = GetMethodID(NULL, intclass, "<init>","(I)V");
	if (newint == NULL) {
		log_text("JNI-Init: unable to find constructor in java.lang.Integer");
	}

	intvalue = GetMethodID(NULL, intclass, "intValue","()I");
	if (intvalue == NULL) {
		log_text("JNI-Init: unable to find method \"intValue\" in java.lang.Integer");
	}

	removemid = GetMethodID(NULL, ihmclass, "remove","(Ljava/lang/Object;)Ljava/lang/Object;");
	if (removemid == NULL) {
		log_text("JNI-DeleteGlobalRef: unable to find method \"remove\" in java.lang.Object");
	}
	
	/* set NewGlobalRef, DeleteGlobalRef envTable entry to real implementation */

	JNI_JNIEnvTable.NewGlobalRef = &NewGlobalRef;
	JNI_JNIEnvTable.DeleteGlobalRef = &DeleteGlobalRef;
}


/* JNI invocation table *******************************************************/

const struct JNIInvokeInterface JNI_JavaVMTable = {
	NULL,
	NULL,
	NULL,

	DestroyJavaVM,
	AttachCurrentThread,
	DetachCurrentThread,
	GetEnv,
	AttachCurrentThreadAsDaemon
};


/* JNI function table *********************************************************/

struct JNINativeInterface JNI_JNIEnvTable = {
	NULL,
	NULL,
	NULL,
	NULL,    
	&GetVersion,

	&DefineClass,
	&FindClass,
	&FromReflectedMethod,
	&FromReflectedField,
	&ToReflectedMethod,
	&GetSuperclass,
	&IsAssignableFrom,
	&ToReflectedField,

	&Throw,
	&ThrowNew,
	&ExceptionOccurred,
	&ExceptionDescribe,
	&ExceptionClear,
	&FatalError,
	&PushLocalFrame,
	&PopLocalFrame,

	&jni_init1, /* &NewGlobalRef,    initialize Global_Ref_Table*/
	&jni_init2, /* &DeleteGlobalRef,*/
	&DeleteLocalRef,
	&IsSameObject,
	&NewLocalRef,
	&EnsureLocalCapacity,

	&AllocObject,
	&NewObject,
	&NewObjectV,
	&NewObjectA,

	&GetObjectClass,
	&IsInstanceOf,

	&GetMethodID,

	&CallObjectMethod,
	&CallObjectMethodV,
	&CallObjectMethodA,
	&CallBooleanMethod,
	&CallBooleanMethodV,
	&CallBooleanMethodA,
	&CallByteMethod,
	&CallByteMethodV,
	&CallByteMethodA,
	&CallCharMethod,
	&CallCharMethodV,
	&CallCharMethodA,
	&CallShortMethod,
	&CallShortMethodV,
	&CallShortMethodA,
	&CallIntMethod,
	&CallIntMethodV,
	&CallIntMethodA,
	&CallLongMethod,
	&CallLongMethodV,
	&CallLongMethodA,
	&CallFloatMethod,
	&CallFloatMethodV,
	&CallFloatMethodA,
	&CallDoubleMethod,
	&CallDoubleMethodV,
	&CallDoubleMethodA,
	&CallVoidMethod,
	&CallVoidMethodV,
	&CallVoidMethodA,

	&CallNonvirtualObjectMethod,
	&CallNonvirtualObjectMethodV,
	&CallNonvirtualObjectMethodA,
	&CallNonvirtualBooleanMethod,
	&CallNonvirtualBooleanMethodV,
	&CallNonvirtualBooleanMethodA,
	&CallNonvirtualByteMethod,
	&CallNonvirtualByteMethodV,
	&CallNonvirtualByteMethodA,
	&CallNonvirtualCharMethod,
	&CallNonvirtualCharMethodV,
	&CallNonvirtualCharMethodA,
	&CallNonvirtualShortMethod,
	&CallNonvirtualShortMethodV,
	&CallNonvirtualShortMethodA,
	&CallNonvirtualIntMethod,
	&CallNonvirtualIntMethodV,
	&CallNonvirtualIntMethodA,
	&CallNonvirtualLongMethod,
	&CallNonvirtualLongMethodV,
	&CallNonvirtualLongMethodA,
	&CallNonvirtualFloatMethod,
	&CallNonvirtualFloatMethodV,
	&CallNonvirtualFloatMethodA,
	&CallNonvirtualDoubleMethod,
	&CallNonvirtualDoubleMethodV,
	&CallNonvirtualDoubleMethodA,
	&CallNonvirtualVoidMethod,
	&CallNonvirtualVoidMethodV,
	&CallNonvirtualVoidMethodA,

	&GetFieldID,

	&GetObjectField,
	&GetBooleanField,
	&GetByteField,
	&GetCharField,
	&GetShortField,
	&GetIntField,
	&GetLongField,
	&GetFloatField,
	&GetDoubleField,
	&SetObjectField,
	&SetBooleanField,
	&SetByteField,
	&SetCharField,
	&SetShortField,
	&SetIntField,
	&SetLongField,
	&SetFloatField,
	&SetDoubleField,

	&GetStaticMethodID,

	&CallStaticObjectMethod,
	&CallStaticObjectMethodV,
	&CallStaticObjectMethodA,
	&CallStaticBooleanMethod,
	&CallStaticBooleanMethodV,
	&CallStaticBooleanMethodA,
	&CallStaticByteMethod,
	&CallStaticByteMethodV,
	&CallStaticByteMethodA,
	&CallStaticCharMethod,
	&CallStaticCharMethodV,
	&CallStaticCharMethodA,
	&CallStaticShortMethod,
	&CallStaticShortMethodV,
	&CallStaticShortMethodA,
	&CallStaticIntMethod,
	&CallStaticIntMethodV,
	&CallStaticIntMethodA,
	&CallStaticLongMethod,
	&CallStaticLongMethodV,
	&CallStaticLongMethodA,
	&CallStaticFloatMethod,
	&CallStaticFloatMethodV,
	&CallStaticFloatMethodA,
	&CallStaticDoubleMethod,
	&CallStaticDoubleMethodV,
	&CallStaticDoubleMethodA,
	&CallStaticVoidMethod,
	&CallStaticVoidMethodV,
	&CallStaticVoidMethodA,

	&GetStaticFieldID,

	&GetStaticObjectField,
	&GetStaticBooleanField,
	&GetStaticByteField,
	&GetStaticCharField,
	&GetStaticShortField,
	&GetStaticIntField,
	&GetStaticLongField,
	&GetStaticFloatField,
	&GetStaticDoubleField,
	&SetStaticObjectField,
	&SetStaticBooleanField,
	&SetStaticByteField,
	&SetStaticCharField,
	&SetStaticShortField,
	&SetStaticIntField,
	&SetStaticLongField,
	&SetStaticFloatField,
	&SetStaticDoubleField,

	&NewString,
	&GetStringLength,
	&GetStringChars,
	&ReleaseStringChars,

	&NewStringUTF,
	&GetStringUTFLength,
	&GetStringUTFChars,
	&ReleaseStringUTFChars,

	&GetArrayLength,

	&NewObjectArray,
	&GetObjectArrayElement,
	&SetObjectArrayElement,

	&NewBooleanArray,
	&NewByteArray,
	&NewCharArray,
	&NewShortArray,
	&NewIntArray,
	&NewLongArray,
	&NewFloatArray,
	&NewDoubleArray,

	&GetBooleanArrayElements,
	&GetByteArrayElements,
	&GetCharArrayElements,
	&GetShortArrayElements,
	&GetIntArrayElements,
	&GetLongArrayElements,
	&GetFloatArrayElements,
	&GetDoubleArrayElements,

	&ReleaseBooleanArrayElements,
	&ReleaseByteArrayElements,
	&ReleaseCharArrayElements,
	&ReleaseShortArrayElements,
	&ReleaseIntArrayElements,
	&ReleaseLongArrayElements,
	&ReleaseFloatArrayElements,
	&ReleaseDoubleArrayElements,

	&GetBooleanArrayRegion,
	&GetByteArrayRegion,
	&GetCharArrayRegion,
	&GetShortArrayRegion,
	&GetIntArrayRegion,
	&GetLongArrayRegion,
	&GetFloatArrayRegion,
	&GetDoubleArrayRegion,
	&SetBooleanArrayRegion,
	&SetByteArrayRegion,
	&SetCharArrayRegion,
	&SetShortArrayRegion,
	&SetIntArrayRegion,
	&SetLongArrayRegion,
	&SetFloatArrayRegion,
	&SetDoubleArrayRegion,

	&RegisterNatives,
	&UnregisterNatives,

	&MonitorEnter,
	&MonitorExit,

	&GetJavaVM,

	/* new JNI 1.2 functions */

	&GetStringRegion,
	&GetStringUTFRegion,

	&GetPrimitiveArrayCritical,
	&ReleasePrimitiveArrayCritical,

	&GetStringCritical,
	&ReleaseStringCritical,

	&NewWeakGlobalRef,
	&DeleteWeakGlobalRef,

	&ExceptionCheck,

	/* new JNI 1.4 functions */

	&NewDirectByteBuffer,
	&GetDirectBufferAddress,
	&GetDirectBufferCapacity
};


/* Invocation API Functions ***************************************************/

/* JNI_GetDefaultJavaVMInitArgs ************************************************

   Returns a default configuration for the Java VM.

*******************************************************************************/

jint JNI_GetDefaultJavaVMInitArgs(void *vm_args)
{
	JDK1_1InitArgs *_vm_args = (JDK1_1InitArgs *) vm_args;

	/* GNU classpath currently supports JNI 1.2 */

	_vm_args->version = JNI_VERSION_1_2;

	return 0;
}


/* JNI_GetCreatedJavaVMs *******************************************************

   Returns all Java VMs that have been created. Pointers to VMs are written in
   the buffer vmBuf in the order they are created. At most bufLen number of
   entries will be written. The total number of created VMs is returned in
   *nVMs.

*******************************************************************************/

jint JNI_GetCreatedJavaVMs(JavaVM **vmBuf, jsize bufLen, jsize *nVMs)
{
	log_text("JNI_GetCreatedJavaVMs: IMPLEMENT ME!!!");

	return 0;
}


/* JNI_CreateJavaVM ************************************************************

   Loads and initializes a Java VM. The current thread becomes the main thread.
   Sets the env argument to the JNI interface pointer of the main thread.

*******************************************************************************/

jint JNI_CreateJavaVM(JavaVM **p_vm, JNIEnv **p_env, void *vm_args)
{
	const struct JNIInvokeInterface *vm;
	struct JNINativeInterface *env;

	vm = &JNI_JavaVMTable;
	env = &JNI_JNIEnvTable;

	*p_vm = (JavaVM *) vm;
	*p_env = (JNIEnv *) env;

	return 0;
}


jobject *jni_method_invokeNativeHelper(JNIEnv *env, methodinfo *methodID,
									   jobject obj, java_objectarray *params)
{
	jni_callblock *blk;
	jobject        o;
	s4             argcount;

	if (methodID == 0) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError); 
		return NULL;
	}

	argcount = methodID->parseddesc->paramcount;

	/* if method is non-static, remove the `this' pointer */

	if (!(methodID->flags & ACC_STATIC))
		argcount--;

	/* the method is an instance method the obj has to be an instance of the 
	   class the method belongs to. For static methods the obj parameter
	   is ignored. */

	if (!(methodID->flags & ACC_STATIC) && obj &&
		(!builtin_instanceof((java_objectheader *) obj, methodID->class))) {
		*exceptionptr =
			new_exception_message(string_java_lang_IllegalArgumentException,
											  "Object parameter of wrong type in Java_java_lang_reflect_Method_invokeNative");
		return NULL;
	}


#ifdef arglimit
	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. invokeNativeHelper does not support that");
		return NULL;
	}
#endif

	if (((params == NULL) && (argcount != 0)) ||
		(params && (params->header.size != argcount))) {
		*exceptionptr =
			new_exception(string_java_lang_IllegalArgumentException);
		return NULL;
	}


	if (!(methodID->flags & ACC_STATIC) && !obj)  {
		*exceptionptr =
			new_exception_message(string_java_lang_NullPointerException,
								  "Static mismatch in Java_java_lang_reflect_Method_invokeNative");
		return NULL;
	}

	if ((methodID->flags & ACC_STATIC) && (obj))
		obj = NULL;

	if (obj) {
		if ((methodID->flags & ACC_ABSTRACT) ||
			(methodID->class->flags & ACC_INTERFACE)) {
			methodID = get_virtual(obj, methodID);
		}
	}

	blk = MNEW(jni_callblock, /*4 */argcount + 2);

	if (!fill_callblock_from_objectarray(obj, methodID->parseddesc, blk,
										 params))
		return NULL;

	switch (methodID->parseddesc->returntype.decltype) {
	case TYPE_VOID:
		(void) asm_calljavafunction2(methodID, argcount + 1,
									 (argcount + 1) * sizeof(jni_callblock),
									 blk);
		o = NULL; /*native_new_and_init(loader_load(utf_new_char("java/lang/Void")));*/
		break;

	case PRIMITIVETYPE_INT: {
		s4 i;
		i = asm_calljavafunction2int(methodID, argcount + 1,
									 (argcount + 1) * sizeof(jni_callblock),
									 blk);

		o = native_new_and_init_int(class_java_lang_Integer, i);
	}
	break;

	case PRIMITIVETYPE_BYTE: {
		s4 i;
		i = asm_calljavafunction2int(methodID, argcount + 1,
									 (argcount + 1) * sizeof(jni_callblock),
									 blk);

/*  		o = native_new_and_init_int(class_java_lang_Byte, i); */
		o = builtin_new(class_java_lang_Byte);
		CallVoidMethod(env,
					   o,
					   class_resolvemethod(o->vftbl->class,
										   utf_init,
										   utf_byte__void),
					   i);
	}
	break;

	case PRIMITIVETYPE_CHAR: {
		s4 intVal;
		intVal = asm_calljavafunction2int(methodID,
										  argcount + 1,
										  (argcount + 1) * sizeof(jni_callblock),
										  blk);
		o = builtin_new(class_java_lang_Character);
		CallVoidMethod(env,
					   o,
					   class_resolvemethod(o->vftbl->class,
										   utf_init,
										   utf_char__void),
					   intVal);
	}
	break;

	case PRIMITIVETYPE_SHORT: {
		s4 intVal;
		intVal = asm_calljavafunction2int(methodID,
										  argcount + 1,
										  (argcount + 1) * sizeof(jni_callblock),
										  blk);
		o = builtin_new(class_java_lang_Short);
		CallVoidMethod(env,
					   o,
					   class_resolvemethod(o->vftbl->class,
										   utf_init,
										   utf_short__void),
					   intVal);
	}
	break;

	case PRIMITIVETYPE_BOOLEAN: {
		s4 intVal;
		intVal = asm_calljavafunction2int(methodID,
										  argcount + 1,
										  (argcount + 1) * sizeof(jni_callblock),
										  blk);
		o = builtin_new(class_java_lang_Boolean);
		CallVoidMethod(env,
					   o,
					   class_resolvemethod(o->vftbl->class,
										   utf_init,
										   utf_boolean__void),
					   intVal);
	}
	break;

	case 'J': {
		jlong longVal;
		longVal = asm_calljavafunction2long(methodID,
											argcount + 1,
											(argcount + 1) * sizeof(jni_callblock),
											blk);
		o = builtin_new(class_java_lang_Long);
		CallVoidMethod(env,
					   o,
					   class_resolvemethod(o->vftbl->class,
										   utf_init,
										   utf_long__void),
					   longVal);
	}
	break;

	case PRIMITIVETYPE_FLOAT: {
		jdouble floatVal;	
		floatVal = asm_calljavafunction2float(methodID,
											  argcount + 1,
											  (argcount + 1) * sizeof(jni_callblock),
											  blk);
		o = builtin_new(class_java_lang_Float);
		CallVoidMethod(env,
					   o,
					   class_resolvemethod(o->vftbl->class,
										   utf_init,
										   utf_float__void),
					   floatVal);
	}
	break;

	case PRIMITIVETYPE_DOUBLE: {
		jdouble doubleVal;
		doubleVal = asm_calljavafunction2double(methodID,
												argcount + 1,
												(argcount + 1) * sizeof(jni_callblock),
												blk);
		o = builtin_new(class_java_lang_Double);
		CallVoidMethod(env,
					   o,
					   class_resolvemethod(o->vftbl->class,
										   utf_init,
										   utf_double__void),
					   doubleVal);
	}
	break;

	case TYPE_ADR:
		o = asm_calljavafunction2(methodID, argcount + 1,
								  (argcount + 1) * sizeof(jni_callblock), blk);
		break;

	default:
		/* if this happens the exception has already been set by              */
		/* fill_callblock_from_objectarray                                    */

		MFREE(blk, jni_callblock, /*4 */ argcount+2);
		return (jobject *) 0;
	}

	MFREE(blk, jni_callblock, /* 4 */ argcount+2);

	if (*exceptionptr) {
		java_objectheader *cause;

		cause = *exceptionptr;

		/* clear exception pointer, we are calling JIT code again */

		*exceptionptr = NULL;

		*exceptionptr =
			new_exception_throwable(string_java_lang_reflect_InvocationTargetException,
									(java_lang_Throwable *) cause);
	}

	return (jobject *) o;
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
