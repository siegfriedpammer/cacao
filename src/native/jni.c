/* src/native/jni.c - implementation of the Java Native Interface functions

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


#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "vm/types.h"

#include "mm/gc-common.h"
#include "mm/memory.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/localref.h"
#include "native/native.h"

#if defined(ENABLE_JAVASE)
# if defined(WITH_CLASSPATH_GNU)
#  include "native/include/gnu_classpath_Pointer.h"

#  if SIZEOF_VOID_P == 8
#   include "native/include/gnu_classpath_Pointer64.h"
#  else
#   include "native/include/gnu_classpath_Pointer32.h"
#  endif
# endif
#endif

#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Throwable.h"

#if defined(ENABLE_JAVASE)
# if defined(WITH_CLASSPATH_SUN)
#  include "native/include/java_nio_ByteBuffer.h"       /* required by j.l.CL */
# endif

/* java_lang_ClassLoader is used in java_lang_Class and vice versa, so
   we pre-define it here to prevent a compiler warning for Sun
   configurations. */

struct java_lang_ClassLoader;

# include "native/include/java_lang_Class.h"
# include "native/include/java_lang_ClassLoader.h"

# include "native/include/java_lang_reflect_Constructor.h"
# include "native/include/java_lang_reflect_Field.h"
# include "native/include/java_lang_reflect_Method.h"

# include "native/include/java_nio_Buffer.h"

# if defined(WITH_CLASSPATH_GNU)
#  include "native/include/java_nio_DirectByteBufferImpl.h"
# endif
#endif

#if defined(ENABLE_JVMTI)
# include "native/jvmti/cacaodbg.h"
#endif

#include "native/vm/java_lang_Class.h"

#if defined(ENABLE_JAVASE)
# include "native/vm/reflect.h"
#endif

#include "threads/lock-common.h"
#include "threads/threads-common.h"

#include "toolbox/logging.h"

#include "vm/array.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/primitive.h"
#include "vm/resolve.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vm/jit/argument.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/stacktrace.h"

#include "vmcore/loader.h"
#include "vmcore/options.h"
#include "vmcore/statistics.h"


/* debug **********************************************************************/

#if !defined(NDEBUG)
# define TRACEJNICALLS(text)					\
    do {										\
        if (opt_TraceJNICalls) {				\
            log_println text;					\
        }										\
    } while (0)
#else
# define TRACEJNICALLS(text)
#endif


/* global variables ***********************************************************/

/* global reference table *****************************************************/

/* hashsize must be power of 2 */

#define HASHTABLE_GLOBAL_REF_SIZE    64 /* initial size of globalref-hash     */

static hashtable *hashtable_global_ref; /* hashtable for globalrefs           */


/* direct buffer stuff ********************************************************/

#if defined(ENABLE_JAVASE)
static classinfo *class_java_nio_Buffer;

# if defined(WITH_CLASSPATH_GNU)

static classinfo *class_java_nio_DirectByteBufferImpl;
static classinfo *class_java_nio_DirectByteBufferImpl_ReadWrite;

#  if SIZEOF_VOID_P == 8
static classinfo *class_gnu_classpath_Pointer64;
#  else
static classinfo *class_gnu_classpath_Pointer32;
#  endif

static methodinfo *dbbirw_init;

# elif defined(WITH_CLASSPATH_SUN)

static classinfo *class_sun_nio_ch_DirectBuffer;
static classinfo *class_java_nio_DirectByteBuffer;

static methodinfo *dbb_init;

# endif
#endif


/* some forward declarations **************************************************/

jobject _Jv_JNI_NewLocalRef(JNIEnv *env, jobject ref);


/* jni_init ********************************************************************

   Initialize the JNI subsystem.

*******************************************************************************/

bool jni_init(void)
{
	/* create global ref hashtable */

	hashtable_global_ref = NEW(hashtable);

	hashtable_create(hashtable_global_ref, HASHTABLE_GLOBAL_REF_SIZE);


#if defined(ENABLE_JAVASE)
	/* Direct buffer stuff. */

	if (!(class_java_nio_Buffer =
		  load_class_bootstrap(utf_new_char("java/nio/Buffer"))) ||
		!link_class(class_java_nio_Buffer))
		return false;

# if defined(WITH_CLASSPATH_GNU)

	if (!(class_java_nio_DirectByteBufferImpl =
		  load_class_bootstrap(utf_new_char("java/nio/DirectByteBufferImpl"))) ||
		!link_class(class_java_nio_DirectByteBufferImpl))
		return false;

	if (!(class_java_nio_DirectByteBufferImpl_ReadWrite =
		  load_class_bootstrap(utf_new_char("java/nio/DirectByteBufferImpl$ReadWrite"))) ||
		!link_class(class_java_nio_DirectByteBufferImpl_ReadWrite))
		return false;

	if (!(dbbirw_init =
		class_resolvemethod(class_java_nio_DirectByteBufferImpl_ReadWrite,
							utf_init,
							utf_new_char("(Ljava/lang/Object;Lgnu/classpath/Pointer;III)V"))))
		return false;

#  if SIZEOF_VOID_P == 8
	if (!(class_gnu_classpath_Pointer64 =
		  load_class_bootstrap(utf_new_char("gnu/classpath/Pointer64"))) ||
		!link_class(class_gnu_classpath_Pointer64))
		return false;
#  else
	if (!(class_gnu_classpath_Pointer32 =
		  load_class_bootstrap(utf_new_char("gnu/classpath/Pointer32"))) ||
		!link_class(class_gnu_classpath_Pointer32))
		return false;
#  endif

# elif defined(WITH_CLASSPATH_SUN)

	if (!(class_sun_nio_ch_DirectBuffer =
		  load_class_bootstrap(utf_new_char("sun/nio/ch/DirectBuffer"))))
		vm_abort("jni_init: loading sun/nio/ch/DirectBuffer failed");

	if (!link_class(class_sun_nio_ch_DirectBuffer))
		vm_abort("jni_init: linking sun/nio/ch/DirectBuffer failed");

	if (!(class_java_nio_DirectByteBuffer =
		  load_class_bootstrap(utf_new_char("java/nio/DirectByteBuffer"))))
		vm_abort("jni_init: loading java/nio/DirectByteBuffer failed");

	if (!link_class(class_java_nio_DirectByteBuffer))
		vm_abort("jni_init: linking java/nio/DirectByteBuffer failed");

	if (!(dbb_init =
		  class_resolvemethod(class_java_nio_DirectByteBuffer,
							  utf_init,
							  utf_new_char("(JI)V"))))
		vm_abort("jni_init: resolving java/nio/DirectByteBuffer.init(JI)V failed");

# endif

#endif /* defined(ENABLE_JAVASE) */

	return true;
}


/* jni_version_check ***********************************************************

   Check if the given JNI version is supported.

   IN:
       version....JNI version to check

   RETURN VALUE:
       true.......supported
       false......not supported

*******************************************************************************/

bool jni_version_check(int version)
{
	switch (version) {
	case JNI_VERSION_1_1:
	case JNI_VERSION_1_2:
	case JNI_VERSION_1_4:
	case JNI_VERSION_1_6:
		return true;
	default:
		return false;
	}
}


/* _Jv_jni_CallObjectMethod ****************************************************

   Internal function to call Java Object methods.

*******************************************************************************/

