/* src/native/vm/sun/jvm.c - HotSpot JVM interface functions

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: java_lang_Class.c 8132 2007-06-22 11:15:47Z twisti $

*/


#define PRINTJVM 0

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <ltdl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#if defined(HAVE_SYS_IOCTL_H)
#define BSD_COMP /* Get FIONREAD on Solaris2 */
#include <sys/ioctl.h>
#endif

#include <sys/socket.h>
#include <sys/stat.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "native/jni.h"
#include "native/native.h"

#include "native/include/java_lang_AssertionStatusDirectives.h"
#include "native/include/java_lang_String.h"            /* required by j.l.CL */
#include "native/include/java_nio_ByteBuffer.h"         /* required by j.l.CL */
#include "native/include/java_lang_ClassLoader.h"        /* required by j.l.C */
#include "native/include/java_lang_StackTraceElement.h"
#include "native/include/java_lang_Throwable.h"
#include "native/include/java_security_ProtectionDomain.h"

#include "native/vm/java_lang_Class.h"
#include "native/vm/java_lang_ClassLoader.h"
#include "native/vm/java_lang_Object.h"
#include "native/vm/java_lang_Runtime.h"
#include "native/vm/java_lang_String.h"
#include "native/vm/java_lang_Thread.h"
#include "native/vm/java_lang_reflect_Constructor.h"
#include "native/vm/java_lang_reflect_Method.h"
#include "native/vm/java_util_concurrent_atomic_AtomicLong.h"

#include "threads/lock-common.h"

#include "toolbox/logging.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/initialize.h"
#include "vm/properties.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vm/jit/stacktrace.h"

#include "vmcore/classcache.h"
#include "vmcore/primitive.h"


/* debugging macro ************************************************************/

#if !defined(NDEBUG)
# define DEBUG_JVM(x) \
    do { \
        if (opt_TraceJVM) { \
            log_println("%s", (x)); \
        } \
    } while (0)
#else
# define DEBUG_JVM(x)
#endif


typedef struct {
    /* Naming convention of RE build version string: n.n.n[_uu[c]][-<identifier>]-bxx */
    unsigned int jvm_version;   /* Consists of major, minor, micro (n.n.n) */
                                /* and build number (xx) */
    unsigned int update_version : 8;         /* Update release version (uu) */
    unsigned int special_update_version : 8; /* Special update release version (c) */
    unsigned int reserved1 : 16; 
    unsigned int reserved2; 

    /* The following bits represents JVM supports that JDK has dependency on.
     * JDK can use these bits to determine which JVM version
     * and support it has to maintain runtime compatibility.
     *
     * When a new bit is added in a minor or update release, make sure
     * the new bit is also added in the main/baseline.
     */
    unsigned int is_attachable : 1;
    unsigned int : 31;
    unsigned int : 32;
    unsigned int : 32;
} jvm_version_info;


/*
 * A structure used to a capture exception table entry in a Java method.
 */
typedef struct {
    jint start_pc;
    jint end_pc;
    jint handler_pc;
    jint catchType;
} JVM_ExceptionTableEntryType;


int jio_vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
	if ((intptr_t) count <= 0)
		return -1;

	return vsnprintf(str, count, fmt, args);
}


int jio_snprintf(char *str, size_t count, const char *fmt, ...)
{
	va_list ap;
	int     len;

	va_start(ap, fmt);
	len = jio_vsnprintf(str, count, fmt, ap);
	va_end(ap);

	return len;
}


int jio_fprintf(FILE* f, const char *fmt, ...)
{
	log_println("jio_fprintf: IMPLEMENT ME!");
}


int jio_vfprintf(FILE* f, const char *fmt, va_list args)
{
	log_println("jio_vfprintf: IMPLEMENT ME!");
}


int jio_printf(const char *fmt, ...)
{
	log_println("jio_printf: IMPLEMENT ME!");
}


/* JVM_GetInterfaceVersion */

jint JVM_GetInterfaceVersion()
{
	/* This is defined in hotspot/src/share/vm/prims/jvm.h */

#define JVM_INTERFACE_VERSION 4

	return JVM_INTERFACE_VERSION;
}


/* JVM_CurrentTimeMillis */

jlong JVM_CurrentTimeMillis(JNIEnv *env, jclass ignored)
{
#if PRINTJVM
	log_println("JVM_CurrentTimeMillis");
#endif
	return (jlong) builtin_currenttimemillis();
}


/* JVM_NanoTime */

jlong JVM_NanoTime(JNIEnv *env, jclass ignored)
{
#if PRINTJVM
	log_println("JVM_NanoTime");
#endif
	return (jlong) builtin_nanotime();
}


/* JVM_ArrayCopy */

void JVM_ArrayCopy(JNIEnv *env, jclass ignored, jobject src, jint src_pos, jobject dst, jint dst_pos, jint length)
{
#if PRINTJVM
	log_println("JVM_ArrayCopy: src=%p, src_pos=%d, dst=%p, dst_pos=%d, length=%d", src, src_pos, dst, dst_pos, length);
#endif
	builtin_arraycopy(src, src_pos, dst, dst_pos, length);
}


/* JVM_InitProperties */

jobject JVM_InitProperties(JNIEnv *env, jobject properties)
{
#if PRINTJVM
	log_println("JVM_InitProperties: properties=%d", properties);
#endif
	properties_system_add_all((java_objectheader *) properties);
}


/* JVM_Exit */

void JVM_Exit(jint code)
{
	log_println("JVM_Exit: IMPLEMENT ME!");
}


/* JVM_Halt */

void JVM_Halt(jint code)
{
#if PRINTJVM
	log_println("JVM_Halt: code=%d", code);
#endif
/* 	vm_exit(code); */
	vm_shutdown(code);
}


/* JVM_OnExit(void (*func)) */

void JVM_OnExit(void (*func)(void))
{
	log_println("JVM_OnExit(void (*func): IMPLEMENT ME!");
}


/* JVM_GC */

void JVM_GC(void)
{
#if PRINTJVM
	log_println("JVM_GC");
#endif
	_Jv_java_lang_Runtime_gc();
}


/* JVM_MaxObjectInspectionAge */

jlong JVM_MaxObjectInspectionAge(void)
{
	log_println("JVM_MaxObjectInspectionAge: IMPLEMENT ME!");
}


/* JVM_TraceInstructions */

void JVM_TraceInstructions(jboolean on)
{
	log_println("JVM_TraceInstructions: IMPLEMENT ME!");
}


/* JVM_TraceMethodCalls */

void JVM_TraceMethodCalls(jboolean on)
{
	log_println("JVM_TraceMethodCalls: IMPLEMENT ME!");
}


/* JVM_TotalMemory */

jlong JVM_TotalMemory(void)
{
#if PRINTJVM
	log_println("JVM_TotalMemory");
#endif
	return _Jv_java_lang_Runtime_totalMemory();
}


/* JVM_FreeMemory */

jlong JVM_FreeMemory(void)
{
#if PRINTJVM
	log_println("JVM_FreeMemory");
#endif
	return _Jv_java_lang_Runtime_freeMemory();
}


/* JVM_MaxMemory */

jlong JVM_MaxMemory(void)
{
	log_println("JVM_MaxMemory: IMPLEMENT ME!");
}


/* JVM_ActiveProcessorCount */

jint JVM_ActiveProcessorCount(void)
{
	log_println("JVM_ActiveProcessorCount: IMPLEMENT ME!");
}


/* JVM_FillInStackTrace */

void JVM_FillInStackTrace(JNIEnv *env, jobject receiver)
{
	java_lang_Throwable *o;
	stacktracecontainer *stc;

#if PRINTJVM
	log_println("JVM_FillInStackTrace: receiver=%p", receiver);
#endif

	o = (java_lang_Throwable *) receiver;

	stc = stacktrace_fillInStackTrace();

	if (stc == NULL)
		return;

    o->backtrace = (java_lang_Object *) stc;
}


