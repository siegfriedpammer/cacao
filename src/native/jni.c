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

   $Id: jni.c 3438 2005-10-14 11:27:40Z twisti $

*/


#include <assert.h>
#include <string.h>

#include "config.h"

#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"

#include "native/include/gnu_classpath_Pointer.h"

#if SIZEOF_VOID_P == 8
# include "native/include/gnu_classpath_Pointer64.h"
#else
# include "native/include/gnu_classpath_Pointer32.h"
#endif

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
#include "native/include/java_nio_Buffer.h"
#include "native/include/java_nio_DirectByteBufferImpl.h"

#if defined(ENABLE_JVMTI)
# include "native/jvmti/jvmti.h"
#endif

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
void *ptr_env = (void*) &JNI_JNIEnvTable;


#define PTR_TO_ITEM(ptr)   ((u8)(size_t)(ptr))

/* global variables ***********************************************************/

/* global reference table *****************************************************/

static java_objectheader **global_ref_table;

/* jmethodID and jclass caching variables for NewGlobalRef and DeleteGlobalRef*/
static classinfo *ihmclass = NULL;
static methodinfo *putmid = NULL;
static methodinfo *getmid = NULL;
static methodinfo *removemid = NULL;


/* direct buffer stuff ********************************************************/

static utf *utf_java_nio_DirectByteBufferImpl;
#if SIZEOF_VOID_P == 8
static utf *utf_gnu_classpath_Pointer64;
#else
static utf *utf_gnu_classpath_Pointer32;
#endif

static classinfo *class_java_nio_DirectByteBufferImpl;
#if SIZEOF_VOID_P == 8
static classinfo *class_gnu_classpath_Pointer64;
#else
static classinfo *class_gnu_classpath_Pointer32;
#endif


/* local reference table ******************************************************/

#if defined(USE_THREADS)
#define LOCALREFTABLE    (THREADINFO->_localref_table)
#else
#define LOCALREFTABLE    (_no_threads_localref_table)
#endif

#if !defined(USE_THREADS)
static localref_table *_no_threads_localref_table;
#endif


/********************* accessing instance-fields **********************************/

#define setField(obj,typ,var,val) *((typ*) ((long int) obj + (long int) var->offset))=val;  
#define getField(obj,typ,var)     *((typ*) ((long int) obj + (long int) var->offset))
#define setfield_critical(clazz,obj,name,sig,jdatatype,val) \
    setField(obj, jdatatype, getFieldID_critical(env,clazz,name,sig), val);


/* some forward declarations **************************************************/

jobject NewLocalRef(JNIEnv *env, jobject ref);


/* jni_init ********************************************************************

   Initialize the JNI subsystem.

*******************************************************************************/

