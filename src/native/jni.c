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

   $Id: jni.c 2194 2005-04-03 16:13:27Z twisti $

*/


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

#include "native/include/java_lang_Class.h" /* for java_lang_VMClass.h */
#include "native/include/java_lang_VMClass.h"
#include "native/include/java_lang_VMClassLoader.h"

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
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/resolve.h"


/* XXX TWISTI hack: define it extern so they can be found in this file */
extern const struct JNIInvokeInterface JNI_JavaVMTable;
extern struct JNINativeInterface JNI_JNIEnvTable;

/* pointers to VM and the environment needed by GetJavaVM and GetEnv */
static JavaVM ptr_jvm = (JavaVM) &JNI_JavaVMTable;
static void* ptr_env = (void*) &JNI_JNIEnvTable;


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


/********************* accessing instance-fields **********************************/

#define setField(obj,typ,var,val) *((typ*) ((long int) obj + (long int) var->offset))=val;  
#define getField(obj,typ,var)     *((typ*) ((long int) obj + (long int) var->offset))
#define setfield_critical(clazz,obj,name,sig,jdatatype,val) setField(obj,jdatatype,getFieldID_critical(env,clazz,name,sig),val); 

static void fill_callblock(void *obj, methoddesc *descr, jni_callblock blk[], va_list data, int rettype)
{
    int cnt;
    u4 dummy;
	int i;
	typedesc *paramtype;

	if (obj) {
		/* the this pointer */
		blk[0].itemtype = TYPE_ADR;
		blk[0].item = PTR_TO_ITEM(obj);
		cnt = 1;
	} 
	else 
		cnt = 0;

	paramtype = descr->paramtypes;
	for (i=0; i<descr->paramcount; ++i,++cnt,++paramtype) {
		switch (paramtype->decltype) {
			/* primitive types */
		case PRIMITIVETYPE_BYTE:
		case PRIMITIVETYPE_CHAR:
		case PRIMITIVETYPE_SHORT: 
		case PRIMITIVETYPE_BOOLEAN: 
			blk[cnt].itemtype = TYPE_INT;
			blk[cnt].item = (u8) va_arg(data, int);
			break;

		case PRIMITIVETYPE_INT:
			blk[cnt].itemtype = TYPE_INT;
			dummy = va_arg(data, u4);
			/*printf("fill_callblock: pos:%d, value:%d\n",cnt,dummy);*/
			blk[cnt].item = (u8) dummy;
			break;

		case PRIMITIVETYPE_LONG:
			blk[cnt].itemtype = TYPE_LNG;
			blk[cnt].item = (u8) va_arg(data, jlong);
			break;

		case PRIMITIVETYPE_FLOAT:
			blk[cnt].itemtype = TYPE_FLT;
			*((jfloat *) (&blk[cnt].item)) = (jfloat) va_arg(data, jdouble);
			break;

		case PRIMITIVETYPE_DOUBLE:
			blk[cnt].itemtype = TYPE_DBL;
			*((jdouble *) (&blk[cnt].item)) = (jdouble) va_arg(data, jdouble);
			break;

		case TYPE_ADR: 
			blk[cnt].itemtype = TYPE_ADR;
			blk[cnt].item = PTR_TO_ITEM(va_arg(data, void*));
			break;
		}
	}

	/*the standard doesn't say anything about return value checking, but it appears to be usefull*/
	if (rettype != descr->returntype.decltype)
		log_text("\n====\nWarning call*Method called for function with wrong return type\n====");
}

/* XXX it could be considered if we should do typechecking here in the future */
static bool fill_callblock_objA(void *obj, methoddesc *descr, jni_callblock blk[], java_objectarray* params,
								int *rettype)
{
    jobject param;
    int cnt;
    int cnts;
	typedesc *paramtype;

    /* determine number of parameters */
	if (obj) {
		/* this pointer */
		blk[0].itemtype = TYPE_ADR;
		blk[0].item = PTR_TO_ITEM(obj);
		cnt=1;
	} else {
		cnt = 0;
	}

	paramtype = descr->paramtypes;
	for (cnts=0; cnts < descr->paramcount; ++cnts,++cnt,++paramtype) {
		switch (paramtype->type) {
			/* primitive types */
			case TYPE_INT:
			case TYPE_LONG:
			case TYPE_FLOAT:
			case TYPE_DOUBLE:
			
				param = params->data[cnts];
				if (!param)
					goto illegal_arg;

				/* internally used data type */
				blk[cnt].itemtype = paramtype->type;

				/* convert the value according to its declared type */
				switch (paramtype->decltype) {
						case PRIMITIVETYPE_BOOLEAN:
							if (param->vftbl->class == primitivetype_table[paramtype->decltype].class_wrap)
								blk[cnt].item = (u8) ((java_lang_Boolean *) param)->value;
							else
								goto illegal_arg;
							break;
						case PRIMITIVETYPE_BYTE:
							if (param->vftbl->class == primitivetype_table[paramtype->decltype].class_wrap)
								blk[cnt].item = (u8) ((java_lang_Byte *) param)->value;
							else
								goto illegal_arg;
							break;
						case PRIMITIVETYPE_CHAR:
							if (param->vftbl->class == primitivetype_table[paramtype->decltype].class_wrap)
								blk[cnt].item = (u8) ((java_lang_Character *) param)->value;
							else
								goto illegal_arg;
							break;
						case PRIMITIVETYPE_SHORT:
							if (param->vftbl->class == primitivetype_table[paramtype->decltype].class_wrap)
								blk[cnt].item = (u8) ((java_lang_Short *) param)->value;
							else if (param->vftbl->class == primitivetype_table[PRIMITIVETYPE_BYTE].class_wrap)
								blk[cnt].item = (u8) ((java_lang_Byte *) param)->value;
							else
								goto illegal_arg;
							break;
						case PRIMITIVETYPE_INT:
							if (param->vftbl->class == primitivetype_table[paramtype->decltype].class_wrap)
								blk[cnt].item = (u8) ((java_lang_Integer *) param)->value;
							else if (param->vftbl->class == primitivetype_table[PRIMITIVETYPE_SHORT].class_wrap)
								blk[cnt].item = (u8) ((java_lang_Short *) param)->value;
							else if (param->vftbl->class == primitivetype_table[PRIMITIVETYPE_BYTE].class_wrap)
								blk[cnt].item = (u8) ((java_lang_Byte *) param)->value;
							else
								goto illegal_arg;
							break;
						case PRIMITIVETYPE_LONG:
							if (param->vftbl->class == primitivetype_table[paramtype->decltype].class_wrap)
								blk[cnt].item = (u8) ((java_lang_Long *) param)->value;
							else if (param->vftbl->class == primitivetype_table[PRIMITIVETYPE_INT].class_wrap)
								blk[cnt].item = (u8) ((java_lang_Integer *) param)->value;
							else if (param->vftbl->class == primitivetype_table[PRIMITIVETYPE_SHORT].class_wrap)
								blk[cnt].item = (u8) ((java_lang_Short *) param)->value;
							else if (param->vftbl->class == primitivetype_table[PRIMITIVETYPE_BYTE].class_wrap)
								blk[cnt].item = (u8) ((java_lang_Byte *) param)->value;
							else
								goto illegal_arg;
							break;
						case PRIMITIVETYPE_FLOAT:
							if (param->vftbl->class == primitivetype_table[paramtype->decltype].class_wrap)
								*((jfloat *) (&blk[cnt].item)) = (jfloat) ((java_lang_Float *) param)->value;
							else
								goto illegal_arg;
							break;
						case PRIMITIVETYPE_DOUBLE:
							if (param->vftbl->class == primitivetype_table[paramtype->decltype].class_wrap)
								*((jdouble *) (&blk[cnt].item)) = (jdouble) ((java_lang_Float *) param)->value;
							else if (param->vftbl->class == primitivetype_table[PRIMITIVETYPE_FLOAT].class_wrap)
								*((jfloat *) (&blk[cnt].item)) = (jfloat) ((java_lang_Float *) param)->value;
							else
								goto illegal_arg;
							break;
						default:
							goto illegal_arg;
				} /* end declared type switch */
				break;
		
			case TYPE_ADDRESS:
				{
					classinfo *cls;
				   
					if (!resolve_class_from_typedesc(paramtype,true,&cls))
						return false; /* exception */
					if (params->data[cnts] != 0) {
						if (paramtype->arraydim > 0) {
							if (!builtin_arrayinstanceof(params->data[cnts], cls->vftbl))
								goto illegal_arg;
						}
						else {
							if (!builtin_instanceof(params->data[cnts], cls))
								goto illegal_arg;
						}
					}
					blk[cnt].itemtype = TYPE_ADR;
					blk[cnt].item = PTR_TO_ITEM(params->data[cnts]);
				}
				break;			

			default:
				goto illegal_arg;
		} /* end param type switch */

	} /* end param loop */

	if (rettype)
		*rettype = descr->returntype.decltype;
	return true;

illegal_arg:
	*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
	return false;
}