static java_handle_t *_Jv_jni_CallObjectMethod(java_handle_t *o,
											   vftbl_t *vftbl,
											   methodinfo *m, va_list ap)
{
	methodinfo    *resm;
	java_handle_t *ro;

	STATISTICS(jniinvokation());

	if (m == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* Class initialization is done by the JIT compiler.  This is ok
	   since a static method always belongs to the declaring class. */

	if (m->flags & ACC_STATIC) {
		/* For static methods we reset the object. */

		if (o != NULL)
			o = NULL;

		/* for convenience */

		resm = m;

	} else {
		/* For instance methods we make a virtual function table lookup. */

		resm = method_vftbl_lookup(vftbl, m);
	}

	STATISTICS(jnicallXmethodnvokation());

	ro = vm_call_method_valist(resm, o, ap);

	return ro;
}


/* _Jv_jni_CallObjectMethodA ***************************************************

   Internal function to call Java Object methods.

*******************************************************************************/

static java_handle_t *_Jv_jni_CallObjectMethodA(java_handle_t *o,
												vftbl_t *vftbl,
												methodinfo *m,
												const jvalue *args)
{
	methodinfo    *resm;
	java_handle_t *ro;

	STATISTICS(jniinvokation());

	if (m == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* Class initialization is done by the JIT compiler.  This is ok
	   since a static method always belongs to the declaring class. */

	if (m->flags & ACC_STATIC) {
		/* For static methods we reset the object. */

		if (o != NULL)
			o = NULL;

		/* for convenience */

		resm = m;

	} else {
		/* For instance methods we make a virtual function table lookup. */

		resm = method_vftbl_lookup(vftbl, m);
	}

	STATISTICS(jnicallXmethodnvokation());

	ro = vm_call_method_jvalue(resm, o, args);

	return ro;
}


/* _Jv_jni_CallIntMethod *******************************************************

   Internal function to call Java integer class methods (boolean,
   byte, char, short, int).

*******************************************************************************/

static jint _Jv_jni_CallIntMethod(java_handle_t *o, vftbl_t *vftbl,
								  methodinfo *m, va_list ap)
{
	methodinfo *resm;
	jint        i;

	STATISTICS(jniinvokation());

	if (m == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}
        
	/* Class initialization is done by the JIT compiler.  This is ok
	   since a static method always belongs to the declaring class. */

	if (m->flags & ACC_STATIC) {
		/* For static methods we reset the object. */

		if (o != NULL)
			o = NULL;

		/* for convenience */

		resm = m;

	} else {
		/* For instance methods we make a virtual function table lookup. */

		resm = method_vftbl_lookup(vftbl, m);
	}

	STATISTICS(jnicallXmethodnvokation());

	i = vm_call_method_int_valist(resm, o, ap);

	return i;
}


/* _Jv_jni_CallIntMethodA ******************************************************

   Internal function to call Java integer class methods (boolean,
   byte, char, short, int).

*******************************************************************************/

static jint _Jv_jni_CallIntMethodA(java_handle_t *o, vftbl_t *vftbl,
								   methodinfo *m, const jvalue *args)
{
	methodinfo *resm;
	jint        i;

	STATISTICS(jniinvokation());

	if (m == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}
        
	/* Class initialization is done by the JIT compiler.  This is ok
	   since a static method always belongs to the declaring class. */

	if (m->flags & ACC_STATIC) {
		/* For static methods we reset the object. */

		if (o != NULL)
			o = NULL;

		/* for convenience */

		resm = m;

	} else {
		/* For instance methods we make a virtual function table lookup. */

		resm = method_vftbl_lookup(vftbl, m);
	}

	STATISTICS(jnicallXmethodnvokation());

	i = vm_call_method_int_jvalue(resm, o, args);

	return i;
}


/* _Jv_jni_CallLongMethod ******************************************************

   Internal function to call Java long methods.

*******************************************************************************/

static jlong _Jv_jni_CallLongMethod(java_handle_t *o, vftbl_t *vftbl,
									methodinfo *m, va_list ap)
{
	methodinfo *resm;
	jlong       l;

	STATISTICS(jniinvokation());

	if (m == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}

	/* Class initialization is done by the JIT compiler.  This is ok
	   since a static method always belongs to the declaring class. */

	if (m->flags & ACC_STATIC) {
		/* For static methods we reset the object. */

		if (o != NULL)
			o = NULL;

		/* for convenience */

		resm = m;

	} else {
		/* For instance methods we make a virtual function table lookup. */

		resm = method_vftbl_lookup(vftbl, m);
	}

	STATISTICS(jnicallXmethodnvokation());

	l = vm_call_method_long_valist(resm, o, ap);

	return l;
}


/* _Jv_jni_CallLongMethodA *****************************************************

   Internal function to call Java long methods.

*******************************************************************************/

static jlong _Jv_jni_CallLongMethodA(java_handle_t *o, vftbl_t *vftbl,
									 methodinfo *m, const jvalue *args)
{
	methodinfo *resm;
	jlong       l;

	STATISTICS(jniinvokation());

	if (m == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}

	/* Class initialization is done by the JIT compiler.  This is ok
	   since a static method always belongs to the declaring class. */

	if (m->flags & ACC_STATIC) {
		/* For static methods we reset the object. */

		if (o != NULL)
			o = NULL;

		/* for convenience */

		resm = m;
	}
	else {
		/* For instance methods we make a virtual function table lookup. */

		resm = method_vftbl_lookup(vftbl, m);
	}

	STATISTICS(jnicallXmethodnvokation());

	l = vm_call_method_long_jvalue(resm, o, args);

	return l;
}


/* _Jv_jni_CallFloatMethod *****************************************************

   Internal function to call Java float methods.

*******************************************************************************/

static jfloat _Jv_jni_CallFloatMethod(java_handle_t *o, vftbl_t *vftbl,
									  methodinfo *m, va_list ap)
{
	methodinfo *resm;
	jfloat      f;

	/* Class initialization is done by the JIT compiler.  This is ok
	   since a static method always belongs to the declaring class. */

	if (m->flags & ACC_STATIC) {
		/* For static methods we reset the object. */

		if (o != NULL)
			o = NULL;

		/* for convenience */

		resm = m;

	} else {
		/* For instance methods we make a virtual function table lookup. */

		resm = method_vftbl_lookup(vftbl, m);
	}

	STATISTICS(jnicallXmethodnvokation());

	f = vm_call_method_float_valist(resm, o, ap);

	return f;
}


/* _Jv_jni_CallFloatMethodA ****************************************************

   Internal function to call Java float methods.

*******************************************************************************/

static jfloat _Jv_jni_CallFloatMethodA(java_handle_t *o, vftbl_t *vftbl,
									   methodinfo *m, const jvalue *args)
{
	methodinfo *resm;
	jfloat      f;

	/* Class initialization is done by the JIT compiler.  This is ok
	   since a static method always belongs to the declaring class. */

	if (m->flags & ACC_STATIC) {
		/* For static methods we reset the object. */

		if (o != NULL)
			o = NULL;

		/* for convenience */

		resm = m;
	}
	else {
		/* For instance methods we make a virtual function table lookup. */

		resm = method_vftbl_lookup(vftbl, m);
	}

	STATISTICS(jnicallXmethodnvokation());

	f = vm_call_method_float_jvalue(resm, o, args);

	return f;
}


/* _Jv_jni_CallDoubleMethod ****************************************************

   Internal function to call Java double methods.

*******************************************************************************/

static jdouble _Jv_jni_CallDoubleMethod(java_handle_t *o, vftbl_t *vftbl,
										methodinfo *m, va_list ap)
{
	methodinfo *resm;
	jdouble     d;

	/* Class initialization is done by the JIT compiler.  This is ok
	   since a static method always belongs to the declaring class. */

	if (m->flags & ACC_STATIC) {
		/* For static methods we reset the object. */

		if (o != NULL)
			o = NULL;

		/* for convenience */

		resm = m;

	} else {
		/* For instance methods we make a virtual function table lookup. */

		resm = method_vftbl_lookup(vftbl, m);
	}

	d = vm_call_method_double_valist(resm, o, ap);

	return d;
}


/* _Jv_jni_CallDoubleMethodA ***************************************************

   Internal function to call Java double methods.

*******************************************************************************/

static jdouble _Jv_jni_CallDoubleMethodA(java_handle_t *o, vftbl_t *vftbl,
										 methodinfo *m, const jvalue *args)
{
	methodinfo *resm;
	jdouble     d;

	/* Class initialization is done by the JIT compiler.  This is ok
	   since a static method always belongs to the declaring class. */

	if (m->flags & ACC_STATIC) {
		/* For static methods we reset the object. */

		if (o != NULL)
			o = NULL;

		/* for convenience */

		resm = m;
	}
	else {
		/* For instance methods we make a virtual function table lookup. */

		resm = method_vftbl_lookup(vftbl, m);
	}

	d = vm_call_method_double_jvalue(resm, o, args);

	return d;
}


/* _Jv_jni_CallVoidMethod ******************************************************

   Internal function to call Java void methods.

*******************************************************************************/

static void _Jv_jni_CallVoidMethod(java_handle_t *o, vftbl_t *vftbl,
								   methodinfo *m, va_list ap)
{ 	
	methodinfo *resm;

	if (m == NULL) {
		exceptions_throw_nullpointerexception();
		return;
	}

	/* Class initialization is done by the JIT compiler.  This is ok
	   since a static method always belongs to the declaring class. */

	if (m->flags & ACC_STATIC) {
		/* For static methods we reset the object. */

		if (o != NULL)
			o = NULL;

		/* for convenience */

		resm = m;

	} else {
		/* For instance methods we make a virtual function table lookup. */

		resm = method_vftbl_lookup(vftbl, m);
	}

	STATISTICS(jnicallXmethodnvokation());

	(void) vm_call_method_valist(resm, o, ap);
}


/* _Jv_jni_CallVoidMethodA *****************************************************

   Internal function to call Java void methods.

*******************************************************************************/

static void _Jv_jni_CallVoidMethodA(java_handle_t *o, vftbl_t *vftbl,
									methodinfo *m, const jvalue *args)
{ 	
	methodinfo *resm;

	if (m == NULL) {
		exceptions_throw_nullpointerexception();
		return;
	}

	/* Class initialization is done by the JIT compiler.  This is ok
	   since a static method always belongs to the declaring class. */

	if (m->flags & ACC_STATIC) {
		/* For static methods we reset the object. */

		if (o != NULL)
			o = NULL;

		/* for convenience */

		resm = m;

	} else {
		/* For instance methods we make a virtual function table lookup. */

		resm = method_vftbl_lookup(vftbl, m);
	}

	STATISTICS(jnicallXmethodnvokation());

	(void) vm_call_method_jvalue(resm, o, args);
}


/* _Jv_jni_invokeNative ********************************************************

   Invoke a method on the given object with the given arguments.

   For instance methods OBJ must be != NULL and the method is looked up
   in the vftbl of the object.

   For static methods, OBJ is ignored.

*******************************************************************************/

java_handle_t *_Jv_jni_invokeNative(methodinfo *m, java_handle_t *o,
									java_handle_objectarray_t *params)
{
	methodinfo    *resm;
	java_handle_t *ro;
	s4             argcount;
	s4             paramcount;

	if (m == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	argcount = m->parseddesc->paramcount;
	paramcount = argcount;

	/* if method is non-static, remove the `this' pointer */

	if (!(m->flags & ACC_STATIC))
		paramcount--;

	/* For instance methods the object has to be an instance of the
	   class the method belongs to. For static methods the obj
	   parameter is ignored. */

	if (!(m->flags & ACC_STATIC) && o && (!builtin_instanceof(o, m->class))) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	/* check if we got the right number of arguments */

	if (((params == NULL) && (paramcount != 0)) ||
		(params && (LLNI_array_size(params) != paramcount))) 
	{
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	/* for instance methods we need an object */

	if (!(m->flags & ACC_STATIC) && (o == NULL)) {
		/* XXX not sure if that is the correct exception */
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* for static methods, zero object to make subsequent code simpler */
	if (m->flags & ACC_STATIC)
		o = NULL;

	if (o != NULL) {
		/* for instance methods we must do a vftbl lookup */
		resm = method_vftbl_lookup(LLNI_vftbl_direct(o), m);
	}
	else {
		/* for static methods, just for convenience */
		resm = m;
	}

	ro = vm_call_method_objectarray(resm, o, params);

	return ro;
}


/* GetVersion ******************************************************************

   Returns the major version number in the higher 16 bits and the
   minor version number in the lower 16 bits.

*******************************************************************************/

jint _Jv_JNI_GetVersion(JNIEnv *env)
{
	TRACEJNICALLS(("_Jv_JNI_GetVersion(env=%p)", env));

	/* We support JNI 1.6. */

	return JNI_VERSION_1_6;
}


/* Class Operations ***********************************************************/

/* DefineClass *****************************************************************

   Loads a class from a buffer of raw class data. The buffer
   containing the raw class data is not referenced by the VM after the
   DefineClass call returns, and it may be discarded if desired.

*******************************************************************************/

jclass _Jv_JNI_DefineClass(JNIEnv *env, const char *name, jobject loader,
						   const jbyte *buf, jsize bufLen)
{
#if defined(ENABLE_JAVASE)
	utf             *u;
	classloader     *cl;
	classinfo       *c;
	java_lang_Class *co;

	TRACEJNICALLS(("_Jv_JNI_DefineClass(env=%p, name=%s, loader=%p, buf=%p, bufLen=%d)", env, name, loader, buf, bufLen));

	u  = utf_new_char(name);
	cl = loader_hashtable_classloader_add((java_handle_t *) loader);

	c = class_define(u, cl, bufLen, (uint8_t *) buf, NULL);

	co = LLNI_classinfo_wrap(c);

	return (jclass) _Jv_JNI_NewLocalRef(env, (jobject) co);
#else
	vm_abort("_Jv_JNI_DefineClass: not implemented in this configuration");

	/* keep compiler happy */

	return 0;
#endif
}


/* FindClass *******************************************************************

   This function loads a locally-defined class. It searches the
   directories and zip files specified by the CLASSPATH environment
   variable for the class with the specified name.

*******************************************************************************/

jclass _Jv_JNI_FindClass(JNIEnv *env, const char *name)
{
#if defined(ENABLE_JAVASE)

	utf             *u;
	classinfo       *cc;
	classinfo       *c;
	java_lang_Class *co;

	TRACEJNICALLS(("_Jv_JNI_FindClass(env=%p, name=%s)", env, name));

	u = utf_new_char_classname((char *) name);

	/* Check stacktrace for classloader, if one found use it,
	   otherwise use the system classloader. */

	/* Quote from the JNI documentation:
	 
	   In the Java 2 Platform, FindClass locates the class loader
	   associated with the current native method.  If the native code
	   belongs to a system class, no class loader will be
	   involved. Otherwise, the proper class loader will be invoked to
	   load and link the named class. When FindClass is called through
	   the Invocation Interface, there is no current native method or
	   its associated class loader. In that case, the result of
	   ClassLoader.getBaseClassLoader is used." */

	cc = stacktrace_get_current_class();

	if (cc == NULL)
		c = load_class_from_sysloader(u);
	else
		c = load_class_from_classloader(u, cc->classloader);

	if (c == NULL)
		return NULL;

	if (!link_class(c))
		return NULL;

	co = LLNI_classinfo_wrap(c);

  	return (jclass) _Jv_JNI_NewLocalRef(env, (jobject) co);

#elif defined(ENABLE_JAVAME_CLDC1_1)

	utf       *u;
	classinfo *c;

	TRACEJNICALLS(("_Jv_JNI_FindClass(env=%p, name=%s)", env, name));

	u = utf_new_char_classname((char *) name);
	c = load_class_bootstrap(u);

	if (c == NULL)
		return NULL;

	if (!link_class(c))
		return NULL;

  	return (jclass) _Jv_JNI_NewLocalRef(env, (jobject) c);
  	
#else
	vm_abort("_Jv_JNI_FindClass: not implemented in this configuration");

	/* keep compiler happy */

	return NULL;
#endif
}
  

/* GetSuperclass ***************************************************************

   If clazz represents any class other than the class Object, then
   this function returns the object that represents the superclass of
   the class specified by clazz.

*******************************************************************************/
 
jclass _Jv_JNI_GetSuperclass(JNIEnv *env, jclass sub)
{
	classinfo       *c;
	classinfo       *super;
	java_lang_Class *co;

	TRACEJNICALLS(("_Jv_JNI_GetSuperclass(env=%p, sub=%p)", env, sub));

	c = LLNI_classinfo_unwrap(sub);

	if (c == NULL)
		return NULL;

	super = class_get_superclass(c);

	co = LLNI_classinfo_wrap(super);

	return (jclass) _Jv_JNI_NewLocalRef(env, (jobject) co);
}
  
 
/* IsAssignableFrom ************************************************************

   Determines whether an object of sub can be safely cast to sup.

*******************************************************************************/

jboolean _Jv_JNI_IsAssignableFrom(JNIEnv *env, jclass sub, jclass sup)
{
	java_lang_Class *csup;
	java_lang_Class *csub;

	csup = (java_lang_Class *) sup;
	csub = (java_lang_Class *) sub;

	STATISTICS(jniinvokation());

	return _Jv_java_lang_Class_isAssignableFrom(csup, csub);
}


/* Throw ***********************************************************************

   Causes a java.lang.Throwable object to be thrown.

*******************************************************************************/

jint _Jv_JNI_Throw(JNIEnv *env, jthrowable obj)
{
	java_handle_t *o;

	STATISTICS(jniinvokation());

	o = (java_handle_t *) obj;

	exceptions_set_exception(o);

	return JNI_OK;
}


/* ThrowNew ********************************************************************

   Constructs an exception object from the specified class with the
   message specified by message and causes that exception to be
   thrown.

*******************************************************************************/

jint _Jv_JNI_ThrowNew(JNIEnv* env, jclass clazz, const char *msg) 
{
	classinfo     *c;
	java_handle_t *o;
	java_handle_t *s;

	STATISTICS(jniinvokation());

	c = LLNI_classinfo_unwrap(clazz);
	if (msg == NULL)
		msg = "";
	s = javastring_new_from_utf_string(msg);

  	/* instantiate exception object */

	o = native_new_and_init_string(c, s);

	if (o == NULL)
		return -1;

	exceptions_set_exception(o);

	return 0;
}


/* ExceptionOccurred ***********************************************************

   Determines if an exception is being thrown. The exception stays
   being thrown until either the native code calls ExceptionClear(),
   or the Java code handles the exception.

*******************************************************************************/

jthrowable _Jv_JNI_ExceptionOccurred(JNIEnv *env)
{
	java_handle_t *o;

	TRACEJNICALLS(("_Jv_JNI_ExceptionOccurred(env=%p)", env));

	o = exceptions_get_exception();

	return _Jv_JNI_NewLocalRef(env, (jthrowable) o);
}


/* ExceptionDescribe ***********************************************************

   Prints an exception and a backtrace of the stack to a system
   error-reporting channel, such as stderr. This is a convenience
   routine provided for debugging.

*******************************************************************************/

void _Jv_JNI_ExceptionDescribe(JNIEnv *env)
{
	java_handle_t *o;
	classinfo     *c;
	methodinfo    *m;

	TRACEJNICALLS(("_Jv_JNI_ExceptionDescribe(env=%p)", env));

	/* Clear exception, because we are probably calling Java code
	   again. */

	o = exceptions_get_and_clear_exception();

	if (o != NULL) {
		/* get printStackTrace method from exception class */

		LLNI_class_get(o, c);

		m = class_resolveclassmethod(c,
									 utf_printStackTrace,
									 utf_void__void,
									 NULL,
									 true);

		if (m == NULL)
			vm_abort("_Jv_JNI_ExceptionDescribe: could not find printStackTrace");

		/* Print the stacktrace. */

		(void) vm_call_method(m, o);
	}
}


/* ExceptionClear **************************************************************

   Clears any exception that is currently being thrown. If no
   exception is currently being thrown, this routine has no effect.

*******************************************************************************/

void _Jv_JNI_ExceptionClear(JNIEnv *env)
{
	STATISTICS(jniinvokation());

	exceptions_clear_exception();
}


/* FatalError ******************************************************************

   Raises a fatal error and does not expect the VM to recover. This
   function does not return.

*******************************************************************************/

void _Jv_JNI_FatalError(JNIEnv *env, const char *msg)
{
	STATISTICS(jniinvokation());

	/* this seems to be the best way */

	vm_abort("JNI Fatal error: %s", msg);
}


/* PushLocalFrame **************************************************************

   Creates a new local reference frame, in which at least a given
   number of local references can be created.

*******************************************************************************/

jint _Jv_JNI_PushLocalFrame(JNIEnv* env, jint capacity)
{
	STATISTICS(jniinvokation());

	if (capacity <= 0)
		return -1;

	/* add new local reference frame to current table */

	if (!localref_frame_push(capacity))
		return -1;

	return 0;
}


/* PopLocalFrame ***************************************************************

   Pops off the current local reference frame, frees all the local
   references, and returns a local reference in the previous local
   reference frame for the given result object.

*******************************************************************************/

jobject _Jv_JNI_PopLocalFrame(JNIEnv* env, jobject result)
{
	STATISTICS(jniinvokation());

	/* release all current local frames */

	localref_frame_pop_all();

	/* add local reference and return the value */

	return _Jv_JNI_NewLocalRef(env, result);
}


/* DeleteLocalRef **************************************************************

   Deletes the local reference pointed to by localRef.

*******************************************************************************/

void _Jv_JNI_DeleteLocalRef(JNIEnv *env, jobject localRef)
{
	java_handle_t *o;

	STATISTICS(jniinvokation());

	o = (java_handle_t *) localRef;

	if (o == NULL)
		return;

	/* delete the reference */

	localref_del(o);
}


/* IsSameObject ****************************************************************

   Tests whether two references refer to the same Java object.

*******************************************************************************/

jboolean _Jv_JNI_IsSameObject(JNIEnv *env, jobject ref1, jobject ref2)
{
	java_handle_t *o1;
	java_handle_t *o2;
	jboolean       result;

	STATISTICS(jniinvokation());

	o1 = (java_handle_t *) ref1;
	o2 = (java_handle_t *) ref2;

	LLNI_CRITICAL_START;

	if (LLNI_UNWRAP(o1) == LLNI_UNWRAP(o2))
		result = JNI_TRUE;
	else
		result = JNI_FALSE;

	LLNI_CRITICAL_END;

	return result;
}


/* NewLocalRef *****************************************************************

   Creates a new local reference that refers to the same object as ref.

*******************************************************************************/

jobject _Jv_JNI_NewLocalRef(JNIEnv *env, jobject ref)
{
	java_handle_t *o;
	java_handle_t *localref;

	STATISTICS(jniinvokation());

	o = (java_handle_t *) ref;

	if (o == NULL)
		return NULL;

	/* insert the reference */

	localref = localref_add(LLNI_DIRECT(o));

	return (jobject) localref;
}


/* EnsureLocalCapacity *********************************************************

   Ensures that at least a given number of local references can be
   created in the current thread

*******************************************************************************/

jint _Jv_JNI_EnsureLocalCapacity(JNIEnv* env, jint capacity)
{
	localref_table *lrt;

	STATISTICS(jniinvokation());

	/* get local reference table (thread specific) */

	lrt = LOCALREFTABLE;

	/* check if capacity elements are available in the local references table */

	if ((lrt->used + capacity) > lrt->capacity)
		return _Jv_JNI_PushLocalFrame(env, capacity);

	return 0;
}


/* AllocObject *****************************************************************

   Allocates a new Java object without invoking any of the
   constructors for the object. Returns a reference to the object.

*******************************************************************************/

jobject _Jv_JNI_AllocObject(JNIEnv *env, jclass clazz)
{
	classinfo     *c;
	java_handle_t *o;

	STATISTICS(jniinvokation());

	c = LLNI_classinfo_unwrap(clazz);

	if ((c->flags & ACC_INTERFACE) || (c->flags & ACC_ABSTRACT)) {
		exceptions_throw_instantiationexception(c);
		return NULL;
	}
		
	o = builtin_new(c);

	return _Jv_JNI_NewLocalRef(env, (jobject) o);
}


/* NewObject *******************************************************************

   Programmers place all arguments that are to be passed to the
   constructor immediately following the methodID
   argument. NewObject() accepts these arguments and passes them to
   the Java method that the programmer wishes to invoke.

*******************************************************************************/

jobject _Jv_JNI_NewObject(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	java_handle_t *o;
	classinfo     *c;
	methodinfo    *m;
	va_list        ap;

	STATISTICS(jniinvokation());

	c = LLNI_classinfo_unwrap(clazz);
	m = (methodinfo *) methodID;

	/* create object */

	o = builtin_new(c);
	
	if (o == NULL)
		return NULL;

	/* call constructor */

	va_start(ap, methodID);
	_Jv_jni_CallVoidMethod(o, LLNI_vftbl_direct(o), m, ap);
	va_end(ap);

	return _Jv_JNI_NewLocalRef(env, (jobject) o);
}


/* NewObjectV ******************************************************************

   Programmers place all arguments that are to be passed to the
   constructor in an args argument of type va_list that immediately
   follows the methodID argument. NewObjectV() accepts these
   arguments, and, in turn, passes them to the Java method that the
   programmer wishes to invoke.

*******************************************************************************/

jobject _Jv_JNI_NewObjectV(JNIEnv* env, jclass clazz, jmethodID methodID,
						   va_list args)
{
	java_handle_t *o;
	classinfo     *c;
	methodinfo    *m;

	STATISTICS(jniinvokation());

	c = LLNI_classinfo_unwrap(clazz);
	m = (methodinfo *) methodID;

	/* create object */

	o = builtin_new(c);
	
	if (o == NULL)
		return NULL;

	/* call constructor */

	_Jv_jni_CallVoidMethod(o, LLNI_vftbl_direct(o), m, args);

	return _Jv_JNI_NewLocalRef(env, (jobject) o);
}


/* NewObjectA ***************************************************************** 

   Programmers place all arguments that are to be passed to the
   constructor in an args array of jvalues that immediately follows
   the methodID argument. NewObjectA() accepts the arguments in this
   array, and, in turn, passes them to the Java method that the
   programmer wishes to invoke.

*******************************************************************************/

jobject _Jv_JNI_NewObjectA(JNIEnv* env, jclass clazz, jmethodID methodID,
						   const jvalue *args)
{
	java_handle_t *o;
	classinfo     *c;
	methodinfo    *m;

	STATISTICS(jniinvokation());

	c = LLNI_classinfo_unwrap(clazz);
	m = (methodinfo *) methodID;

	/* create object */

	o = builtin_new(c);
	
	if (o == NULL)
		return NULL;

	/* call constructor */

	_Jv_jni_CallVoidMethodA(o, LLNI_vftbl_direct(o), m, args);

	return _Jv_JNI_NewLocalRef(env, (jobject) o);
}


/* GetObjectClass **************************************************************

 Returns the class of an object.

*******************************************************************************/

jclass _Jv_JNI_GetObjectClass(JNIEnv *env, jobject obj)
{
	java_handle_t   *o;
	classinfo       *c;
	java_lang_Class *co;

	STATISTICS(jniinvokation());

	o = (java_handle_t *) obj;

	if ((o == NULL) || (LLNI_vftbl_direct(o) == NULL))
		return NULL;

	LLNI_class_get(o, c);

	co = LLNI_classinfo_wrap(c);

	return (jclass) _Jv_JNI_NewLocalRef(env, (jobject) co);
}


/* IsInstanceOf ****************************************************************

   Tests whether an object is an instance of a class.

*******************************************************************************/

jboolean _Jv_JNI_IsInstanceOf(JNIEnv *env, jobject obj, jclass clazz)
{
	java_lang_Class  *c;
	java_lang_Object *o;

	STATISTICS(jniinvokation());

	c = (java_lang_Class *) clazz;
	o = (java_lang_Object *) obj;

	return _Jv_java_lang_Class_isInstance(c, o);
}


/* Reflection Support *********************************************************/

/* FromReflectedMethod *********************************************************

   Converts java.lang.reflect.Method or java.lang.reflect.Constructor
   object to a method ID.
  
*******************************************************************************/
  
jmethodID _Jv_JNI_FromReflectedMethod(JNIEnv *env, jobject method)
{
#if defined(ENABLE_JAVASE)
	java_handle_t *o;
	classinfo     *c;
	methodinfo    *m;
	s4             slot;

	STATISTICS(jniinvokation());

	o = (java_handle_t *) method;

	if (o == NULL)
		return NULL;
	
	if (builtin_instanceof(o, class_java_lang_reflect_Method)) {
		java_lang_reflect_Method *rm;

		rm   = (java_lang_reflect_Method *) method;
		LLNI_field_get_cls(rm, clazz, c);
		LLNI_field_get_val(rm, slot , slot);
	}
	else if (builtin_instanceof(o, class_java_lang_reflect_Constructor)) {
		java_lang_reflect_Constructor *rc;

		rc   = (java_lang_reflect_Constructor *) method;
		LLNI_field_get_cls(rc, clazz, c);
		LLNI_field_get_val(rc, slot , slot);
	}
	else
		return NULL;

	m = &(c->methods[slot]);

	return (jmethodID) m;
#else
	vm_abort("_Jv_JNI_FromReflectedMethod: not implemented in this configuration");

	/* keep compiler happy */

	return NULL;
#endif
}


/* FromReflectedField **********************************************************

   Converts a java.lang.reflect.Field to a field ID.

*******************************************************************************/
 
jfieldID _Jv_JNI_FromReflectedField(JNIEnv* env, jobject field)
{
#if defined(ENABLE_JAVASE)
	java_lang_reflect_Field *rf;
	classinfo               *c;
	fieldinfo               *f;
	int32_t                  slot;

	STATISTICS(jniinvokation());

	rf = (java_lang_reflect_Field *) field;

	if (rf == NULL)
		return NULL;

	LLNI_field_get_cls(rf, clazz, c);
	LLNI_field_get_val(rf, slot , slot);
	f = &(c->fields[slot]);

	return (jfieldID) f;
#else
	vm_abort("_Jv_JNI_FromReflectedField: not implemented in this configuration");

	/* keep compiler happy */

	return NULL;
#endif
}


/* ToReflectedMethod ***********************************************************

   Converts a method ID derived from cls to an instance of the
   java.lang.reflect.Method class or to an instance of the
   java.lang.reflect.Constructor class.

*******************************************************************************/

jobject _Jv_JNI_ToReflectedMethod(JNIEnv* env, jclass cls, jmethodID methodID,
								  jboolean isStatic)
{
#if defined(ENABLE_JAVASE)
	methodinfo                    *m;
	java_lang_reflect_Constructor *rc;
	java_lang_reflect_Method      *rm;

	TRACEJNICALLS(("_Jv_JNI_ToReflectedMethod(env=%p, cls=%p, methodID=%p, isStatic=%d)", env, cls, methodID, isStatic));

	m = (methodinfo *) methodID;

	/* HotSpot does the same assert. */

	assert(((m->flags & ACC_STATIC) != 0) == (isStatic != 0));

	if (m->name == utf_init) {
		rc = reflect_constructor_new(m);

		return (jobject) rc;
	}
	else {
		rm = reflect_method_new(m);

		return (jobject) rm;
	}
#else
	vm_abort("_Jv_JNI_ToReflectedMethod: not implemented in this configuration");

	/* keep compiler happy */

	return NULL;
#endif
}


/* ToReflectedField ************************************************************

   Converts a field ID derived from cls to an instance of the
   java.lang.reflect.Field class.

*******************************************************************************/

jobject _Jv_JNI_ToReflectedField(JNIEnv* env, jclass cls, jfieldID fieldID,
								 jboolean isStatic)
{
	STATISTICS(jniinvokation());

	log_text("JNI-Call: ToReflectedField: IMPLEMENT ME!");

	return NULL;
}


/* Calling Instance Methods ***************************************************/

/* GetMethodID *****************************************************************

   Returns the method ID for an instance (nonstatic) method of a class
   or interface. The method may be defined in one of the clazz's
   superclasses and inherited by clazz. The method is determined by
   its name and signature.

   GetMethodID() causes an uninitialized class to be initialized.

*******************************************************************************/

jmethodID _Jv_JNI_GetMethodID(JNIEnv* env, jclass clazz, const char *name,
							  const char *sig)
{
	classinfo  *c;
	utf        *uname;
	utf        *udesc;
	methodinfo *m;

	STATISTICS(jniinvokation());

	c = LLNI_classinfo_unwrap(clazz);

	if (c == NULL)
		return NULL;

	if (!(c->state & CLASS_INITIALIZED))
		if (!initialize_class(c))
			return NULL;

	/* try to get the method of the class or one of it's superclasses */

	uname = utf_new_char((char *) name);
	udesc = utf_new_char((char *) sig);

 	m = class_resolvemethod(c, uname, udesc);

	if ((m == NULL) || (m->flags & ACC_STATIC)) {
		exceptions_throw_nosuchmethoderror(c, uname, udesc);

		return NULL;
	}

	return (jmethodID) m;
}


/* JNI-functions for calling instance methods *********************************/

#define JNI_CALL_VIRTUAL_METHOD(name, type, intern)         \
type _Jv_JNI_Call##name##Method(JNIEnv *env, jobject obj,   \
								jmethodID methodID, ...)    \
{                                                           \
	java_handle_t *o;                                       \
	methodinfo    *m;                                       \
	va_list        ap;                                      \
	type           ret;                                     \
                                                            \
	o = (java_handle_t *) obj;                              \
	m = (methodinfo *) methodID;                            \
                                                            \
	va_start(ap, methodID);                                 \
	ret = _Jv_jni_Call##intern##Method(o, LLNI_vftbl_direct(o), m, ap); \
	va_end(ap);                                             \
                                                            \
	return ret;                                             \
}

JNI_CALL_VIRTUAL_METHOD(Boolean, jboolean, Int)
JNI_CALL_VIRTUAL_METHOD(Byte,    jbyte,    Int)
JNI_CALL_VIRTUAL_METHOD(Char,    jchar,    Int)
JNI_CALL_VIRTUAL_METHOD(Short,   jshort,   Int)
JNI_CALL_VIRTUAL_METHOD(Int,     jint,     Int)
JNI_CALL_VIRTUAL_METHOD(Long,    jlong,    Long)
JNI_CALL_VIRTUAL_METHOD(Float,   jfloat,   Float)
JNI_CALL_VIRTUAL_METHOD(Double,  jdouble,  Double)


#define JNI_CALL_VIRTUAL_METHOD_V(name, type, intern)              \
type _Jv_JNI_Call##name##MethodV(JNIEnv *env, jobject obj,         \
								 jmethodID methodID, va_list args) \
{                                                                  \
	java_handle_t *o;                                              \
	methodinfo    *m;                                              \
	type           ret;                                            \
                                                                   \
	o = (java_handle_t *) obj;                                     \
	m = (methodinfo *) methodID;                                   \
                                                                   \
	ret = _Jv_jni_Call##intern##Method(o, LLNI_vftbl_direct(o), m, args);      \
                                                                   \
	return ret;                                                    \
}

JNI_CALL_VIRTUAL_METHOD_V(Boolean, jboolean, Int)
JNI_CALL_VIRTUAL_METHOD_V(Byte,    jbyte,    Int)
JNI_CALL_VIRTUAL_METHOD_V(Char,    jchar,    Int)
JNI_CALL_VIRTUAL_METHOD_V(Short,   jshort,   Int)
JNI_CALL_VIRTUAL_METHOD_V(Int,     jint,     Int)
JNI_CALL_VIRTUAL_METHOD_V(Long,    jlong,    Long)
JNI_CALL_VIRTUAL_METHOD_V(Float,   jfloat,   Float)
JNI_CALL_VIRTUAL_METHOD_V(Double,  jdouble,  Double)


#define JNI_CALL_VIRTUAL_METHOD_A(name, type, intern)          \
type _Jv_JNI_Call##name##MethodA(JNIEnv *env, jobject obj,     \
								 jmethodID methodID,           \
								 const jvalue *args)           \
{                                                              \
	java_handle_t *o;                                          \
	methodinfo    *m;                                          \
	type           ret;                                        \
                                                               \
	o = (java_handle_t *) obj;                                 \
	m = (methodinfo *) methodID;                               \
                                                               \
	ret = _Jv_jni_Call##intern##MethodA(o, LLNI_vftbl_direct(o), m, args); \
                                                               \
	return ret;                                                \
}

JNI_CALL_VIRTUAL_METHOD_A(Boolean, jboolean, Int)
JNI_CALL_VIRTUAL_METHOD_A(Byte,    jbyte,    Int)
JNI_CALL_VIRTUAL_METHOD_A(Char,    jchar,    Int)
JNI_CALL_VIRTUAL_METHOD_A(Short,   jshort,   Int)
JNI_CALL_VIRTUAL_METHOD_A(Int,     jint,     Int)
JNI_CALL_VIRTUAL_METHOD_A(Long,    jlong,    Long)
JNI_CALL_VIRTUAL_METHOD_A(Float,   jfloat,   Float)
JNI_CALL_VIRTUAL_METHOD_A(Double,  jdouble,  Double)


jobject _Jv_JNI_CallObjectMethod(JNIEnv *env, jobject obj, jmethodID methodID,
								 ...)
{
	java_handle_t *o;
	methodinfo    *m;
	java_handle_t *ret;
	va_list        ap;

	o = (java_handle_t *) obj;
	m = (methodinfo *) methodID;

	va_start(ap, methodID);
	ret = _Jv_jni_CallObjectMethod(o, LLNI_vftbl_direct(o), m, ap);
	va_end(ap);

	return _Jv_JNI_NewLocalRef(env, (jobject) ret);
}


jobject _Jv_JNI_CallObjectMethodV(JNIEnv *env, jobject obj, jmethodID methodID,
								  va_list args)
{
	java_handle_t *o;
	methodinfo    *m;
	java_handle_t *ret;

	o = (java_handle_t *) obj;
	m = (methodinfo *) methodID;

	ret = _Jv_jni_CallObjectMethod(o, LLNI_vftbl_direct(o), m, args);

	return _Jv_JNI_NewLocalRef(env, (jobject) ret);
}


jobject _Jv_JNI_CallObjectMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
								  const jvalue *args)
{
	java_handle_t *o;
	methodinfo    *m;
	java_handle_t *ret;

	o = (java_handle_t *) obj;
	m = (methodinfo *) methodID;

	ret = _Jv_jni_CallObjectMethodA(o, LLNI_vftbl_direct(o), m, args);

	return _Jv_JNI_NewLocalRef(env, (jobject) ret);
}



void _Jv_JNI_CallVoidMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	java_handle_t *o;
	methodinfo    *m;
	va_list        ap;

	o = (java_handle_t *) obj;
	m = (methodinfo *) methodID;

	va_start(ap, methodID);
	_Jv_jni_CallVoidMethod(o, LLNI_vftbl_direct(o), m, ap);
	va_end(ap);
}