bool jni_init(void)
{
	/* initalize global reference table */

	if (!(ihmclass =
		  load_class_bootstrap(utf_new_char("java/util/IdentityHashMap"))))
		return false;

	global_ref_table = GCNEW(jobject, 1);

	if (!(*global_ref_table = native_new_and_init(ihmclass)))
		return false;

	if (!(getmid = class_resolvemethod(ihmclass, utf_get,
									   utf_java_lang_Object__java_lang_Object)))
		return false;

	if (!(putmid = class_resolvemethod(ihmclass, utf_put,
									   utf_new_char("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"))))
		return false;

	if (!(removemid =
		  class_resolvemethod(ihmclass, utf_remove,
							  utf_java_lang_Object__java_lang_Object)))
		return false;


	/* direct buffer stuff */

	utf_java_nio_DirectByteBufferImpl =
		utf_new_char("java/nio/DirectByteBufferImpl");

	if (!(class_java_nio_DirectByteBufferImpl =
		  load_class_bootstrap(utf_java_nio_DirectByteBufferImpl)))
		return false;

	if (!link_class(class_java_nio_DirectByteBufferImpl))
		return false;

#if SIZEOF_VOID_P == 8
	utf_gnu_classpath_Pointer64 = utf_new_char("gnu/classpath/Pointer64");

	if (!(class_gnu_classpath_Pointer64 =
		  load_class_bootstrap(utf_gnu_classpath_Pointer64)))
		return false;

	if (!link_class(class_gnu_classpath_Pointer64))
		return false;
#else
	utf_gnu_classpath_Pointer32 = utf_new_char("gnu/classpath/Pointer32");

	if (!(class_gnu_classpath_Pointer32 =
		  load_class_bootstrap(utf_gnu_classpath_Pointer32)))
		return false;

	if (!link_class(class_gnu_classpath_Pointer32))
		return false;
#endif

	return true;
}


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

	if (!( ((methodID->flags & ACC_STATIC) && (obj == 0)) ||
		((!(methodID->flags & ACC_STATIC)) && (obj != 0)) )) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

	if (obj && !builtin_instanceof(obj, methodID->class)) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

	argcount = methodID->parseddesc->paramcount;

	blk = MNEW(jni_callblock, argcount);

	fill_callblock_from_vargs(obj, methodID->parseddesc, blk, args, TYPE_ADR);

	STATS(jnicallXmethodnvokation();)

	ret = asm_calljavafunction2(methodID,
								argcount,
								argcount * sizeof(jni_callblock),
								blk);

	MFREE(blk, jni_callblock, argcount);

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
        
	if (!( ((methodID->flags & ACC_STATIC) && (obj == 0)) ||
		((!(methodID->flags & ACC_STATIC)) && (obj != 0)) )) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

	if (obj && !builtin_instanceof(obj, methodID->class)) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

	argcount = methodID->parseddesc->paramcount;

	blk = MNEW(jni_callblock, argcount);

	fill_callblock_from_vargs(obj, methodID->parseddesc, blk, args, retType);

	STATS(jnicallXmethodnvokation();)

	ret = asm_calljavafunction2int(methodID,
								   argcount,
								   argcount * sizeof(jni_callblock),
								   blk);

	MFREE(blk, jni_callblock, argcount);

	return ret;
}


/* callLongMethod **************************************************************

   Core function for long class functions.

*******************************************************************************/

static jlong callLongMethod(jobject obj, jmethodID methodID, va_list args)
{
	s4             argcount;
	jni_callblock *blk;
	jlong          ret;

	STATS(jniinvokation();)

	if (methodID == 0) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError); 
		return 0;
	}

	if (!( ((methodID->flags & ACC_STATIC) && (obj == 0)) ||
		   ((!(methodID->flags & ACC_STATIC)) && (obj != 0)) )) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

	if (obj && !builtin_instanceof(obj, methodID->class)) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

	argcount = methodID->parseddesc->paramcount;

	blk = MNEW(jni_callblock, argcount);

	fill_callblock_from_vargs(obj, methodID->parseddesc, blk, args, TYPE_LNG);

	STATS(jnicallXmethodnvokation();)

	ret = asm_calljavafunction2long(methodID,
									argcount,
									argcount * sizeof(jni_callblock),
									blk);

	MFREE(blk, jni_callblock, argcount);

	return ret;
}


/*core function for float class methods (float,double)*/
static jdouble callFloatMethod(jobject obj, jmethodID methodID, va_list args,int retType)
{
	int argcount = methodID->parseddesc->paramcount;
	jni_callblock *blk;
	jdouble ret;

	STATS(jniinvokation();)

	assert(0);

	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. CallObjectMethod does not support that");
		return 0;
	}

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