static jmethodID get_virtual(jobject obj,jmethodID methodID) {
	if (obj->vftbl->class==methodID->class) return methodID;
	return class_resolvemethod (obj->vftbl->class, methodID->name, methodID->descriptor);
}


static jmethodID get_nonvirtual(jclass clazz,jmethodID methodID) {
	if (clazz==methodID->class) return methodID;
/*class_resolvemethod -> classfindmethod? (JOWENN)*/
	return class_resolvemethod (clazz, methodID->name, methodID->descriptor);
}


static jobject callObjectMethod (jobject obj, jmethodID methodID, va_list args)
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

	fill_callblock(obj, methodID->parseddesc, blk, args, TYPE_ADR);
	/*      printf("parameter: obj: %p",blk[0].item); */
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

	fill_callblock(obj, methodID->parseddesc, blk, args, retType);

	/*      printf("parameter: obj: %p",blk[0].item); */
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

	fill_callblock(obj, methodID->parseddesc, blk, args, TYPE_LNG);

	/*      printf("parameter: obj: %p",blk[0].item); */
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

	fill_callblock(obj, methodID->parseddesc, blk, args, retType);

	/*      printf("parameter: obj: %p",blk[0].item); */
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
	/* GNU classpath currently supports JNI 1.2 */

	return JNI_VERSION_1_2;
}


/* Class Operations ***********************************************************/

/* DefineClass *****************************************************************

   Loads a class from a buffer of raw class data. The buffer
   containing the raw class data is not referenced by the VM after the
   DefineClass call returns, and it may be discarded if desired.

*******************************************************************************/

jclass DefineClass(JNIEnv *env, const char *name, jobject loader, const jbyte *buf, jsize bufLen)
{
	jclass c;

	c = (jclass) Java_java_lang_VMClassLoader_defineClass(env,
														  NULL,
														  (java_lang_ClassLoader *) loader,
														  javastring_new_char(name),
														  (java_bytearray *) buf,
														  0,
														  bufLen,
														  NULL);

	return c;
}


/* FindClass *******************************************************************

   This function loads a locally-defined class. It searches the
   directories and zip files specified by the CLASSPATH environment
   variable for the class with the specified name.

*******************************************************************************/

jclass FindClass(JNIEnv *env, const char *name)
{
	classinfo *c;  
  
	if (!load_class_bootstrap(utf_new_char_classname((char *) name),&c) || !link_class(c)) {
		class_remove(c);

		return NULL;
	}

	use_class_as_object(c);

  	return c;
}
  

/*******************************************************************************

	converts java.lang.reflect.Method or 
 	java.lang.reflect.Constructor object to a method ID  
  
*******************************************************************************/
  
jmethodID FromReflectedMethod(JNIEnv* env, jobject method)
{
	log_text("JNI-Call: FromReflectedMethod: IMPLEMENT ME!!!");

	return 0;
}


/* GetSuperclass ***************************************************************

   If clazz represents any class other than the class Object, then
   this function returns the object that represents the superclass of
   the class specified by clazz.

*******************************************************************************/
 
jclass GetSuperclass(JNIEnv *env, jclass sub)
{
	classinfo *c;

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
	return Java_java_lang_VMClass_isAssignableFrom(env,
												   NULL,
												   (java_lang_Class *) sup,
												   (java_lang_Class *) sub);
}


/***** converts a field ID derived from cls to a java.lang.reflect.Field object ***/

jobject ToReflectedField(JNIEnv* env, jclass cls, jfieldID fieldID, jboolean isStatic)
{
	log_text("JNI-Call: ToReflectedField: IMPLEMENT ME!!!");

	return NULL;
}


/* Throw ***********************************************************************

   Causes a java.lang.Throwable object to be thrown.

*******************************************************************************/

jint Throw(JNIEnv *env, jthrowable obj)
{
	*exceptionptr = (java_objectheader *) obj;

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
	*exceptionptr = NULL;
}


/* FatalError ******************************************************************

   Raises a fatal error and does not expect the VM to recover. This
   function does not return.

*******************************************************************************/

void FatalError(JNIEnv *env, const char *msg)
{
	throw_cacao_exception_exit(string_java_lang_InternalError, msg);
}


/******************* creates a new local reference frame **************************/ 