/* JVM_PrintStackTrace */

void JVM_PrintStackTrace(JNIEnv *env, jobject receiver, jobject printable)
{
	log_println("JVM_PrintStackTrace: IMPLEMENT ME!");
}


/* JVM_GetStackTraceDepth */

jint JVM_GetStackTraceDepth(JNIEnv *env, jobject throwable)
{
	java_lang_Throwable *o;
	stacktracecontainer *stc;
	stacktracebuffer    *stb;

#if PRINTJVM
	log_println("JVM_GetStackTraceDepth: throwable=%p", throwable);
#endif

	o   = (java_lang_Throwable *) throwable;
	stc = (stacktracecontainer *) o->backtrace;
	stb = &(stc->stb);

	return stb->used;
}


/* JVM_GetStackTraceElement */

jobject JVM_GetStackTraceElement(JNIEnv *env, jobject throwable, jint index)
{
	java_lang_Throwable *t;
	stacktracecontainer *stc;
	stacktracebuffer    *stb;
	stacktrace_entry    *ste;
	java_lang_StackTraceElement *o;
	java_lang_String            *declaringclass;
	java_lang_String            *filename;
	s4                           linenumber;

#if PRINTJVM
	log_println("JVM_GetStackTraceElement: throwable=%p, index=%d", throwable, index);
#endif

	t   = (java_lang_Throwable *) throwable;
	stc = (stacktracecontainer *) t->backtrace;
	stb = &(stc->stb);

	if ((index < 0) || (index >= stb->used)) {
		/* XXX This should be an IndexOutOfBoundsException (check this
		   again). */

		exceptions_throw_arrayindexoutofboundsexception();
		return NULL;
	}

	ste = &(stb->entries[index]);

	/* allocate a new StackTraceElement */

	o = (java_lang_StackTraceElement *)
		builtin_new(class_java_lang_StackTraceElement);

	if (o == NULL)
		return NULL;

	/* get filename */

	if (!(ste->method->flags & ACC_NATIVE)) {
		if (ste->method->class->sourcefile)
			filename = (java_lang_String *) javastring_new(ste->method->class->sourcefile);
		else
			filename = NULL;
	}
	else
		filename = NULL;

	/* get line number */

	if (ste->method->flags & ACC_NATIVE)
		linenumber = -2;
	else
		linenumber = (ste->linenumber == 0) ? -1 : ste->linenumber;

	/* get declaring class name */

	declaringclass =
		_Jv_java_lang_Class_getName((java_lang_Class *) ste->method->class);

	/* fill the java.lang.StackTraceElement element */

	o->declaringClass = declaringclass;
	o->methodName     = (java_lang_String *) javastring_new(ste->method->name);
	o->fileName       = filename;
	o->lineNumber     = linenumber;

	return o;
}


/* JVM_IHashCode */

jint JVM_IHashCode(JNIEnv* env, jobject handle)
{
#if PRINTJVM
	log_println("JVM_IHashCode: jobject=%p", handle);
#endif
	return (jint) ((ptrint) handle);
}


/* JVM_MonitorWait */

void JVM_MonitorWait(JNIEnv* env, jobject handle, jlong ms)
{
#if PRINTJVM
	log_println("JVM_MonitorWait: handle=%p, ms=%ld", handle, ms);
#endif
	_Jv_java_lang_Object_wait((java_lang_Object *) handle, ms, 0);
}


/* JVM_MonitorNotify */

void JVM_MonitorNotify(JNIEnv* env, jobject handle)
{
#if PRINTJVM
	log_println("JVM_MonitorNotify: IMPLEMENT ME!");
#endif
	_Jv_java_lang_Object_notify((java_lang_Object *) handle);
}


/* JVM_MonitorNotifyAll */

void JVM_MonitorNotifyAll(JNIEnv* env, jobject handle)
{
#if PRINTJVM
	log_println("JVM_MonitorNotifyAll: handle=%p", handle);
#endif
	_Jv_java_lang_Object_notifyAll((java_lang_Object *) handle);
}


/* JVM_Clone */

jobject JVM_Clone(JNIEnv* env, jobject handle)
{
#if PRINTJVM
	log_println("JVM_Clone: handle=%p", handle);
#endif
	return (jobject) builtin_clone(env, (java_objectheader *) handle);
}


/* JVM_InitializeCompiler  */

void JVM_InitializeCompiler (JNIEnv *env, jclass compCls)
{
	log_println("JVM_InitializeCompiler : IMPLEMENT ME!");
}


/* JVM_IsSilentCompiler */

jboolean JVM_IsSilentCompiler(JNIEnv *env, jclass compCls)
{
	log_println("JVM_IsSilentCompiler: IMPLEMENT ME!");
}


/* JVM_CompileClass */

jboolean JVM_CompileClass(JNIEnv *env, jclass compCls, jclass cls)
{
	log_println("JVM_CompileClass: IMPLEMENT ME!");
}


/* JVM_CompileClasses */

jboolean JVM_CompileClasses(JNIEnv *env, jclass cls, jstring jname)
{
	log_println("JVM_CompileClasses: IMPLEMENT ME!");
}


/* JVM_CompilerCommand */

jobject JVM_CompilerCommand(JNIEnv *env, jclass compCls, jobject arg)
{
	log_println("JVM_CompilerCommand: IMPLEMENT ME!");
}


/* JVM_EnableCompiler */

void JVM_EnableCompiler(JNIEnv *env, jclass compCls)
{
	log_println("JVM_EnableCompiler: IMPLEMENT ME!");
}


/* JVM_DisableCompiler */

void JVM_DisableCompiler(JNIEnv *env, jclass compCls)
{
	log_println("JVM_DisableCompiler: IMPLEMENT ME!");
}


/* JVM_GetLastErrorString */

jint JVM_GetLastErrorString(char *buf, int len)
{
	const char *s;
	int n;

    if (errno == 0) {
		return 0;
    }
	else {
		s = strerror(errno);
		n = strlen(s);

		if (n >= len)
			n = len - 1;

		strncpy(buf, s, n);

		buf[n] = '\0';

		return n;
    }
}


/* JVM_NativePath */

char* JVM_NativePath(char* path)
{
#if PRINTJVM
	log_println("JVM_NativePath: path=%s", path);
#endif
	/* XXX is this correct? */

	return path;
}


/* JVM_GetCallerClass */

jclass JVM_GetCallerClass(JNIEnv* env, int depth)
{
	java_objectarray *oa;

#if PRINTJVM
	log_println("JVM_GetCallerClass: depth=%d", depth);
#endif

	oa = stacktrace_getClassContext();

	if (oa == NULL)
		return NULL;

	if (oa->header.size < depth)
		return NULL;

	return (jclass) oa->data[depth - 1];

}


/* JVM_FindPrimitiveClass */

jclass JVM_FindPrimitiveClass(JNIEnv* env, const char* utf)
{
#if PRINTVM
	log_println("JVM_FindPrimitiveClass: utf=%s", utf);
#endif
	return (jclass) primitive_class_get_by_name(utf_new_char(utf));
}


/* JVM_ResolveClass */

void JVM_ResolveClass(JNIEnv* env, jclass cls)
{
	log_println("JVM_ResolveClass: IMPLEMENT ME!");
}


/* JVM_FindClassFromClassLoader */

jclass JVM_FindClassFromClassLoader(JNIEnv* env, const char* name, jboolean init, jobject loader, jboolean throwError)
{
	classinfo *c;

#if PRINTJVM
	log_println("JVM_FindClassFromClassLoader: name=%s, init=%d, loader=%p, throwError=%d", name, init, loader, throwError);
#endif

	c = load_class_from_classloader(utf_new_char(name), (java_objectheader *) loader);

	if (c == NULL)
		return NULL;

	if (init)
		if (!(c->state & CLASS_INITIALIZED))
			if (!initialize_class(c))
				return NULL;

	return (jclass) c;
}