static void cacao_jni_CallVoidMethod(jobject obj, jmethodID m, va_list ap)
{ 	
	s4             paramcount;
	jni_callblock *blk;

	if (m == 0) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError); 
		return;
	}

	if (!( ((m->flags & ACC_STATIC) && (obj == 0)) ||
		((!(m->flags & ACC_STATIC)) && (obj != 0)) )) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return;
	}

	if (obj && !builtin_instanceof(obj, m->class)) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return;
	}

	paramcount = m->parseddesc->paramcount;

	if (!(m->flags & ACC_STATIC))
		paramcount++;

	blk = MNEW(jni_callblock, paramcount);

	fill_callblock_from_vargs(obj, m->parseddesc, blk, ap, TYPE_VOID);

	STATS(jnicallXmethodnvokation();)

	(void) asm_calljavafunction2(m,
								 paramcount,
								 paramcount * sizeof(jni_callblock),
								 blk);

	MFREE(blk, jni_callblock, paramcount);

	return;
}


/* GetVersion ******************************************************************

   Returns the major version number in the higher 16 bits and the
   minor version number in the lower 16 bits.

*******************************************************************************/

jint GetVersion(JNIEnv *env)
{
	STATS(jniinvokation();)

	/* we support JNI 1.4 */

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

	return (jclass) NewLocalRef(env, (jobject) c);
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

#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__MIPS__) || defined(__POWERPC__) || defined(__X86_64__)
	/* these JITs support stacktraces, and so does the interpreter */

	cl = cacao_currentClassLoader();
#else
# if defined(ENABLE_INTRP)
	/* the interpreter supports stacktraces, even if the JIT does not */

	if (opt_intrp)
		cl = cacao_currentClassLoader();
	else
# endif
		cl = NULL;
#endif

	if (!(c = load_class_from_classloader(u, cl)))
		return NULL;

	if (!link_class(c))
		return NULL;

	if (!use_class_as_object(c))
		return NULL;

  	return (jclass) NewLocalRef(env, (jobject) c);
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

	if (!use_class_as_object(c))
		return NULL;

	return (jclass) NewLocalRef(env, (jobject) c);
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
	java_objectheader *e;

	STATS(jniinvokation();)

	e = *exceptionptr;

	return NewLocalRef(env, (jthrowable) e);
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


/* PushLocalFrame **************************************************************

   Creates a new local reference frame, in which at least a given
   number of local references can be created.

*******************************************************************************/

jint PushLocalFrame(JNIEnv* env, jint capacity)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: PushLocalFrame: IMPLEMENT ME!");

	assert(0);

	return 0;
}

/* PopLocalFrame ***************************************************************

   Pops off the current local reference frame, frees all the local
   references, and returns a local reference in the previous local
   reference frame for the given result object.

*******************************************************************************/

jobject PopLocalFrame(JNIEnv* env, jobject result)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: PopLocalFrame: IMPLEMENT ME!");

	assert(0);

	/* add local reference and return the value */

	return NewLocalRef(env, NULL);
}


/* DeleteLocalRef **************************************************************

   Deletes the local reference pointed to by localRef.

*******************************************************************************/

void DeleteLocalRef(JNIEnv *env, jobject localRef)
{
	java_objectheader *o;
	localref_table    *lrt;
	s4                 i;

	STATS(jniinvokation();)

	o = (java_objectheader *) localRef;

	/* get local reference table (thread specific) */

	lrt = LOCALREFTABLE;

	/* remove the reference */

	for (i = 0; i < lrt->capacity; i++) {
		if (lrt->refs[i] == o) {
			lrt->refs[i] = NULL;
			lrt->used--;

			return;
		}
	}

	/* this should not happen */

/*  	if (opt_checkjni) */
/*  	FatalError(env, "Bad global or local ref passed to JNI"); */
	log_text("JNI-DeleteLocalRef: Bad global or local ref passed to JNI");
}


/* IsSameObject ****************************************************************

   Tests whether two references refer to the same Java object.

*******************************************************************************/

jboolean IsSameObject(JNIEnv *env, jobject ref1, jobject ref2)
{
	STATS(jniinvokation();)

	if (ref1 == ref2)
		return JNI_TRUE;
	else
		return JNI_FALSE;
}


/* NewLocalRef *****************************************************************

   Creates a new local reference that refers to the same object as ref.

*******************************************************************************/