jint PushLocalFrame(JNIEnv* env, jint capacity)
{
	log_text("JNI-Call: PushLocalFrame: IMPLEMENT ME!");

	return 0;
}

/**************** Pops off the current local reference frame **********************/

jobject PopLocalFrame(JNIEnv* env, jobject result)
{
	log_text("JNI-Call: PopLocalFrame: IMPLEMENT ME!");

	return NULL;
}


/* DeleteLocalRef **************************************************************

   Deletes the local reference pointed to by localRef.

*******************************************************************************/

void DeleteLocalRef(JNIEnv *env, jobject localRef)
{
	log_text("JNI-Call: DeleteLocalRef: IMPLEMENT ME!");
}


/* IsSameObject ****************************************************************

   Tests whether two references refer to the same Java object.

*******************************************************************************/

jboolean IsSameObject(JNIEnv *env, jobject ref1, jobject ref2)
{
	return (ref1 == ref2);
}


/* NewLocalRef *****************************************************************

   Creates a new local reference that refers to the same object as ref.

*******************************************************************************/

jobject NewLocalRef(JNIEnv *env, jobject ref)
{
	log_text("JNI-Call: NewLocalRef: IMPLEMENT ME!");

	return ref;
}

/*********************************************************************************** 

	Ensures that at least a given number of local references can 
	be created in the current thread

 **********************************************************************************/   

jint EnsureLocalCapacity (JNIEnv* env, jint capacity)
{
	return 0; /* return 0 on success */
}


/* AllocObject *****************************************************************

   Allocates a new Java object without invoking any of the
   constructors for the object. Returns a reference to the object.

*******************************************************************************/

jobject AllocObject(JNIEnv *env, jclass clazz)
{
	java_objectheader *o;

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

	return NULL;
}


/* GetObjectClass **************************************************************

 Returns the class of an object.

*******************************************************************************/

jclass GetObjectClass(JNIEnv *env, jobject obj)
{
	classinfo *c;
	
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
	return Java_java_lang_VMClass_isInstance(env,
											 NULL,
											 (java_lang_Class *) clazz,
											 (java_lang_Object *) obj);
}


/***************** converts a java.lang.reflect.Field to a field ID ***************/
 
jfieldID FromReflectedField(JNIEnv* env, jobject field)
{
	log_text("JNI-Call: FromReflectedField");

	return 0;
}


/**********************************************************************************

	converts a method ID to a java.lang.reflect.Method or 
	java.lang.reflect.Constructor object

**********************************************************************************/

jobject ToReflectedMethod(JNIEnv* env, jclass cls, jmethodID methodID, jboolean isStatic)
{
	log_text("JNI-Call: ToReflectedMethod");

	return NULL;
}


/* GetMethodID *****************************************************************

   returns the method ID for an instance method

*******************************************************************************/

jmethodID GetMethodID(JNIEnv* env, jclass clazz, const char *name, const char *sig)
{
	jmethodID m;

 	m = class_resolvemethod(clazz, 
							utf_new_char((char *) name), 
							utf_new_char((char *) sig));

	if (!m || (m->flags & ACC_STATIC)) {
		*exceptionptr =
			new_exception_message(string_java_lang_NoSuchMethodError, name);

		return 0;
	}

	return m;
}


/******************** JNI-functions for calling instance methods ******************/

jobject CallObjectMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jobject ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallObjectMethod");*/

	va_start(vaargs, methodID);
	ret = callObjectMethod(obj, methodID, vaargs);
	va_end(vaargs);

	return ret;
}


jobject CallObjectMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	return callObjectMethod(obj,methodID,args);
}


jobject CallObjectMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallObjectMethodA");

	return NULL;
}




jboolean CallBooleanMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jboolean ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallBooleanMethod");*/

	va_start(vaargs,methodID);
	ret = (jboolean)callIntegerMethod(obj,get_virtual(obj,methodID),PRIMITIVETYPE_BOOLEAN,vaargs);
	va_end(vaargs);
	return ret;

}

jboolean CallBooleanMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	return (jboolean)callIntegerMethod(obj,get_virtual(obj,methodID),PRIMITIVETYPE_BOOLEAN,args);

}

jboolean CallBooleanMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallBooleanMethodA");

	return 0;
}

jbyte CallByteMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jbyte ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallVyteMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj,get_virtual(obj,methodID),PRIMITIVETYPE_BYTE,vaargs);
	va_end(vaargs);
	return ret;

}

jbyte CallByteMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
/*	log_text("JNI-Call: CallByteMethodV");*/
	return callIntegerMethod(obj,methodID,PRIMITIVETYPE_BYTE,args);
}


jbyte CallByteMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallByteMethodA");

	return 0;
}


jchar CallCharMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jchar ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallCharMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj, get_virtual(obj, methodID),PRIMITIVETYPE_CHAR, vaargs);
	va_end(vaargs);

	return ret;
}


jchar CallCharMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
/*	log_text("JNI-Call: CallCharMethodV");*/
	return callIntegerMethod(obj,get_virtual(obj,methodID),PRIMITIVETYPE_CHAR,args);
}


jchar CallCharMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallCharMethodA");

	return 0;
}


jshort CallShortMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jshort ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallShortMethod");*/

	va_start(vaargs, methodID);
	ret = callIntegerMethod(obj, get_virtual(obj, methodID),PRIMITIVETYPE_SHORT, vaargs);
	va_end(vaargs);

	return ret;
}


jshort CallShortMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	return callIntegerMethod(obj, get_virtual(obj, methodID),PRIMITIVETYPE_SHORT, args);
}


jshort CallShortMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallShortMethodA");

	return 0;
}



jint CallIntMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jint ret;
	va_list vaargs;

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj, get_virtual(obj, methodID),PRIMITIVETYPE_INT, vaargs);
	va_end(vaargs);

	return ret;
}


jint CallIntMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	return callIntegerMethod(obj, get_virtual(obj, methodID),PRIMITIVETYPE_INT, args);
}


jint CallIntMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallIntMethodA");

	return 0;
}



jlong CallLongMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jlong ret;
	va_list vaargs;
	
	va_start(vaargs,methodID);
	ret = callLongMethod(obj,get_virtual(obj, methodID),vaargs);
	va_end(vaargs);

	return ret;
}


jlong CallLongMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	return 	callLongMethod(obj,get_virtual(obj, methodID),args);
}


jlong CallLongMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallLongMethodA");

	return 0;
}



jfloat CallFloatMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jfloat ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallFloatMethod");*/

	va_start(vaargs,methodID);
	ret = callFloatMethod(obj, get_virtual(obj, methodID), vaargs, PRIMITIVETYPE_FLOAT);
	va_end(vaargs);

	return ret;
}