/* JVM_FindClassFromClass */

jclass JVM_FindClassFromClass(JNIEnv *env, const char *name, jboolean init, jclass from)
{
	log_println("JVM_FindClassFromClass: IMPLEMENT ME!");
}


/* JVM_DefineClass */

jclass JVM_DefineClass(JNIEnv *env, const char *name, jobject loader, const jbyte *buf, jsize len, jobject pd)
{
	log_println("JVM_DefineClass: IMPLEMENT ME!");
}


/* JVM_DefineClassWithSource */

jclass JVM_DefineClassWithSource(JNIEnv *env, const char *name, jobject loader, const jbyte *buf, jsize len, jobject pd, const char *source)
{
#if PRINTJVM
	log_println("JVM_DefineClassWithSource: name=%s, loader=%p, buf=%p, len=%d, pd=%p, source=%s", name, loader, buf, len, pd, source);
#endif
	/* XXX do something with pd and source */

	return (jclass) class_define(utf_new_char(name), (java_objectheader *) loader, len, (u1 *) buf);

}


/* JVM_FindLoadedClass */

jclass JVM_FindLoadedClass(JNIEnv *env, jobject loader, jstring name)
{
#if PRINTJVM
	log_println("JVM_FindLoadedClass: loader=%p, name=%p", loader, name);
#endif
	return (jclass) classcache_lookup((classloader *) loader, javastring_toutf(name, true));
}


/* JVM_GetClassName */

jstring JVM_GetClassName(JNIEnv *env, jclass cls)
{
#if PRINTJVM
	log_println("JVM_GetClassName: cls=%p", cls);
#endif
	return (jstring) _Jv_java_lang_Class_getName((java_lang_Class *) cls);
}


/* JVM_GetClassInterfaces */

jobjectArray JVM_GetClassInterfaces(JNIEnv *env, jclass cls)
{
#if PRINTJVM
	log_println("JVM_GetClassInterfaces: cls=%p", cls);
#endif
	return (jobjectArray) _Jv_java_lang_Class_getInterfaces((java_lang_Class *) cls);
}


/* JVM_GetClassLoader */

jobject JVM_GetClassLoader(JNIEnv *env, jclass cls)
{
#if PRINTJVM
	log_println("JVM_GetClassLoader: cls=%p", cls);
#endif
	return (jobject) _Jv_java_lang_Class_getClassLoader((java_lang_Class *) cls);
}


/* JVM_IsInterface */

jboolean JVM_IsInterface(JNIEnv *env, jclass cls)
{
	classinfo *c;

#if PRINTJVM
	log_println("JVM_IsInterface: cls=%p", cls);
#endif

	c = (classinfo *) cls;

	return class_is_interface(c);
}


/* JVM_GetClassSigners */

jobjectArray JVM_GetClassSigners(JNIEnv *env, jclass cls)
{
	log_println("JVM_GetClassSigners: IMPLEMENT ME!");
}


/* JVM_SetClassSigners */

void JVM_SetClassSigners(JNIEnv *env, jclass cls, jobjectArray signers)
{
	log_println("JVM_SetClassSigners: IMPLEMENT ME!");
}


/* JVM_GetProtectionDomain */