jobject NewLocalRef(JNIEnv *env, jobject ref)
{
	localref_table *lrt;
	s4              i;

	STATS(jniinvokation();)

	if (ref == NULL)
		return NULL;

	/* get local reference table (thread specific) */

	lrt = LOCALREFTABLE;

	/* check if we have space for the requested reference */

	if (lrt->used == lrt->capacity)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Too many local references");

	/* insert the reference */

	for (i = 0; i < lrt->capacity; i++) {
		if (lrt->refs[i] == NULL) {
			lrt->refs[i] = (java_objectheader *) ref;
			lrt->used++;

			return ref;
		}
	}

	/* should not happen, just to be sure */

	assert(0);

	/* keep compiler happy */

	return NULL;
}


/* EnsureLocalCapacity *********************************************************

   Ensures that at least a given number of local references can be
   created in the current thread

*******************************************************************************/

jint EnsureLocalCapacity(JNIEnv* env, jint capacity)
{
	localref_table *lrt;

	STATS(jniinvokation();)

	/* get local reference table (thread specific) */

	lrt = LOCALREFTABLE;

	/* check if capacity elements are available in the local references table */

	if ((lrt->used + capacity) > lrt->capacity) {
		*exceptionptr = new_exception(string_java_lang_OutOfMemoryError);
		return -1;
	}

	return 0;
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

	return NewLocalRef(env, o);
}


/* NewObject *******************************************************************

   Programmers place all arguments that are to be passed to the
   constructor immediately following the methodID
   argument. NewObject() accepts these arguments and passes them to
   the Java method that the programmer wishes to invoke.

*******************************************************************************/

jobject NewObject(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	java_objectheader *o;
	va_list            ap;

	STATS(jniinvokation();)

	/* create object */

	o = builtin_new(clazz);
	
	if (!o)
		return NULL;

	/* call constructor */

	va_start(ap, methodID);
	cacao_jni_CallVoidMethod(o, methodID, ap);
	va_end(ap);

	return NewLocalRef(env, o);
}


/*********************************************************************************** 

       Constructs a new Java object
       arguments that are to be passed to the constructor are placed in va_list args 

***********************************************************************************/

jobject NewObjectV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: NewObjectV: IMPLEMENT ME!");

	return NewLocalRef(env, NULL);
}


/*********************************************************************************** 

	Constructs a new Java object
	arguments that are to be passed to the constructor are placed in 
	args array of jvalues 

***********************************************************************************/

jobject NewObjectA(JNIEnv* env, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: NewObjectA: IMPLEMENT ME!");

	return NewLocalRef(env, NULL);
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

	if (!use_class_as_object(c))
		return NULL;

	return (jclass) NewLocalRef(env, (jobject) c);
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
	methodinfo *m;

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
	java_objectheader* ret;
	va_list            vaargs;

	STATS(jniinvokation();)

	va_start(vaargs, methodID);
	ret = callObjectMethod(obj, methodID, vaargs);
	va_end(vaargs);

	return NewLocalRef(env, ret);
}


jobject CallObjectMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	java_objectheader* ret;

	STATS(jniinvokation();)

	ret = callObjectMethod(obj, methodID, args);

	return NewLocalRef(env, ret);
}


jobject CallObjectMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: CallObjectMethodA: IMPLEMENT ME!");

	return NewLocalRef(env, NULL);
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



jobject CallNonvirtualObjectMethod(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: CallNonvirtualObjectMethod: IMPLEMENT ME!");

	return NewLocalRef(env, NULL);
}


jobject CallNonvirtualObjectMethodV(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: CallNonvirtualObjectMethodV: IMPLEMENT ME!");

	return NewLocalRef(env, NULL);
}