jfloat CallFloatMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallFloatMethodV");
	return callFloatMethod(obj, get_virtual(obj, methodID), args, PRIMITIVETYPE_FLOAT);
}


jfloat CallFloatMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallFloatMethodA");

	return 0;
}



jdouble CallDoubleMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jdouble ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallDoubleMethod");*/

	va_start(vaargs,methodID);
	ret = callFloatMethod(obj, get_virtual(obj, methodID), vaargs, PRIMITIVETYPE_DOUBLE);
	va_end(vaargs);

	return ret;
}


jdouble CallDoubleMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallDoubleMethodV");
	return callFloatMethod(obj, get_virtual(obj, methodID), args, PRIMITIVETYPE_DOUBLE);
}


jdouble CallDoubleMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallDoubleMethodA");
	return 0;
}



void CallVoidMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	va_list vaargs;

	va_start(vaargs,methodID);
	(void) callIntegerMethod(obj, get_virtual(obj, methodID),TYPE_VOID, vaargs);
	va_end(vaargs);
}


void CallVoidMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallVoidMethodV");
	(void)callIntegerMethod(obj,get_virtual(obj,methodID),TYPE_VOID,args);
}


void CallVoidMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallVoidMethodA");
}



jobject CallNonvirtualObjectMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualObjectMethod");

	return NULL;
}


jobject CallNonvirtualObjectMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualObjectMethodV");

	return NULL;
}


jobject CallNonvirtualObjectMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallNonvirtualObjectMethodA");

	return NULL;
}



jboolean CallNonvirtualBooleanMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jboolean ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallNonvirtualBooleanMethod");*/

	va_start(vaargs,methodID);
	ret = (jboolean)callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_BOOLEAN,vaargs);
	va_end(vaargs);
	return ret;

}


jboolean CallNonvirtualBooleanMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
/*	log_text("JNI-Call: CallNonvirtualBooleanMethodV");*/
	return (jboolean)callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_BOOLEAN,args);
}


jboolean CallNonvirtualBooleanMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallNonvirtualBooleanMethodA");

	return 0;
}



jbyte CallNonvirtualByteMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jbyte ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallNonvirutalByteMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_BYTE,vaargs);
	va_end(vaargs);
	return ret;
}


jbyte CallNonvirtualByteMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	/*log_text("JNI-Call: CallNonvirtualByteMethodV"); */
	return callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_BYTE,args);

}


jbyte CallNonvirtualByteMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualByteMethodA");

	return 0;
}



jchar CallNonvirtualCharMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jchar ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallNonVirtualCharMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_CHAR,vaargs);
	va_end(vaargs);
	return ret;
}


jchar CallNonvirtualCharMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	/*log_text("JNI-Call: CallNonvirtualCharMethodV");*/
	return callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_CHAR,args);
}


jchar CallNonvirtualCharMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualCharMethodA");

	return 0;
}



jshort CallNonvirtualShortMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jshort ret;
	va_list vaargs;

	/*log_text("JNI-Call: CallNonvirtualShortMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_SHORT,vaargs);
	va_end(vaargs);
	return ret;
}


jshort CallNonvirtualShortMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	/*log_text("JNI-Call: CallNonvirtualShortMethodV");*/
	return callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_SHORT,args);
}


jshort CallNonvirtualShortMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualShortMethodA");

	return 0;
}



jint CallNonvirtualIntMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{

        jint ret;
        va_list vaargs;

	/*log_text("JNI-Call: CallNonvirtualIntMethod");*/

        va_start(vaargs,methodID);
        ret = callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_INT,vaargs);
        va_end(vaargs);
        return ret;
}


jint CallNonvirtualIntMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	/*log_text("JNI-Call: CallNonvirtualIntMethodV");*/
        return callIntegerMethod(obj,get_nonvirtual(clazz,methodID),PRIMITIVETYPE_INT,args);
}


jint CallNonvirtualIntMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualIntMethodA");

	return 0;
}



jlong CallNonvirtualLongMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualLongMethod");

	return 0;
}


jlong CallNonvirtualLongMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualLongMethodV");

	return 0;
}


jlong CallNonvirtualLongMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualLongMethodA");

	return 0;
}



jfloat CallNonvirtualFloatMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jfloat ret;
	va_list vaargs;

	/*log_text("JNI-Call: CallNonvirtualFloatMethod");*/


	va_start(vaargs,methodID);
	ret = callFloatMethod(obj,get_nonvirtual(clazz,methodID),vaargs,PRIMITIVETYPE_FLOAT);
	va_end(vaargs);
	return ret;

}


jfloat CallNonvirtualFloatMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualFloatMethodV");
	return callFloatMethod(obj,get_nonvirtual(clazz,methodID),args,PRIMITIVETYPE_FLOAT);
}


jfloat CallNonvirtualFloatMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualFloatMethodA");

	return 0;
}



jdouble CallNonvirtualDoubleMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jdouble ret;
	va_list vaargs;
	log_text("JNI-Call: CallNonvirtualDoubleMethod");

	va_start(vaargs,methodID);
	ret = callFloatMethod(obj,get_nonvirtual(clazz,methodID),vaargs,PRIMITIVETYPE_DOUBLE);
	va_end(vaargs);
	return ret;

}


jdouble CallNonvirtualDoubleMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
/*	log_text("JNI-Call: CallNonvirtualDoubleMethodV");*/
	return callFloatMethod(obj,get_nonvirtual(clazz,methodID),args,PRIMITIVETYPE_DOUBLE);
}


jdouble CallNonvirtualDoubleMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualDoubleMethodA");

	return 0;
}



void CallNonvirtualVoidMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
        va_list vaargs;

/*      log_text("JNI-Call: CallNonvirtualVoidMethod");*/

        va_start(vaargs,methodID);
        (void)callIntegerMethod(obj,get_nonvirtual(clazz,methodID),TYPE_VOID,vaargs);
        va_end(vaargs);

}


void CallNonvirtualVoidMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
/*	log_text("JNI-Call: CallNonvirtualVoidMethodV");*/

        (void)callIntegerMethod(obj,get_nonvirtual(clazz,methodID),TYPE_VOID,args);

}


void CallNonvirtualVoidMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallNonvirtualVoidMethodA");
}

/************************* JNI-functions for accessing fields ************************/

jfieldID GetFieldID (JNIEnv *env, jclass clazz, const char *name, const char *sig) 
{
	jfieldID f;

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

    if (!id) {
       log_text("class:");
       utf_display(clazz->name);
       log_text("\nfield:");
       log_text(name);
       log_text("sig:");
       log_text(sig);

       panic("setfield_critical failed"); 
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

	return getField(obj,jobject,fieldID);
}

jboolean GetBooleanField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jboolean,fieldID);
}


