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

*/


#define PRINTJVM 0

#include "config.h"

#include <assert.h>
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
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_AssertionStatusDirectives.h"
#include "native/include/java_lang_String.h"            /* required by j.l.CL */
#include "native/include/java_nio_ByteBuffer.h"         /* required by j.l.CL */
#include "native/include/java_lang_ClassLoader.h"        /* required by j.l.C */
#include "native/include/java_lang_StackTraceElement.h"
#include "native/include/java_lang_Throwable.h"
#include "native/include/java_security_ProtectionDomain.h"
#include "native/include/java_lang_Integer.h"
#include "native/include/java_lang_Long.h"
#include "native/include/java_lang_Short.h"
#include "native/include/java_lang_Byte.h"
#include "native/include/java_lang_Character.h"
#include "native/include/java_lang_Boolean.h"
#include "native/include/java_lang_Float.h"
#include "native/include/java_lang_Double.h"

#if defined(ENABLE_ANNOTATIONS)
#include "native/include/sun_reflect_ConstantPool.h"
#endif

#include "native/vm/java_lang_Class.h"
#include "native/vm/java_lang_ClassLoader.h"
#include "native/vm/java_lang_Runtime.h"
#include "native/vm/java_lang_Thread.h"
#include "native/vm/java_lang_reflect_Constructor.h"
#include "native/vm/java_lang_reflect_Method.h"
#include "native/vm/reflect.h"

#include "threads/lock-common.h"
#include "threads/threads-common.h"

#include "toolbox/logging.h"

#include "vm/array.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/primitive.h"
#include "vm/properties.h"
#include "vm/resolve.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vm/jit/stacktrace.h"

#include "vmcore/classcache.h"
#include "vmcore/options.h"


/* debugging macros ***********************************************************/

#if !defined(NDEBUG)

# define TRACEJVMCALLS(...) \
    do { \
        if (opt_TraceJVMCalls) { \
            log_println(__VA_ARGS__); \
        } \
    } while (0)

# define PRINTJVMWARNINGS(...)
/*     do { \ */
/*         if (opt_PrintJVMWarnings) { \ */
/*             log_println(__VA_ARGS__); \ */
/*         } \ */
/*     } while (0) */

#else

# define TRACEJVMCALLS(...)
# define PRINTJVMWARNINGS(...)

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
	java_handle_t *s;
	java_handle_t *d;

	s = (java_handle_t *) src;
	d = (java_handle_t *) dst;

#if PRINTJVM
	log_println("JVM_ArrayCopy: src=%p, src_pos=%d, dst=%p, dst_pos=%d, length=%d", src, src_pos, dst, dst_pos, length);
#endif

	(void) builtin_arraycopy(s, src_pos, d, dst_pos, length);
}


/* JVM_InitProperties */

jobject JVM_InitProperties(JNIEnv *env, jobject properties)
{
#if PRINTJVM
	log_println("JVM_InitProperties: properties=%d", properties);
#endif
	properties_system_add_all((java_handle_t *) properties);
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
	TRACEJVMCALLS("JVM_GC()");

	gc_call();
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
	TRACEJVMCALLS("JVM_TotalMemory()");

	return gc_get_heap_size();
}


/* JVM_FreeMemory */

jlong JVM_FreeMemory(void)
{
	TRACEJVMCALLS("JVM_FreeMemory()");

	return gc_get_free_bytes();
}


/* JVM_MaxMemory */

jlong JVM_MaxMemory(void)
{
	TRACEJVMCALLS("JVM_MaxMemory()");

	return gc_get_max_heap_size();
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
		_Jv_java_lang_Class_getName(LLNI_classinfo_wrap(ste->method->class));

	/* fill the java.lang.StackTraceElement element */

	o->declaringClass = declaringclass;
	o->methodName     = (java_lang_String *) javastring_new(ste->method->name);
	o->fileName       = filename;
	o->lineNumber     = linenumber;

	return (jobject) o;
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
#if defined(ENABLE_THREADS)
	java_handle_t *o;
#endif

	TRACEJVMCALLS("JVM_MonitorWait(env=%p, handle=%p, ms=%ld)", env, handle, ms);
    if (ms < 0) {
/* 		exceptions_throw_illegalargumentexception("argument out of range"); */
		exceptions_throw_illegalargumentexception();
		return;
	}

#if defined(ENABLE_THREADS)
	o = (java_handle_t *) handle;

	LLNI_CRITICAL_START;
	lock_wait_for_object(LLNI_DIRECT(o), ms, 0);
	LLNI_CRITICAL_END;
#endif
}


/* JVM_MonitorNotify */

void JVM_MonitorNotify(JNIEnv* env, jobject handle)
{
#if defined(ENABLE_THREADS)
	java_handle_t *o;
#endif

	TRACEJVMCALLS("JVM_MonitorNotify(env=%p, handle=%p)", env, handle);

#if defined(ENABLE_THREADS)
	o = (java_handle_t *) handle;

	LLNI_CRITICAL_START;
	lock_notify_object(LLNI_DIRECT(o));
	LLNI_CRITICAL_END;
#endif
}


/* JVM_MonitorNotifyAll */

void JVM_MonitorNotifyAll(JNIEnv* env, jobject handle)
{
#if defined(ENABLE_THREADS)
	java_handle_t *o;
#endif

	TRACEJVMCALLS("JVM_MonitorNotifyAll(env=%p, handle=%p)", env, handle);

#if defined(ENABLE_THREADS)
	o = (java_handle_t *) handle;

	LLNI_CRITICAL_START;
	lock_notify_all_object(LLNI_DIRECT(o));
	LLNI_CRITICAL_END;
#endif
}


/* JVM_Clone */