void _Jv_JNI_CallVoidMethodV(JNIEnv *env, jobject obj, jmethodID methodID,
							 va_list args)
{
	java_handle_t *o;
	methodinfo    *m;

	o = (java_handle_t *) obj;
	m = (methodinfo *) methodID;

	_Jv_jni_CallVoidMethod(o, LLNI_vftbl_direct(o), m, args);
}


void _Jv_JNI_CallVoidMethodA(JNIEnv *env, jobject obj, jmethodID methodID,
							 const jvalue *args)
{
	java_handle_t *o;
	methodinfo    *m;

	o = (java_handle_t *) obj;
	m = (methodinfo *) methodID;

	_Jv_jni_CallVoidMethodA(o, LLNI_vftbl_direct(o), m, args);
}



#define JNI_CALL_NONVIRTUAL_METHOD(name, type, intern)                      \
type _Jv_JNI_CallNonvirtual##name##Method(JNIEnv *env, jobject obj,         \
										  jclass clazz, jmethodID methodID, \
										  ...)                              \
{                                                                           \
	java_handle_t *o;                                                       \
	classinfo     *c;                                                       \
	methodinfo    *m;                                                       \
	va_list        ap;                                                      \
	type           ret;                                                     \
                                                                            \
	o = (java_handle_t *) obj;                                              \
	c = LLNI_classinfo_unwrap(clazz);                                       \
	m = (methodinfo *) methodID;                                            \
                                                                            \
	va_start(ap, methodID);                                                 \
	ret = _Jv_jni_Call##intern##Method(o, c->vftbl, m, ap);                 \
	va_end(ap);                                                             \
                                                                            \
	return ret;                                                             \
}