jbyte GetByteField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
       	return getField(obj,jbyte,fieldID);
}


jchar GetCharField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jchar,fieldID);
}


jshort GetShortField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jshort,fieldID);
}


jint GetIntField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jint,fieldID);
}


jlong GetLongField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jlong,fieldID);
}


jfloat GetFloatField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jfloat,fieldID);
}


jdouble GetDoubleField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jdouble,fieldID);
}

void SetObjectField (JNIEnv *env, jobject obj, jfieldID fieldID, jobject val)
{
        setField(obj,jobject,fieldID,val);
}


void SetBooleanField (JNIEnv *env, jobject obj, jfieldID fieldID, jboolean val)
{
        setField(obj,jboolean,fieldID,val);
}


void SetByteField (JNIEnv *env, jobject obj, jfieldID fieldID, jbyte val)
{
        setField(obj,jbyte,fieldID,val);
}


void SetCharField (JNIEnv *env, jobject obj, jfieldID fieldID, jchar val)
{
        setField(obj,jchar,fieldID,val);
}


void SetShortField (JNIEnv *env, jobject obj, jfieldID fieldID, jshort val)
{
        setField(obj,jshort,fieldID,val);
}


void SetIntField (JNIEnv *env, jobject obj, jfieldID fieldID, jint val)
{
        setField(obj,jint,fieldID,val);
}


void SetLongField (JNIEnv *env, jobject obj, jfieldID fieldID, jlong val)
{
        setField(obj,jlong,fieldID,val);
}


void SetFloatField (JNIEnv *env, jobject obj, jfieldID fieldID, jfloat val)
{
        setField(obj,jfloat,fieldID,val);
}


void SetDoubleField (JNIEnv *env, jobject obj, jfieldID fieldID, jdouble val)
{
        setField(obj,jdouble,fieldID,val);
}


/**************** JNI-functions for calling static methods **********************/ 

jmethodID GetStaticMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig)
{
	jmethodID m;

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

	/* log_text("JNI-Call: CallStaticObjectMethod");*/

	va_start(vaargs, methodID);
	ret = callObjectMethod(0, methodID, vaargs);
	va_end(vaargs);

	return ret;
}


jobject CallStaticObjectMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	/* log_text("JNI-Call: CallStaticObjectMethodV"); */
	
	return callObjectMethod(0,methodID,args);
}


jobject CallStaticObjectMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticObjectMethodA");

	return NULL;
}


jboolean CallStaticBooleanMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jboolean ret;
	va_list vaargs;

	va_start(vaargs, methodID);
	ret = (jboolean) callIntegerMethod(0, methodID, PRIMITIVETYPE_BOOLEAN, vaargs);
	va_end(vaargs);

	return ret;
}


jboolean CallStaticBooleanMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	return (jboolean) callIntegerMethod(0, methodID, PRIMITIVETYPE_BOOLEAN, args);
}


jboolean CallStaticBooleanMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticBooleanMethodA");

	return 0;
}


jbyte CallStaticByteMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jbyte ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticByteMethod");*/

	va_start(vaargs, methodID);
	ret = (jbyte) callIntegerMethod(0, methodID, PRIMITIVETYPE_BYTE, vaargs);
	va_end(vaargs);

	return ret;
}


jbyte CallStaticByteMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	return (jbyte) callIntegerMethod(0, methodID, PRIMITIVETYPE_BYTE, args);
}


jbyte CallStaticByteMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticByteMethodA");

	return 0;
}


jchar CallStaticCharMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jchar ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticByteMethod");*/

	va_start(vaargs, methodID);
	ret = (jchar) callIntegerMethod(0, methodID, PRIMITIVETYPE_CHAR, vaargs);
	va_end(vaargs);

	return ret;
}


jchar CallStaticCharMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	return (jchar) callIntegerMethod(0, methodID, PRIMITIVETYPE_CHAR, args);
}


jchar CallStaticCharMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticCharMethodA");

	return 0;
}



jshort CallStaticShortMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jshort ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticByteMethod");*/

	va_start(vaargs, methodID);
	ret = (jshort) callIntegerMethod(0, methodID, PRIMITIVETYPE_SHORT, vaargs);
	va_end(vaargs);

	return ret;
}


jshort CallStaticShortMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	/*log_text("JNI-Call: CallStaticShortMethodV");*/
	return (jshort) callIntegerMethod(0, methodID, PRIMITIVETYPE_SHORT, args);
}


jshort CallStaticShortMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticShortMethodA");

	return 0;
}



jint CallStaticIntMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jint ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticIntMethod");*/

	va_start(vaargs, methodID);
	ret = callIntegerMethod(0, methodID, PRIMITIVETYPE_INT, vaargs);
	va_end(vaargs);

	return ret;
}


jint CallStaticIntMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticIntMethodV");

	return callIntegerMethod(0, methodID, PRIMITIVETYPE_INT, args);
}


jint CallStaticIntMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticIntMethodA");

	return 0;
}



jlong CallStaticLongMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jlong ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticLongMethod");*/

	va_start(vaargs, methodID);
	ret = callLongMethod(0, methodID, vaargs);
	va_end(vaargs);

	return ret;
}


jlong CallStaticLongMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticLongMethodV");
	
	return callLongMethod(0,methodID,args);
}


jlong CallStaticLongMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticLongMethodA");

	return 0;
}



jfloat CallStaticFloatMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jfloat ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticLongMethod");*/

	va_start(vaargs, methodID);
	ret = callFloatMethod(0, methodID, vaargs, PRIMITIVETYPE_FLOAT);
	va_end(vaargs);

	return ret;
}


jfloat CallStaticFloatMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{

	return callFloatMethod(0, methodID, args, PRIMITIVETYPE_FLOAT);

}


jfloat CallStaticFloatMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticFloatMethodA");

	return 0;
}



jdouble CallStaticDoubleMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jdouble ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticDoubleMethod");*/

	va_start(vaargs,methodID);
	ret = callFloatMethod(0, methodID, vaargs, PRIMITIVETYPE_DOUBLE);
	va_end(vaargs);

	return ret;
}


jdouble CallStaticDoubleMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticDoubleMethodV");

	return callFloatMethod(0, methodID, args, PRIMITIVETYPE_DOUBLE);
}


jdouble CallStaticDoubleMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticDoubleMethodA");

	return 0;
}


void CallStaticVoidMethod(JNIEnv *env, jclass cls, jmethodID methodID, ...)
{
	va_list vaargs;

	va_start(vaargs, methodID);
	(void) callIntegerMethod(0, methodID, TYPE_VOID, vaargs);
	va_end(vaargs);
}