jobject JVM_Clone(JNIEnv* env, jobject handle)
{
#if PRINTJVM
	log_println("JVM_Clone: handle=%p", handle);
#endif
	return (jobject) builtin_clone(env, (java_handle_t *) handle);
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
	TRACEJVMCALLS("JVM_EnableCompiler(env=%p, compCls=%p)", env, compCls);
	PRINTJVMWARNINGS("JVM_EnableCompiler not supported");
}


/* JVM_DisableCompiler */

void JVM_DisableCompiler(JNIEnv *env, jclass compCls)
{
	TRACEJVMCALLS("JVM_DisableCompiler(env=%p, compCls=%p)", env, compCls);
	PRINTJVMWARNINGS("JVM_DisableCompiler not supported");
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

char *JVM_NativePath(char *path)
{
	TRACEJVMCALLS("JVM_NativePath(path=%s)", path);

	/* XXX is this correct? */

	return path;
}


/* JVM_GetCallerClass */

jclass JVM_GetCallerClass(JNIEnv* env, int depth)
{
	java_handle_objectarray_t *oa;

	TRACEJVMCALLS("JVM_GetCallerClass(env=%p, depth=%d)", env, depth);

	oa = stacktrace_getClassContext();

	if (oa == NULL)
		return NULL;

	if (oa->header.size < depth)
		return NULL;

	return (jclass) oa->data[depth - 1];

}


/* JVM_FindPrimitiveClass */

jclass JVM_FindPrimitiveClass(JNIEnv* env, const char* s)
{
	classinfo *c;
	utf       *u;

	TRACEJVMCALLS("JVM_FindPrimitiveClass(env=%p, s=%s)", env, s);

	u = utf_new_char(s);
	c = primitive_class_get_by_name(u);

	return (jclass) LLNI_classinfo_wrap(c);
}


/* JVM_ResolveClass */

void JVM_ResolveClass(JNIEnv* env, jclass cls)
{
	TRACEJVMCALLS("JVM_ResolveClass(env=%p, cls=%p)", env, cls);
	PRINTJVMWARNINGS("JVM_ResolveClass not implemented");
}


/* JVM_FindClassFromClassLoader */

jclass JVM_FindClassFromClassLoader(JNIEnv* env, const char* name, jboolean init, jobject loader, jboolean throwError)
{
	classinfo   *c;
	utf         *u;
	classloader *cl;

	TRACEJVMCALLS("JVM_FindClassFromClassLoader: name=%s, init=%d, loader=%p, throwError=%d", name, init, loader, throwError);

	u  = utf_new_char(name);
	cl = loader_hashtable_classloader_add((java_handle_t *) loader);

	c = load_class_from_classloader(u, cl);

	if (c == NULL)
		return NULL;

	if (init)
		if (!(c->state & CLASS_INITIALIZED))
			if (!initialize_class(c))
				return NULL;

	return (jclass) LLNI_classinfo_wrap(c);
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
	classinfo   *c;
	utf         *u;
	classloader *cl;

	TRACEJVMCALLS("JVM_DefineClassWithSource(env=%p, name=%s, loader=%p, buf=%p, len=%d, pd=%p, source=%s)", env, name, loader, buf, len, pd, source);

	u  = utf_new_char(name);
	cl = loader_hashtable_classloader_add((java_handle_t *) loader);

	/* XXX do something with source */

	c = class_define(u, cl, len, (const uint8_t *) buf, (java_handle_t *) pd);

	return (jclass) LLNI_classinfo_wrap(c);
}


/* JVM_FindLoadedClass */

jclass JVM_FindLoadedClass(JNIEnv *env, jobject loader, jstring name)
{
	classloader *cl;
	utf         *u;
	classinfo   *c;

	TRACEJVMCALLS("JVM_FindLoadedClass(env=%p, loader=%p, name=%p)", env, loader, name);

	cl = loader_hashtable_classloader_add((java_handle_t *) loader);

	u = javastring_toutf((java_handle_t *) name, true);
	c = classcache_lookup(cl, u);

	return (jclass) LLNI_classinfo_wrap(c);
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
	classinfo                 *c;
	java_handle_objectarray_t *oa;

	TRACEJVMCALLS("JVM_GetClassInterfaces(env=%p, cls=%p)", env, cls);

	c = LLNI_classinfo_unwrap(cls);

	oa = class_get_interfaces(c);

	return (jobjectArray) oa;
}


/* JVM_GetClassLoader */

jobject JVM_GetClassLoader(JNIEnv *env, jclass cls)
{
	classinfo   *c;
	classloader *cl;

	TRACEJVMCALLS("JVM_GetClassLoader(env=%p, cls=%p)", env, cls);

	c  = LLNI_classinfo_unwrap(cls);
	cl = class_get_classloader(c);

	return (jobject) cl;
}


/* JVM_IsInterface */

jboolean JVM_IsInterface(JNIEnv *env, jclass cls)
{
	classinfo *c;

#if PRINTJVM
	log_println("JVM_IsInterface: cls=%p", cls);
#endif

	c = LLNI_classinfo_unwrap(cls);

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

	TRACEJVMCALLS("JVM_GetProtectionDomain(env=%p, cls=%p)", env, cls);

	c = LLNI_classinfo_unwrap(cls);

	if (c == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

    /* Primitive types do not have a protection domain. */

	if (class_is_primitive(c))
		return NULL;

	return (jobject) c->protectiondomain;
}


/* JVM_SetProtectionDomain */

void JVM_SetProtectionDomain(JNIEnv *env, jclass cls, jobject protection_domain)
{
	log_println("JVM_SetProtectionDomain: IMPLEMENT ME!");
}


/* JVM_DoPrivileged */

jobject JVM_DoPrivileged(JNIEnv *env, jclass cls, jobject action, jobject context, jboolean wrapException)
{
	java_handle_t *o;
	classinfo     *c;
	methodinfo    *m;
	java_handle_t *result;
	java_handle_t *e;

#if PRINTJVM
	log_println("JVM_DoPrivileged: action=%p, context=%p, wrapException=%d", action, context, wrapException);
#endif

	o = (java_handle_t *) action;
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
	return class_is_array(LLNI_classinfo_unwrap(cls));
}


/* JVM_IsPrimitiveClass */

jboolean JVM_IsPrimitiveClass(JNIEnv *env, jclass cls)
{
	classinfo *c;

	c = LLNI_classinfo_unwrap(cls);

#if PRINTJVM
	log_println("JVM_IsPrimitiveClass(cls=%p)", cls);
#endif

	return class_is_primitive(c);
}


/* JVM_GetComponentType */

jclass JVM_GetComponentType(JNIEnv *env, jclass cls)
{
	classinfo *component;
	classinfo *c;
	
	TRACEJVMCALLS("JVM_GetComponentType(env=%p, cls=%p)", env, cls);

	c = LLNI_classinfo_unwrap(cls);
	
	component = class_get_componenttype(c);

	return (jclass) LLNI_classinfo_wrap(component);
}


/* JVM_GetClassModifiers */

jint JVM_GetClassModifiers(JNIEnv *env, jclass cls)
{
	classinfo *c;

#if PRINTJVM
	log_println("JVM_GetClassModifiers: cls=%p", cls);
#endif

	c = LLNI_classinfo_unwrap(cls);

	/* XXX is this correct? */

	return c->flags & ACC_CLASS_REFLECT_MASK;
}


/* JVM_GetDeclaredClasses */

jobjectArray JVM_GetDeclaredClasses(JNIEnv *env, jclass ofClass)
{
	classinfo                 *c;
	java_handle_objectarray_t *oa;

	TRACEJVMCALLS("JVM_GetDeclaredClasses(env=%p, ofClass=%p)", env, ofClass);

	c = LLNI_classinfo_unwrap(ofClass);

	oa = class_get_declaredclasses(c, false);

	return (jobjectArray) oa;
}


/* JVM_GetDeclaringClass */

jclass JVM_GetDeclaringClass(JNIEnv *env, jclass ofClass)
{
	classinfo *c;
	classinfo *dc;

	TRACEJVMCALLS("JVM_GetDeclaringClass(env=%p, ofClass=%p)", env, ofClass);

	c = LLNI_classinfo_unwrap(ofClass);

	dc = class_get_declaringclass(c);

	return (jclass) LLNI_classinfo_wrap(dc);
}


/* JVM_GetClassSignature */

jstring JVM_GetClassSignature(JNIEnv *env, jclass cls)
{
	classinfo     *c;
	utf           *u;
	java_handle_t *s;

	TRACEJVMCALLS("JVM_GetClassSignature(env=%p, cls=%p)", env, cls);

	c = LLNI_classinfo_unwrap(cls);

	/* Get the signature of the class. */

	u = class_get_signature(c);

	if (u == NULL)
		return NULL;

	/* Convert UTF-string to a Java-string. */

	s = javastring_new(u);

	return (jstring) s;
}


/* JVM_GetClassAnnotations */

jbyteArray JVM_GetClassAnnotations(JNIEnv *env, jclass cls)
{
	classinfo               *c           = NULL; /* classinfo for 'cls'  */
	java_handle_bytearray_t *annotations = NULL; /* unparsed annotations */

	TRACEJVMCALLS("JVM_GetClassAnnotations: cls=%p", cls);

	if (cls == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}
	
	c = LLNI_classinfo_unwrap(cls);

	/* get annotations: */
	annotations = class_get_annotations(c);

	return (jbyteArray)annotations;
}


/* JVM_GetFieldAnnotations */

jbyteArray JVM_GetFieldAnnotations(JNIEnv *env, jobject field)
{
	java_lang_reflect_Field *rf = NULL; /* java.lang.reflect.Field for 'field' */
	java_handle_bytearray_t *ba = NULL; /* unparsed annotations */

	TRACEJVMCALLS("JVM_GetFieldAnnotations: field=%p", field);

	if (field == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	rf = (java_lang_reflect_Field*)field;

	LLNI_field_get_ref(rf, annotations, ba);

	return (jbyteArray)ba;
}


/* JVM_GetMethodAnnotations */

jbyteArray JVM_GetMethodAnnotations(JNIEnv *env, jobject method)
{
	java_lang_reflect_Method *rm = NULL; /* java.lang.reflect.Method for 'method' */
	java_handle_bytearray_t  *ba = NULL; /* unparsed annotations */

	TRACEJVMCALLS("JVM_GetMethodAnnotations: method=%p", method);

	if (method == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	rm = (java_lang_reflect_Method*)method;

	LLNI_field_get_ref(rm, annotations, ba);

	return (jbyteArray)ba;
}


/* JVM_GetMethodDefaultAnnotationValue */

jbyteArray JVM_GetMethodDefaultAnnotationValue(JNIEnv *env, jobject method)
{
	java_lang_reflect_Method *rm = NULL; /* java.lang.reflect.Method for 'method' */
	java_handle_bytearray_t  *ba = NULL; /* unparsed annotation default value */

	TRACEJVMCALLS("JVM_GetMethodDefaultAnnotationValue: method=%p", method);

	if (method == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	rm = (java_lang_reflect_Method*)method;

	LLNI_field_get_ref(rm, annotationDefault, ba);

	return (jbyteArray)ba;
}


/* JVM_GetMethodParameterAnnotations */

jbyteArray JVM_GetMethodParameterAnnotations(JNIEnv *env, jobject method)
{
	java_lang_reflect_Method *rm = NULL; /* java.lang.reflect.Method for 'method' */
	java_handle_bytearray_t  *ba = NULL; /* unparsed parameter annotations */

	TRACEJVMCALLS("JVM_GetMethodParameterAnnotations: method=%p", method);

	if (method == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	rm = (java_lang_reflect_Method*)method;

	LLNI_field_get_ref(rm, parameterAnnotations, ba);

	return (jbyteArray)ba;
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

	c = LLNI_classinfo_unwrap(cls);

	return c->flags & ACC_CLASS_REFLECT_MASK;
}


/* JVM_GetClassConstantPool */

jobject JVM_GetClassConstantPool(JNIEnv *env, jclass cls)
{
#if defined(ENABLE_ANNOTATIONS)
	sun_reflect_ConstantPool *constantPool    = NULL;
	              /* constant pool object for the class refered by 'cls' */
	java_lang_Object         *constantPoolOop = (java_lang_Object*)cls;
	              /* constantPoolOop field of the constant pool object   */

	TRACEJVMCALLS("JVM_GetClassConstantPool(env=%p, cls=%p)", env, cls);

	constantPool = 
		(sun_reflect_ConstantPool*)native_new_and_init(
			class_sun_reflect_ConstantPool);
	
	if (constantPool == NULL) {
		/* out of memory */
		return NULL;
	}
	
	LLNI_field_set_ref(constantPool, constantPoolOop, constantPoolOop);

	return (jobject)constantPool;
#else
	log_println("JVM_GetClassConstantPool(env=%p, cls=%p): not implemented in this configuration!", env, cls);
	return NULL;
#endif
}


/* JVM_ConstantPoolGetSize */

jint JVM_ConstantPoolGetSize(JNIEnv *env, jobject unused, jobject jcpool)
{
	classinfo *c; /* classinfo of the class for which 'this' is the constant pool */

	TRACEJVMCALLS("JVM_ConstantPoolGetSize(env=%p, unused=%p, jcpool=%p)", env, unused, jcpool);

	c = LLNI_classinfo_unwrap(jcpool);

	return c->cpcount;
}


/* JVM_ConstantPoolGetClassAt */

jclass JVM_ConstantPoolGetClassAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	constant_classref *ref;    /* classref to the class at constant pool index 'index' */
	classinfo         *c;      /* classinfo of the class for which 'this' is the constant pool */
	classinfo         *result; /* classinfo of the class at constant pool index 'index' */

	TRACEJVMCALLS("JVM_ConstantPoolGetClassAt(env=%p, jcpool=%p, index=%d)", env, jcpool, index);

	c = LLNI_classinfo_unwrap(jcpool);

	ref = (constant_classref *) class_getconstant(c, index, CONSTANT_Class);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	result = resolve_classref_eager(ref);

	return (jclass) LLNI_classinfo_wrap(result);
}


/* JVM_ConstantPoolGetClassAtIfLoaded */

jclass JVM_ConstantPoolGetClassAtIfLoaded(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	constant_classref *ref;    /* classref to the class at constant pool index 'index' */
	classinfo         *c;      /* classinfo of the class for which 'this' is the constant pool */
	classinfo         *result; /* classinfo of the class at constant pool index 'index' */

	TRACEJVMCALLS("JVM_ConstantPoolGetClassAtIfLoaded(env=%p, unused=%p, jcpool=%p, index=%d)", env, unused, jcpool, index);

	c = LLNI_classinfo_unwrap(jcpool);

	ref = (constant_classref *) class_getconstant(c, index, CONSTANT_Class);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}
	
	if (!resolve_classref(NULL, ref, resolveLazy, true, true, &result)) {
		return NULL;
	}

	if ((result == NULL) || !(result->state & CLASS_LOADED)) {
		return NULL;
	}
	
	return (jclass) LLNI_classinfo_wrap(result);
}


/* JVM_ConstantPoolGetMethodAt */

jobject JVM_ConstantPoolGetMethodAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	constant_FMIref *ref; /* reference to the method in constant pool at index 'index' */
	classinfo *cls;       /* classinfo of the class for which 'this' is the constant pool */
	
	TRACEJVMCALLS("JVM_ConstantPoolGetMethodAt: jcpool=%p, index=%d", jcpool, index);
	
	cls = LLNI_classinfo_unwrap(jcpool);
	ref = (constant_FMIref*)class_getconstant(cls, index, CONSTANT_Methodref);
	
	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	/* XXX: is that right? or do I have to use resolve_method_*? */
	return (jobject)reflect_method_new(ref->p.method);
}


/* JVM_ConstantPoolGetMethodAtIfLoaded */

jobject JVM_ConstantPoolGetMethodAtIfLoaded(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	constant_FMIref *ref; /* reference to the method in constant pool at index 'index' */
	classinfo *c = NULL;  /* resolved declaring class of the method */
	classinfo *cls;       /* classinfo of the class for which 'this' is the constant pool */

	TRACEJVMCALLS("JVM_ConstantPoolGetMethodAtIfLoaded: jcpool=%p, index=%d", jcpool, index);

	cls = LLNI_classinfo_unwrap(jcpool);
	ref = (constant_FMIref*)class_getconstant(cls, index, CONSTANT_Methodref);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	if (!resolve_classref(NULL, ref->p.classref, resolveLazy, true, true, &c)) {
		return NULL;
	}

	if (c == NULL || !(c->state & CLASS_LOADED)) {
		return NULL;
	}

	return (jobject)reflect_method_new(ref->p.method);
}


/* JVM_ConstantPoolGetFieldAt */

jobject JVM_ConstantPoolGetFieldAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	constant_FMIref *ref; /* reference to the field in constant pool at index 'index' */
	classinfo *cls;       /* classinfo of the class for which 'this' is the constant pool */
	
	TRACEJVMCALLS("JVM_ConstantPoolGetFieldAt: jcpool=%p, index=%d", jcpool, index);

	cls = LLNI_classinfo_unwrap(jcpool);
	ref = (constant_FMIref*)class_getconstant(cls, index, CONSTANT_Fieldref);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	return (jobject)reflect_field_new(ref->p.field);
}


/* JVM_ConstantPoolGetFieldAtIfLoaded */

jobject JVM_ConstantPoolGetFieldAtIfLoaded(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	constant_FMIref *ref; /* reference to the field in constant pool at index 'index' */
	classinfo *c;         /* resolved declaring class for the field */
	classinfo *cls;       /* classinfo of the class for which 'this' is the constant pool */

	TRACEJVMCALLS("JVM_ConstantPoolGetFieldAtIfLoaded: jcpool=%p, index=%d", jcpool, index);

	cls = LLNI_classinfo_unwrap(jcpool);
	ref = (constant_FMIref*)class_getconstant(cls, index, CONSTANT_Fieldref);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	if (!resolve_classref(NULL, ref->p.classref, resolveLazy, true, true, &c)) {
		return NULL;
	}

	if (c == NULL || !(c->state & CLASS_LOADED)) {
		return NULL;
	}

	return (jobject)reflect_field_new(ref->p.field);
}


/* JVM_ConstantPoolGetMemberRefInfoAt */

jobjectArray JVM_ConstantPoolGetMemberRefInfoAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	log_println("JVM_ConstantPoolGetMemberRefInfoAt: jcpool=%p, index=%d, IMPLEMENT ME!", jcpool, index);

	/* TODO: implement. (this is yet unused be OpenJDK but, so very low priority) */

	return NULL;
}


/* JVM_ConstantPoolGetIntAt */

jint JVM_ConstantPoolGetIntAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	constant_integer *ref; /* reference to the int value in constant pool at index 'index' */
	classinfo *cls;        /* classinfo of the class for which 'this' is the constant pool */

	TRACEJVMCALLS("JVM_ConstantPoolGetIntAt: jcpool=%p, index=%d", jcpool, index);

	cls = LLNI_classinfo_unwrap(jcpool);
	ref = (constant_integer*)class_getconstant(cls, index, CONSTANT_Integer);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return 0;
	}

	return ref->value;
}