JNI_CALL_NONVIRTUAL_METHOD(Boolean, jboolean, Int)
JNI_CALL_NONVIRTUAL_METHOD(Byte,    jbyte,    Int)
JNI_CALL_NONVIRTUAL_METHOD(Char,    jchar,    Int)
JNI_CALL_NONVIRTUAL_METHOD(Short,   jshort,   Int)
JNI_CALL_NONVIRTUAL_METHOD(Int,     jint,     Int)
JNI_CALL_NONVIRTUAL_METHOD(Long,    jlong,    Long)
JNI_CALL_NONVIRTUAL_METHOD(Float,   jfloat,   Float)
JNI_CALL_NONVIRTUAL_METHOD(Double,  jdouble,  Double)


#define JNI_CALL_NONVIRTUAL_METHOD_V(name, type, intern)                     \
type _Jv_JNI_CallNonvirtual##name##MethodV(JNIEnv *env, jobject obj,         \
										   jclass clazz, jmethodID methodID, \
										   va_list args)                     \
{                                                                            \
	java_handle_t *o;                                                        \
	classinfo     *c;                                                        \
	methodinfo    *m;                                                        \
	type           ret;                                                      \
                                                                             \
	o = (java_handle_t *) obj;                                               \
	c = LLNI_classinfo_unwrap(clazz);                                        \
	m = (methodinfo *) methodID;                                             \
                                                                             \
	ret = _Jv_jni_CallIntMethod(o, c->vftbl, m, args);                       \
                                                                             \
	return ret;                                                              \
}

JNI_CALL_NONVIRTUAL_METHOD_V(Boolean, jboolean, Int)
JNI_CALL_NONVIRTUAL_METHOD_V(Byte,    jbyte,    Int)
JNI_CALL_NONVIRTUAL_METHOD_V(Char,    jchar,    Int)
JNI_CALL_NONVIRTUAL_METHOD_V(Short,   jshort,   Int)
JNI_CALL_NONVIRTUAL_METHOD_V(Int,     jint,     Int)
JNI_CALL_NONVIRTUAL_METHOD_V(Long,    jlong,    Long)
JNI_CALL_NONVIRTUAL_METHOD_V(Float,   jfloat,   Float)
JNI_CALL_NONVIRTUAL_METHOD_V(Double,  jdouble,  Double)


#define JNI_CALL_NONVIRTUAL_METHOD_A(name, type, intern)                     \
type _Jv_JNI_CallNonvirtual##name##MethodA(JNIEnv *env, jobject obj,         \
										   jclass clazz, jmethodID methodID, \
										   const jvalue *args)               \
{                                                                            \
	log_text("JNI-Call: CallNonvirtual##name##MethodA: IMPLEMENT ME!");      \
                                                                             \
	return 0;                                                                \
}

JNI_CALL_NONVIRTUAL_METHOD_A(Boolean, jboolean, Int)
JNI_CALL_NONVIRTUAL_METHOD_A(Byte,    jbyte,    Int)
JNI_CALL_NONVIRTUAL_METHOD_A(Char,    jchar,    Int)
JNI_CALL_NONVIRTUAL_METHOD_A(Short,   jshort,   Int)
JNI_CALL_NONVIRTUAL_METHOD_A(Int,     jint,     Int)
JNI_CALL_NONVIRTUAL_METHOD_A(Long,    jlong,    Long)
JNI_CALL_NONVIRTUAL_METHOD_A(Float,   jfloat,   Float)
JNI_CALL_NONVIRTUAL_METHOD_A(Double,  jdouble,  Double)

jobject _Jv_JNI_CallNonvirtualObjectMethod(JNIEnv *env, jobject obj,
										   jclass clazz, jmethodID methodID,
										   ...)
{
	java_handle_t *o;
	classinfo     *c;
	methodinfo    *m;
	java_handle_t *r;
	va_list        ap;

	o = (java_handle_t *) obj;
	c = LLNI_classinfo_unwrap(clazz);
	m = (methodinfo *) methodID;

	va_start(ap, methodID);
	r = _Jv_jni_CallObjectMethod(o, c->vftbl, m, ap);
	va_end(ap);

	return _Jv_JNI_NewLocalRef(env, (jobject) r);
}


jobject _Jv_JNI_CallNonvirtualObjectMethodV(JNIEnv *env, jobject obj,
											jclass clazz, jmethodID methodID,
											va_list args)
{
	java_handle_t *o;
	classinfo     *c;
	methodinfo    *m;
	java_handle_t *r;

	o = (java_handle_t *) obj;
	c = LLNI_classinfo_unwrap(clazz);
	m = (methodinfo *) methodID;

	r = _Jv_jni_CallObjectMethod(o, c->vftbl, m, args);

	return _Jv_JNI_NewLocalRef(env, (jobject) r);
}


jobject _Jv_JNI_CallNonvirtualObjectMethodA(JNIEnv *env, jobject obj,
											jclass clazz, jmethodID methodID,
											const jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualObjectMethodA: IMPLEMENT ME!");

	return _Jv_JNI_NewLocalRef(env, NULL);
}


void _Jv_JNI_CallNonvirtualVoidMethod(JNIEnv *env, jobject obj, jclass clazz,
									  jmethodID methodID, ...)
{
	java_handle_t *o;
	classinfo     *c;
	methodinfo    *m;
	va_list        ap;

	o = (java_handle_t *) obj;
	c = LLNI_classinfo_unwrap(clazz);
	m = (methodinfo *) methodID;

	va_start(ap, methodID);
	_Jv_jni_CallVoidMethod(o, c->vftbl, m, ap);
	va_end(ap);
}


void _Jv_JNI_CallNonvirtualVoidMethodV(JNIEnv *env, jobject obj, jclass clazz,
									   jmethodID methodID, va_list args)
{
	java_handle_t *o;
	classinfo     *c;
	methodinfo    *m;

	o = (java_handle_t *) obj;
	c = LLNI_classinfo_unwrap(clazz);
	m = (methodinfo *) methodID;

	_Jv_jni_CallVoidMethod(o, c->vftbl, m, args);
}


void _Jv_JNI_CallNonvirtualVoidMethodA(JNIEnv *env, jobject obj, jclass clazz,
									   jmethodID methodID, const jvalue * args)
{	
	java_handle_t *o;
	classinfo     *c;
	methodinfo    *m;

	o = (java_handle_t *) obj;
	c = LLNI_classinfo_unwrap(clazz);
	m = (methodinfo *) methodID;

	_Jv_jni_CallVoidMethodA(o, c->vftbl, m, args);
}


/* Accessing Fields of Objects ************************************************/

/* GetFieldID ******************************************************************

   Returns the field ID for an instance (nonstatic) field of a
   class. The field is specified by its name and signature. The
   Get<type>Field and Set<type>Field families of accessor functions
   use field IDs to retrieve object fields.

*******************************************************************************/

jfieldID _Jv_JNI_GetFieldID(JNIEnv *env, jclass clazz, const char *name,
							const char *sig)
{
	classinfo *c;
	fieldinfo *f;
	utf       *uname;
	utf       *udesc;

	STATISTICS(jniinvokation());

	c = LLNI_classinfo_unwrap(clazz);

	/* XXX NPE check? */

	uname = utf_new_char((char *) name);
	udesc = utf_new_char((char *) sig);

	f = class_findfield(c, uname, udesc); 
	
	if (f == NULL)
		exceptions_throw_nosuchfielderror(c, uname);  

	return (jfieldID) f;
}


/* Get<type>Field Routines *****************************************************

   This family of accessor routines returns the value of an instance
   (nonstatic) field of an object. The field to access is specified by
   a field ID obtained by calling GetFieldID().

*******************************************************************************/

#define GET_FIELD(o,type,f) \
    *((type *) (((intptr_t) (o)) + ((intptr_t) ((fieldinfo *) (f))->offset)))

#define JNI_GET_FIELD(name, type, intern)                                 \
type _Jv_JNI_Get##name##Field(JNIEnv *env, jobject obj, jfieldID fieldID) \
{                                                                         \
	intern ret;                                                           \
                                                                          \
	STATISTICS(jniinvokation());                                          \
                                                                          \
	LLNI_CRITICAL_START;                                                  \
                                                                          \
	ret = GET_FIELD(LLNI_DIRECT((java_handle_t *) obj), intern, fieldID); \
                                                                          \
	LLNI_CRITICAL_END;                                                    \
                                                                          \
	return (type) ret;                                                    \
}

JNI_GET_FIELD(Boolean, jboolean, s4)
JNI_GET_FIELD(Byte,    jbyte,    s4)
JNI_GET_FIELD(Char,    jchar,    s4)
JNI_GET_FIELD(Short,   jshort,   s4)
JNI_GET_FIELD(Int,     jint,     s4)
JNI_GET_FIELD(Long,    jlong,    s8)
JNI_GET_FIELD(Float,   jfloat,   float)
JNI_GET_FIELD(Double,  jdouble,  double)


jobject _Jv_JNI_GetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID)
{
	java_handle_t *o;

	TRACEJNICALLS(("_Jv_JNI_GetObjectField(env=%p, obj=%p, fieldId=%p)", env, obj, fieldID));

	LLNI_CRITICAL_START;

	o = LLNI_WRAP(GET_FIELD(LLNI_DIRECT((java_handle_t *) obj), java_object_t*, fieldID));

	LLNI_CRITICAL_END;

	return _Jv_JNI_NewLocalRef(env, (jobject) o);
}


/* Set<type>Field Routines *****************************************************

   This family of accessor routines sets the value of an instance
   (nonstatic) field of an object. The field to access is specified by
   a field ID obtained by calling GetFieldID().

*******************************************************************************/

#define SET_FIELD(o,type,f,value) \
    *((type *) (((intptr_t) (o)) + ((intptr_t) ((fieldinfo *) (f))->offset))) = (type) (value)

#define JNI_SET_FIELD(name, type, intern)                                  \
void _Jv_JNI_Set##name##Field(JNIEnv *env, jobject obj, jfieldID fieldID,  \
							  type value)                                  \
{                                                                          \
	STATISTICS(jniinvokation());                                           \
                                                                           \
	LLNI_CRITICAL_START;                                                   \
                                                                           \
	SET_FIELD(LLNI_DIRECT((java_handle_t *) obj), intern, fieldID, value); \
	                                                                       \
	LLNI_CRITICAL_START;                                                   \
}

JNI_SET_FIELD(Boolean, jboolean, s4)
JNI_SET_FIELD(Byte,    jbyte,    s4)
JNI_SET_FIELD(Char,    jchar,    s4)
JNI_SET_FIELD(Short,   jshort,   s4)
JNI_SET_FIELD(Int,     jint,     s4)
JNI_SET_FIELD(Long,    jlong,    s8)
JNI_SET_FIELD(Float,   jfloat,   float)
JNI_SET_FIELD(Double,  jdouble,  double)


void _Jv_JNI_SetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID,
							jobject value)
{
	TRACEJNICALLS(("_Jv_JNI_SetObjectField(env=%p, obj=%p, fieldId=%p, value=%p)", env, obj, fieldID, value));

	LLNI_CRITICAL_START;

	SET_FIELD(obj, java_handle_t*, fieldID, LLNI_UNWRAP((java_handle_t*) value));

	LLNI_CRITICAL_END;
}


/* Calling Static Methods *****************************************************/

/* GetStaticMethodID ***********************************************************

   Returns the method ID for a static method of a class. The method is
   specified by its name and signature.

   GetStaticMethodID() causes an uninitialized class to be
   initialized.

*******************************************************************************/

jmethodID _Jv_JNI_GetStaticMethodID(JNIEnv *env, jclass clazz, const char *name,
									const char *sig)
{
	classinfo  *c;
	utf        *uname;
	utf        *udesc;
	methodinfo *m;

	TRACEJNICALLS(("_Jv_JNI_GetStaticMethodID(env=%p, clazz=%p, name=%s, sig=%s)", env, clazz, name, sig));

	c = LLNI_classinfo_unwrap(clazz);

	if (c == NULL)
		return NULL;

	if (!(c->state & CLASS_INITIALIZED))
		if (!initialize_class(c))
			return NULL;

	/* try to get the static method of the class */

	uname = utf_new_char((char *) name);
	udesc = utf_new_char((char *) sig);

 	m = class_resolvemethod(c, uname, udesc);

	if ((m == NULL) || !(m->flags & ACC_STATIC)) {
		exceptions_throw_nosuchmethoderror(c, uname, udesc);

		return NULL;
	}

	return (jmethodID) m;
}


#define JNI_CALL_STATIC_METHOD(name, type, intern)               \
type _Jv_JNI_CallStatic##name##Method(JNIEnv *env, jclass clazz, \
									  jmethodID methodID, ...)   \
{                                                                \
	methodinfo *m;                                               \
	va_list     ap;                                              \
	type        res;                                             \
                                                                 \
	m = (methodinfo *) methodID;                                 \
                                                                 \
	va_start(ap, methodID);                                      \
	res = _Jv_jni_Call##intern##Method(NULL, NULL, m, ap);       \
	va_end(ap);                                                  \
                                                                 \
	return res;                                                  \
}

JNI_CALL_STATIC_METHOD(Boolean, jboolean, Int)
JNI_CALL_STATIC_METHOD(Byte,    jbyte,    Int)
JNI_CALL_STATIC_METHOD(Char,    jchar,    Int)
JNI_CALL_STATIC_METHOD(Short,   jshort,   Int)
JNI_CALL_STATIC_METHOD(Int,     jint,     Int)
JNI_CALL_STATIC_METHOD(Long,    jlong,    Long)
JNI_CALL_STATIC_METHOD(Float,   jfloat,   Float)
JNI_CALL_STATIC_METHOD(Double,  jdouble,  Double)