void CallStaticVoidMethodV(JNIEnv *env, jclass cls, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticVoidMethodV");
	(void)callIntegerMethod(0, methodID, TYPE_VOID, args);
}


void CallStaticVoidMethodA(JNIEnv *env, jclass cls, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallStaticVoidMethodA");
}


/****************** JNI-functions for accessing static fields ********************/

jfieldID GetStaticFieldID (JNIEnv *env, jclass clazz, const char *name, const char *sig) 
{
	jfieldID f;

	f = jclass_findfield(clazz,
			    utf_new_char ((char*) name), 
			    utf_new_char ((char*) sig)
		 	    ); 
	
	if (!f) *exceptionptr =	new_exception(string_java_lang_NoSuchFieldError);  

	return f;
}


jobject GetStaticObjectField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.a;       
}


jboolean GetStaticBooleanField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jbyte GetStaticByteField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jchar GetStaticCharField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jshort GetStaticShortField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jint GetStaticIntField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jlong GetStaticLongField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.l;
}


jfloat GetStaticFloatField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
 	return fieldID->value.f;
}


jdouble GetStaticDoubleField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.d;
}



void SetStaticObjectField (JNIEnv *env, jclass clazz, jfieldID fieldID, jobject value)
{
	class_init(clazz);
	fieldID->value.a = value;
}