jobject CallNonvirtualObjectMethodA(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: CallNonvirtualObjectMethodA: IMPLEMENT ME!");

	return NewLocalRef(env, NULL);
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

jfieldID GetFieldID(JNIEnv *env, jclass clazz, const char *name, const char *sig) 
{
	jfieldID f;

	STATS(jniinvokation();)

	f = class_findfield(clazz, utf_new_char((char *) name), utf_new_char((char *) sig)); 
	
	if (!f)
		*exceptionptr =	new_exception(string_java_lang_NoSuchFieldError);  

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

jobject GetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID)
{
	java_objectheader *o;

	STATS(jniinvokation();)

	o = getField(obj, java_objectheader*, fieldID);

	return NewLocalRef(env, o);
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
	java_objectheader *ret;
	va_list            vaargs;

	STATS(jniinvokation();)

	va_start(vaargs, methodID);
	ret = callObjectMethod(0, methodID, vaargs);
	va_end(vaargs);

	return NewLocalRef(env, ret);
}


jobject CallStaticObjectMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	java_objectheader *ret;

	STATS(jniinvokation();)
	
	ret = callObjectMethod(0, methodID, args);

	return NewLocalRef(env, ret);
}


jobject CallStaticObjectMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: CallStaticObjectMethodA: IMPLEMENT ME!");

	return NewLocalRef(env, NULL);
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


jlong CallStaticLongMethodV(JNIEnv *env, jclass clazz, jmethodID methodID,
							va_list args)
{
	STATS(jniinvokation();)
	
	return callLongMethod(0, methodID, args);
}


jlong CallStaticLongMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	STATS(jniinvokation();)

	log_text("JNI-Call: CallStaticLongMethodA: IMPLEMENT ME!!!");

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

	f = class_findfield(clazz,
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

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return NULL;

	return NewLocalRef(env, fieldID->value.a);
}


jboolean GetStaticBooleanField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return false;

	return fieldID->value.i;       
}


jbyte GetStaticByteField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return 0;

	return fieldID->value.i;       
}


jchar GetStaticCharField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return 0;

	return fieldID->value.i;       
}


jshort GetStaticShortField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return 0;

	return fieldID->value.i;       
}


jint GetStaticIntField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return 0;

	return fieldID->value.i;       
}


jlong GetStaticLongField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return 0;

	return fieldID->value.l;
}


jfloat GetStaticFloatField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return 0.0;

 	return fieldID->value.f;
}


jdouble GetStaticDoubleField(JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
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

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return;

	fieldID->value.a = value;
}


void SetStaticBooleanField(JNIEnv *env, jclass clazz, jfieldID fieldID, jboolean value)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return;

	fieldID->value.i = value;
}


void SetStaticByteField(JNIEnv *env, jclass clazz, jfieldID fieldID, jbyte value)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return;

	fieldID->value.i = value;
}


void SetStaticCharField(JNIEnv *env, jclass clazz, jfieldID fieldID, jchar value)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return;

	fieldID->value.i = value;
}


void SetStaticShortField(JNIEnv *env, jclass clazz, jfieldID fieldID, jshort value)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return;

	fieldID->value.i = value;
}


void SetStaticIntField(JNIEnv *env, jclass clazz, jfieldID fieldID, jint value)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return;

	fieldID->value.i = value;
}


void SetStaticLongField(JNIEnv *env, jclass clazz, jfieldID fieldID, jlong value)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return;

	fieldID->value.l = value;
}


void SetStaticFloatField(JNIEnv *env, jclass clazz, jfieldID fieldID, jfloat value)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
		if (!initialize_class(clazz))
			return;

	fieldID->value.f = value;
}


void SetStaticDoubleField(JNIEnv *env, jclass clazz, jfieldID fieldID, jdouble value)
{
	STATS(jniinvokation();)

	if (!clazz->initialized)
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

	return (jstring) NewLocalRef(env, (jobject) s);
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
	java_lang_String *s;

	STATS(jniinvokation();)

	s = javastring_new(utf_new_char(bytes));

    return (jstring) NewLocalRef(env, (jobject) s);
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
	s4                i;

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

	return (jobjectArray) NewLocalRef(env, (jobject) oa);
}