/* JVM_ConstantPoolGetLongAt */

jlong JVM_ConstantPoolGetLongAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	constant_long *ref; /* reference to the long value in constant pool at index 'index' */
	classinfo *cls;     /* classinfo of the class for which 'this' is the constant pool */

	TRACEJVMCALLS("JVM_ConstantPoolGetLongAt: jcpool=%p, index=%d", jcpool, index);

	cls = LLNI_classinfo_unwrap(jcpool);
	ref = (constant_long*)class_getconstant(cls, index, CONSTANT_Long);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return 0;
	}

	return ref->value;
}


/* JVM_ConstantPoolGetFloatAt */

jfloat JVM_ConstantPoolGetFloatAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	constant_float *ref; /* reference to the float value in constant pool at index 'index' */
	classinfo *cls;      /* classinfo of the class for which 'this' is the constant pool */

	TRACEJVMCALLS("JVM_ConstantPoolGetFloatAt: jcpool=%p, index=%d", jcpool, index);

	cls = LLNI_classinfo_unwrap(jcpool);
	ref = (constant_float*)class_getconstant(cls, index, CONSTANT_Float);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return 0;
	}

	return ref->value;
}


/* JVM_ConstantPoolGetDoubleAt */

jdouble JVM_ConstantPoolGetDoubleAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	constant_double *ref; /* reference to the double value in constant pool at index 'index' */
	classinfo *cls;       /* classinfo of the class for which 'this' is the constant pool */

	TRACEJVMCALLS("JVM_ConstantPoolGetDoubleAt: jcpool=%p, index=%d", jcpool, index);

	cls = LLNI_classinfo_unwrap(jcpool);
	ref = (constant_double*)class_getconstant(cls, index, CONSTANT_Double);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return 0;
	}

	return ref->value;
}