void SetStaticBooleanField (JNIEnv *env, jclass clazz, jfieldID fieldID, jboolean value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticByteField (JNIEnv *env, jclass clazz, jfieldID fieldID, jbyte value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticCharField (JNIEnv *env, jclass clazz, jfieldID fieldID, jchar value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticShortField (JNIEnv *env, jclass clazz, jfieldID fieldID, jshort value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticIntField (JNIEnv *env, jclass clazz, jfieldID fieldID, jint value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticLongField (JNIEnv *env, jclass clazz, jfieldID fieldID, jlong value)
{
	class_init(clazz);
	fieldID->value.l = value;
}


void SetStaticFloatField (JNIEnv *env, jclass clazz, jfieldID fieldID, jfloat value)
{
	class_init(clazz);
	fieldID->value.f = value;
}


void SetStaticDoubleField (JNIEnv *env, jclass clazz, jfieldID fieldID, jdouble value)
{
	class_init(clazz);
	fieldID->value.d = value;
}


/*****  create new java.lang.String object from an array of Unicode characters ****/ 

jstring NewString (JNIEnv *env, const jchar *buf, jsize len)
{
	u4 i;
	java_lang_String *s;
	java_chararray *a;
	
	s = (java_lang_String*) builtin_new (class_java_lang_String);
	a = builtin_newarray_char (len);

	/* javastring or characterarray could not be created */
	if ( (!a) || (!s) ) return NULL;

	/* copy text */
	for (i=0; i<len; i++) a->data[i] = buf[i];
	s -> value = a;
	s -> offset = 0;
	s -> count = len;

	return (jstring) s;
}


static char emptyString[]="";
static jchar emptyStringJ[]={0,0};

/******************* returns the length of a Java string ***************************/

jsize GetStringLength (JNIEnv *env, jstring str)
{
	return ((java_lang_String*) str)->count;
}


/********************  convertes javastring to u2-array ****************************/
	
u2 *javastring_tou2 (jstring so) 
{
	java_lang_String *s = (java_lang_String*) so;
	java_chararray *a;
	u4 i;
	u2 *stringbuffer;
	
	if (!s) return NULL;

	a = s->value;
	if (!a) return NULL;

	/* allocate memory */
	stringbuffer = MNEW( u2 , s->count + 1 );

	/* copy text */
	for (i=0; i<s->count; i++) stringbuffer[i] = a->data[s->offset+i];
	
	/* terminate string */
	stringbuffer[i] = '\0';

	return stringbuffer;
}

/********* returns a pointer to an array of Unicode characters of the string *******/

const jchar *GetStringChars (JNIEnv *env, jstring str, jboolean *isCopy)
{	
	jchar *jc=javastring_tou2(str);

	if (jc)	{
		if (isCopy) *isCopy=JNI_TRUE;
		return jc;
	}
	if (isCopy) *isCopy=JNI_TRUE;
	return emptyStringJ;
}

/**************** native code no longer needs access to chars **********************/

void ReleaseStringChars (JNIEnv *env, jstring str, const jchar *chars)
{
	if (chars==emptyStringJ) return;
	MFREE(((jchar*) chars),jchar,((java_lang_String*) str)->count+1);
}


/* NewStringUTF ****************************************************************

   Constructs a new java.lang.String object from an array of UTF-8 characters.

*******************************************************************************/

jstring NewStringUTF(JNIEnv *env, const char *bytes)
{
    return (jstring) javastring_new(utf_new_char(bytes));
}


/****************** returns the utf8 length in bytes of a string *******************/

jsize GetStringUTFLength (JNIEnv *env, jstring string)
{   
    java_lang_String *s = (java_lang_String*) string;

    return (jsize) u2_utflength(s->value->data, s->count); 
}


/************ converts a Javastring to an array of UTF-8 characters ****************/

const char* GetStringUTFChars(JNIEnv *env, jstring string, jboolean *isCopy)
{
    utf *u;

    u = javastring_toutf((java_lang_String *) string, false);

    if (isCopy)
		*isCopy = JNI_FALSE;
	
    if (u)
		return u->text;

    return emptyString;
	
}


/***************** native code no longer needs access to utf ***********************/

void ReleaseStringUTFChars (JNIEnv *env, jstring str, const char* chars)
{
    /*we don't release utf chars right now, perhaps that should be done later. Since there is always one reference
	the garbage collector will never get them*/
	/*
    log_text("JNI-Call: ReleaseStringUTFChars");
    utf_display(utf_new_char(chars));
	*/
}

/************************** array operations ***************************************/

jsize GetArrayLength(JNIEnv *env, jarray array)
{
    return array->size;
}


jobjectArray NewObjectArray (JNIEnv *env, jsize len, jclass clazz, jobject init)
{
	java_objectarray *j;

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_anewarray(len, clazz);

    return j;
}


jobject GetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index)
{
    jobject j = NULL;

    if (index < array->header.size)	
		j = array->data[index];
    else
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);
    
    return j;
}


void SetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index, jobject val)
{
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

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_double(len);

    return j;
}


jboolean * GetBooleanArrayElements (JNIEnv *env, jbooleanArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jbyte * GetByteArrayElements (JNIEnv *env, jbyteArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jchar * GetCharArrayElements (JNIEnv *env, jcharArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jshort * GetShortArrayElements (JNIEnv *env, jshortArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jint * GetIntArrayElements (JNIEnv *env, jintArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jlong * GetLongArrayElements (JNIEnv *env, jlongArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jfloat * GetFloatArrayElements (JNIEnv *env, jfloatArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jdouble * GetDoubleArrayElements (JNIEnv *env, jdoubleArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}



void ReleaseBooleanArrayElements (JNIEnv *env, jbooleanArray array, jboolean *elems, jint mode)
{
    /* empty */
}


void ReleaseByteArrayElements (JNIEnv *env, jbyteArray array, jbyte *elems, jint mode)
{
    /* empty */
}


void ReleaseCharArrayElements (JNIEnv *env, jcharArray array, jchar *elems, jint mode)
{
    /* empty */
}


void ReleaseShortArrayElements (JNIEnv *env, jshortArray array, jshort *elems, jint mode)
{
    /* empty */
}


void ReleaseIntArrayElements (JNIEnv *env, jintArray array, jint *elems, jint mode)
{
    /* empty */
}


void ReleaseLongArrayElements (JNIEnv *env, jlongArray array, jlong *elems, jint mode)
{
    /* empty */
}


void ReleaseFloatArrayElements (JNIEnv *env, jfloatArray array, jfloat *elems, jint mode)
{
    /* empty */
}


void ReleaseDoubleArrayElements (JNIEnv *env, jdoubleArray array, jdouble *elems, jint mode)
{
    /* empty */
}


void GetBooleanArrayRegion(JNIEnv* env, jbooleanArray array, jsize start, jsize len, jboolean *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetByteArrayRegion(JNIEnv* env, jbyteArray array, jsize start, jsize len, jbyte *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size) 
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetCharArrayRegion(JNIEnv* env, jcharArray array, jsize start, jsize len, jchar *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetShortArrayRegion(JNIEnv* env, jshortArray array, jsize start, jsize len, jshort *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else	
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetIntArrayRegion(JNIEnv* env, jintArray array, jsize start, jsize len, jint *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetLongArrayRegion(JNIEnv* env, jlongArray array, jsize start, jsize len, jlong *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetFloatArrayRegion(JNIEnv* env, jfloatArray array, jsize start, jsize len, jfloat *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetDoubleArrayRegion(JNIEnv* env, jdoubleArray array, jsize start, jsize len, jdouble *buf)
{
    if (start < 0 || len < 0 || start+len>array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void SetBooleanArrayRegion(JNIEnv* env, jbooleanArray array, jsize start, jsize len, jboolean *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));
}


void SetByteArrayRegion(JNIEnv* env, jbyteArray array, jsize start, jsize len, jbyte *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));
}


void SetCharArrayRegion(JNIEnv* env, jcharArray array, jsize start, jsize len, jchar *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));

}


void SetShortArrayRegion(JNIEnv* env, jshortArray array, jsize start, jsize len, jshort *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));
}


void SetIntArrayRegion(JNIEnv* env, jintArray array, jsize start, jsize len, jint *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));

}


void SetLongArrayRegion(JNIEnv* env, jlongArray array, jsize start, jsize len, jlong *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));

}


void SetFloatArrayRegion(JNIEnv* env, jfloatArray array, jsize start, jsize len, jfloat *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));

}


void SetDoubleArrayRegion(JNIEnv* env, jdoubleArray array, jsize start, jsize len, jdouble *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));
}


jint RegisterNatives (JNIEnv* env, jclass clazz, const JNINativeMethod *methods, jint nMethods)
{
    log_text("JNI-Call: RegisterNatives");
    return 0;
}


jint UnregisterNatives (JNIEnv* env, jclass clazz)
{
    log_text("JNI-Call: UnregisterNatives");
    return 0;
}


/* Monitor Operations *********************************************************/

/* MonitorEnter ****************************************************************

   Enters the monitor associated with the underlying Java object
   referred to by obj.

*******************************************************************************/

jint MonitorEnter(JNIEnv *env, jobject obj)
{
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
    *vm = &ptr_jvm;

	return 0;
}


void GetStringRegion (JNIEnv* env, jstring str, jsize start, jsize len, jchar *buf)
{
	log_text("JNI-Call: GetStringRegion");
}


void GetStringUTFRegion (JNIEnv* env, jstring str, jsize start, jsize len, char *buf)
{
	log_text("JNI-Call: GetStringUTFRegion");
}


/************** obtain direct pointer to array elements ***********************/

void * GetPrimitiveArrayCritical (JNIEnv* env, jarray array, jboolean *isCopy)
{
	java_objectheader *s = (java_objectheader*) array;
	arraydescriptor *desc = s->vftbl->arraydesc;

	if (!desc) return NULL;

	return ((u1*)s) + desc->dataoffset;
}


void ReleasePrimitiveArrayCritical (JNIEnv* env, jarray array, void *carray, jint mode)
{
	log_text("JNI-Call: ReleasePrimitiveArrayCritical");

	/* empty */
}

/**** returns a pointer to an array of Unicode characters of the string *******/

const jchar * GetStringCritical (JNIEnv* env, jstring string, jboolean *isCopy)
{
	log_text("JNI-Call: GetStringCritical");

	return GetStringChars(env,string,isCopy);
}

/*********** native code no longer needs access to chars **********************/

void ReleaseStringCritical (JNIEnv* env, jstring string, const jchar *cstring)
{
	log_text("JNI-Call: ReleaseStringCritical");

	ReleaseStringChars(env,string,cstring);
}


jweak NewWeakGlobalRef (JNIEnv* env, jobject obj)
{
	log_text("JNI-Call: NewWeakGlobalRef");

	return obj;
}


void DeleteWeakGlobalRef (JNIEnv* env, jweak ref)
{
	log_text("JNI-Call: DeleteWeakGlobalRef");

	/* empty */
}


/** Creates a new global reference to the object referred to by the obj argument **/
    
jobject NewGlobalRef(JNIEnv* env, jobject lobj)
{
	jobject refcount;
	jint val;
	jobject newval;

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
	return *exceptionptr ? JNI_TRUE : JNI_FALSE;
}


/* New JNI 1.4 functions ******************************************************/

/* NewDirectByteBuffer *********************************************************

   Allocates and returns a direct java.nio.ByteBuffer referring to the block of
   memory starting at the memory address address and extending capacity bytes.

*******************************************************************************/

jobject NewDirectByteBuffer(JNIEnv *env, void *address, jlong capacity)
{
	log_text("NewDirectByteBuffer: IMPLEMENT ME!");

	return NULL;
}


/* GetDirectBufferAddress ******************************************************

   Fetches and returns the starting address of the memory region referenced by
   the given direct java.nio.Buffer.

*******************************************************************************/

void *GetDirectBufferAddress(JNIEnv *env, jobject buf)
{
	log_text("GetDirectBufferAddress: IMPLEMENT ME!");

	return NULL;
}


/* GetDirectBufferCapacity *****************************************************

   Fetches and returns the capacity in bytes of the memory region referenced by
   the given direct java.nio.Buffer.

*******************************************************************************/

jlong GetDirectBufferCapacity(JNIEnv* env, jobject buf)
{
	log_text("GetDirectBufferCapacity: IMPLEMENT ME!");

	return 0;
}


jint DestroyJavaVM(JavaVM *vm)
{
	log_text("DestroyJavaVM called");

	return 0;
}


jint AttachCurrentThread(JavaVM *vm, void **par1, void *par2)
{
	log_text("AttachCurrentThread called");

	return 0;
}


jint DetachCurrentThread(JavaVM *vm)
{
	log_text("DetachCurrentThread called");

	return 0;
}


jint GetEnv(JavaVM *vm, void **env, jint version)
{
	if ((version != JNI_VERSION_1_1) && (version != JNI_VERSION_1_2) &&
		(version != JNI_VERSION_1_4)) {
		*env = NULL;
		return JNI_EVERSION;
	}

	/*
	  TODO: If the current thread is not attached to the VM...
	if (0) {
		*env = NULL;
		return JNI_EDETACHED;
	}
	*/

	*env = &ptr_env;

	return JNI_OK;
}


jint AttachCurrentThreadAsDaemon(JavaVM *vm, void **par1, void *par2)
{
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

void jni_init(){
	jmethodID mid;

	initrunning = true;
	log_text("JNI-Init: initialize global_ref_table");
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
	*p_vm = (JavaVM *) &JNI_JavaVMTable;
	*p_env = (JNIEnv *) &JNI_JNIEnvTable;

	return 0;
}


jobject *jni_method_invokeNativeHelper(JNIEnv *env, struct methodinfo *methodID, jobject obj, java_objectarray *params)
{
	int argcount;
	jni_callblock *blk;
	int retT;
	jobject retVal;

	if (methodID == 0) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError); 
		return NULL;
	}

	argcount = methodID->parseddesc->paramcount;

	/* the method is an instance method the obj has to be an instance of the 
	   class the method belongs to. For static methods the obj parameter
	   is ignored. */
	if (!(methodID->flags & ACC_STATIC) && obj &&
		(!builtin_instanceof((java_objectheader *) obj, methodID->class))) {
		*exceptionptr = new_exception_message(string_java_lang_IllegalArgumentException,
											  "Object parameter of wrong type in Java_java_lang_reflect_Method_invokeNative");
		return 0;
	}


#ifdef arglimit
	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. invokeNativeHelper does not support that");
		return 0;
	}