jobject GetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index)
{
    jobject o;

	STATS(jniinvokation();)

	if (index >= array->header.size) {
		*exceptionptr =
			new_exception(string_java_lang_ArrayIndexOutOfBoundsException);
		return NULL;
	}

	o = array->data[index];
    
    return NewLocalRef(env, o);
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
	java_booleanarray *ba;

	STATS(jniinvokation();)

	if (len < 0) {
		*exceptionptr = new_negativearraysizeexception();
		return NULL;
	}

	ba = builtin_newarray_boolean(len);

	return (jbooleanArray) NewLocalRef(env, (jobject) ba);
}


jbyteArray NewByteArray(JNIEnv *env, jsize len)
{
	java_bytearray *ba;

	STATS(jniinvokation();)

	if (len < 0) {
		*exceptionptr = new_negativearraysizeexception();
		return NULL;
	}

	ba = builtin_newarray_byte(len);

	return (jbyteArray) NewLocalRef(env, (jobject) ba);
}


jcharArray NewCharArray(JNIEnv *env, jsize len)
{
	java_chararray *ca;

	STATS(jniinvokation();)

	if (len < 0) {
		*exceptionptr = new_negativearraysizeexception();
		return NULL;
	}

	ca = builtin_newarray_char(len);

	return (jcharArray) NewLocalRef(env, (jobject) ca);
}


jshortArray NewShortArray(JNIEnv *env, jsize len)
{
	java_shortarray *sa;

	STATS(jniinvokation();)

	if (len < 0) {
		*exceptionptr = new_negativearraysizeexception();
		return NULL;
	}

	sa = builtin_newarray_short(len);

	return (jshortArray) NewLocalRef(env, (jobject) sa);
}


jintArray NewIntArray(JNIEnv *env, jsize len)
{
	java_intarray *ia;

	STATS(jniinvokation();)

	if (len < 0) {
		*exceptionptr = new_negativearraysizeexception();
		return NULL;
	}

	ia = builtin_newarray_int(len);

	return (jintArray) NewLocalRef(env, (jobject) ia);
}


jlongArray NewLongArray(JNIEnv *env, jsize len)
{
	java_longarray *la;

	STATS(jniinvokation();)

	if (len < 0) {
		*exceptionptr = new_negativearraysizeexception();
		return NULL;
	}

	la = builtin_newarray_long(len);

	return (jlongArray) NewLocalRef(env, (jobject) la);
}


jfloatArray NewFloatArray(JNIEnv *env, jsize len)
{
	java_floatarray *fa;

	STATS(jniinvokation();)

	if (len < 0) {
		*exceptionptr = new_negativearraysizeexception();
		return NULL;
	}

	fa = builtin_newarray_float(len);

	return (jfloatArray) NewLocalRef(env, (jobject) fa);
}


jdoubleArray NewDoubleArray(JNIEnv *env, jsize len)
{
	java_doublearray *da;

	STATS(jniinvokation();)

	if (len < 0) {
		*exceptionptr = new_negativearraysizeexception();
		return NULL;
	}

	da = builtin_newarray_double(len);

	return (jdoubleArray) NewLocalRef(env, (jobject) da);
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
	/* do the same as Kaffe does */

	return GetByteArrayElements(env, (jbyteArray) array, isCopy);
}


/* ReleasePrimitiveArrayCritical ***********************************************

   No specific documentation.

*******************************************************************************/