#define JNI_CALL_STATIC_METHOD_V(name, type, intern)                     \
type _Jv_JNI_CallStatic##name##MethodV(JNIEnv *env, jclass clazz,        \
									   jmethodID methodID, va_list args) \
{                                                                        \
	methodinfo *m;                                                       \
	type        res;                                                     \
                                                                         \
	m = (methodinfo *) methodID;                                         \
                                                                         \
	res = _Jv_jni_Call##intern##Method(NULL, NULL, m, args);             \
                                                                         \
	return res;                                                          \
}

JNI_CALL_STATIC_METHOD_V(Boolean, jboolean, Int)
JNI_CALL_STATIC_METHOD_V(Byte,    jbyte,    Int)
JNI_CALL_STATIC_METHOD_V(Char,    jchar,    Int)
JNI_CALL_STATIC_METHOD_V(Short,   jshort,   Int)
JNI_CALL_STATIC_METHOD_V(Int,     jint,     Int)
JNI_CALL_STATIC_METHOD_V(Long,    jlong,    Long)
JNI_CALL_STATIC_METHOD_V(Float,   jfloat,   Float)
JNI_CALL_STATIC_METHOD_V(Double,  jdouble,  Double)


#define JNI_CALL_STATIC_METHOD_A(name, type, intern)                           \
type _Jv_JNI_CallStatic##name##MethodA(JNIEnv *env, jclass clazz,              \
									   jmethodID methodID, const jvalue *args) \
{                                                                              \
	methodinfo *m;                                                             \
	type        res;                                                           \
                                                                               \
	m = (methodinfo *) methodID;                                               \
                                                                               \
	res = _Jv_jni_Call##intern##MethodA(NULL, NULL, m, args);                  \
                                                                               \
	return res;                                                                \
}

JNI_CALL_STATIC_METHOD_A(Boolean, jboolean, Int)
JNI_CALL_STATIC_METHOD_A(Byte,    jbyte,    Int)
JNI_CALL_STATIC_METHOD_A(Char,    jchar,    Int)
JNI_CALL_STATIC_METHOD_A(Short,   jshort,   Int)
JNI_CALL_STATIC_METHOD_A(Int,     jint,     Int)
JNI_CALL_STATIC_METHOD_A(Long,    jlong,    Long)
JNI_CALL_STATIC_METHOD_A(Float,   jfloat,   Float)
JNI_CALL_STATIC_METHOD_A(Double,  jdouble,  Double)


jobject _Jv_JNI_CallStaticObjectMethod(JNIEnv *env, jclass clazz,
									   jmethodID methodID, ...)
{
	methodinfo    *m;
	java_handle_t *o;
	va_list        ap;

	m = (methodinfo *) methodID;

	va_start(ap, methodID);
	o = _Jv_jni_CallObjectMethod(NULL, NULL, m, ap);
	va_end(ap);

	return _Jv_JNI_NewLocalRef(env, (jobject) o);
}


jobject _Jv_JNI_CallStaticObjectMethodV(JNIEnv *env, jclass clazz,
										jmethodID methodID, va_list args)
{
	methodinfo    *m;
	java_handle_t *o;

	m = (methodinfo *) methodID;

	o = _Jv_jni_CallObjectMethod(NULL, NULL, m, args);

	return _Jv_JNI_NewLocalRef(env, (jobject) o);
}


jobject _Jv_JNI_CallStaticObjectMethodA(JNIEnv *env, jclass clazz,
										jmethodID methodID, const jvalue *args)
{
	methodinfo    *m;
	java_handle_t *o;

	m = (methodinfo *) methodID;

	o = _Jv_jni_CallObjectMethodA(NULL, NULL, m, args);

	return _Jv_JNI_NewLocalRef(env, (jobject) o);
}


void _Jv_JNI_CallStaticVoidMethod(JNIEnv *env, jclass clazz,
								  jmethodID methodID, ...)
{
	methodinfo *m;
	va_list     ap;

	m = (methodinfo *) methodID;

	va_start(ap, methodID);
	_Jv_jni_CallVoidMethod(NULL, NULL, m, ap);
	va_end(ap);
}


void _Jv_JNI_CallStaticVoidMethodV(JNIEnv *env, jclass clazz,
								   jmethodID methodID, va_list args)
{
	methodinfo *m;

	m = (methodinfo *) methodID;

	_Jv_jni_CallVoidMethod(NULL, NULL, m, args);
}


void _Jv_JNI_CallStaticVoidMethodA(JNIEnv *env, jclass clazz,
								   jmethodID methodID, const jvalue * args)
{
	methodinfo *m;

	m = (methodinfo *) methodID;

	_Jv_jni_CallVoidMethodA(NULL, NULL, m, args);
}


/* Accessing Static Fields ****************************************************/

/* GetStaticFieldID ************************************************************

   Returns the field ID for a static field of a class. The field is
   specified by its name and signature. The GetStatic<type>Field and
   SetStatic<type>Field families of accessor functions use field IDs
   to retrieve static fields.

*******************************************************************************/

jfieldID _Jv_JNI_GetStaticFieldID(JNIEnv *env, jclass clazz, const char *name,
								  const char *sig)
{
	classinfo *c;
	fieldinfo *f;
	utf       *uname;
	utf       *usig;

	STATISTICS(jniinvokation());

	c = LLNI_classinfo_unwrap(clazz);

	uname = utf_new_char((char *) name);
	usig  = utf_new_char((char *) sig);

	f = class_findfield(c, uname, usig);
	
	if (f == NULL)
		exceptions_throw_nosuchfielderror(c, uname);

	return (jfieldID) f;
}


/* GetStatic<type>Field ********************************************************

   This family of accessor routines returns the value of a static
   field of an object.

*******************************************************************************/

#define JNI_GET_STATIC_FIELD(name, type, field)                \
type _Jv_JNI_GetStatic##name##Field(JNIEnv *env, jclass clazz, \
									jfieldID fieldID)          \
{                                                              \
	classinfo *c;                                              \
	fieldinfo *f;                                              \
                                                               \
	STATISTICS(jniinvokation());                               \
                                                               \
	c = LLNI_classinfo_unwrap(clazz);                          \
	f = (fieldinfo *) fieldID;                                 \
                                                               \
	if (!(c->state & CLASS_INITIALIZED))                       \
		if (!initialize_class(c))                              \
			return 0;                                          \
                                                               \
	return f->value->field;                                    \
}

JNI_GET_STATIC_FIELD(Boolean, jboolean, i)
JNI_GET_STATIC_FIELD(Byte,    jbyte,    i)
JNI_GET_STATIC_FIELD(Char,    jchar,    i)
JNI_GET_STATIC_FIELD(Short,   jshort,   i)
JNI_GET_STATIC_FIELD(Int,     jint,     i)
JNI_GET_STATIC_FIELD(Long,    jlong,    l)
JNI_GET_STATIC_FIELD(Float,   jfloat,   f)
JNI_GET_STATIC_FIELD(Double,  jdouble,  d)


jobject _Jv_JNI_GetStaticObjectField(JNIEnv *env, jclass clazz,
									 jfieldID fieldID)
{
	classinfo     *c;
	fieldinfo     *f;
	java_handle_t *h;

	STATISTICS(jniinvokation());

	c = LLNI_classinfo_unwrap(clazz);
	f = (fieldinfo *) fieldID;

	if (!(c->state & CLASS_INITIALIZED))
		if (!initialize_class(c))
			return NULL;

	h = LLNI_WRAP(f->value->a);

	return _Jv_JNI_NewLocalRef(env, (jobject) h);
}


/*  SetStatic<type>Field *******************************************************

	This family of accessor routines sets the value of a static field
	of an object.

*******************************************************************************/

#define JNI_SET_STATIC_FIELD(name, type, field)                \
void _Jv_JNI_SetStatic##name##Field(JNIEnv *env, jclass clazz, \
									jfieldID fieldID,          \
									type value)                \
{                                                              \
	classinfo *c;                                              \
	fieldinfo *f;                                              \
                                                               \
	STATISTICS(jniinvokation());                               \
                                                               \
	c = LLNI_classinfo_unwrap(clazz);                          \
	f = (fieldinfo *) fieldID;                                 \
                                                               \
	if (!(c->state & CLASS_INITIALIZED))                       \
		if (!initialize_class(c))                              \
			return;                                            \
                                                               \
	f->value->field = value;                                   \
}

JNI_SET_STATIC_FIELD(Boolean, jboolean, i)
JNI_SET_STATIC_FIELD(Byte,    jbyte,    i)
JNI_SET_STATIC_FIELD(Char,    jchar,    i)
JNI_SET_STATIC_FIELD(Short,   jshort,   i)
JNI_SET_STATIC_FIELD(Int,     jint,     i)
JNI_SET_STATIC_FIELD(Long,    jlong,    l)
JNI_SET_STATIC_FIELD(Float,   jfloat,   f)
JNI_SET_STATIC_FIELD(Double,  jdouble,  d)


void _Jv_JNI_SetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID fieldID,
								  jobject value)
{
	classinfo *c;
	fieldinfo *f;

	STATISTICS(jniinvokation());

	c = LLNI_classinfo_unwrap(clazz);
	f = (fieldinfo *) fieldID;

	if (!(c->state & CLASS_INITIALIZED))
		if (!initialize_class(c))
			return;

	f->value->a = LLNI_UNWRAP((java_handle_t *) value);
}


/* String Operations **********************************************************/

/* NewString *******************************************************************

   Create new java.lang.String object from an array of Unicode
   characters.

*******************************************************************************/

jstring _Jv_JNI_NewString(JNIEnv *env, const jchar *buf, jsize len)
{
	java_lang_String        *s;
	java_handle_chararray_t *a;
	u4                       i;

	STATISTICS(jniinvokation());
	
	s = (java_lang_String *) builtin_new(class_java_lang_String);
	a = builtin_newarray_char(len);

	/* javastring or characterarray could not be created */
	if ((a == NULL) || (s == NULL))
		return NULL;

	/* copy text */
	for (i = 0; i < len; i++)
		LLNI_array_direct(a, i) = buf[i];

	LLNI_field_set_ref(s, value , a);
	LLNI_field_set_val(s, offset, 0);
	LLNI_field_set_val(s, count , len);

	return (jstring) _Jv_JNI_NewLocalRef(env, (jobject) s);
}


static jchar emptyStringJ[]={0,0};

/* GetStringLength *************************************************************

   Returns the length (the count of Unicode characters) of a Java
   string.

*******************************************************************************/

jsize _Jv_JNI_GetStringLength(JNIEnv *env, jstring str)
{
	java_lang_String *s;
	jsize             len;

	TRACEJNICALLS(("_Jv_JNI_GetStringLength(env=%p, str=%p)", env, str));

	s = (java_lang_String *) str;

	LLNI_field_get_val(s, count, len);

	return len;
}


/********************  convertes javastring to u2-array ****************************/
	
u2 *javastring_tou2(jstring so) 
{
	java_lang_String        *s;
	java_handle_chararray_t *a;
	u2                      *stringbuffer;
	u4                       i;
	int32_t                  count;
	int32_t                  offset;

	STATISTICS(jniinvokation());
	
	s = (java_lang_String *) so;

	if (!s)
		return NULL;

	LLNI_field_get_ref(s, value, a);

	if (!a)
		return NULL;

	LLNI_field_get_val(s, count, count);
	LLNI_field_get_val(s, offset, offset);

	/* allocate memory */

	stringbuffer = MNEW(u2, count + 1);

	/* copy text */

	for (i = 0; i < count; i++)
		stringbuffer[i] = LLNI_array_direct(a, offset + i);
	
	/* terminate string */

	stringbuffer[i] = '\0';

	return stringbuffer;
}


/* GetStringChars **************************************************************

   Returns a pointer to the array of Unicode characters of the
   string. This pointer is valid until ReleaseStringChars() is called.

*******************************************************************************/