/* JVM_ConstantPoolGetStringAt */

jstring JVM_ConstantPoolGetStringAt(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	utf *ref;       /* utf object for the string in constant pool at index 'index' */
	classinfo *cls; /* classinfo of the class for which 'this' is the constant pool */

	TRACEJVMCALLS("JVM_ConstantPoolGetStringAt: jcpool=%p, index=%d", jcpool, index);
	
	cls = LLNI_classinfo_unwrap(jcpool);
	ref = (utf*)class_getconstant(cls, index, CONSTANT_String);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	/* XXX: I hope literalstring_new is the right Function. */
	return (jstring)literalstring_new(ref);
}


/* JVM_ConstantPoolGetUTF8At */

jstring JVM_ConstantPoolGetUTF8At(JNIEnv *env, jobject unused, jobject jcpool, jint index)
{
	utf *ref; /* utf object for the utf8 data in constant pool at index 'index' */
	classinfo *cls; /* classinfo of the class for which 'this' is the constant pool */

	TRACEJVMCALLS("JVM_ConstantPoolGetUTF8At: jcpool=%p, index=%d", jcpool, index);

	cls = LLNI_classinfo_unwrap(jcpool);
	ref = (utf*)class_getconstant(cls, index, CONSTANT_Utf8);

	if (ref == NULL) {
		exceptions_throw_illegalargumentexception();
		return NULL;
	}

	/* XXX: I hope literalstring_new is the right Function. */
	return (jstring)literalstring_new(ref);
}