void ReleasePrimitiveArrayCritical(JNIEnv *env, jarray array, void *carray,
								   jint mode)
{
	STATS(jniinvokation();)

	/* do the same as Kaffe does */

	ReleaseByteArrayElements(env, (jbyteArray) array, (jbyte *) carray, mode);
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


/* NewGlobalRef ****************************************************************

   Creates a new global reference to the object referred to by the obj
   argument.

*******************************************************************************/
    
jobject NewGlobalRef(JNIEnv* env, jobject lobj)
{
	java_lang_Integer *refcount;
	java_objectheader *newval;

	STATS(jniinvokation();)

#if defined(USE_THREADS)
	builtin_monitorenter(*global_ref_table);
#endif
	
	refcount = (java_lang_Integer *)
		asm_calljavafunction(getmid, *global_ref_table, lobj, NULL, NULL);

	if (refcount == NULL) {
		newval = native_new_and_init_int(class_java_lang_Integer, 1);

		if (newval == NULL) {
#if defined(USE_THREADS)
			builtin_monitorexit(*global_ref_table);
#endif
			return NULL;
		}

		asm_calljavafunction(putmid, *global_ref_table, lobj, newval, NULL);

	} else {
		/* we can access the object itself, as we are in a
           synchronized section */

		refcount->value++;
	}

#if defined(USE_THREADS)
	builtin_monitorexit(*global_ref_table);
#endif

	return lobj;
}


/* DeleteGlobalRef *************************************************************

   Deletes the global reference pointed to by globalRef.

*******************************************************************************/

void DeleteGlobalRef(JNIEnv* env, jobject globalRef)
{
	java_lang_Integer *refcount;
	s4                 val;

	STATS(jniinvokation();)

#if defined(USE_THREADS)
	builtin_monitorenter(*global_ref_table);
#endif

	refcount = (java_lang_Integer *)
		asm_calljavafunction(getmid, *global_ref_table, globalRef, NULL, NULL);

	if (refcount == NULL) {
		log_text("JNI-DeleteGlobalRef: unable to find global reference");
		return;
	}

	/* we can access the object itself, as we are in a synchronized
	   section */

	val = refcount->value - 1;

	if (val == 0) {
		asm_calljavafunction(removemid, *global_ref_table, refcount, NULL,
							 NULL);

	} else {
		/* we do not create a new object, but set the new value into
           the old one */

		refcount->value = val;
	}

#if defined(USE_THREADS)
	builtin_monitorexit(*global_ref_table);
#endif
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
	java_nio_DirectByteBufferImpl *nbuf;
#if SIZEOF_VOID_P == 8
	gnu_classpath_Pointer64 *paddress;
#else
	gnu_classpath_Pointer32 *paddress;
#endif

	STATS(jniinvokation();)

	log_text("JNI-NewDirectByteBuffer: called");

	/* allocate a java.nio.DirectByteBufferImpl object */

	if (!(nbuf = (java_nio_DirectByteBufferImpl *) builtin_new(class_java_nio_DirectByteBufferImpl)))
		return NULL;

	/* alocate a gnu.classpath.Pointer{32,64} object */

#if SIZEOF_VOID_P == 8
	if (!(paddress = (gnu_classpath_Pointer64 *) builtin_new(class_gnu_classpath_Pointer64)))
#else
	if (!(paddress = (gnu_classpath_Pointer32 *) builtin_new(class_gnu_classpath_Pointer32)))
#endif
		return NULL;

	/* fill gnu.classpath.Pointer{32,64} with address */

	paddress->data = (ptrint) address;

	/* fill java.nio.Buffer object */

	nbuf->cap     = (s4) capacity;
	nbuf->limit   = (s4) capacity;
	nbuf->pos     = 0;
	nbuf->address = (gnu_classpath_Pointer *) paddress;

	/* add local reference and return the value */

	return NewLocalRef(env, (jobject) nbuf);
}


/* GetDirectBufferAddress ******************************************************

   Fetches and returns the starting address of the memory region
   referenced by the given direct java.nio.Buffer.

*******************************************************************************/

void *GetDirectBufferAddress(JNIEnv *env, jobject buf)
{
	java_nio_DirectByteBufferImpl *nbuf;
#if SIZEOF_VOID_P == 8
	gnu_classpath_Pointer64       *address;
#else
	gnu_classpath_Pointer32       *address;
#endif

	STATS(jniinvokation();)

#if 0
	if (!builtin_instanceof(buf, class_java_nio_DirectByteBufferImpl))
		return NULL;
#endif

	nbuf = (java_nio_DirectByteBufferImpl *) buf;

#if SIZEOF_VOID_P == 8
	address = (gnu_classpath_Pointer64 *) nbuf->address;
#else
	address = (gnu_classpath_Pointer32 *) nbuf->address;
#endif

	return (void *) address->data;
}


/* GetDirectBufferCapacity *****************************************************

   Fetches and returns the capacity in bytes of the memory region
   referenced by the given direct java.nio.Buffer.

*******************************************************************************/

jlong GetDirectBufferCapacity(JNIEnv* env, jobject buf)
{
	java_nio_Buffer *nbuf;

	STATS(jniinvokation();)

	if (buf == NULL)
		return -1;

	nbuf = (java_nio_Buffer *) buf;

	return (jlong) nbuf->cap;
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

#if defined(ENABLE_JVMTI)
	if (version == JVMTI_VERSION_1_0) {
		*env = (void *) new_jvmtienv();

		if (env != NULL)
			return JNI_OK;
	}
#endif
	
	*env = NULL;

	return JNI_EVERSION;
}



jint AttachCurrentThreadAsDaemon(JavaVM *vm, void **par1, void *par2)
{
	STATS(jniinvokation();)
	log_text("AttachCurrentThreadAsDaemon called");

	return 0;
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

	&NewGlobalRef,
	&DeleteGlobalRef,
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
	s4             paramcount;

	if (methodID == 0) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError); 
		return NULL;
	}

	argcount = methodID->parseddesc->paramcount;
	paramcount = argcount;

	/* if method is non-static, remove the `this' pointer */

	if (!(methodID->flags & ACC_STATIC))
		paramcount--;

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

	if (((params == NULL) && (paramcount != 0)) ||
		(params && (params->header.size != paramcount))) {
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

	blk = MNEW(jni_callblock, argcount);

	if (!fill_callblock_from_objectarray(obj, methodID->parseddesc, blk,
										 params))
		return NULL;

	switch (methodID->parseddesc->returntype.decltype) {
	case TYPE_VOID:
		(void) asm_calljavafunction2(methodID, argcount,
									 argcount * sizeof(jni_callblock),
									 blk);
		o = NULL; /*native_new_and_init(loader_load(utf_new_char("java/lang/Void")));*/
		break;

	case PRIMITIVETYPE_INT: {
		s4 i;
		i = asm_calljavafunction2int(methodID, argcount,
									 argcount * sizeof(jni_callblock),
									 blk);

		o = native_new_and_init_int(class_java_lang_Integer, i);
	}
	break;

	case PRIMITIVETYPE_BYTE: {
		s4 i;
		i = asm_calljavafunction2int(methodID, argcount,
									 argcount * sizeof(jni_callblock),
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
										  argcount,
										  argcount * sizeof(jni_callblock),
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
										  argcount,
										  argcount * sizeof(jni_callblock),
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
										  argcount,
										  argcount * sizeof(jni_callblock),
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

	case PRIMITIVETYPE_LONG: {
		jlong longVal;
		longVal = asm_calljavafunction2long(methodID,
											argcount,
											argcount * sizeof(jni_callblock),
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
											  argcount,
											  argcount * sizeof(jni_callblock),
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
												argcount,
												argcount * sizeof(jni_callblock),
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
		o = asm_calljavafunction2(methodID, argcount,
								  argcount * sizeof(jni_callblock), blk);
		break;

	default:
		/* if this happens the exception has already been set by              */
		/* fill_callblock_from_objectarray                                    */

		MFREE(blk, jni_callblock, argcount);
		return (jobject *) 0;
	}

	MFREE(blk, jni_callblock, argcount);

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