jobject JVM_GetProtectionDomain(JNIEnv *env, jclass cls)
{
	classinfo *c;

#if PRINTJVM || 1
	log_println("JVM_GetProtectionDomain: cls=%p");
#endif

	c = (classinfo *) cls;

	if (c == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

    /* Primitive types do not have a protection domain. */

	if (class_is_primitive(c))
		return NULL;
}


/* JVM_SetProtectionDomain */

void JVM_SetProtectionDomain(JNIEnv *env, jclass cls, jobject protection_domain)
{
	log_println("JVM_SetProtectionDomain: IMPLEMENT ME!");
}


/* JVM_DoPrivileged */

jobject JVM_DoPrivileged(JNIEnv *env, jclass cls, jobject action, jobject context, jboolean wrapException)
{
	java_objectheader *o;
	classinfo         *c;
	methodinfo        *m;
	java_objectheader *result;
	java_objectheader *e;

#if PRINTJVM
	log_println("JVM_DoPrivileged: action=%p, context=%p, wrapException=%d", action, context, wrapException);
#endif

	o = (java_objectheader *) action;
	c = o->vftbl->class;

	if (action == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* lookup run() method (throw no exceptions) */

	m = class_resolveclassmethod(c, utf_run, utf_void__java_lang_Object, c,
								 false);

	if ((m == NULL) || !(m->flags & ACC_PUBLIC) || (m->flags & ACC_STATIC)) {
		exceptions_throw_internalerror("No run method");
		return NULL;
	}

	/* XXX It seems something with a privileged stack needs to be done
	   here. */

	result = vm_call_method(m, o);

	e = exceptions_get_and_clear_exception();

	if (e != NULL) {
		exceptions_throw_privilegedactionexception(e);
		return NULL;
	}

	return (jobject) result;
}


/* JVM_GetInheritedAccessControlContext */

jobject JVM_GetInheritedAccessControlContext(JNIEnv *env, jclass cls)
{
	log_println("JVM_GetInheritedAccessControlContext: IMPLEMENT ME!");
}


/* JVM_GetStackAccessControlContext */

jobject JVM_GetStackAccessControlContext(JNIEnv *env, jclass cls)
{
	log_println("JVM_GetStackAccessControlContext: IMPLEMENT ME!");
}


/* JVM_IsArrayClass */

jboolean JVM_IsArrayClass(JNIEnv *env, jclass cls)
{
#if PRINTJVM
	log_println("JVM_IsArrayClass: cls=%p", cls);
#endif
	return class_is_array((classinfo *) cls);
}


/* JVM_IsPrimitiveClass */

jboolean JVM_IsPrimitiveClass(JNIEnv *env, jclass cls)
{
	classinfo *c;

	c = (classinfo *) cls;

#if PRINTJVM
	log_println("JVM_IsPrimitiveClass(cls=%p)", cls);
#endif

	return class_is_primitive(c);
}


/* JVM_GetComponentType */

jclass JVM_GetComponentType(JNIEnv *env, jclass cls)
{
#if PRINTJVM
	log_println("JVM_GetComponentType: cls=%p", cls);
#endif
	return (jclass) _Jv_java_lang_Class_getComponentType((java_lang_Class *) cls);
}


/* JVM_GetClassModifiers */

jint JVM_GetClassModifiers(JNIEnv *env, jclass cls)
{
	classinfo *c;

#if PRINTJVM
	log_println("JVM_GetClassModifiers: cls=%p", cls);
#endif

	c = (classinfo *) cls;

	/* XXX is this correct? */

	return c->flags & ACC_CLASS_REFLECT_MASK;
}


/* JVM_GetDeclaredClasses */

jobjectArray JVM_GetDeclaredClasses(JNIEnv *env, jclass ofClass)
{
	log_println("JVM_GetDeclaredClasses: IMPLEMENT ME!");
}


/* JVM_GetDeclaringClass */

jclass JVM_GetDeclaringClass(JNIEnv *env, jclass ofClass)
{
	log_println("JVM_GetDeclaringClass: IMPLEMENT ME!");
}


/* JVM_GetClassSignature */

jstring JVM_GetClassSignature(JNIEnv *env, jclass cls)
{
	log_println("JVM_GetClassSignature: IMPLEMENT ME!");
}


/* JVM_GetClassAnnotations */

jbyteArray JVM_GetClassAnnotations(JNIEnv *env, jclass cls)
{
	log_println("JVM_GetClassAnnotations: IMPLEMENT ME!");
}


/* JVM_GetFieldAnnotations */

jbyteArray JVM_GetFieldAnnotations(JNIEnv *env, jobject field)
{
	log_println("JVM_GetFieldAnnotations: IMPLEMENT ME!");
}


/* JVM_GetMethodAnnotations */

jbyteArray JVM_GetMethodAnnotations(JNIEnv *env, jobject method)
{
	log_println("JVM_GetMethodAnnotations: IMPLEMENT ME!");
}


/* JVM_GetMethodDefaultAnnotationValue */

jbyteArray JVM_GetMethodDefaultAnnotationValue(JNIEnv *env, jobject method)
{
	log_println("JVM_GetMethodDefaultAnnotationValue: IMPLEMENT ME!");
}


/* JVM_GetMethodParameterAnnotations */

jbyteArray JVM_GetMethodParameterAnnotations(JNIEnv *env, jobject method)
{
	log_println("JVM_GetMethodParameterAnnotations: IMPLEMENT ME!");
}


/* JVM_GetClassDeclaredFields */

jobjectArray JVM_GetClassDeclaredFields(JNIEnv *env, jclass ofClass, jboolean publicOnly)
{
#if PRINTJVM
	log_println("JVM_GetClassDeclaredFields: ofClass=%p, publicOnly=%d", ofClass, publicOnly);
#endif
	return (jobjectArray) _Jv_java_lang_Class_getDeclaredFields((java_lang_Class *) ofClass, publicOnly);
}


/* JVM_GetClassDeclaredMethods */

jobjectArray JVM_GetClassDeclaredMethods(JNIEnv *env, jclass ofClass, jboolean publicOnly)
{
#if PRINTJVM
	log_println("JVM_GetClassDeclaredMethods: ofClass=%p, publicOnly=%d", ofClass, publicOnly);
#endif
	return (jobjectArray) _Jv_java_lang_Class_getDeclaredMethods((java_lang_Class *) ofClass, publicOnly);
}


/* JVM_GetClassDeclaredConstructors */

jobjectArray JVM_GetClassDeclaredConstructors(JNIEnv *env, jclass ofClass, jboolean publicOnly)
{
#if PRINTJVM
	log_println("JVM_GetClassDeclaredConstructors: ofClass=%p, publicOnly=%d", ofClass, publicOnly);
#endif
	return (jobjectArray) _Jv_java_lang_Class_getDeclaredConstructors((java_lang_Class *) ofClass, publicOnly);
}


/* JVM_GetClassAccessFlags */

jint JVM_GetClassAccessFlags(JNIEnv *env, jclass cls)
{
	classinfo *c;

#if PRINTJVM
	log_println("JVM_GetClassAccessFlags: cls=%p", cls);
#endif

	c = (classinfo *) cls;

	return c->flags & ACC_CLASS_REFLECT_MASK;
}


/* JVM_GetClassConstantPool */

jobject JVM_GetClassConstantPool(JNIEnv *env, jclass cls)
{
	log_println("JVM_GetClassConstantPool: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetSize */

jint JVM_ConstantPoolGetSize(JNIEnv *env, jobject unused, jobject jcpool)
{
	log_println("JVM_ConstantPoolGetSize: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetClassAt */

jclass JVM_ConstantPoolGetClassAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetClassAt: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetClassAtIfLoaded */

jclass JVM_ConstantPoolGetClassAtIfLoaded(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetClassAtIfLoaded: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetMethodAt */

jobject JVM_ConstantPoolGetMethodAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetMethodAt: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetMethodAtIfLoaded */

jobject JVM_ConstantPoolGetMethodAtIfLoaded(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetMethodAtIfLoaded: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetFieldAt */

jobject JVM_ConstantPoolGetFieldAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetFieldAt: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetFieldAtIfLoaded */

jobject JVM_ConstantPoolGetFieldAtIfLoaded(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetFieldAtIfLoaded: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetMemberRefInfoAt */

jobjectArray JVM_ConstantPoolGetMemberRefInfoAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetMemberRefInfoAt: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetIntAt */

jint JVM_ConstantPoolGetIntAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetIntAt: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetLongAt */

jlong JVM_ConstantPoolGetLongAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetLongAt: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetFloatAt */

jfloat JVM_ConstantPoolGetFloatAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetFloatAt: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetDoubleAt */

jdouble JVM_ConstantPoolGetDoubleAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetDoubleAt: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetStringAt */

jstring JVM_ConstantPoolGetStringAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetStringAt: IMPLEMENT ME!");
}


/* JVM_ConstantPoolGetUTF8At */

jstring JVM_ConstantPoolGetUTF8At(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetUTF8At: IMPLEMENT ME!");
}


/* JVM_DesiredAssertionStatus */

jboolean JVM_DesiredAssertionStatus(JNIEnv *env, jclass unused, jclass cls)
{
	log_println("JVM_DesiredAssertionStatus: cls=%p, IMPLEMENT ME!", cls);

	return false;
}


/* JVM_AssertionStatusDirectives */

jobject JVM_AssertionStatusDirectives(JNIEnv *env, jclass unused)
{
	classinfo         *c;
	java_lang_AssertionStatusDirectives *o;
	java_objectarray  *classes;
	java_objectarray  *packages;

#if PRINTJVM || 1
	log_println("JVM_AssertionStatusDirectives");
#endif
	/* XXX this is not completely implemented */

	c = load_class_bootstrap(utf_new_char("java/lang/AssertionStatusDirectives"));

	if (c == NULL)
		return NULL;

	o = (java_lang_AssertionStatusDirectives *) builtin_new(c);

	if (o == NULL)
		return NULL;

	classes = builtin_anewarray(0, class_java_lang_Object);

	if (classes == NULL)
		return NULL;

	packages = builtin_anewarray(0, class_java_lang_Object);

	if (packages == NULL)
		return NULL;

	/* set instance fields */

	o->classes  = classes;
	o->packages = packages;

	return (jobject) o;
}


/* JVM_GetClassNameUTF */

const char* JVM_GetClassNameUTF(JNIEnv *env, jclass cls)
{
	log_println("JVM_GetClassNameUTF: IMPLEMENT ME!");
}


/* JVM_GetClassCPTypes */

void JVM_GetClassCPTypes(JNIEnv *env, jclass cls, unsigned char *types)
{
	log_println("JVM_GetClassCPTypes: IMPLEMENT ME!");
}


/* JVM_GetClassCPEntriesCount */

jint JVM_GetClassCPEntriesCount(JNIEnv *env, jclass cls)
{
	log_println("JVM_GetClassCPEntriesCount: IMPLEMENT ME!");
}


/* JVM_GetClassFieldsCount */

jint JVM_GetClassFieldsCount(JNIEnv *env, jclass cls)
{
	log_println("JVM_GetClassFieldsCount: IMPLEMENT ME!");
}


/* JVM_GetClassMethodsCount */

jint JVM_GetClassMethodsCount(JNIEnv *env, jclass cls)
{
	log_println("JVM_GetClassMethodsCount: IMPLEMENT ME!");
}


/* JVM_GetMethodIxExceptionIndexes */

void JVM_GetMethodIxExceptionIndexes(JNIEnv *env, jclass cls, jint method_index, unsigned short *exceptions)
{
	log_println("JVM_GetMethodIxExceptionIndexes: IMPLEMENT ME!");
}


/* JVM_GetMethodIxExceptionsCount */

jint JVM_GetMethodIxExceptionsCount(JNIEnv *env, jclass cls, jint method_index)
{
	log_println("JVM_GetMethodIxExceptionsCount: IMPLEMENT ME!");
}


/* JVM_GetMethodIxByteCode */

void JVM_GetMethodIxByteCode(JNIEnv *env, jclass cls, jint method_index, unsigned char *code)
{
	log_println("JVM_GetMethodIxByteCode: IMPLEMENT ME!");
}


/* JVM_GetMethodIxByteCodeLength */

jint JVM_GetMethodIxByteCodeLength(JNIEnv *env, jclass cls, jint method_index)
{
	log_println("JVM_GetMethodIxByteCodeLength: IMPLEMENT ME!");
}


/* JVM_GetMethodIxExceptionTableEntry */

void JVM_GetMethodIxExceptionTableEntry(JNIEnv *env, jclass cls, jint method_index, jint entry_index, JVM_ExceptionTableEntryType *entry)
{
	log_println("JVM_GetMethodIxExceptionTableEntry: IMPLEMENT ME!");
}


/* JVM_GetMethodIxExceptionTableLength */

jint JVM_GetMethodIxExceptionTableLength(JNIEnv *env, jclass cls, int method_index)
{
	log_println("JVM_GetMethodIxExceptionTableLength: IMPLEMENT ME!");
}


/* JVM_GetMethodIxModifiers */

jint JVM_GetMethodIxModifiers(JNIEnv *env, jclass cls, int method_index)
{
	log_println("JVM_GetMethodIxModifiers: IMPLEMENT ME!");
}


/* JVM_GetFieldIxModifiers */

jint JVM_GetFieldIxModifiers(JNIEnv *env, jclass cls, int field_index)
{
	log_println("JVM_GetFieldIxModifiers: IMPLEMENT ME!");
}


/* JVM_GetMethodIxLocalsCount */

jint JVM_GetMethodIxLocalsCount(JNIEnv *env, jclass cls, int method_index)
{
	log_println("JVM_GetMethodIxLocalsCount: IMPLEMENT ME!");
}


/* JVM_GetMethodIxArgsSize */

jint JVM_GetMethodIxArgsSize(JNIEnv *env, jclass cls, int method_index)
{
	log_println("JVM_GetMethodIxArgsSize: IMPLEMENT ME!");
}


/* JVM_GetMethodIxMaxStack */

jint JVM_GetMethodIxMaxStack(JNIEnv *env, jclass cls, int method_index)
{
	log_println("JVM_GetMethodIxMaxStack: IMPLEMENT ME!");
}


/* JVM_IsConstructorIx */

jboolean JVM_IsConstructorIx(JNIEnv *env, jclass cls, int method_index)
{
	log_println("JVM_IsConstructorIx: IMPLEMENT ME!");
}


/* JVM_GetMethodIxNameUTF */

const char* JVM_GetMethodIxNameUTF(JNIEnv *env, jclass cls, jint method_index)
{
	log_println("JVM_GetMethodIxNameUTF: IMPLEMENT ME!");
}


/* JVM_GetMethodIxSignatureUTF */

const char* JVM_GetMethodIxSignatureUTF(JNIEnv *env, jclass cls, jint method_index)
{
	log_println("JVM_GetMethodIxSignatureUTF: IMPLEMENT ME!");
}


/* JVM_GetCPFieldNameUTF */

const char* JVM_GetCPFieldNameUTF(JNIEnv *env, jclass cls, jint cp_index)
{
	log_println("JVM_GetCPFieldNameUTF: IMPLEMENT ME!");
}


/* JVM_GetCPMethodNameUTF */

const char* JVM_GetCPMethodNameUTF(JNIEnv *env, jclass cls, jint cp_index)
{
	log_println("JVM_GetCPMethodNameUTF: IMPLEMENT ME!");
}


/* JVM_GetCPMethodSignatureUTF */

const char* JVM_GetCPMethodSignatureUTF(JNIEnv *env, jclass cls, jint cp_index)
{
	log_println("JVM_GetCPMethodSignatureUTF: IMPLEMENT ME!");
}


/* JVM_GetCPFieldSignatureUTF */

const char* JVM_GetCPFieldSignatureUTF(JNIEnv *env, jclass cls, jint cp_index)
{
	log_println("JVM_GetCPFieldSignatureUTF: IMPLEMENT ME!");
}


/* JVM_GetCPClassNameUTF */

const char* JVM_GetCPClassNameUTF(JNIEnv *env, jclass cls, jint cp_index)
{
	log_println("JVM_GetCPClassNameUTF: IMPLEMENT ME!");
}


/* JVM_GetCPFieldClassNameUTF */

const char* JVM_GetCPFieldClassNameUTF(JNIEnv *env, jclass cls, jint cp_index)
{
	log_println("JVM_GetCPFieldClassNameUTF: IMPLEMENT ME!");
}


/* JVM_GetCPMethodClassNameUTF */

const char* JVM_GetCPMethodClassNameUTF(JNIEnv *env, jclass cls, jint cp_index)
{
	log_println("JVM_GetCPMethodClassNameUTF: IMPLEMENT ME!");
}


/* JVM_GetCPFieldModifiers */

jint JVM_GetCPFieldModifiers(JNIEnv *env, jclass cls, int cp_index, jclass called_cls)
{
	log_println("JVM_GetCPFieldModifiers: IMPLEMENT ME!");
}


/* JVM_GetCPMethodModifiers */

jint JVM_GetCPMethodModifiers(JNIEnv *env, jclass cls, int cp_index, jclass called_cls)
{
	log_println("JVM_GetCPMethodModifiers: IMPLEMENT ME!");
}


/* JVM_ReleaseUTF */

void JVM_ReleaseUTF(const char *utf)
{
	log_println("JVM_ReleaseUTF: IMPLEMENT ME!");
}


/* JVM_IsSameClassPackage */

jboolean JVM_IsSameClassPackage(JNIEnv *env, jclass class1, jclass class2)
{
	log_println("JVM_IsSameClassPackage: IMPLEMENT ME!");
}


/* JVM_Open */

jint JVM_Open(const char *fname, jint flags, jint mode)
{
	int result;

#if PRINTJVM
	log_println("JVM_Open: fname=%s, flags=%d, mode=%d", fname, flags, mode);
#endif

	result = open(fname, flags, mode);

	if (result >= 0) {
		return result;
	}
	else {
		switch(errno) {
		case EEXIST:
			/* XXX don't know what to do here */
/* 			return JVM_EEXIST; */
			return -1;
		default:
			return -1;
		}
	}
}


/* JVM_Close */

jint JVM_Close(jint fd)
{
#if PRINTJVM
	log_println("JVM_Close: fd=%d", fd);
#endif
	return close(fd);
}


/* JVM_Read */

jint JVM_Read(jint fd, char *buf, jint nbytes)
{
#if PRINTJVM
	log_println("JVM_Read: fd=%d, buf=%p, nbytes=%d", fd, buf, nbytes);
#endif
	return read(fd, buf, nbytes);
}


/* JVM_Write */

jint JVM_Write(jint fd, char *buf, jint nbytes)
{
#if PRINTJVM
	log_println("JVM_Write: fd=%d, buf=%s, nbytes=%d", fd, buf, nbytes);
#endif
	return write(fd, buf, nbytes);
}


/* JVM_Available */

jint JVM_Available(jint fd, jlong *pbytes)
{
#if defined(FIONREAD)
	ssize_t n;

	*pbytes = 0;

	if (ioctl(fd, FIONREAD, (char *) &n) != 0)
		return 0;

	*pbytes = n;

	return 1;
#elif defined(HAVE_FSTAT)
	struct stat statBuffer;
	off_t n;
	int result;

	*pbytes = 0;

	if ((fstat(fd, &statBuffer) == 0) && S_ISREG (statBuffer.st_mode)) {
		n = lseek (fd, 0, SEEK_CUR);

		if (n != -1) {
			*pbytes = statBuffer.st_size - n; 
			result = 1;
		}
		else {
			result = 0;
		}
	}
	else {
		result = 0;
	}
  
	return result;
#elif defined(HAVE_SELECT)
	fd_set filedescriptset;
	struct timeval tv;
	int result;

	*pbytes = 0;
  
	FD_ZERO(&filedescriptset);
	FD_SET(fd, &filedescriptset);
	memset(&tv, 0, sizeof(tv));

	switch (select(fd+1, &filedescriptset, NULL, NULL, &tv))
		{
		case -1: 
			result = errno; 
			break;
		case  0:
			*pbytes = 0;
			result = CPNATIVE_OK;
			break;      
		default: 
			*pbytes = 1;
			result = CPNATIVE_OK;
			break;
		}
	return result;
#else
	*pbytes = 0;
	return 0;
#endif
}


/* JVM_Lseek */

jlong JVM_Lseek(jint fd, jlong offset, jint whence)
{
#if PRINTJVM
	log_println("JVM_Lseek: fd=%d, offset=%ld, whence=%d", fd, offset, whence);
#endif
	return (jlong) lseek(fd, (off_t) offset, whence);
}


/* JVM_SetLength */

jint JVM_SetLength(jint fd, jlong length)
{
	log_println("JVM_SetLength: IMPLEMENT ME!");
}


/* JVM_Sync */

jint JVM_Sync(jint fd)
{
	log_println("JVM_Sync: IMPLEMENT ME!");
}


/* JVM_StartThread */

void JVM_StartThread(JNIEnv* env, jobject jthread)
{
#if PRINTJVM
	log_println("JVM_StartThread: jthread=%p", jthread);
#endif
	_Jv_java_lang_Thread_start((java_lang_Thread *) jthread, 0);
}


/* JVM_StopThread */

void JVM_StopThread(JNIEnv* env, jobject jthread, jobject throwable)
{
	log_println("JVM_StopThread: IMPLEMENT ME!");
}


/* JVM_IsThreadAlive */

jboolean JVM_IsThreadAlive(JNIEnv* env, jobject jthread)
{
#if PRINTJVM
	log_println("JVM_IsThreadAlive: jthread=%p", jthread);
#endif
	return _Jv_java_lang_Thread_isAlive((java_lang_Thread *) jthread);
}


/* JVM_SuspendThread */

void JVM_SuspendThread(JNIEnv* env, jobject jthread)
{
	log_println("JVM_SuspendThread: IMPLEMENT ME!");
}


/* JVM_ResumeThread */

void JVM_ResumeThread(JNIEnv* env, jobject jthread)
{
	log_println("JVM_ResumeThread: IMPLEMENT ME!");
}


/* JVM_SetThreadPriority */

void JVM_SetThreadPriority(JNIEnv* env, jobject jthread, jint prio)
{
#if PRINTJVM
	log_println("JVM_SetThreadPriority: jthread=%p, prio=%d", jthread, prio);
#endif
	_Jv_java_lang_Thread_setPriority((java_lang_Thread *) jthread, prio);
}


/* JVM_Yield */

void JVM_Yield(JNIEnv *env, jclass threadClass)
{
	log_println("JVM_Yield: IMPLEMENT ME!");
}


/* JVM_Sleep */

void JVM_Sleep(JNIEnv* env, jclass threadClass, jlong millis)
{
#if PRINTJVM
	log_println("JVM_Sleep: threadClass=%p, millis=%ld", threadClass, millis);
#endif
	_Jv_java_lang_Thread_sleep(millis);
}


/* JVM_CurrentThread */

jobject JVM_CurrentThread(JNIEnv* env, jclass threadClass)
{
#if PRINTJVM
	log_println("JVM_CurrentThread: threadClass=%p", threadClass);
#endif
	return (jobject) _Jv_java_lang_Thread_currentThread();
}


/* JVM_CountStackFrames */

jint JVM_CountStackFrames(JNIEnv* env, jobject jthread)
{
	log_println("JVM_CountStackFrames: IMPLEMENT ME!");
}


/* JVM_Interrupt */

void JVM_Interrupt(JNIEnv* env, jobject jthread)
{
	log_println("JVM_Interrupt: IMPLEMENT ME!");
}


/* JVM_IsInterrupted */

jboolean JVM_IsInterrupted(JNIEnv* env, jobject jthread, jboolean clear_interrupted)
{
#if PRINTJVM
	log_println("JVM_IsInterrupted: jthread=%p, clear_interrupted=%d", jthread, clear_interrupted);
#endif
	/* XXX do something with clear_interrupted */
	return _Jv_java_lang_Thread_isInterrupted((java_lang_Thread *) jthread);
}


/* JVM_HoldsLock */

jboolean JVM_HoldsLock(JNIEnv* env, jclass threadClass, jobject obj)
{
	log_println("JVM_HoldsLock: IMPLEMENT ME!");
}


/* JVM_DumpAllStacks */

void JVM_DumpAllStacks(JNIEnv* env, jclass unused)
{
	log_println("JVM_DumpAllStacks: IMPLEMENT ME!");
}


/* JVM_CurrentLoadedClass */

jclass JVM_CurrentLoadedClass(JNIEnv *env)
{
	log_println("JVM_CurrentLoadedClass: IMPLEMENT ME!");
}


/* JVM_CurrentClassLoader */

jobject JVM_CurrentClassLoader(JNIEnv *env)
{
	log_println("JVM_CurrentClassLoader: IMPLEMENT ME!");
}


/* JVM_GetClassContext */

jobjectArray JVM_GetClassContext(JNIEnv *env)
{
#if PRINTJVM
	log_println("JVM_GetClassContext");
#endif
	return (jobjectArray) stacktrace_getClassContext();
}


/* JVM_ClassDepth */

jint JVM_ClassDepth(JNIEnv *env, jstring name)
{
	log_println("JVM_ClassDepth: IMPLEMENT ME!");
}


/* JVM_ClassLoaderDepth */

jint JVM_ClassLoaderDepth(JNIEnv *env)
{
	log_println("JVM_ClassLoaderDepth: IMPLEMENT ME!");
}


/* JVM_GetSystemPackage */

jstring JVM_GetSystemPackage(JNIEnv *env, jstring name)
{
	log_println("JVM_GetSystemPackage: IMPLEMENT ME!");
}


/* JVM_GetSystemPackages */

jobjectArray JVM_GetSystemPackages(JNIEnv *env)
{
	log_println("JVM_GetSystemPackages: IMPLEMENT ME!");
}


/* JVM_AllocateNewObject */

jobject JVM_AllocateNewObject(JNIEnv *env, jobject receiver, jclass currClass, jclass initClass)
{
	log_println("JVM_AllocateNewObject: IMPLEMENT ME!");
}


/* JVM_AllocateNewArray */

jobject JVM_AllocateNewArray(JNIEnv *env, jobject obj, jclass currClass, jint length)
{
	log_println("JVM_AllocateNewArray: IMPLEMENT ME!");
}


/* JVM_LatestUserDefinedLoader */

jobject JVM_LatestUserDefinedLoader(JNIEnv *env)
{
	log_println("JVM_LatestUserDefinedLoader: IMPLEMENT ME!");
}


/* JVM_LoadClass0 */

jclass JVM_LoadClass0(JNIEnv *env, jobject receiver, jclass currClass, jstring currClassName)
{
	log_println("JVM_LoadClass0: IMPLEMENT ME!");
}


/* JVM_GetArrayLength */

jint JVM_GetArrayLength(JNIEnv *env, jobject arr)
{
	log_println("JVM_GetArrayLength: IMPLEMENT ME!");
}


/* JVM_GetArrayElement */

jobject JVM_GetArrayElement(JNIEnv *env, jobject arr, jint index)
{
	log_println("JVM_GetArrayElement: IMPLEMENT ME!");
}


/* JVM_GetPrimitiveArrayElement */

jvalue JVM_GetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jint wCode)
{
	log_println("JVM_GetPrimitiveArrayElement: IMPLEMENT ME!");
}


/* JVM_SetArrayElement */

void JVM_SetArrayElement(JNIEnv *env, jobject arr, jint index, jobject val)
{
	log_println("JVM_SetArrayElement: IMPLEMENT ME!");
}


/* JVM_SetPrimitiveArrayElement */

void JVM_SetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jvalue v, unsigned char vCode)
{
	log_println("JVM_SetPrimitiveArrayElement: IMPLEMENT ME!");
}


/* JVM_NewArray */

jobject JVM_NewArray(JNIEnv *env, jclass eltClass, jint length)
{
	classinfo        *c;
	classinfo        *pc;
	java_arrayheader *a;
	java_objectarray *oa;

	c = (classinfo *) eltClass;

#if PRINTJVM
	log_println("JVM_NewArray(eltClass=%p, length=%d)", eltClass, length);
#endif

	/* create primitive or object array */

	if (class_is_primitive(c)) {
		pc = primitive_arrayclass_get_by_name(c->name);
		a = builtin_newarray(length, pc);

		return (jobject) a;
	}
	else {
		oa = builtin_anewarray(length, c);

		return (jobject) oa;
	}
}


/* JVM_NewMultiArray */

jobject JVM_NewMultiArray(JNIEnv *env, jclass eltClass, jintArray dim)
{
	log_println("JVM_NewMultiArray: IMPLEMENT ME!");
}


/* JVM_InitializeSocketLibrary */

jint JVM_InitializeSocketLibrary()
{
	log_println("JVM_InitializeSocketLibrary: IMPLEMENT ME!");
}


/* JVM_Socket */

jint JVM_Socket(jint domain, jint type, jint protocol)
{
#if PRINTJVM || 1
	log_println("JVM_Socket: domain=%d, type=%d, protocol=%d", domain, type, protocol);
#endif
	return socket(domain, type, protocol);
}


/* JVM_SocketClose */

jint JVM_SocketClose(jint fd)
{
#if PRINTJVM || 1
	log_println("JVM_SocketClose: fd=%d", fd);
#endif
	return close(fd);
}


/* JVM_SocketShutdown */

jint JVM_SocketShutdown(jint fd, jint howto)
{
#if PRINTJVM || 1
	log_println("JVM_SocketShutdown: fd=%d, howto=%d", fd, howto);
#endif
	return shutdown(fd, howto);
}


/* JVM_Recv */

jint JVM_Recv(jint fd, char *buf, jint nBytes, jint flags)
{
	log_println("JVM_Recv: IMPLEMENT ME!");
}


/* JVM_Send */

jint JVM_Send(jint fd, char *buf, jint nBytes, jint flags)
{
	log_println("JVM_Send: IMPLEMENT ME!");
}


/* JVM_Timeout */

jint JVM_Timeout(int fd, long timeout)
{
	log_println("JVM_Timeout: IMPLEMENT ME!");
}


/* JVM_Listen */

jint JVM_Listen(jint fd, jint count)
{
#if PRINTJVM || 1
	log_println("JVM_Listen: fd=%d, count=%d", fd, count);
#endif
	return listen(fd, count);
}


/* JVM_Connect */

jint JVM_Connect(jint fd, struct sockaddr *him, jint len)
{
#if PRINTJVM || 1
	log_println("JVM_Connect: fd=%d, him=%p, len=%d", fd, him, len);
#endif
	return connect(fd, him, len);
}


/* JVM_Bind */

jint JVM_Bind(jint fd, struct sockaddr *him, jint len)
{
	log_println("JVM_Bind: IMPLEMENT ME!");
}


/* JVM_Accept */

jint JVM_Accept(jint fd, struct sockaddr *him, jint *len)
{
#if PRINTJVM || 1
	log_println("JVM_Accept: fd=%d, him=%p, len=%p", fd, him, len);
#endif
	return accept(fd, him, (socklen_t *) len);
}


/* JVM_RecvFrom */

jint JVM_RecvFrom(jint fd, char *buf, int nBytes, int flags, struct sockaddr *from, int *fromlen)
{
	log_println("JVM_RecvFrom: IMPLEMENT ME!");
}


/* JVM_GetSockName */

jint JVM_GetSockName(jint fd, struct sockaddr *him, int *len)
{
#if PRINTJVM || 1
	log_println("JVM_GetSockName: fd=%d, him=%p, len=%p", fd, him, len);
#endif
	return getsockname(fd, him, (socklen_t *) len);
}


/* JVM_SendTo */

jint JVM_SendTo(jint fd, char *buf, int len, int flags, struct sockaddr *to, int tolen)
{
	log_println("JVM_SendTo: IMPLEMENT ME!");
}


/* JVM_SocketAvailable */

jint JVM_SocketAvailable(jint fd, jint *pbytes)
{
	log_println("JVM_SocketAvailable: IMPLEMENT ME!");
}


/* JVM_GetSockOpt */

jint JVM_GetSockOpt(jint fd, int level, int optname, char *optval, int *optlen)
{
	log_println("JVM_GetSockOpt: IMPLEMENT ME!");
}


/* JVM_SetSockOpt */

jint JVM_SetSockOpt(jint fd, int level, int optname, const char *optval, int optlen)
{
#if PRINTJVM || 1
	log_println("JVM_SetSockOpt: fd=%d, level=%d, optname=%d, optval=%s, optlen=%d", fd, level, optname, optval, optlen);
#endif
	return setsockopt(fd, level, optname, optval, optlen);
}


/* JVM_GetHostName */

int JVM_GetHostName(char* name, int namelen)
{
#if PRINTJVM || 1
	log_println("JVM_GetHostName: name=%s, namelen=%d", name, namelen);
#endif
	return gethostname(name, namelen);
}


/* JVM_GetHostByAddr */

struct hostent* JVM_GetHostByAddr(const char* name, int len, int type)
{
	log_println("JVM_GetHostByAddr: IMPLEMENT ME!");
}


/* JVM_GetHostByName */

struct hostent* JVM_GetHostByName(char* name)
{
	log_println("JVM_GetHostByName: IMPLEMENT ME!");
}


/* JVM_GetProtoByName */

struct protoent* JVM_GetProtoByName(char* name)
{
	log_println("JVM_GetProtoByName: IMPLEMENT ME!");
}


/* JVM_LoadLibrary */

void* JVM_LoadLibrary(const char* name)
{
#if PRINTJVM
	log_println("JVM_LoadLibrary: name=%s", name);
#endif
    return native_library_open(utf_new_char(name));
}


/* JVM_UnloadLibrary */

void JVM_UnloadLibrary(void* handle)
{
	log_println("JVM_UnloadLibrary: IMPLEMENT ME!");
}


/* JVM_FindLibraryEntry */

void* JVM_FindLibraryEntry(void* handle, const char* name)
{
	lt_ptr symbol;

#if PRINTJVM
	log_println("JVM_FindLibraryEntry: handle=%p, name=%s", handle, name);
#endif

	symbol = lt_dlsym(handle, name);

	return symbol;
}


/* JVM_IsNaN */

jboolean JVM_IsNaN(jdouble a)
{
	log_println("JVM_IsNaN: IMPLEMENT ME!");
}


/* JVM_IsSupportedJNIVersion */

jboolean JVM_IsSupportedJNIVersion(jint version)
{
#if PRINTJVM
	log_println("JVM_IsSupportedJNIVersion: version=%d", version);
#endif
	switch (version) {
	case JNI_VERSION_1_1:
	case JNI_VERSION_1_2:
	case JNI_VERSION_1_4:
		return true;
	default:
		return false;
	}
}


/* JVM_InternString */

jstring JVM_InternString(JNIEnv *env, jstring str)
{
#if PRINTJVM
	log_println("JVM_InternString: str=%p", str);
#endif
	return (jstring) _Jv_java_lang_String_intern((java_lang_String *) str);
}


/* JVM_RawMonitorCreate */

JNIEXPORT void* JNICALL JVM_RawMonitorCreate(void)
{
	java_objectheader *o;

#if PRINTJVM
	log_println("JVM_RawMonitorCreate");
#endif

	o = NEW(java_objectheader);

	lock_init_object_lock(o);

	return o;
}


/* JVM_RawMonitorDestroy */

JNIEXPORT void JNICALL JVM_RawMonitorDestroy(void *mon)
{
#if PRINTJVM
	log_println("JVM_RawMonitorDestroy: mon=%p", mon);
#endif
	FREE(mon, java_objectheader);
}


/* JVM_RawMonitorEnter */

JNIEXPORT jint JNICALL JVM_RawMonitorEnter(void *mon)
{
#if PRINTJVM
	log_println("JVM_RawMonitorEnter: mon=%p", mon);
#endif
	(void) lock_monitor_enter((java_objectheader *) mon);

	return 0;
}


/* JVM_RawMonitorExit */

JNIEXPORT void JNICALL JVM_RawMonitorExit(void *mon)
{
#if PRINTJVM
	log_println("JVM_RawMonitorExit: mon=%p", mon);
#endif
	(void) lock_monitor_exit((java_objectheader *) mon);
}


/* JVM_SetPrimitiveFieldValues */

void JVM_SetPrimitiveFieldValues(JNIEnv *env, jclass cb, jobject obj, jlongArray fieldIDs, jcharArray typecodes, jbyteArray data)
{
	log_println("JVM_SetPrimitiveFieldValues: IMPLEMENT ME!");
}


/* JVM_GetPrimitiveFieldValues */

void JVM_GetPrimitiveFieldValues(JNIEnv *env, jclass cb, jobject obj, jlongArray fieldIDs, jcharArray typecodes, jbyteArray data)
{
	log_println("JVM_GetPrimitiveFieldValues: IMPLEMENT ME!");
}


/* JVM_AccessVMBooleanFlag */

jboolean JVM_AccessVMBooleanFlag(const char* name, jboolean* value, jboolean is_get)
{
	log_println("JVM_AccessVMBooleanFlag: IMPLEMENT ME!");
}


/* JVM_AccessVMIntFlag */

jboolean JVM_AccessVMIntFlag(const char* name, jint* value, jboolean is_get)
{
	log_println("JVM_AccessVMIntFlag: IMPLEMENT ME!");
}


/* JVM_VMBreakPoint */

void JVM_VMBreakPoint(JNIEnv *env, jobject obj)
{
	log_println("JVM_VMBreakPoint: IMPLEMENT ME!");
}


/* JVM_GetClassFields */

jobjectArray JVM_GetClassFields(JNIEnv *env, jclass cls, jint which)
{
	log_println("JVM_GetClassFields: IMPLEMENT ME!");
}


/* JVM_GetClassMethods */

jobjectArray JVM_GetClassMethods(JNIEnv *env, jclass cls, jint which)
{
	log_println("JVM_GetClassMethods: IMPLEMENT ME!");
}


/* JVM_GetClassConstructors */

jobjectArray JVM_GetClassConstructors(JNIEnv *env, jclass cls, jint which)
{
	log_println("JVM_GetClassConstructors: IMPLEMENT ME!");
}


/* JVM_GetClassField */

jobject JVM_GetClassField(JNIEnv *env, jclass cls, jstring name, jint which)
{
	log_println("JVM_GetClassField: IMPLEMENT ME!");
}


/* JVM_GetClassMethod */

jobject JVM_GetClassMethod(JNIEnv *env, jclass cls, jstring name, jobjectArray types, jint which)
{
	log_println("JVM_GetClassMethod: IMPLEMENT ME!");
}


/* JVM_GetClassConstructor */

jobject JVM_GetClassConstructor(JNIEnv *env, jclass cls, jobjectArray types, jint which)
{
	log_println("JVM_GetClassConstructor: IMPLEMENT ME!");
}


/* JVM_NewInstance */

jobject JVM_NewInstance(JNIEnv *env, jclass cls)
{
	log_println("JVM_NewInstance: IMPLEMENT ME!");
}


/* JVM_GetField */

jobject JVM_GetField(JNIEnv *env, jobject field, jobject obj)
{
	log_println("JVM_GetField: IMPLEMENT ME!");
}


/* JVM_GetPrimitiveField */

jvalue JVM_GetPrimitiveField(JNIEnv *env, jobject field, jobject obj, unsigned char wCode)
{
	log_println("JVM_GetPrimitiveField: IMPLEMENT ME!");
}


/* JVM_SetField */

void JVM_SetField(JNIEnv *env, jobject field, jobject obj, jobject val)
{
	log_println("JVM_SetField: IMPLEMENT ME!");
}


/* JVM_SetPrimitiveField */

void JVM_SetPrimitiveField(JNIEnv *env, jobject field, jobject obj, jvalue v, unsigned char vCode)
{
	log_println("JVM_SetPrimitiveField: IMPLEMENT ME!");
}


/* JVM_InvokeMethod */

jobject JVM_InvokeMethod(JNIEnv *env, jobject method, jobject obj, jobjectArray args0)
{
#if PRINTJVM
	log_println("JVM_InvokeMethod: method=%p, obj=%p, args0=%p", method, obj, args0);
#endif
	return (jobject) _Jv_java_lang_reflect_Method_invoke((java_lang_reflect_Method *) method, (java_lang_Object *) obj, (java_objectarray *) args0);
}


/* JVM_NewInstanceFromConstructor */

jobject JVM_NewInstanceFromConstructor(JNIEnv *env, jobject c, jobjectArray args0)
{
#if PRINTJVM
	log_println("JVM_NewInstanceFromConstructor: c=%p, args0=%p", c, args0);
#endif
	return (jobject) _Jv_java_lang_reflect_Constructor_newInstance(env, (java_lang_reflect_Constructor *) c, (java_objectarray *) args0);
}


/* JVM_SupportsCX8 */

jboolean JVM_SupportsCX8()
{
#if PRINTJVM
	log_println("JVM_SupportsCX8");
#endif
	return _Jv_java_util_concurrent_atomic_AtomicLong_VMSupportsCS8(NULL, NULL);
}


/* JVM_CX8Field */

jboolean JVM_CX8Field(JNIEnv *env, jobject obj, jfieldID fid, jlong oldVal, jlong newVal)
{
	log_println("JVM_CX8Field: IMPLEMENT ME!");
}


/* JVM_GetAllThreads */

jobjectArray JVM_GetAllThreads(JNIEnv *env, jclass dummy)
{
	log_println("JVM_GetAllThreads: IMPLEMENT ME!");
}


/* JVM_DumpThreads */

jobjectArray JVM_DumpThreads(JNIEnv *env, jclass threadClass, jobjectArray threads)
{
	log_println("JVM_DumpThreads: IMPLEMENT ME!");
}


/* JVM_GetManagement */

void* JVM_GetManagement(jint version)
{
	log_println("JVM_GetManagement: IMPLEMENT ME!");
}


/* JVM_InitAgentProperties */

jobject JVM_InitAgentProperties(JNIEnv *env, jobject properties)
{
	log_println("JVM_InitAgentProperties: IMPLEMENT ME!");
}


/* JVM_GetEnclosingMethodInfo */

jobjectArray JVM_GetEnclosingMethodInfo(JNIEnv *env, jclass ofClass)
{
	log_println("JVM_GetEnclosingMethodInfo: IMPLEMENT ME!");
}


/* JVM_GetThreadStateValues */

jintArray JVM_GetThreadStateValues(JNIEnv* env, jint javaThreadState)
{
	log_println("JVM_GetThreadStateValues: IMPLEMENT ME!");
}


/* JVM_GetThreadStateValues */

jobjectArray JVM_GetThreadStateNames(JNIEnv* env, jint javaThreadState, jintArray values)
{
	log_println("JVM_GetThreadStateValues: IMPLEMENT ME!");
}


/* JVM_GetVersionInfo */

void JVM_GetVersionInfo(JNIEnv* env, jvm_version_info* info, size_t info_size)
{
	log_println("JVM_GetVersionInfo: IMPLEMENT ME!");
}


/* OS: JVM_RegisterSignal */

void* JVM_RegisterSignal(jint sig, void* handler)
{
	log_println("JVM_RegisterSignal: sig=%d, handler=%p, IMPLEMENT ME!", sig, handler);
	return NULL;
}


/* OS: JVM_FindSignal */

jint JVM_FindSignal(const char *name)
{
	log_println("JVM_FindSignal: name=%s", name);
	return 0;
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