/* JVM_DesiredAssertionStatus */

jboolean JVM_DesiredAssertionStatus(JNIEnv *env, jclass unused, jclass cls)
{
	TRACEJVMCALLS("JVM_DesiredAssertionStatus(env=%p, unused=%p, cls=%p)", env, unused, cls);

	/* TODO: Implement this one, but false should be OK. */

	return false;
}


/* JVM_AssertionStatusDirectives */

jobject JVM_AssertionStatusDirectives(JNIEnv *env, jclass unused)
{
	classinfo                           *c;
	java_lang_AssertionStatusDirectives *o;
	java_handle_objectarray_t           *classes;
	java_handle_objectarray_t           *packages;

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
	int bytes;

	TRACEJVMCALLS("JVM_Available(fd=%d, pbytes=%p)", fd, pbytes);

	*pbytes = 0;

	if (ioctl(fd, FIONREAD, &bytes) < 0)
		return 0;

	*pbytes = bytes;

	return 1;
#else
# error FIONREAD not defined
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
	TRACEJVMCALLS("JVM_Yield(env=%p, threadClass=%p)", env, threadClass);

	threads_yield();
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
    /* XXX if a method in a class in a trusted loader is in a
	   doPrivileged, return NULL */

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
	log_println("JVM_GetSystemPackage(env=%p, name=%p)");
	javastring_print((java_handle_t *) name);
	printf("\n");

	return NULL;
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
	java_handle_t *a;

	TRACEJVMCALLS("JVM_GetArrayLength(arr=%p)", arr);

	a = (java_handle_t *) arr;

	return array_length_get(a);
}