#endif
	if (((params==0) && (argcount != 0)) || (params && (params->header.size != argcount))) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;
	}


	if (((methodID->flags & ACC_STATIC)==0) && (0==obj))  {
		*exceptionptr =
			new_exception_message(string_java_lang_NullPointerException,
								  "Static mismatch in Java_java_lang_reflect_Method_invokeNative");
		return 0;
	}

	if ((methodID->flags & ACC_STATIC) && (obj)) obj = 0;

	if (obj) {
		if ( (methodID->flags & ACC_ABSTRACT) || (methodID->class->flags & ACC_INTERFACE) ) {
			methodID=get_virtual(obj,methodID);
		}
	}

	blk = MNEW(jni_callblock, /*4 */argcount+2);

	if (!fill_callblock_objA(obj, methodID->parseddesc, blk, params,&retT))
		return 0; /* exception */

	switch (retT) {
	case TYPE_VOID:
		(void) asm_calljavafunction2(methodID,
									 argcount + 1,
									 (argcount + 1) * sizeof(jni_callblock),
									 blk);
		retVal = NULL; /*native_new_and_init(loader_load(utf_new_char("java/lang/Void")));*/
		break;

	case PRIMITIVETYPE_INT: {
		s4 intVal;
		intVal = asm_calljavafunction2int(methodID,
										  argcount + 1,
										  (argcount + 1) * sizeof(jni_callblock),
										  blk);
		retVal = builtin_new(class_java_lang_Integer);
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
										   utf_init,
										   utf_int__void),
					   intVal);
	}
	break;

	case PRIMITIVETYPE_BYTE: {
		s4 intVal;
		intVal = asm_calljavafunction2int(methodID,
										  argcount + 1,
										  (argcount + 1) * sizeof(jni_callblock),
										  blk);
		retVal = builtin_new(class_java_lang_Byte);
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
										   utf_init,
										   utf_byte__void),
					   intVal);
	}
	break;

	case PRIMITIVETYPE_CHAR: {
		s4 intVal;
		intVal = asm_calljavafunction2int(methodID,
										  argcount + 1,
										  (argcount + 1) * sizeof(jni_callblock),
										  blk);
		retVal = builtin_new(class_java_lang_Character);
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
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
		retVal = builtin_new(class_java_lang_Short);
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
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
		retVal = builtin_new(class_java_lang_Boolean);
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
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
		retVal = builtin_new(class_java_lang_Long);
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
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
		retVal = builtin_new(class_java_lang_Float);
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
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
		retVal = builtin_new(class_java_lang_Double);
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
										   utf_init,
										   utf_double__void),
					   doubleVal);
	}
	break;

	case TYPE_ADR:
		retVal = asm_calljavafunction2(methodID,
									   argcount + 1,
									   (argcount + 1) * sizeof(jni_callblock),
									   blk);
		break;

	default:
		/* if this happens the acception has already been set by fill_callblock_objA*/
		MFREE(blk, jni_callblock, /*4 */ argcount+2);
		return (jobject *) 0;
	}

	MFREE(blk, jni_callblock, /* 4 */ argcount+2);

	if (*exceptionptr) {
		java_objectheader *exceptionToWrap = *exceptionptr;
		classinfo *ivtec;
		java_objectheader *ivte;

		*exceptionptr = NULL;
		ivtec = class_new(utf_new_char("java/lang/reflect/InvocationTargetException"));
		ivte = builtin_new(ivtec);
		asm_calljavafunction(class_resolvemethod(ivtec,
												 utf_new_char("<init>"),
												 utf_new_char("(Ljava/lang/Throwable;)V")),
							 ivte,
							 exceptionToWrap,
							 0,
							 0);

		if (*exceptionptr != NULL)
			panic("jni.c: error while creating InvocationTargetException wrapper");

		*exceptionptr = ivte;
	}

	return (jobject *) retVal;
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