const jchar *_Jv_JNI_GetStringChars(JNIEnv *env, jstring str, jboolean *isCopy)
{	
	jchar *jc;

	STATISTICS(jniinvokation());

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

void _Jv_JNI_ReleaseStringChars(JNIEnv *env, jstring str, const jchar *chars)
{
	java_lang_String *s;

	STATISTICS(jniinvokation());

	if (chars == emptyStringJ)
		return;

	s = (java_lang_String *) str;

	MFREE(((jchar *) chars), jchar, LLNI_field_direct(s, count) + 1);
}


/* NewStringUTF ****************************************************************

   Constructs a new java.lang.String object from an array of UTF-8
   characters.

*******************************************************************************/

jstring _Jv_JNI_NewStringUTF(JNIEnv *env, const char *bytes)
{
	java_lang_String *s;

	TRACEJNICALLS(("_Jv_JNI_NewStringUTF(env=%p, bytes=%s)", env, bytes));

	s = (java_lang_String *) javastring_safe_new_from_utf8(bytes);

    return (jstring) _Jv_JNI_NewLocalRef(env, (jobject) s);
}


/****************** returns the utf8 length in bytes of a string *******************/

jsize _Jv_JNI_GetStringUTFLength(JNIEnv *env, jstring string)
{   
	java_lang_String *s;
	s4                length;

	TRACEJNICALLS(("_Jv_JNI_GetStringUTFLength(env=%p, string=%p)", env, string));

	s = (java_lang_String *) string;

	length = u2_utflength(LLNI_field_direct(s, value)->data, LLNI_field_direct(s, count));

	return length;
}


/* GetStringUTFChars ***********************************************************

   Returns a pointer to an array of UTF-8 characters of the
   string. This array is valid until it is released by
   ReleaseStringUTFChars().

*******************************************************************************/

const char *_Jv_JNI_GetStringUTFChars(JNIEnv *env, jstring string,
									  jboolean *isCopy)
{
	utf *u;

	STATISTICS(jniinvokation());

	if (string == NULL)
		return "";

	if (isCopy)
		*isCopy = JNI_TRUE;
	
	u = javastring_toutf((java_handle_t *) string, false);

	if (u != NULL)
		return u->text;

	return "";
}


/* ReleaseStringUTFChars *******************************************************

   Informs the VM that the native code no longer needs access to
   utf. The utf argument is a pointer derived from string using
   GetStringUTFChars().

*******************************************************************************/

void _Jv_JNI_ReleaseStringUTFChars(JNIEnv *env, jstring string, const char *utf)
{
	STATISTICS(jniinvokation());

    /* XXX we don't release utf chars right now, perhaps that should be done 
	   later. Since there is always one reference the garbage collector will
	   never get them */
}


/* Array Operations ***********************************************************/

/* GetArrayLength **************************************************************

   Returns the number of elements in the array.

*******************************************************************************/

jsize _Jv_JNI_GetArrayLength(JNIEnv *env, jarray array)
{
	java_handle_t *a;
	jsize          size;

	STATISTICS(jniinvokation());

	a = (java_handle_t *) array;

	size = LLNI_array_size(a);

	return size;
}


/* NewObjectArray **************************************************************

   Constructs a new array holding objects in class elementClass. All
   elements are initially set to initialElement.

*******************************************************************************/

jobjectArray _Jv_JNI_NewObjectArray(JNIEnv *env, jsize length,
									jclass elementClass, jobject initialElement)
{
	classinfo                 *c;
	java_handle_t             *o;
	java_handle_objectarray_t *oa;
	s4                         i;

	STATISTICS(jniinvokation());

	c = LLNI_classinfo_unwrap(elementClass);
	o = (java_handle_t *) initialElement;

	if (length < 0) {
		exceptions_throw_negativearraysizeexception();
		return NULL;
	}

    oa = builtin_anewarray(length, c);

	if (oa == NULL)
		return NULL;

	/* set all elements to initialElement */

	for (i = 0; i < length; i++)
		array_objectarray_element_set(oa, i, o);

	return (jobjectArray) _Jv_JNI_NewLocalRef(env, (jobject) oa);
}


jobject _Jv_JNI_GetObjectArrayElement(JNIEnv *env, jobjectArray array,
									  jsize index)
{
	java_handle_objectarray_t *oa;
	java_handle_t             *o;

	STATISTICS(jniinvokation());

	oa = (java_handle_objectarray_t *) array;

	if (index >= LLNI_array_size(oa)) {
		exceptions_throw_arrayindexoutofboundsexception();
		return NULL;
	}

	o = array_objectarray_element_get(oa, index);

	return _Jv_JNI_NewLocalRef(env, (jobject) o);
}


void _Jv_JNI_SetObjectArrayElement(JNIEnv *env, jobjectArray array,
								   jsize index, jobject val)
{
	java_handle_objectarray_t *oa;
	java_handle_t             *o;

	STATISTICS(jniinvokation());

	oa = (java_handle_objectarray_t *) array;
	o  = (java_handle_t *) val;

	if (index >= LLNI_array_size(oa)) {
		exceptions_throw_arrayindexoutofboundsexception();
		return;
	}

	/* check if the class of value is a subclass of the element class
	   of the array */

	if (!builtin_canstore(oa, o))
		return;

	array_objectarray_element_set(oa, index, o);
}


#define JNI_NEW_ARRAY(name, type, intern)                \
type _Jv_JNI_New##name##Array(JNIEnv *env, jsize len)    \
{                                                        \
	java_handle_##intern##array_t *a;                    \
                                                         \
	STATISTICS(jniinvokation());                         \
                                                         \
	if (len < 0) {                                       \
		exceptions_throw_negativearraysizeexception();   \
		return NULL;                                     \
	}                                                    \
                                                         \
	a = builtin_newarray_##intern(len);                  \
                                                         \
	return (type) _Jv_JNI_NewLocalRef(env, (jobject) a); \
}

JNI_NEW_ARRAY(Boolean, jbooleanArray, boolean)
JNI_NEW_ARRAY(Byte,    jbyteArray,    byte)
JNI_NEW_ARRAY(Char,    jcharArray,    char)
JNI_NEW_ARRAY(Short,   jshortArray,   byte)
JNI_NEW_ARRAY(Int,     jintArray,     int)
JNI_NEW_ARRAY(Long,    jlongArray,    long)
JNI_NEW_ARRAY(Float,   jfloatArray,   float)
JNI_NEW_ARRAY(Double,  jdoubleArray,  double)


/* Get<PrimitiveType>ArrayElements *********************************************

   A family of functions that returns the body of the primitive array.

*******************************************************************************/

#define JNI_GET_ARRAY_ELEMENTS(name, type, intern)                     \
type *_Jv_JNI_Get##name##ArrayElements(JNIEnv *env, type##Array array, \
										 jboolean *isCopy)             \
{                                                                      \
	java_handle_##intern##array_t *a;                                  \
                                                                       \
	STATISTICS(jniinvokation());                                       \
                                                                       \
	a = (java_handle_##intern##array_t *) array;                       \
                                                                       \
	if (isCopy)                                                        \
		*isCopy = JNI_FALSE;                                           \
                                                                       \
	return (type *) LLNI_array_data(a);                                \
}

JNI_GET_ARRAY_ELEMENTS(Boolean, jboolean, boolean)
JNI_GET_ARRAY_ELEMENTS(Byte,    jbyte,    byte)
JNI_GET_ARRAY_ELEMENTS(Char,    jchar,    char)
JNI_GET_ARRAY_ELEMENTS(Short,   jshort,   short)
JNI_GET_ARRAY_ELEMENTS(Int,     jint,     int)
JNI_GET_ARRAY_ELEMENTS(Long,    jlong,    long)
JNI_GET_ARRAY_ELEMENTS(Float,   jfloat,   float)
JNI_GET_ARRAY_ELEMENTS(Double,  jdouble,  double)


/* Release<PrimitiveType>ArrayElements *****************************************

   A family of functions that informs the VM that the native code no
   longer needs access to elems. The elems argument is a pointer
   derived from array using the corresponding
   Get<PrimitiveType>ArrayElements() function. If necessary, this
   function copies back all changes made to elems to the original
   array.

*******************************************************************************/

#define JNI_RELEASE_ARRAY_ELEMENTS(name, type, intern, intern2)            \
void _Jv_JNI_Release##name##ArrayElements(JNIEnv *env, type##Array array,  \
										  type *elems, jint mode)          \
{                                                                          \
	java_handle_##intern##array_t *a;                                      \
                                                                           \
	STATISTICS(jniinvokation());                                           \
                                                                           \
	a = (java_handle_##intern##array_t *) array;                           \
                                                                           \
	if (elems != (type *) LLNI_array_data(a)) {                            \
		switch (mode) {                                                    \
		case JNI_COMMIT:                                                   \
			MCOPY(LLNI_array_data(a), elems, intern2, LLNI_array_size(a)); \
			break;                                                         \
		case 0:                                                            \
			MCOPY(LLNI_array_data(a), elems, intern2, LLNI_array_size(a)); \
			/* XXX TWISTI how should it be freed? */                       \
			break;                                                         \
		case JNI_ABORT:                                                    \
			/* XXX TWISTI how should it be freed? */                       \
			break;                                                         \
		}                                                                  \
	}                                                                      \
}

JNI_RELEASE_ARRAY_ELEMENTS(Boolean, jboolean, boolean, u1)
JNI_RELEASE_ARRAY_ELEMENTS(Byte,    jbyte,    byte,    s1)
JNI_RELEASE_ARRAY_ELEMENTS(Char,    jchar,    char,    u2)
JNI_RELEASE_ARRAY_ELEMENTS(Short,   jshort,   short,   s2)
JNI_RELEASE_ARRAY_ELEMENTS(Int,     jint,     int,     s4)
JNI_RELEASE_ARRAY_ELEMENTS(Long,    jlong,    long,    s8)
JNI_RELEASE_ARRAY_ELEMENTS(Float,   jfloat,   float,   float)
JNI_RELEASE_ARRAY_ELEMENTS(Double,  jdouble,  double,  double)


/*  Get<PrimitiveType>ArrayRegion **********************************************

	A family of functions that copies a region of a primitive array
	into a buffer.

*******************************************************************************/

#define JNI_GET_ARRAY_REGION(name, type, intern, intern2)               \
void _Jv_JNI_Get##name##ArrayRegion(JNIEnv *env, type##Array array,     \
									jsize start, jsize len, type *buf)  \
{                                                                       \
	java_handle_##intern##array_t *a;                                   \
                                                                        \
	STATISTICS(jniinvokation());                                        \
                                                                        \
	a = (java_handle_##intern##array_t *) array;                        \
                                                                        \
	if ((start < 0) || (len < 0) || (start + len > LLNI_array_size(a))) \
		exceptions_throw_arrayindexoutofboundsexception();              \
	else                                                                \
		MCOPY(buf, &LLNI_array_direct(a, start), intern2, len);         \
}

JNI_GET_ARRAY_REGION(Boolean, jboolean, boolean, u1)
JNI_GET_ARRAY_REGION(Byte,    jbyte,    byte,    s1)
JNI_GET_ARRAY_REGION(Char,    jchar,    char,    u2)
JNI_GET_ARRAY_REGION(Short,   jshort,   short,   s2)
JNI_GET_ARRAY_REGION(Int,     jint,     int,     s4)
JNI_GET_ARRAY_REGION(Long,    jlong,    long,    s8)
JNI_GET_ARRAY_REGION(Float,   jfloat,   float,   float)
JNI_GET_ARRAY_REGION(Double,  jdouble,  double,  double)


/*  Set<PrimitiveType>ArrayRegion **********************************************

	A family of functions that copies back a region of a primitive
	array from a buffer.

*******************************************************************************/

#define JNI_SET_ARRAY_REGION(name, type, intern, intern2)                    \
void _Jv_JNI_Set##name##ArrayRegion(JNIEnv *env, type##Array array,          \
									jsize start, jsize len, const type *buf) \
{                                                                            \
	java_handle_##intern##array_t *a;                                        \
                                                                             \
	STATISTICS(jniinvokation());                                             \
                                                                             \
	a = (java_handle_##intern##array_t *) array;                             \
                                                                             \
	if ((start < 0) || (len < 0) || (start + len > LLNI_array_size(a)))      \
		exceptions_throw_arrayindexoutofboundsexception();                   \
	else                                                                     \
		MCOPY(&LLNI_array_direct(a, start), buf, intern2, len);              \
}

JNI_SET_ARRAY_REGION(Boolean, jboolean, boolean, u1)
JNI_SET_ARRAY_REGION(Byte,    jbyte,    byte,    s1)
JNI_SET_ARRAY_REGION(Char,    jchar,    char,    u2)
JNI_SET_ARRAY_REGION(Short,   jshort,   short,   s2)
JNI_SET_ARRAY_REGION(Int,     jint,     int,     s4)
JNI_SET_ARRAY_REGION(Long,    jlong,    long,    s8)
JNI_SET_ARRAY_REGION(Float,   jfloat,   float,   float)
JNI_SET_ARRAY_REGION(Double,  jdouble,  double,  double)


/* Registering Native Methods *************************************************/

/* RegisterNatives *************************************************************

   Registers native methods with the class specified by the clazz
   argument. The methods parameter specifies an array of
   JNINativeMethod structures that contain the names, signatures, and
   function pointers of the native methods. The nMethods parameter
   specifies the number of native methods in the array.

*******************************************************************************/

jint _Jv_JNI_RegisterNatives(JNIEnv *env, jclass clazz,
							 const JNINativeMethod *methods, jint nMethods)
{
	classinfo *c;

	STATISTICS(jniinvokation());

	c = LLNI_classinfo_unwrap(clazz);

	/* XXX: if implemented this needs a call to jvmti_NativeMethodBind
	if (jvmti) jvmti_NativeMethodBind(method, address,  new_address_ptr);
	*/

	native_method_register(c->name, methods, nMethods);

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

jint _Jv_JNI_UnregisterNatives(JNIEnv *env, jclass clazz)
{
	STATISTICS(jniinvokation());

	/* XXX TWISTI hmm, maybe we should not support that (like kaffe) */

    log_text("JNI-Call: UnregisterNatives: IMPLEMENT ME!!!");

    return 0;
}


/* Monitor Operations *********************************************************/

/* MonitorEnter ****************************************************************

   Enters the monitor associated with the underlying Java object
   referred to by obj.

*******************************************************************************/

jint _Jv_JNI_MonitorEnter(JNIEnv *env, jobject obj)
{
	STATISTICS(jniinvokation());

	if (obj == NULL) {
		exceptions_throw_nullpointerexception();
		return JNI_ERR;
	}

	LOCK_MONITOR_ENTER(obj);

	return JNI_OK;
}


/* MonitorExit *****************************************************************

   The current thread must be the owner of the monitor associated with
   the underlying Java object referred to by obj. The thread
   decrements the counter indicating the number of times it has
   entered this monitor. If the value of the counter becomes zero, the
   current thread releases the monitor.

*******************************************************************************/

jint _Jv_JNI_MonitorExit(JNIEnv *env, jobject obj)
{
	STATISTICS(jniinvokation());

	if (obj == NULL) {
		exceptions_throw_nullpointerexception();
		return JNI_ERR;
	}

	LOCK_MONITOR_EXIT(obj);

	return JNI_OK;
}


/* JavaVM Interface ***********************************************************/

/* GetJavaVM *******************************************************************

   Returns the Java VM interface (used in the Invocation API)
   associated with the current thread. The result is placed at the
   location pointed to by the second argument, vm.

*******************************************************************************/

jint _Jv_JNI_GetJavaVM(JNIEnv *env, JavaVM **vm)
{
	STATISTICS(jniinvokation());

    *vm = (JavaVM *) _Jv_jvm;

	return 0;
}


/* GetStringRegion *************************************************************

   Copies len number of Unicode characters beginning at offset start
   to the given buffer buf.

   Throws StringIndexOutOfBoundsException on index overflow.

*******************************************************************************/

void _Jv_JNI_GetStringRegion(JNIEnv* env, jstring str, jsize start, jsize len,
							 jchar *buf)
{
	java_lang_String        *s;
	java_handle_chararray_t *ca;

	STATISTICS(jniinvokation());

	s  = (java_lang_String *) str;
	LLNI_field_get_ref(s, value, ca);

	if ((start < 0) || (len < 0) || (start > LLNI_field_direct(s, count)) ||
		(start + len > LLNI_field_direct(s, count))) {
		exceptions_throw_stringindexoutofboundsexception();
		return;
	}

	MCOPY(buf, &LLNI_array_direct(ca, start), u2, len);
}


/* GetStringUTFRegion **********************************************************

    Translates len number of Unicode characters beginning at offset
    start into UTF-8 format and place the result in the given buffer
    buf.

    Throws StringIndexOutOfBoundsException on index overflow. 

*******************************************************************************/

void _Jv_JNI_GetStringUTFRegion(JNIEnv* env, jstring str, jsize start,
								jsize len, char *buf)
{
	java_lang_String        *s;
	java_handle_chararray_t *ca;
	s4                       i;
	int32_t                  count;
	int32_t                  offset;

	TRACEJNICALLS(("_Jv_JNI_GetStringUTFRegion(env=%p, str=%p, start=%d, len=%d, buf=%p)", env, str, start, len, buf));

	s  = (java_lang_String *) str;
	LLNI_field_get_ref(s, value, ca);
	LLNI_field_get_val(s, count, count);
	LLNI_field_get_val(s, offset, offset);

	if ((start < 0) || (len < 0) || (start > count) || (start + len > count)) {
		exceptions_throw_stringindexoutofboundsexception();
		return;
	}

	for (i = 0; i < len; i++)
		buf[i] = LLNI_array_direct(ca, offset + start + i);

	buf[i] = '\0';
}


/* GetPrimitiveArrayCritical ***************************************************

   Obtain a direct pointer to array elements.

*******************************************************************************/

void *_Jv_JNI_GetPrimitiveArrayCritical(JNIEnv *env, jarray array,
										jboolean *isCopy)
{
	java_handle_bytearray_t *ba;
	jbyte                   *bp;

	ba = (java_handle_bytearray_t *) array;

	/* do the same as Kaffe does */

	bp = _Jv_JNI_GetByteArrayElements(env, (jbyteArray) ba, isCopy);

	return (void *) bp;
}


/* ReleasePrimitiveArrayCritical ***********************************************

   No specific documentation.

*******************************************************************************/

void _Jv_JNI_ReleasePrimitiveArrayCritical(JNIEnv *env, jarray array,
										   void *carray, jint mode)
{
	STATISTICS(jniinvokation());

	/* do the same as Kaffe does */

	_Jv_JNI_ReleaseByteArrayElements(env, (jbyteArray) array, (jbyte *) carray,
									 mode);
}


/* GetStringCritical ***********************************************************

   The semantics of these two functions are similar to the existing
   Get/ReleaseStringChars functions.

*******************************************************************************/

const jchar *_Jv_JNI_GetStringCritical(JNIEnv *env, jstring string,
									   jboolean *isCopy)
{
	STATISTICS(jniinvokation());

	return _Jv_JNI_GetStringChars(env, string, isCopy);
}


void _Jv_JNI_ReleaseStringCritical(JNIEnv *env, jstring string,
								   const jchar *cstring)
{
	STATISTICS(jniinvokation());

	_Jv_JNI_ReleaseStringChars(env, string, cstring);
}


jweak _Jv_JNI_NewWeakGlobalRef(JNIEnv* env, jobject obj)
{
	TRACEJNICALLS(("_Jv_JNI_NewWeakGlobalRef(env=%p, obj=%p): IMPLEMENT ME!", env, obj));

	return obj;
}


void _Jv_JNI_DeleteWeakGlobalRef(JNIEnv* env, jweak ref)
{
	TRACEJNICALLS(("_Jv_JNI_DeleteWeakGlobalRef(env=%p, ref=%p): IMPLEMENT ME", env, ref));
}


/* NewGlobalRef ****************************************************************

   Creates a new global reference to the object referred to by the obj
   argument.

*******************************************************************************/
    
jobject _Jv_JNI_NewGlobalRef(JNIEnv* env, jobject obj)
{
	hashtable_global_ref_entry *gre;
	u4   key;                           /* hashkey                            */
	u4   slot;                          /* slot in hashtable                  */
	java_handle_t *o;

	STATISTICS(jniinvokation());

	o = (java_handle_t *) obj;

	LOCK_MONITOR_ENTER(hashtable_global_ref->header);

	LLNI_CRITICAL_START;

	/* normally addresses are aligned to 4, 8 or 16 bytes */

	key  = heap_hashcode(LLNI_DIRECT(o)) >> 4; /* align to 16-byte boundaries */
	slot = key & (hashtable_global_ref->size - 1);
	gre  = hashtable_global_ref->ptr[slot];
	
	/* search external hash chain for the entry */

	while (gre) {
		if (gre->o == LLNI_DIRECT(o)) {
			/* global object found, increment the reference */

			gre->refs++;

			break;
		}

		gre = gre->hashlink;                /* next element in external chain */
	}

	LLNI_CRITICAL_END;

	/* global ref not found, create a new one */

	if (gre == NULL) {
		gre = NEW(hashtable_global_ref_entry);

#if defined(ENABLE_GC_CACAO)
		/* register global ref with the GC */

		gc_reference_register(&(gre->o), GC_REFTYPE_JNI_GLOBALREF);
#endif

		LLNI_CRITICAL_START;

		gre->o    = LLNI_DIRECT(o);
		gre->refs = 1;

		LLNI_CRITICAL_END;

		/* insert entry into hashtable */

		gre->hashlink = hashtable_global_ref->ptr[slot];

		hashtable_global_ref->ptr[slot] = gre;

		/* update number of hashtable-entries */

		hashtable_global_ref->entries++;
	}

	LOCK_MONITOR_EXIT(hashtable_global_ref->header);

#if defined(ENABLE_HANDLES)
	return gre;
#else
	return obj;
#endif
}


/* DeleteGlobalRef *************************************************************

   Deletes the global reference pointed to by globalRef.

*******************************************************************************/

void _Jv_JNI_DeleteGlobalRef(JNIEnv* env, jobject globalRef)
{
	hashtable_global_ref_entry *gre;
	hashtable_global_ref_entry *prevgre;
	u4   key;                           /* hashkey                            */
	u4   slot;                          /* slot in hashtable                  */
	java_handle_t              *o;

	STATISTICS(jniinvokation());

	o = (java_handle_t *) globalRef;

	LOCK_MONITOR_ENTER(hashtable_global_ref->header);

	LLNI_CRITICAL_START;

	/* normally addresses are aligned to 4, 8 or 16 bytes */

	key  = heap_hashcode(LLNI_DIRECT(o)) >> 4; /* align to 16-byte boundaries */
	slot = key & (hashtable_global_ref->size - 1);
	gre  = hashtable_global_ref->ptr[slot];

	/* initialize prevgre */

	prevgre = NULL;

	/* search external hash chain for the entry */

	while (gre) {
		if (gre->o == LLNI_DIRECT(o)) {
			/* global object found, decrement the reference count */

			gre->refs--;

			/* if reference count is 0, remove the entry */

			if (gre->refs == 0) {
				/* special handling if it's the first in the chain */

				if (prevgre == NULL)
					hashtable_global_ref->ptr[slot] = gre->hashlink;
				else
					prevgre->hashlink = gre->hashlink;

#if defined(ENABLE_GC_CACAO)
				/* unregister global ref with the GC */

				gc_reference_unregister(&(gre->o));
#endif

				FREE(gre, hashtable_global_ref_entry);
			}

			LLNI_CRITICAL_END;

			LOCK_MONITOR_EXIT(hashtable_global_ref->header);

			return;
		}

		prevgre = gre;                    /* save current pointer for removal */
		gre     = gre->hashlink;            /* next element in external chain */
	}

	log_println("JNI-DeleteGlobalRef: global reference not found");

	LLNI_CRITICAL_END;

	LOCK_MONITOR_EXIT(hashtable_global_ref->header);
}


/* ExceptionCheck **************************************************************

   Returns JNI_TRUE when there is a pending exception; otherwise,
   returns JNI_FALSE.

*******************************************************************************/

jboolean _Jv_JNI_ExceptionCheck(JNIEnv *env)
{
	java_handle_t *o;

	STATISTICS(jniinvokation());

	o = exceptions_get_exception();

	return (o != NULL) ? JNI_TRUE : JNI_FALSE;
}


/* New JNI 1.4 functions ******************************************************/

/* NewDirectByteBuffer *********************************************************

   Allocates and returns a direct java.nio.ByteBuffer referring to the
   block of memory starting at the memory address address and
   extending capacity bytes.

*******************************************************************************/

jobject _Jv_JNI_NewDirectByteBuffer(JNIEnv *env, void *address, jlong capacity)
{
#if defined(ENABLE_JAVASE)
# if defined(WITH_CLASSPATH_GNU)
	java_handle_t           *nbuf;

# if SIZEOF_VOID_P == 8
	gnu_classpath_Pointer64 *paddress;
# else
	gnu_classpath_Pointer32 *paddress;
# endif

	TRACEJNICALLS(("_Jv_JNI_NewDirectByteBuffer(env=%p, address=%p, capacity=%ld", env, address, capacity));

	/* alocate a gnu.classpath.Pointer{32,64} object */

# if SIZEOF_VOID_P == 8
	if (!(paddress = (gnu_classpath_Pointer64 *)
		  builtin_new(class_gnu_classpath_Pointer64)))
# else
	if (!(paddress = (gnu_classpath_Pointer32 *)
		  builtin_new(class_gnu_classpath_Pointer32)))
# endif
		return NULL;

	/* fill gnu.classpath.Pointer{32,64} with address */

	LLNI_field_set_val(paddress, data, (ptrint) address);

	/* create a java.nio.DirectByteBufferImpl$ReadWrite object */

	nbuf = (*env)->NewObject(env, class_java_nio_DirectByteBufferImpl_ReadWrite,
							 (jmethodID) dbbirw_init, NULL, paddress,
							 (jint) capacity, (jint) capacity, (jint) 0);

	/* add local reference and return the value */

	return _Jv_JNI_NewLocalRef(env, nbuf);

# elif defined(WITH_CLASSPATH_SUN)

	jobject o;
	int64_t addr;
	int32_t cap;

	TRACEJNICALLS(("_Jv_JNI_NewDirectByteBuffer(env=%p, address=%p, capacity=%ld", env, address, capacity));

	/* Be paranoid about address sign-extension. */

	addr = (int64_t) ((uintptr_t) address);
	cap  = (int32_t) capacity;

	o = (*env)->NewObject(env, (jclass) class_java_nio_DirectByteBuffer,
						  (jmethodID) dbb_init, addr, cap);

	/* Add local reference and return the value. */

	return _Jv_JNI_NewLocalRef(env, o);

# else
#  error unknown classpath configuration
# endif

#else
	vm_abort("_Jv_JNI_NewDirectByteBuffer: not implemented in this configuration");

	/* keep compiler happy */

	return NULL;
#endif
}


/* GetDirectBufferAddress ******************************************************

   Fetches and returns the starting address of the memory region
   referenced by the given direct java.nio.Buffer.

*******************************************************************************/

void *_Jv_JNI_GetDirectBufferAddress(JNIEnv *env, jobject buf)
{
#if defined(ENABLE_JAVASE)
	java_handle_t                 *h;

# if defined(WITH_CLASSPATH_GNU)

	java_nio_DirectByteBufferImpl *nbuf;
#  if SIZEOF_VOID_P == 8
	gnu_classpath_Pointer64       *paddress;
#  else
	gnu_classpath_Pointer32       *paddress;
#  endif
	void                          *address;

	TRACEJNICALLS(("_Jv_JNI_GetDirectBufferAddress(env=%p, buf=%p)", env, buf));

	/* Prevent compiler warning. */

	h = (java_handle_t *) buf;

	if ((h != NULL) && !builtin_instanceof(h, class_java_nio_Buffer))
		return NULL;

	nbuf = (java_nio_DirectByteBufferImpl *) buf;

	LLNI_field_get_ref(nbuf, address, paddress);

	if (paddress == NULL)
		return NULL;

	LLNI_field_get_val(paddress, data, address);
	/* this was the cast to avaoid warning: (void *) paddress->data */

	return address;

# elif defined(WITH_CLASSPATH_SUN)

	java_nio_Buffer *o;
	int64_t          address;
	void            *p;

	TRACEJNICALLS(("_Jv_JNI_GetDirectBufferAddress(env=%p, buf=%p)", env, buf));

	/* Prevent compiler warning. */

	h = (java_handle_t *) buf;

	if ((h != NULL) && !builtin_instanceof(h, class_sun_nio_ch_DirectBuffer))
		return NULL;

	o = (java_nio_Buffer *) buf;

	LLNI_field_get_val(o, address, address);

	p = (void *) (intptr_t) address;

	return p;

# else
#  error unknown classpath configuration
# endif

#else

	vm_abort("_Jv_JNI_GetDirectBufferAddress: not implemented in this configuration");

	/* keep compiler happy */

	return NULL;

#endif
}


/* GetDirectBufferCapacity *****************************************************

   Fetches and returns the capacity in bytes of the memory region
   referenced by the given direct java.nio.Buffer.

*******************************************************************************/

jlong _Jv_JNI_GetDirectBufferCapacity(JNIEnv* env, jobject buf)
{
#if defined(ENABLE_JAVASE) && defined(WITH_CLASSPATH_GNU)
	java_handle_t   *o;
	java_nio_Buffer *nbuf;
	jlong            capacity;

	STATISTICS(jniinvokation());

	o = (java_handle_t *) buf;

	if (!builtin_instanceof(o, class_java_nio_DirectByteBufferImpl))
		return -1;

	nbuf = (java_nio_Buffer *) o;

	LLNI_field_get_val(nbuf, cap, capacity);

	return capacity;
#else
	vm_abort("_Jv_JNI_GetDirectBufferCapacity: not implemented in this configuration");

	/* keep compiler happy */

	return 0;
#endif
}


/* GetObjectRefType ************************************************************

   Returns the type of the object referred to by the obj argument. The
   argument obj can either be a local, global or weak global
   reference.

*******************************************************************************/

jobjectRefType jni_GetObjectRefType(JNIEnv *env, jobject obj)
{
	log_println("jni_GetObjectRefType: IMPLEMENT ME!");

	return -1;
}


/* DestroyJavaVM ***************************************************************

   Unloads a Java VM and reclaims its resources. Only the main thread
   can unload the VM. The system waits until the main thread is only
   remaining user thread before it destroys the VM.

*******************************************************************************/

jint _Jv_JNI_DestroyJavaVM(JavaVM *vm)
{
	int32_t status;

	TRACEJNICALLS(("_Jv_JNI_DestroyJavaVM(vm=%p)", vm));

    status = vm_destroy(vm);

	return status;
}


/* AttachCurrentThread *********************************************************

   Attaches the current thread to a Java VM. Returns a JNI interface
   pointer in the JNIEnv argument.

   Trying to attach a thread that is already attached is a no-op.

   A native thread cannot be attached simultaneously to two Java VMs.

   When a thread is attached to the VM, the context class loader is
   the bootstrap loader.

*******************************************************************************/

static s4 jni_attach_current_thread(void **p_env, void *thr_args, bool isdaemon)
{
	JavaVMAttachArgs *vm_aargs;

#if defined(ENABLE_THREADS)
	if (threads_get_current_threadobject() == NULL) {
		vm_aargs = (JavaVMAttachArgs *) thr_args;

		if (vm_aargs != NULL) {
			if ((vm_aargs->version != JNI_VERSION_1_2) &&
				(vm_aargs->version != JNI_VERSION_1_4))
				return JNI_EVERSION;
		}

		if (!threads_attach_current_thread(vm_aargs, false))
			return JNI_ERR;

		if (!localref_table_init())
			return JNI_ERR;
	}
#endif

	*p_env = _Jv_env;

	return JNI_OK;
}


jint _Jv_JNI_AttachCurrentThread(JavaVM *vm, void **p_env, void *thr_args)
{
	STATISTICS(jniinvokation());

	return jni_attach_current_thread(p_env, thr_args, false);
}


/* DetachCurrentThread *********************************************************

   Detaches the current thread from a Java VM. All Java monitors held
   by this thread are released. All Java threads waiting for this
   thread to die are notified.

   In JDK 1.1, the main thread cannot be detached from the VM. It must
   call DestroyJavaVM to unload the entire VM.

   In the JDK, the main thread can be detached from the VM.

   The main thread, which is the thread that created the Java VM,
   cannot be detached from the VM. Instead, the main thread must call
   JNI_DestroyJavaVM() to unload the entire VM.

*******************************************************************************/

jint _Jv_JNI_DetachCurrentThread(JavaVM *vm)
{
#if defined(ENABLE_THREADS)
	threadobject *thread;

	STATISTICS(jniinvokation());

	thread = threads_get_current_threadobject();

	if (thread == NULL)
		return JNI_ERR;

	/* We need to pop all frames before we can destroy the table. */

	localref_frame_pop_all();

	if (!localref_table_destroy())
		return JNI_ERR;

	if (!threads_detach_thread(thread))
		return JNI_ERR;
#endif

	return JNI_OK;
}


/* GetEnv **********************************************************************

   If the current thread is not attached to the VM, sets *env to NULL,
   and returns JNI_EDETACHED. If the specified version is not
   supported, sets *env to NULL, and returns JNI_EVERSION. Otherwise,
   sets *env to the appropriate interface, and returns JNI_OK.

*******************************************************************************/

jint _Jv_JNI_GetEnv(JavaVM *vm, void **env, jint version)
{
	TRACEJNICALLS(("_Jv_JNI_GetEnv(vm=%p, env=%p, %d=version)", vm, env, version));

#if defined(ENABLE_THREADS)
	if (threads_get_current_threadobject() == NULL) {
		*env = NULL;

		return JNI_EDETACHED;
	}
#endif

	/* Check the JNI version. */

	if (jni_version_check(version) == true) {
		*env = _Jv_env;
		return JNI_OK;
	}

#if defined(ENABLE_JVMTI)
	if ((version & JVMTI_VERSION_MASK_INTERFACE_TYPE) 
		== JVMTI_VERSION_INTERFACE_JVMTI) {

		*env = (void *) jvmti_new_environment();

		if (env != NULL)
			return JNI_OK;
	}
#endif
	
	*env = NULL;

	return JNI_EVERSION;
}


/* AttachCurrentThreadAsDaemon *************************************************

   Same semantics as AttachCurrentThread, but the newly-created
   java.lang.Thread instance is a daemon.

   If the thread has already been attached via either
   AttachCurrentThread or AttachCurrentThreadAsDaemon, this routine
   simply sets the value pointed to by penv to the JNIEnv of the
   current thread. In this case neither AttachCurrentThread nor this
   routine have any effect on the daemon status of the thread.

*******************************************************************************/

jint _Jv_JNI_AttachCurrentThreadAsDaemon(JavaVM *vm, void **penv, void *args)
{
	STATISTICS(jniinvokation());

	return jni_attach_current_thread(penv, args, true);
}


/* JNI invocation table *******************************************************/

const struct JNIInvokeInterface_ _Jv_JNIInvokeInterface = {
	NULL,
	NULL,
	NULL,

	_Jv_JNI_DestroyJavaVM,
	_Jv_JNI_AttachCurrentThread,
	_Jv_JNI_DetachCurrentThread,
	_Jv_JNI_GetEnv,
	_Jv_JNI_AttachCurrentThreadAsDaemon
};


/* JNI function table *********************************************************/

struct JNINativeInterface_ _Jv_JNINativeInterface = {
	NULL,
	NULL,
	NULL,
	NULL,    
	_Jv_JNI_GetVersion,

	_Jv_JNI_DefineClass,
	_Jv_JNI_FindClass,
	_Jv_JNI_FromReflectedMethod,
	_Jv_JNI_FromReflectedField,
	_Jv_JNI_ToReflectedMethod,
	_Jv_JNI_GetSuperclass,
	_Jv_JNI_IsAssignableFrom,
	_Jv_JNI_ToReflectedField,

	_Jv_JNI_Throw,
	_Jv_JNI_ThrowNew,
	_Jv_JNI_ExceptionOccurred,
	_Jv_JNI_ExceptionDescribe,
	_Jv_JNI_ExceptionClear,
	_Jv_JNI_FatalError,
	_Jv_JNI_PushLocalFrame,
	_Jv_JNI_PopLocalFrame,

	_Jv_JNI_NewGlobalRef,
	_Jv_JNI_DeleteGlobalRef,
	_Jv_JNI_DeleteLocalRef,
	_Jv_JNI_IsSameObject,
	_Jv_JNI_NewLocalRef,
	_Jv_JNI_EnsureLocalCapacity,

	_Jv_JNI_AllocObject,
	_Jv_JNI_NewObject,
	_Jv_JNI_NewObjectV,
	_Jv_JNI_NewObjectA,

	_Jv_JNI_GetObjectClass,
	_Jv_JNI_IsInstanceOf,

	_Jv_JNI_GetMethodID,

	_Jv_JNI_CallObjectMethod,
	_Jv_JNI_CallObjectMethodV,
	_Jv_JNI_CallObjectMethodA,
	_Jv_JNI_CallBooleanMethod,
	_Jv_JNI_CallBooleanMethodV,
	_Jv_JNI_CallBooleanMethodA,
	_Jv_JNI_CallByteMethod,
	_Jv_JNI_CallByteMethodV,
	_Jv_JNI_CallByteMethodA,
	_Jv_JNI_CallCharMethod,
	_Jv_JNI_CallCharMethodV,
	_Jv_JNI_CallCharMethodA,
	_Jv_JNI_CallShortMethod,
	_Jv_JNI_CallShortMethodV,
	_Jv_JNI_CallShortMethodA,
	_Jv_JNI_CallIntMethod,
	_Jv_JNI_CallIntMethodV,
	_Jv_JNI_CallIntMethodA,
	_Jv_JNI_CallLongMethod,
	_Jv_JNI_CallLongMethodV,
	_Jv_JNI_CallLongMethodA,
	_Jv_JNI_CallFloatMethod,
	_Jv_JNI_CallFloatMethodV,
	_Jv_JNI_CallFloatMethodA,
	_Jv_JNI_CallDoubleMethod,
	_Jv_JNI_CallDoubleMethodV,
	_Jv_JNI_CallDoubleMethodA,
	_Jv_JNI_CallVoidMethod,
	_Jv_JNI_CallVoidMethodV,
	_Jv_JNI_CallVoidMethodA,

	_Jv_JNI_CallNonvirtualObjectMethod,
	_Jv_JNI_CallNonvirtualObjectMethodV,
	_Jv_JNI_CallNonvirtualObjectMethodA,
	_Jv_JNI_CallNonvirtualBooleanMethod,
	_Jv_JNI_CallNonvirtualBooleanMethodV,
	_Jv_JNI_CallNonvirtualBooleanMethodA,
	_Jv_JNI_CallNonvirtualByteMethod,
	_Jv_JNI_CallNonvirtualByteMethodV,
	_Jv_JNI_CallNonvirtualByteMethodA,
	_Jv_JNI_CallNonvirtualCharMethod,
	_Jv_JNI_CallNonvirtualCharMethodV,
	_Jv_JNI_CallNonvirtualCharMethodA,
	_Jv_JNI_CallNonvirtualShortMethod,
	_Jv_JNI_CallNonvirtualShortMethodV,
	_Jv_JNI_CallNonvirtualShortMethodA,
	_Jv_JNI_CallNonvirtualIntMethod,
	_Jv_JNI_CallNonvirtualIntMethodV,
	_Jv_JNI_CallNonvirtualIntMethodA,
	_Jv_JNI_CallNonvirtualLongMethod,
	_Jv_JNI_CallNonvirtualLongMethodV,
	_Jv_JNI_CallNonvirtualLongMethodA,
	_Jv_JNI_CallNonvirtualFloatMethod,
	_Jv_JNI_CallNonvirtualFloatMethodV,
	_Jv_JNI_CallNonvirtualFloatMethodA,
	_Jv_JNI_CallNonvirtualDoubleMethod,
	_Jv_JNI_CallNonvirtualDoubleMethodV,
	_Jv_JNI_CallNonvirtualDoubleMethodA,
	_Jv_JNI_CallNonvirtualVoidMethod,
	_Jv_JNI_CallNonvirtualVoidMethodV,
	_Jv_JNI_CallNonvirtualVoidMethodA,

	_Jv_JNI_GetFieldID,

	_Jv_JNI_GetObjectField,
	_Jv_JNI_GetBooleanField,
	_Jv_JNI_GetByteField,
	_Jv_JNI_GetCharField,
	_Jv_JNI_GetShortField,
	_Jv_JNI_GetIntField,
	_Jv_JNI_GetLongField,
	_Jv_JNI_GetFloatField,
	_Jv_JNI_GetDoubleField,
	_Jv_JNI_SetObjectField,
	_Jv_JNI_SetBooleanField,
	_Jv_JNI_SetByteField,
	_Jv_JNI_SetCharField,
	_Jv_JNI_SetShortField,
	_Jv_JNI_SetIntField,
	_Jv_JNI_SetLongField,
	_Jv_JNI_SetFloatField,
	_Jv_JNI_SetDoubleField,

	_Jv_JNI_GetStaticMethodID,

	_Jv_JNI_CallStaticObjectMethod,
	_Jv_JNI_CallStaticObjectMethodV,
	_Jv_JNI_CallStaticObjectMethodA,
	_Jv_JNI_CallStaticBooleanMethod,
	_Jv_JNI_CallStaticBooleanMethodV,
	_Jv_JNI_CallStaticBooleanMethodA,
	_Jv_JNI_CallStaticByteMethod,
	_Jv_JNI_CallStaticByteMethodV,
	_Jv_JNI_CallStaticByteMethodA,
	_Jv_JNI_CallStaticCharMethod,
	_Jv_JNI_CallStaticCharMethodV,
	_Jv_JNI_CallStaticCharMethodA,
	_Jv_JNI_CallStaticShortMethod,
	_Jv_JNI_CallStaticShortMethodV,
	_Jv_JNI_CallStaticShortMethodA,
	_Jv_JNI_CallStaticIntMethod,
	_Jv_JNI_CallStaticIntMethodV,
	_Jv_JNI_CallStaticIntMethodA,
	_Jv_JNI_CallStaticLongMethod,
	_Jv_JNI_CallStaticLongMethodV,
	_Jv_JNI_CallStaticLongMethodA,
	_Jv_JNI_CallStaticFloatMethod,
	_Jv_JNI_CallStaticFloatMethodV,
	_Jv_JNI_CallStaticFloatMethodA,
	_Jv_JNI_CallStaticDoubleMethod,
	_Jv_JNI_CallStaticDoubleMethodV,
	_Jv_JNI_CallStaticDoubleMethodA,
	_Jv_JNI_CallStaticVoidMethod,
	_Jv_JNI_CallStaticVoidMethodV,
	_Jv_JNI_CallStaticVoidMethodA,

	_Jv_JNI_GetStaticFieldID,

	_Jv_JNI_GetStaticObjectField,
	_Jv_JNI_GetStaticBooleanField,
	_Jv_JNI_GetStaticByteField,
	_Jv_JNI_GetStaticCharField,
	_Jv_JNI_GetStaticShortField,
	_Jv_JNI_GetStaticIntField,
	_Jv_JNI_GetStaticLongField,
	_Jv_JNI_GetStaticFloatField,
	_Jv_JNI_GetStaticDoubleField,
	_Jv_JNI_SetStaticObjectField,
	_Jv_JNI_SetStaticBooleanField,
	_Jv_JNI_SetStaticByteField,
	_Jv_JNI_SetStaticCharField,
	_Jv_JNI_SetStaticShortField,
	_Jv_JNI_SetStaticIntField,
	_Jv_JNI_SetStaticLongField,
	_Jv_JNI_SetStaticFloatField,
	_Jv_JNI_SetStaticDoubleField,

	_Jv_JNI_NewString,
	_Jv_JNI_GetStringLength,
	_Jv_JNI_GetStringChars,
	_Jv_JNI_ReleaseStringChars,

	_Jv_JNI_NewStringUTF,
	_Jv_JNI_GetStringUTFLength,
	_Jv_JNI_GetStringUTFChars,
	_Jv_JNI_ReleaseStringUTFChars,

	_Jv_JNI_GetArrayLength,

	_Jv_JNI_NewObjectArray,
	_Jv_JNI_GetObjectArrayElement,
	_Jv_JNI_SetObjectArrayElement,

	_Jv_JNI_NewBooleanArray,
	_Jv_JNI_NewByteArray,
	_Jv_JNI_NewCharArray,
	_Jv_JNI_NewShortArray,
	_Jv_JNI_NewIntArray,
	_Jv_JNI_NewLongArray,
	_Jv_JNI_NewFloatArray,
	_Jv_JNI_NewDoubleArray,

	_Jv_JNI_GetBooleanArrayElements,
	_Jv_JNI_GetByteArrayElements,
	_Jv_JNI_GetCharArrayElements,
	_Jv_JNI_GetShortArrayElements,
	_Jv_JNI_GetIntArrayElements,
	_Jv_JNI_GetLongArrayElements,
	_Jv_JNI_GetFloatArrayElements,
	_Jv_JNI_GetDoubleArrayElements,

	_Jv_JNI_ReleaseBooleanArrayElements,
	_Jv_JNI_ReleaseByteArrayElements,
	_Jv_JNI_ReleaseCharArrayElements,
	_Jv_JNI_ReleaseShortArrayElements,
	_Jv_JNI_ReleaseIntArrayElements,
	_Jv_JNI_ReleaseLongArrayElements,
	_Jv_JNI_ReleaseFloatArrayElements,
	_Jv_JNI_ReleaseDoubleArrayElements,

	_Jv_JNI_GetBooleanArrayRegion,
	_Jv_JNI_GetByteArrayRegion,
	_Jv_JNI_GetCharArrayRegion,
	_Jv_JNI_GetShortArrayRegion,
	_Jv_JNI_GetIntArrayRegion,
	_Jv_JNI_GetLongArrayRegion,
	_Jv_JNI_GetFloatArrayRegion,
	_Jv_JNI_GetDoubleArrayRegion,
	_Jv_JNI_SetBooleanArrayRegion,
	_Jv_JNI_SetByteArrayRegion,
	_Jv_JNI_SetCharArrayRegion,
	_Jv_JNI_SetShortArrayRegion,
	_Jv_JNI_SetIntArrayRegion,
	_Jv_JNI_SetLongArrayRegion,
	_Jv_JNI_SetFloatArrayRegion,
	_Jv_JNI_SetDoubleArrayRegion,

	_Jv_JNI_RegisterNatives,
	_Jv_JNI_UnregisterNatives,

	_Jv_JNI_MonitorEnter,
	_Jv_JNI_MonitorExit,

	_Jv_JNI_GetJavaVM,

	/* New JNI 1.2 functions. */

	_Jv_JNI_GetStringRegion,
	_Jv_JNI_GetStringUTFRegion,

	_Jv_JNI_GetPrimitiveArrayCritical,
	_Jv_JNI_ReleasePrimitiveArrayCritical,

	_Jv_JNI_GetStringCritical,
	_Jv_JNI_ReleaseStringCritical,

	_Jv_JNI_NewWeakGlobalRef,
	_Jv_JNI_DeleteWeakGlobalRef,

	_Jv_JNI_ExceptionCheck,

	/* New JNI 1.4 functions. */

	_Jv_JNI_NewDirectByteBuffer,
	_Jv_JNI_GetDirectBufferAddress,
	_Jv_JNI_GetDirectBufferCapacity,

	/* New JNI 1.6 functions. */

	jni_GetObjectRefType
};


/* Invocation API Functions ***************************************************/

/* JNI_GetDefaultJavaVMInitArgs ************************************************

   Returns a default configuration for the Java VM.

*******************************************************************************/

jint JNI_GetDefaultJavaVMInitArgs(void *vm_args)
{
	JavaVMInitArgs *_vm_args;

	_vm_args = (JavaVMInitArgs *) vm_args;

	/* GNU classpath currently supports JNI 1.2 */

	switch (_vm_args->version) {
    case JNI_VERSION_1_1:
		_vm_args->version = JNI_VERSION_1_1;
		break;

    case JNI_VERSION_1_2:
    case JNI_VERSION_1_4:
		_vm_args->ignoreUnrecognized = JNI_FALSE;
		_vm_args->options = NULL;
		_vm_args->nOptions = 0;
		break;

    default:
		return -1;
	}
  
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
	TRACEJNICALLS(("JNI_GetCreatedJavaVMs(vmBuf=%p, jsize=%d, jsize=%p)", vmBuf, bufLen, nVMs));

	if (bufLen <= 0)
		return JNI_ERR;

	/* We currently only support 1 VM running. */

	vmBuf[0] = (JavaVM *) _Jv_jvm;
	*nVMs    = 1;

    return JNI_OK;
}


/* JNI_CreateJavaVM ************************************************************

   Loads and initializes a Java VM. The current thread becomes the main thread.
   Sets the env argument to the JNI interface pointer of the main thread.

*******************************************************************************/

jint JNI_CreateJavaVM(JavaVM **p_vm, void **p_env, void *vm_args)
{
	TRACEJNICALLS(("JNI_CreateJavaVM(p_vm=%p, p_env=%p, vm_args=%p)", p_vm, p_env, vm_args));

	/* actually create the JVM */

	if (!vm_createjvm(p_vm, p_env, vm_args))
		return JNI_ERR;

	return JNI_OK;
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
 * vim:noexpandtab:sw=4:ts=4:
 */