/* JVM_GetArrayElement */

jobject JVM_GetArrayElement(JNIEnv *env, jobject arr, jint index)
{
	java_handle_t *a;
	java_handle_t *o;

	TRACEJVMCALLS("JVM_GetArrayElement(env=%p, arr=%p, index=%d)", env, arr, index);

	a = (java_handle_t *) arr;

/* 	if (!class_is_array(a->objheader.vftbl->class)) { */
/* 		exceptions_throw_illegalargumentexception(); */
/* 		return NULL; */
/* 	} */

	o = array_element_get(a, index);

	return (jobject) o;
}


/* JVM_GetPrimitiveArrayElement */

jvalue JVM_GetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jint wCode)
{
	log_println("JVM_GetPrimitiveArrayElement: IMPLEMENT ME!");
}


/* JVM_SetArrayElement */

void JVM_SetArrayElement(JNIEnv *env, jobject arr, jint index, jobject val)
{
	java_handle_t *a;
	java_handle_t *value;

	TRACEJVMCALLS("JVM_SetArrayElement(env=%p, arr=%p, index=%d, val=%p)", env, arr, index, val);

	a     = (java_handle_t *) arr;
	value = (java_handle_t *) val;

	array_element_set(a, index, value);
}


/* JVM_SetPrimitiveArrayElement */

void JVM_SetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jvalue v, unsigned char vCode)
{
	log_println("JVM_SetPrimitiveArrayElement: IMPLEMENT ME!");
}


/* JVM_NewArray */

jobject JVM_NewArray(JNIEnv *env, jclass eltClass, jint length)
{
	classinfo                 *c;
	classinfo                 *pc;
	java_handle_t             *a;
	java_handle_objectarray_t *oa;

	TRACEJVMCALLS("JVM_NewArray(env=%p, eltClass=%p, length=%d)", env, eltClass, length);

	if (eltClass == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* NegativeArraySizeException is checked in builtin_newarray. */

	c = LLNI_classinfo_unwrap(eltClass);

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
	classinfo                 *c;
	java_handle_intarray_t    *ia;
	int32_t                    length;
	long                      *dims;
	int32_t                    value;
	int32_t                    i;
	classinfo                 *ac;
	java_handle_objectarray_t *a;

	TRACEJVMCALLS("JVM_NewMultiArray(env=%p, eltClass=%p, dim=%p)", env, eltClass, dim);

	if (eltClass == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* NegativeArraySizeException is checked in builtin_newarray. */

	c = LLNI_classinfo_unwrap(eltClass);

	/* XXX This is just a quick hack to get it working. */

	ia = (java_handle_intarray_t *) dim;

	length = array_length_get(ia);

	dims = MNEW(long, length);

	for (i = 0; i < length; i++) {
		value = LLNI_array_direct(ia, i);
		dims[i] = (long) value;
	}

	/* Create an array-class if necessary. */

	if (class_is_primitive(c))
		ac = primitive_arrayclass_get_by_name(c->name);
	else
		ac = class_array_of(c, true);

	if (ac == NULL)
		return NULL;

	a = builtin_multianewarray(length, ac, dims);

	return (jobject) a;
}


/* JVM_InitializeSocketLibrary */

jint JVM_InitializeSocketLibrary()
{
	log_println("JVM_InitializeSocketLibrary: IMPLEMENT ME!");
}


/* JVM_Socket */

jint JVM_Socket(jint domain, jint type, jint protocol)
{
	TRACEJVMCALLS("JVM_Socket(domain=%d, type=%d, protocol=%d)", domain, type, protocol);

	return socket(domain, type, protocol);
}


/* JVM_SocketClose */

jint JVM_SocketClose(jint fd)
{
	TRACEJVMCALLS("JVM_SocketClose(fd=%d)", fd);

	return close(fd);
}


/* JVM_SocketShutdown */

jint JVM_SocketShutdown(jint fd, jint howto)
{
	TRACEJVMCALLS("JVM_SocketShutdown(fd=%d, howto=%d)", fd, howto);

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
	TRACEJVMCALLS("JVM_Listen(fd=%d, count=%d)", fd, count);

	return listen(fd, count);
}


/* JVM_Connect */

jint JVM_Connect(jint fd, struct sockaddr *him, jint len)
{
	TRACEJVMCALLS("JVM_Connect(fd=%d, him=%p, len=%d)", fd, him, len);

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
	TRACEJVMCALLS("JVM_Accept(fd=%d, him=%p, len=%p)", fd, him, len);

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
	TRACEJVMCALLS("JVM_GetSockName(fd=%d, him=%p, len=%p)", fd, him, len);

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
	TRACEJVMCALLS("JVM_SetSockOpt(fd=%d, level=%d, optname=%d, optval=%s, optlen=%d)", fd, level, optname, optval, optlen);

	return setsockopt(fd, level, optname, optval, optlen);
}


/* JVM_GetHostName */

int JVM_GetHostName(char* name, int namelen)
{
	TRACEJVMCALLS("JVM_GetHostName(name=%s, namelen=%d)", name, namelen);

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

void *JVM_LoadLibrary(const char *name)
{
	utf *u;

	TRACEJVMCALLS("JVM_LoadLibrary(name=%s)", name);

	u = utf_new_char(name);

	return native_library_open(u);
}


/* JVM_UnloadLibrary */

void JVM_UnloadLibrary(void* handle)
{
	log_println("JVM_UnloadLibrary: IMPLEMENT ME!");
}


/* JVM_FindLibraryEntry */

void *JVM_FindLibraryEntry(void *handle, const char *name)
{
	lt_ptr symbol;

	TRACEJVMCALLS("JVM_FindLibraryEntry(handle=%p, name=%s)", handle, name);

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
	TRACEJVMCALLS("JVM_IsSupportedJNIVersion(version=%d)", version);

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


/* JVM_InternString */

jstring JVM_InternString(JNIEnv *env, jstring str)
{
	TRACEJVMCALLS("JVM_InternString(env=%p, str=%p)", env, str);

	return (jstring) javastring_intern((java_handle_t *) str);
}


/* JVM_RawMonitorCreate */

JNIEXPORT void* JNICALL JVM_RawMonitorCreate(void)
{
	java_object_t *o;

#if PRINTJVM
	log_println("JVM_RawMonitorCreate");
#endif

	o = NEW(java_object_t);

	lock_init_object_lock(o);

	return o;
}


/* JVM_RawMonitorDestroy */

JNIEXPORT void JNICALL JVM_RawMonitorDestroy(void *mon)
{
#if PRINTJVM
	log_println("JVM_RawMonitorDestroy: mon=%p", mon);
#endif
	FREE(mon, java_object_t);
}


/* JVM_RawMonitorEnter */

JNIEXPORT jint JNICALL JVM_RawMonitorEnter(void *mon)
{
#if PRINTJVM
	log_println("JVM_RawMonitorEnter: mon=%p", mon);
#endif
	(void) lock_monitor_enter((java_object_t *) mon);

	return 0;
}


/* JVM_RawMonitorExit */

JNIEXPORT void JNICALL JVM_RawMonitorExit(void *mon)
{
#if PRINTJVM
	log_println("JVM_RawMonitorExit: mon=%p", mon);
#endif
	(void) lock_monitor_exit((java_object_t *) mon);
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
	return (jobject) _Jv_java_lang_reflect_Method_invoke((java_lang_reflect_Method *) method, (java_lang_Object *) obj, (java_handle_objectarray_t *) args0);
}


/* JVM_NewInstanceFromConstructor */

jobject JVM_NewInstanceFromConstructor(JNIEnv *env, jobject c, jobjectArray args0)
{
#if PRINTJVM
	log_println("JVM_NewInstanceFromConstructor: c=%p, args0=%p", c, args0);
#endif
	return (jobject) _Jv_java_lang_reflect_Constructor_newInstance(env, (java_lang_reflect_Constructor *) c, (java_handle_objectarray_t *) args0);
}


/* JVM_SupportsCX8 */

jboolean JVM_SupportsCX8()
{
	TRACEJVMCALLS("JVM_SupportsCX8()");

	/* IMPLEMENT ME */

	return 0;
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
	java_handle_intarray_t *ia;

	TRACEJVMCALLS("JVM_GetThreadStateValues(env=%p, javaThreadState=%d)",
				  env, javaThreadState);

	/* If new thread states are added in future JDK and VM versions,
	   this should check if the JDK version is compatible with thread
	   states supported by the VM.  Return NULL if not compatible.
	
	   This function must map the VM java_lang_Thread::ThreadStatus
	   to the Java thread state that the JDK supports. */

	switch (javaThreadState) {
    case THREAD_STATE_NEW:
		ia = builtin_newarray_int(1);

		if (ia == NULL)
			return NULL;

		array_intarray_element_set(ia, 0, THREAD_STATE_NEW);
		break; 

    case THREAD_STATE_RUNNABLE:
		ia = builtin_newarray_int(1);

		if (ia == NULL)
			return NULL;

		array_intarray_element_set(ia, 0, THREAD_STATE_RUNNABLE);
		break; 

    case THREAD_STATE_BLOCKED:
		ia = builtin_newarray_int(1);

		if (ia == NULL)
			return NULL;

		array_intarray_element_set(ia, 0, THREAD_STATE_BLOCKED);
		break; 

    case THREAD_STATE_WAITING:
		ia = builtin_newarray_int(2);

		if (ia == NULL)
			return NULL;

		array_intarray_element_set(ia, 0, THREAD_STATE_WAITING);
		/* XXX Implement parked stuff. */
/* 		array_intarray_element_set(ia, 1, PARKED); */
		break; 

    case THREAD_STATE_TIMED_WAITING:
		ia = builtin_newarray_int(3);

		if (ia == NULL)
			return NULL;

		/* XXX Not sure about that one. */
/* 		array_intarray_element_set(ia, 0, SLEEPING); */
		array_intarray_element_set(ia, 0, THREAD_STATE_TIMED_WAITING);
		/* XXX Implement parked stuff. */
/* 		array_intarray_element_set(ia, 2, PARKED); */
		break; 

    case THREAD_STATE_TERMINATED:
		ia = builtin_newarray_int(1);

		if (ia == NULL)
			return NULL;

		array_intarray_element_set(ia, 0, THREAD_STATE_TERMINATED);
		break; 

    default:
		/* Unknown state - probably incompatible JDK version */
		return NULL;
	}

	return (jintArray) ia;
}


/* JVM_GetThreadStateNames */

jobjectArray JVM_GetThreadStateNames(JNIEnv* env, jint javaThreadState, jintArray values)
{
	java_handle_intarray_t    *ia;
	java_handle_objectarray_t *oa;
	java_object_t             *s;

	TRACEJVMCALLS("JVM_GetThreadStateNames(env=%p, javaThreadState=%d, values=%p)",
				  env, javaThreadState, values);

	ia = (java_handle_intarray_t *) values;

	/* If new thread states are added in future JDK and VM versions,
	   this should check if the JDK version is compatible with thread
	   states supported by the VM.  Return NULL if not compatible.
	
	   This function must map the VM java_lang_Thread::ThreadStatus
	   to the Java thread state that the JDK supports. */

	if (values == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	switch (javaThreadState) {
    case THREAD_STATE_NEW:
		assert(ia->header.size == 1 && ia->data[0] == THREAD_STATE_NEW);

		oa = builtin_anewarray(1, class_java_lang_String);

		if (oa == NULL)
			return NULL;

		s = javastring_new(utf_new_char("NEW"));

		if (s == NULL)
			return NULL;

		array_objectarray_element_set(oa, 0, s);
		break; 

    case THREAD_STATE_RUNNABLE:
		oa = builtin_anewarray(1, class_java_lang_String);

		if (oa == NULL)
			return NULL;

		s = javastring_new(utf_new_char("RUNNABLE"));

		if (s == NULL)
			return NULL;

		array_objectarray_element_set(oa, 0, s);
		break; 

    case THREAD_STATE_BLOCKED:
		oa = builtin_anewarray(1, class_java_lang_String);

		if (oa == NULL)
			return NULL;

		s = javastring_new(utf_new_char("BLOCKED"));

		if (s == NULL)
			return NULL;

		array_objectarray_element_set(oa, 0, s);
		break; 

    case THREAD_STATE_WAITING:
		oa = builtin_anewarray(2, class_java_lang_String);

		if (oa == NULL)
			return NULL;

		s = javastring_new(utf_new_char("WAITING.OBJECT_WAIT"));
/* 		s = javastring_new(utf_new_char("WAITING.PARKED")); */

		if (s == NULL)
			return NULL;

		array_objectarray_element_set(oa, 0, s);
/* 		array_objectarray_element_set(oa, 1, s); */
		break; 

    case THREAD_STATE_TIMED_WAITING:
		oa = builtin_anewarray(3, class_java_lang_String);

		if (oa == NULL)
			return NULL;

/* 		s = javastring_new(utf_new_char("TIMED_WAITING.SLEEPING")); */
		s = javastring_new(utf_new_char("TIMED_WAITING.OBJECT_WAIT"));
/* 		s = javastring_new(utf_new_char("TIMED_WAITING.PARKED")); */

		if (s == NULL)
			return NULL;

/* 		array_objectarray_element_set(oa, 0, s); */
		array_objectarray_element_set(oa, 0, s);
/* 		array_objectarray_element_set(oa, 2, s); */
		break; 

    case THREAD_STATE_TERMINATED:
		oa = builtin_anewarray(1, class_java_lang_String);

		if (oa == NULL)
			return NULL;

		s = javastring_new(utf_new_char("TERMINATED"));

		if (s == NULL)
			return NULL;

		array_objectarray_element_set(oa, 0, s);
		break; 

	default:
		/* Unknown state - probably incompatible JDK version */
		return NULL;
	}

	return (jobjectArray) oa;
}


/* JVM_GetVersionInfo */

void JVM_GetVersionInfo(JNIEnv* env, jvm_version_info* info, size_t info_size)
{
	log_println("JVM_GetVersionInfo: IMPLEMENT ME!");
}


/* OS: JVM_RegisterSignal */

void *JVM_RegisterSignal(jint sig, void *handler)
{
	functionptr newHandler;

	TRACEJVMCALLS("JVM_RegisterSignal(sig=%d, handler=%p)", sig, handler);

	if (handler == (void *) 2)
		newHandler = (functionptr) signal_thread_handler;
	else
		newHandler = (functionptr) (uintptr_t) handler;

	switch (sig) {
    case SIGILL:
    case SIGFPE:
    case SIGUSR1:
    case SIGSEGV:
		/* These signals are already used by the VM. */
		return (void *) -1;

    case SIGQUIT:
		/* This signal is used by the VM to dump thread stacks unless
		   ReduceSignalUsage is set, in which case the user is allowed
		   to set his own _native_ handler for this signal; thus, in
		   either case, we do not allow JVM_RegisterSignal to change
		   the handler. */
		return (void *) -1;

	case SIGHUP:
	case SIGINT:
	case SIGTERM:
		break;
	}

	signal_register_signal(sig, newHandler, 0);

	/* XXX Should return old handler. */

	return (void *) 2;
}


/* OS: JVM_RaiseSignal */

jboolean JVM_RaiseSignal(jint sig)
{
	log_println("JVM_RaiseSignal: IMPLEMENT ME! sig=%s", sig);
	return false;
}


/* OS: JVM_FindSignal */

jint JVM_FindSignal(const char *name)
{
	TRACEJVMCALLS("JVM_FindSignal(name=%s)", name);

#if defined(__LINUX__)
	if (strcmp(name, "HUP") == 0)
		return SIGHUP;

	if (strcmp(name, "INT") == 0)
		return SIGINT;

	if (strcmp(name, "TERM") == 0)
		return SIGTERM;
#else
# error not implemented for this OS
#endif

	return -1;
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
