/* src/vmcore/javaobjects.hpp - functions to create and access Java objects

   Copyright (C) 2008
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


#ifndef _JAVAOBJECTS_HPP
#define _JAVAOBJECTS_HPP

#include "config.h"

#include <stdint.h>

#include "native/llni.h"

#include "vm/global.h"

#include "vmcore/field.h"
#include "vmcore/method.h"


#ifdef __cplusplus

/**
 * This class provides low-level functions to access Java object
 * instance fields.
 *
 * These functions do NOT take care about the GC critical section!
 * Please use FieldAccess wherever possible.
 */
class RawFieldAccess {
protected:
	template<class T> static inline T    raw_get(void* address, const off_t offset);
	template<class T> static inline void raw_set(void* address, const off_t offset, T value);
};


template<class T> inline T RawFieldAccess::raw_get(void* address, const off_t offset)
{
	T* p = (T*) (((uintptr_t) address) + offset);
	return *p;
}


template<class T> inline void RawFieldAccess::raw_set(void* address, const off_t offset, T value)
{
	T* p = (T*) (((uintptr_t) address) + offset);
	*p = value;
}


/**
 * This classes provides functions to access Java object instance
 * fields.  These functions enter a critical GC section before
 * accessing the Java object throught the handle and leave it
 * afterwards.
 */
class FieldAccess : private RawFieldAccess {
protected:
	template<class T> static inline T    get(java_handle_t* h, const off_t offset);
	template<class T> static inline void set(java_handle_t* h, const off_t offset, T value);
};

template<class T> inline T FieldAccess::get(java_handle_t* h, const off_t offset)
{
	java_object_t* o;
	T result;
		
	// XXX Move this to a GC inline function, e.g.
	// gc->enter_critical();
	LLNI_CRITICAL_START;

	// XXX This should be _handle->get_object();
	o = LLNI_UNWRAP(h);

	result = raw_get<T>(o, offset);

	// XXX Move this to a GC inline function.
	// gc->leave_critical();
	LLNI_CRITICAL_END;

	return result;
}	

template<> inline java_handle_t* FieldAccess::get(java_handle_t* h, const off_t offset)
{
	java_object_t* o;
	java_object_t* result;
	java_handle_t* hresult;
		
	// XXX Move this to a GC inline function, e.g.
	// gc->enter_critical();
	LLNI_CRITICAL_START;

	// XXX This should be _handle->get_object();
	o = LLNI_UNWRAP(h);

	result = raw_get<java_object_t*>(o, offset);

	hresult = LLNI_WRAP(result);

	// XXX Move this to a GC inline function.
	// gc->leave_critical();
	LLNI_CRITICAL_END;

	return result;
}	


template<class T> inline void FieldAccess::set(java_handle_t* h, const off_t offset, T value)
{
	java_object_t* o;

	// XXX Move this to a GC inline function, e.g.
	// gc->enter_critical();
	LLNI_CRITICAL_START;

	// XXX This should be h->get_object();
	o = LLNI_UNWRAP(h);

	raw_set(o, offset, value);

	// XXX Move this to a GC inline function.
	// gc->leave_critical();
	LLNI_CRITICAL_END;
}

template<> inline void FieldAccess::set<java_handle_t*>(java_handle_t* h, const off_t offset, java_handle_t* value)
{
	java_object_t* o;
	java_object_t* ovalue;

	// XXX Move this to a GC inline function, e.g.
	// gc->enter_critical();
	LLNI_CRITICAL_START;

	// XXX This should be h->get_object();
	o      = LLNI_UNWRAP(h);
	ovalue = LLNI_UNWRAP(value);

	raw_set(o, offset, ovalue);

	// XXX Move this to a GC inline function.
	// gc->leave_critical();
	LLNI_CRITICAL_END;
}


/**
 * java/lang/Object
 *
 * Object layout:
 *
 * 0. object header
 */
class java_lang_Object {
protected:
	// Handle of Java object.
	java_handle_t* _handle;

protected:
	java_lang_Object(java_handle_t* h) : _handle(h) {}
	java_lang_Object(jobject h) : _handle((java_handle_t*) h) {}

public:
	// Getters.
	virtual inline java_handle_t* get_handle() const { return _handle; }
	inline vftbl_t*               get_vftbl () const;
	inline classinfo*             get_Class () const;

	inline bool is_null() const;
};


inline vftbl_t* java_lang_Object::get_vftbl() const
{
	// XXX Move this to a GC inline function, e.g.
	// gc->enter_critical();
	LLNI_CRITICAL_START;

	// XXX This should be h->get_object();
	java_object_t* o = LLNI_UNWRAP(_handle);
	vftbl_t* vftbl = o->vftbl;

	// XXX Move this to a GC inline function.
	// gc->leave_critical();
	LLNI_CRITICAL_END;

	return vftbl;
}

inline classinfo* java_lang_Object::get_Class() const
{
	return get_vftbl()->clazz;
}


inline bool java_lang_Object::is_null() const
{
	return (_handle == NULL);
}


/**
 * java/lang/Boolean
 *
 * Object layout:
 *
 * 0. object header
 * 1. boolean value;
 */
class java_lang_Boolean : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_value = sizeof(java_object_t);

public:
	java_lang_Boolean(java_handle_t* h) : java_lang_Object(h) {}

	inline uint8_t get_value();
	inline void    set_value(uint8_t value);
};

inline uint8_t java_lang_Boolean::get_value()
{
	return get<int32_t>(_handle, offset_value);
}

inline void java_lang_Boolean::set_value(uint8_t value)
{
	set(_handle, offset_value, (uint32_t) value);
}


/**
 * java/lang/Byte
 *
 * Object layout:
 *
 * 0. object header
 * 1. byte value;
 */
class java_lang_Byte : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_value = sizeof(java_object_t);

public:
	java_lang_Byte(java_handle_t* h) : java_lang_Object(h) {}

	inline int8_t get_value();
	inline void   set_value(int8_t value);
};

inline int8_t java_lang_Byte::get_value()
{
	return get<int32_t>(_handle, offset_value);
}

inline void java_lang_Byte::set_value(int8_t value)
{
	set(_handle, offset_value, (int32_t) value);
}


/**
 * java/lang/Character
 *
 * Object layout:
 *
 * 0. object header
 * 1. char value;
 */
class java_lang_Character : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_value = sizeof(java_object_t);

public:
	java_lang_Character(java_handle_t* h) : java_lang_Object(h) {}

	inline uint16_t get_value();
	inline void     set_value(uint16_t value);
};

inline uint16_t java_lang_Character::get_value()
{
	return get<int32_t>(_handle, offset_value);
}

inline void java_lang_Character::set_value(uint16_t value)
{
	set(_handle, offset_value, (uint32_t) value);
}


/**
 * java/lang/Short
 *
 * Object layout:
 *
 * 0. object header
 * 1. short value;
 */
class java_lang_Short : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_value = sizeof(java_object_t);

public:
	java_lang_Short(java_handle_t* h) : java_lang_Object(h) {}

	inline int16_t get_value();
	inline void    set_value(int16_t value);
};

inline int16_t java_lang_Short::get_value()
{
	return get<int32_t>(_handle, offset_value);
}

inline void java_lang_Short::set_value(int16_t value)
{
	set(_handle, offset_value, (int32_t) value);
}


/**
 * java/lang/Integer
 *
 * Object layout:
 *
 * 0. object header
 * 1. int value;
 */
class java_lang_Integer : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_value = sizeof(java_object_t);

public:
	java_lang_Integer(java_handle_t* h) : java_lang_Object(h) {}

	inline int32_t get_value();
	inline void    set_value(int32_t value);
};

inline int32_t java_lang_Integer::get_value()
{
	return get<int32_t>(_handle, offset_value);
}

inline void java_lang_Integer::set_value(int32_t value)
{
	set(_handle, offset_value, value);
}


/**
 * java/lang/Long
 *
 * Object layout:
 *
 * 0. object header
 * 1. long value;
 */
class java_lang_Long : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_value = sizeof(java_object_t);

public:
	java_lang_Long(java_handle_t* h) : java_lang_Object(h) {}

	inline int64_t get_value();
	inline void    set_value(int64_t value);
};

inline int64_t java_lang_Long::get_value()
{
	return get<int64_t>(_handle, offset_value);
}

inline void java_lang_Long::set_value(int64_t value)
{
	set(_handle, offset_value, value);
}


/**
 * java/lang/Float
 *
 * Object layout:
 *
 * 0. object header
 * 1. float value;
 */
class java_lang_Float : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_value = sizeof(java_object_t);

public:
	java_lang_Float(java_handle_t* h) : java_lang_Object(h) {}

	inline float get_value();
	inline void  set_value(float value);
};

inline float java_lang_Float::get_value()
{
	return get<float>(_handle, offset_value);
}

inline void java_lang_Float::set_value(float value)
{
	set(_handle, offset_value, value);
}


/**
 * java/lang/Double
 *
 * Object layout:
 *
 * 0. object header
 * 1. double value;
 */
class java_lang_Double : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_value = sizeof(java_object_t);

public:
	java_lang_Double(java_handle_t* h) : java_lang_Object(h) {}

	inline double get_value();
	inline void   set_value(double value);
};

inline double java_lang_Double::get_value()
{
	return get<double>(_handle, offset_value);
}

inline void java_lang_Double::set_value(double value)
{
	set(_handle, offset_value, value);
}


#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)

/**
 * GNU Classpath java/lang/Class
 *
 * Object layout:
 *
 * 0. object header
 * 1. java.lang.Object[]             signers;
 * 2. java.security.ProtectionDomain pd;
 * 3. java.lang.Object               vmdata;
 * 4. java.lang.reflect.Constructor  constructor;
 */
class java_lang_Class : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_signers     = sizeof(java_object_t);
	static const off_t offset_pd          = offset_signers + SIZEOF_VOID_P;
	static const off_t offset_vmdata      = offset_pd      + SIZEOF_VOID_P;
	static const off_t offset_constructor = offset_vmdata  + SIZEOF_VOID_P;

public:
	java_lang_Class(java_handle_t* h) : java_lang_Object(h) {}

	// Setters.
	inline void set_pd(java_handle_t* value);
	inline void set_pd(jobject value);
};

inline void java_lang_Class::set_pd(java_handle_t* value)
{
	set(_handle, offset_pd, value);
}

inline void java_lang_Class::set_pd(jobject value)
{
	set_pd((java_handle_t*) value);
}


/**
 * GNU Classpath java/lang/StackTraceElement
 *
 * Object layout:
 *
 * 0. object header
 * 1. java.lang.String fileName;
 * 2. int              lineNumber;
 * 3. java.lang.String declaringClass;
 * 4. java.lang.String methodName;
 * 5. boolean          isNative;
 */
class java_lang_StackTraceElement : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_fileName       = sizeof(java_object_t);
	static const off_t offset_lineNumber     = offset_fileName       + SIZEOF_VOID_P;
	static const off_t offset_declaringClass = offset_lineNumber     + sizeof(int32_t) + 4;
	static const off_t offset_methodName     = offset_declaringClass + SIZEOF_VOID_P;
	static const off_t offset_isNative       = offset_methodName     + SIZEOF_VOID_P;

public:
	java_lang_StackTraceElement(java_handle_t* h) : java_lang_Object(h) {}
	java_lang_StackTraceElement(java_handle_t* h, java_handle_t* fileName, int32_t lineNumber, java_handle_t* declaringClass, java_handle_t* methodName, uint8_t isNative);
};

inline java_lang_StackTraceElement::java_lang_StackTraceElement(java_handle_t* h, java_handle_t* fileName, int32_t lineNumber, java_handle_t* declaringClass, java_handle_t* methodName, uint8_t isNative) : java_lang_Object(h)
{
	java_lang_StackTraceElement((java_handle_t*) h);

	set(_handle, offset_fileName,       fileName);
	set(_handle, offset_lineNumber,     lineNumber);
	set(_handle, offset_declaringClass, declaringClass);
	set(_handle, offset_methodName,     methodName);
	set(_handle, offset_isNative,       isNative);
}


/**
 * GNU Classpath java/lang/String
 *
 * Object layout:
 *
 * 0. object header
 * 1. char[] value;
 * 2. int    count;
 * 3. int    cachedHashCode;
 * 4. int    offset;
 */
class java_lang_String : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_value          = sizeof(java_object_t);
	static const off_t offset_count          = offset_value          + SIZEOF_VOID_P;
	static const off_t offset_cachedHashCode = offset_count          + sizeof(int32_t);
	static const off_t offset_offset         = offset_cachedHashCode + sizeof(int32_t);

public:
	java_lang_String(java_handle_t* h) : java_lang_Object(h) {}
	java_lang_String(jstring h);
	java_lang_String(java_handle_t* h, java_handle_chararray_t* value, int32_t count, int32_t offset = 0);

	// Getters.
	inline java_handle_chararray_t* get_value () const;
	inline int32_t                  get_count () const;
	inline int32_t                  get_offset() const;

	// Setters.
	inline void set_value (java_handle_chararray_t* value);
	inline void set_count (int32_t value);
	inline void set_offset(int32_t value);
};

inline java_lang_String::java_lang_String(jstring h) : java_lang_Object(h)
{
	java_lang_String((java_handle_t*) h);
}

inline java_lang_String::java_lang_String(java_handle_t* h, java_handle_chararray_t* value, int32_t count, int32_t offset) : java_lang_Object(h)
{
	set_value(value);
	set_count(count);
	set_offset(offset);
}

inline java_handle_chararray_t* java_lang_String::get_value() const
{
	return get<java_handle_chararray_t*>(_handle, offset_value);
}

inline int32_t java_lang_String::get_count() const
{
	return get<int32_t>(_handle, offset_count);
}

inline int32_t java_lang_String::get_offset() const
{
	return get<int32_t>(_handle, offset_offset);
}

inline void java_lang_String::set_value(java_handle_chararray_t* value)
{
	set(_handle, offset_value, value);
}

inline void java_lang_String::set_count(int32_t value)
{
	set(_handle, offset_count, value);
}

inline void java_lang_String::set_offset(int32_t value)
{
	set(_handle, offset_offset, value);
}


/**
 * GNU Classpath java/lang/Thread
 *
 * Object layout:
 *
 *  0. object header
 *  1. java.lang.VMThread                        vmThread;
 *  2. java.lang.ThreadGroup                     group;
 *  3. java.lang.Runnable                        runnable;
 *  4. java.lang.String                          name;
 *  5. boolean                                   daemon;
 *  6. int                                       priority;
 *  7. long                                      stacksize;
 *  8. java.lang.Throwable                       stillborn;
 *  9. java.lang.ClassLoader                     contextClassLoader;
 * 10. boolean                                   contextClassLoaderIsSystemClassLoader;
 * 11. long                                      threadId;
 * 12. java.lang.Object                          parkBlocker;
 * 13. gnu.java.util.WeakIdentityHashMap         locals;
 * 14. java_lang_Thread_UncaughtExceptionHandler exceptionHandler;
 */
class java_lang_Thread : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_vmThread                              = sizeof(java_object_t);
	static const off_t offset_group                                 = offset_vmThread                              + SIZEOF_VOID_P;
	static const off_t offset_runnable                              = offset_group                                 + SIZEOF_VOID_P;
	static const off_t offset_name                                  = offset_runnable                              + SIZEOF_VOID_P;
	static const off_t offset_daemon                                = offset_name                                  + SIZEOF_VOID_P;
	static const off_t offset_priority                              = offset_daemon                                + sizeof(int32_t); // FIXME
	static const off_t offset_stacksize                             = offset_priority                              + sizeof(int32_t); // FIXME
	static const off_t offset_stillborn                             = offset_stacksize                             + sizeof(int64_t); // FIXME
	static const off_t offset_contextClassLoader                    = offset_stillborn                             + SIZEOF_VOID_P;
	static const off_t offset_contextClassLoaderIsSystemClassLoader = offset_contextClassLoader                    + SIZEOF_VOID_P;
	static const off_t offset_threadId                              = offset_contextClassLoaderIsSystemClassLoader + sizeof(int32_t); // FIXME
	static const off_t offset_parkBlocker                           = offset_threadId                              + sizeof(int64_t); // FIXME
	static const off_t offset_locals                                = offset_parkBlocker                           + SIZEOF_VOID_P;
	static const off_t offset_exceptionHandler                      = offset_locals                                + SIZEOF_VOID_P;

public:
	java_lang_Thread(java_handle_t* h) : java_lang_Object(h) {}
// 	java_lang_Thread(threadobject* t);

	// Getters.
	inline java_handle_t* get_vmThread        () const;
	inline java_handle_t* get_group           () const;
	inline java_handle_t* get_name            () const;
	inline int32_t        get_daemon          () const;
	inline int32_t        get_priority        () const;
	inline java_handle_t* get_exceptionHandler() const;

	// Setters.
	inline void set_group(java_handle_t* value);
};


// inline java_lang_Thread::java_lang_Thread(threadobject* t) : java_lang_Object(h)
// {
// 	java_lang_Thread(thread_get_object(t));
// }


inline java_handle_t* java_lang_Thread::get_vmThread() const
{
	return get<java_handle_t*>(_handle, offset_vmThread);
}

inline java_handle_t* java_lang_Thread::get_group() const
{
	return get<java_handle_t*>(_handle, offset_group);
}

inline java_handle_t* java_lang_Thread::get_name() const
{
	return get<java_handle_t*>(_handle, offset_name);
}

inline int32_t java_lang_Thread::get_daemon() const
{
	return get<int32_t>(_handle, offset_daemon);
}

inline int32_t java_lang_Thread::get_priority() const
{
	return get<int32_t>(_handle, offset_priority);
}

inline java_handle_t* java_lang_Thread::get_exceptionHandler() const
{
	return get<java_handle_t*>(_handle, offset_exceptionHandler);
}


inline void java_lang_Thread::set_group(java_handle_t* value)
{
	set(_handle, offset_group, value);
}


/**
 * GNU Classpath java/lang/VMThread
 *
 * Object layout:
 *
 * 0. object header
 * 1. java.lang.Thread   thread;
 * 2. boolean            running;
 * 3. java.lang.VMThread vmdata;
 */
class java_lang_VMThread : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_thread  = sizeof(java_object_t);
	static const off_t offset_running = offset_thread  + SIZEOF_VOID_P;
	static const off_t offset_vmdata  = offset_running + sizeof(int32_t); // FIXME

public:
	java_lang_VMThread(java_handle_t* h) : java_lang_Object(h) {}
	java_lang_VMThread(jobject h);
	java_lang_VMThread(java_handle_t* h, java_handle_t* thread, threadobject* vmdata);

	// Getters.
	inline java_handle_t* get_thread() const;
	inline threadobject*  get_vmdata() const;

	// Setters.
	inline void set_thread(java_handle_t* value);
	inline void set_vmdata(threadobject* value);
};


inline java_lang_VMThread::java_lang_VMThread(jobject h) : java_lang_Object(h)
{
	java_lang_VMThread((java_handle_t*) h);
}

inline java_lang_VMThread::java_lang_VMThread(java_handle_t* h, java_handle_t* thread, threadobject* vmdata) : java_lang_Object(h)
{
	set_thread(thread);
	set_vmdata(vmdata);
}


inline java_handle_t* java_lang_VMThread::get_thread() const
{
	return get<java_handle_t*>(_handle, offset_thread);
}

inline threadobject* java_lang_VMThread::get_vmdata() const
{
	return get<threadobject*>(_handle, offset_vmdata);
}


inline void java_lang_VMThread::set_thread(java_handle_t* value)
{
	set(_handle, offset_thread, value);
}

inline void java_lang_VMThread::set_vmdata(threadobject* value)
{
	set(_handle, offset_vmdata, value);
}


/**
 * GNU Classpath java/lang/Throwable
 *
 * Object layout:
 *
 * 0. object header
 * 1. java.lang.String              detailMessage;
 * 2. java.lang.Throwable           cause;
 * 3. java.lang.StackTraceElement[] stackTrace;
 * 4. java.lang.VMThrowable         vmState;
 */
class java_lang_Throwable : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_detailMessage = sizeof(java_object_t);
	static const off_t offset_cause         = offset_detailMessage + SIZEOF_VOID_P;
	static const off_t offset_stackTrace    = offset_cause         + SIZEOF_VOID_P;
	static const off_t offset_vmState       = offset_stackTrace    + SIZEOF_VOID_P;

public:
	java_lang_Throwable(java_handle_t* h) : java_lang_Object(h) {}

	// Getters.
	inline java_handle_t* get_detailMessage() const;
	inline java_handle_t* get_cause        () const;
	inline java_handle_t* get_vmState      () const;
};


inline java_handle_t* java_lang_Throwable::get_detailMessage() const
{
	return get<java_handle_t*>(_handle, offset_detailMessage);
}

inline java_handle_t* java_lang_Throwable::get_cause() const
{
	return get<java_handle_t*>(_handle, offset_cause);
}

inline java_handle_t* java_lang_Throwable::get_vmState() const
{
	return get<java_handle_t*>(_handle, offset_vmState);
}


/**
 * GNU Classpath java/lang/VMThrowable
 *
 * Object layout:
 *
 * 0. object header
 * 1. java.lang.Object vmdata;
 */
class java_lang_VMThrowable : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_vmdata = sizeof(java_object_t);

public:
	java_lang_VMThrowable(java_handle_t* h) : java_lang_Object(h) {}
	java_lang_VMThrowable(jobject h);

	inline java_handle_bytearray_t* get_vmdata() const;
	inline void                     set_vmdata(java_handle_bytearray_t* value);
};

inline java_lang_VMThrowable::java_lang_VMThrowable(jobject h) : java_lang_Object(h)
{
	java_lang_VMThrowable((java_handle_t*) h);
}

inline java_handle_bytearray_t* java_lang_VMThrowable::get_vmdata() const
{
	return get<java_handle_bytearray_t*>(_handle, offset_vmdata);
}

inline void java_lang_VMThrowable::set_vmdata(java_handle_bytearray_t* value)
{
	set(_handle, offset_vmdata, value);
}


/**
 * GNU Classpath java/lang/reflect/Constructor
 *
 * Object layout:
 *
 * 0. object header
 * 1. boolean                                     flag;
 * 2. gnu.java.lang.reflect.MethodSignatureParser p;
 * 3. java.lang.reflect.VMConstructor             cons;
 */
class java_lang_reflect_Constructor : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_flag = sizeof(java_object_t);
	static const off_t offset_p    = offset_flag + sizeof(int32_t) + 4;
	static const off_t offset_cons = offset_p    + SIZEOF_VOID_P;

public:
	java_lang_reflect_Constructor(java_handle_t* h) : java_lang_Object(h) {}
	java_lang_reflect_Constructor(jobject h);

	static java_handle_t* create(methodinfo* m);
	static java_handle_t* new_instance(methodinfo* m, java_handle_objectarray_t* args, bool override);

	// Getters.
	inline int32_t        get_flag() const;
	inline java_handle_t* get_cons() const;

	// Setters.
	inline void set_cons(java_handle_t* value);
};

inline java_lang_reflect_Constructor::java_lang_reflect_Constructor(jobject h) : java_lang_Object(h)
{
	java_lang_reflect_Constructor((java_handle_t*) h);
}

inline int32_t java_lang_reflect_Constructor::get_flag() const
{
	return get<int32_t>(_handle, offset_flag);
}

inline java_handle_t* java_lang_reflect_Constructor::get_cons() const
{
	return get<java_handle_t*>(_handle, offset_cons);
}

inline void java_lang_reflect_Constructor::set_cons(java_handle_t* value)
{
	set(_handle, offset_cons, value);
}


/**
 * GNU Classpath java/lang/reflect/VMConstructor
 *
 * Object layout:
 *
 * 0. object header
 * 1. java.lang.Class               clazz;
 * 2. int                           slot;
 * 3. byte[]                        annotations;
 * 4. byte[]                        parameterAnnotations;
 * 5. java.util.Map                 declaredAnnotations;
 * 6. java.lang.reflect.Constructor cons;
 */
class java_lang_reflect_VMConstructor : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_clazz                = sizeof(java_object_t);
	static const off_t offset_slot                 = offset_clazz                + SIZEOF_VOID_P;
	static const off_t offset_annotations          = offset_slot                 + sizeof(int32_t) + 4;
	static const off_t offset_parameterAnnotations = offset_annotations          + SIZEOF_VOID_P;
	static const off_t offset_declaredAnnotations  = offset_parameterAnnotations + SIZEOF_VOID_P;
	static const off_t offset_cons                 = offset_declaredAnnotations  + SIZEOF_VOID_P;

public:
	java_lang_reflect_VMConstructor(java_handle_t* h) : java_lang_Object(h) {}
	java_lang_reflect_VMConstructor(jobject h);
	java_lang_reflect_VMConstructor(java_handle_t* h, methodinfo* m);

	// Getters.
	inline classinfo*               get_clazz               () const;
	inline int32_t                  get_slot                () const;
	inline java_handle_bytearray_t* get_annotations         () const;
	inline java_handle_bytearray_t* get_parameterAnnotations() const;
	inline java_handle_t*           get_declaredAnnotations () const;
	inline java_handle_t*           get_cons                () const;

	// Setters.
	inline void set_clazz               (classinfo* value);
	inline void set_slot                (int32_t value);
	inline void set_annotations         (java_handle_bytearray_t* value);
	inline void set_parameterAnnotations(java_handle_bytearray_t* value);
	inline void set_declaredAnnotations (java_handle_t* value);
	inline void set_cons                (java_handle_t* value);

	// Convenience functions.
	inline methodinfo* get_method();
};

inline java_lang_reflect_VMConstructor::java_lang_reflect_VMConstructor(jobject h) : java_lang_Object(h)
{
	java_lang_reflect_VMConstructor((java_handle_t*) h);
}

inline java_lang_reflect_VMConstructor::java_lang_reflect_VMConstructor(java_handle_t* h, methodinfo* m) : java_lang_Object(h)
{
	int                      slot                 = m - m->clazz->methods;
	java_handle_bytearray_t* annotations          = method_get_annotations(m);
	java_handle_bytearray_t* parameterAnnotations = method_get_parameterannotations(m);

	set_clazz(m->clazz);
	set_slot(slot);
	set_annotations(annotations);
	set_parameterAnnotations(parameterAnnotations);
}

inline classinfo* java_lang_reflect_VMConstructor::get_clazz() const
{
	return get<classinfo*>(_handle, offset_clazz);
}

inline int32_t java_lang_reflect_VMConstructor::get_slot() const
{
	return get<int32_t>(_handle, offset_slot);
}

inline java_handle_bytearray_t* java_lang_reflect_VMConstructor::get_annotations() const
{
	return get<java_handle_bytearray_t*>(_handle, offset_annotations);
}

inline java_handle_bytearray_t* java_lang_reflect_VMConstructor::get_parameterAnnotations() const
{
	return get<java_handle_bytearray_t*>(_handle, offset_parameterAnnotations);
}

inline java_handle_t* java_lang_reflect_VMConstructor::get_declaredAnnotations() const
{
	return get<java_handle_t*>(_handle, offset_declaredAnnotations);
}

inline java_handle_t* java_lang_reflect_VMConstructor::get_cons() const
{
	return get<java_handle_t*>(_handle, offset_cons);
}

inline void java_lang_reflect_VMConstructor::set_clazz(classinfo* value)
{
	set(_handle, offset_clazz, value);
}

inline void java_lang_reflect_VMConstructor::set_slot(int32_t value)
{
	set(_handle, offset_slot, value);
}

inline void java_lang_reflect_VMConstructor::set_annotations(java_handle_bytearray_t* value)
{
	set(_handle, offset_annotations, value);
}

inline void java_lang_reflect_VMConstructor::set_parameterAnnotations(java_handle_bytearray_t* value)
{
	set(_handle, offset_parameterAnnotations, value);
}

inline void java_lang_reflect_VMConstructor::set_declaredAnnotations(java_handle_t* value)
{
	set(_handle, offset_declaredAnnotations, value);
}

inline void java_lang_reflect_VMConstructor::set_cons(java_handle_t* value)
{
	set(_handle, offset_cons, value);
}

inline methodinfo* java_lang_reflect_VMConstructor::get_method()
{
	classinfo*  c    = get_clazz();
	int32_t     slot = get_slot();
	methodinfo* m    = &(c->methods[slot]);
	return m;
}


/**
 * GNU Classpath java/lang/reflect/Field
 *
 * Object layout:
 *
 * 0. object header
 * 1. boolean                                    flag;
 * 2. gnu.java.lang.reflect.FieldSignatureParser p;
 * 3. java.lang.reflect.VMField                  f;
 */
class java_lang_reflect_Field : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_flag = sizeof(java_object_t);
	// Currently we align 64-bit data types to 8-bytes.
	static const off_t offset_p    = offset_flag + sizeof(int32_t) + 4;
	static const off_t offset_f    = offset_p    + SIZEOF_VOID_P;

public:
	java_lang_reflect_Field(java_handle_t* h) : java_lang_Object(h) {}
	java_lang_reflect_Field(jobject h);

	static java_handle_t* create(fieldinfo* f);

	// Getters.
	inline int32_t        get_flag() const;
	inline java_handle_t* get_f() const;

	// Setters.
	inline void set_f(java_handle_t* value);
};

inline java_lang_reflect_Field::java_lang_reflect_Field(jobject h) : java_lang_Object(h)
{
	java_lang_reflect_Field((java_handle_t*) h);
}

inline int32_t java_lang_reflect_Field::get_flag() const
{
	return get<int32_t>(_handle, offset_flag);
}

inline java_handle_t* java_lang_reflect_Field::get_f() const
{
	return get<java_handle_t*>(_handle, offset_f);
}

inline void java_lang_reflect_Field::set_f(java_handle_t* value)
{
	set(_handle, offset_f, value);
}


/**
 * GNU Classpath java/lang/reflect/VMField
 *
 * Object layout:
 *
 * 0. object header
 * 1. java.lang.Class         clazz;
 * 2. java.lang.String        name;
 * 3. int                     slot;
 * 4. byte[]                  annotations;
 * 5. java.lang.Map           declaredAnnotations;
 * 6. java.lang.reflect.Field f;
 */
class java_lang_reflect_VMField : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_clazz               = sizeof(java_object_t);
	static const off_t offset_name                = offset_clazz               + SIZEOF_VOID_P;
	static const off_t offset_slot                = offset_name                + SIZEOF_VOID_P;
	static const off_t offset_annotations         = offset_slot                + sizeof(int32_t) + 4;
	static const off_t offset_declaredAnnotations = offset_annotations         + SIZEOF_VOID_P;
	static const off_t offset_f                   = offset_declaredAnnotations + SIZEOF_VOID_P;

public:
	java_lang_reflect_VMField(java_handle_t* h) : java_lang_Object(h) {}
	java_lang_reflect_VMField(jobject h);
	java_lang_reflect_VMField(java_handle_t* h, fieldinfo* f);

	// Getters.
	inline classinfo*               get_clazz              () const;
	inline int32_t                  get_slot               () const;
	inline java_handle_bytearray_t* get_annotations        () const;
	inline java_handle_t*           get_declaredAnnotations() const;
	inline java_handle_t*           get_f                  () const;

	// Setters.
	inline void set_clazz              (classinfo* value);
	inline void set_name               (java_handle_t* value);
	inline void set_slot               (int32_t value);
	inline void set_annotations        (java_handle_bytearray_t* value);
	inline void set_declaredAnnotations(java_handle_t* value);
	inline void set_f                  (java_handle_t* value);

	// Convenience functions.
	inline fieldinfo* get_field() const;
};

inline java_lang_reflect_VMField::java_lang_reflect_VMField(jobject h) : java_lang_Object(h)
{
	java_lang_reflect_VMField((java_handle_t*) h);
}

inline java_lang_reflect_VMField::java_lang_reflect_VMField(java_handle_t* h, fieldinfo* f) : java_lang_Object(h)
{
	java_handle_t*           name        = javastring_intern(javastring_new(f->name));
	int                      slot        = f - f->clazz->fields;
	java_handle_bytearray_t* annotations = field_get_annotations(f);

	set_clazz(f->clazz);
	set_name(name);
	set_slot(slot);
	set_annotations(annotations);
}

inline classinfo* java_lang_reflect_VMField::get_clazz() const
{
	return get<classinfo*>(_handle, offset_clazz);
}

inline int32_t java_lang_reflect_VMField::get_slot() const
{
	return get<int32_t>(_handle, offset_slot);
}

inline java_handle_bytearray_t* java_lang_reflect_VMField::get_annotations() const
{
	return get<java_handle_bytearray_t*>(_handle, offset_annotations);
}

inline java_handle_t* java_lang_reflect_VMField::get_declaredAnnotations() const
{
	return get<java_handle_t*>(_handle, offset_declaredAnnotations);
}

inline java_handle_t* java_lang_reflect_VMField::get_f() const
{
	return get<java_handle_t*>(_handle, offset_f);
}

inline void java_lang_reflect_VMField::set_clazz(classinfo* value)
{
	set(_handle, offset_clazz, value);
}

inline void java_lang_reflect_VMField::set_name(java_handle_t* value)
{
	set(_handle, offset_name, value);
}

inline void java_lang_reflect_VMField::set_slot(int32_t value)
{
	set(_handle, offset_slot, value);
}

inline void java_lang_reflect_VMField::set_annotations(java_handle_bytearray_t* value)
{
	set(_handle, offset_annotations, value);
}

inline void java_lang_reflect_VMField::set_declaredAnnotations(java_handle_t* value)
{
	set(_handle, offset_declaredAnnotations, value);
}

inline void java_lang_reflect_VMField::set_f(java_handle_t* value)
{
	set(_handle, offset_f, value);
}

inline fieldinfo* java_lang_reflect_VMField::get_field() const
{
	classinfo* c    = get_clazz();
	int32_t    slot = get_slot();
	fieldinfo* f    = &(c->fields[slot]);
	return f;
}


/**
 * GNU Classpath java/lang/reflect/Method
 *
 * Object layout:
 *
 * 0. object header
 * 1. boolean                                     flag;
 * 2. gnu.java.lang.reflect.MethodSignatureParser p;
 * 3. java.lang.reflect.VMMethod                  m;
 */
class java_lang_reflect_Method : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_flag = sizeof(java_object_t);
	// Currently we align 64-bit data types to 8-bytes.
	static const off_t offset_p    = offset_flag + sizeof(int32_t) + 4;
	static const off_t offset_m    = offset_p    + SIZEOF_VOID_P;

public:
	java_lang_reflect_Method(java_handle_t* h) : java_lang_Object(h) {}
	java_lang_reflect_Method(jobject h);

	static java_handle_t* create(methodinfo* m);

	// Getters.
	inline int32_t        get_flag() const;
	inline java_handle_t* get_m() const;

	// Setters.
	inline void set_m(java_handle_t* value);
};

inline java_lang_reflect_Method::java_lang_reflect_Method(jobject h) : java_lang_Object(h)
{
	java_lang_reflect_Method((java_handle_t*) h);
}

inline int32_t java_lang_reflect_Method::get_flag() const
{
	return get<int32_t>(_handle, offset_flag);
}

inline java_handle_t* java_lang_reflect_Method::get_m() const
{
	return get<java_handle_t*>(_handle, offset_m);
}

inline void java_lang_reflect_Method::set_m(java_handle_t* value)
{
	set(_handle, offset_m, value);
}


/**
 * GNU Classpath java/lang/reflect/VMMethod
 *
 * Object layout:
 *
 * 0. object header
 * 1. java.lang.Class          clazz;
 * 2. java.lang.String         name;
 * 3. int                      slot;
 * 4. byte[]                   annotations;
 * 5. byte[]                   parameterAnnotations;
 * 6. byte[]                   annotationDefault;
 * 7. java.lang.Map            declaredAnnotations;
 * 8. java.lang.reflect.Method m;
 */
class java_lang_reflect_VMMethod : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_clazz                = sizeof(java_object_t);
	static const off_t offset_name                 = offset_clazz                + SIZEOF_VOID_P;
	static const off_t offset_slot                 = offset_name                 + SIZEOF_VOID_P;
	static const off_t offset_annotations          = offset_slot                 + sizeof(int32_t) + 4;
	static const off_t offset_parameterAnnotations = offset_annotations          + SIZEOF_VOID_P;
	static const off_t offset_annotationDefault    = offset_parameterAnnotations + SIZEOF_VOID_P;
	static const off_t offset_declaredAnnotations  = offset_annotationDefault    + SIZEOF_VOID_P;
	static const off_t offset_m                    = offset_declaredAnnotations  + SIZEOF_VOID_P;

public:
	java_lang_reflect_VMMethod(java_handle_t* h) : java_lang_Object(h) {}
	java_lang_reflect_VMMethod(jobject h);
	java_lang_reflect_VMMethod(java_handle_t* h, methodinfo* m);

	// Getters.
	inline classinfo*               get_clazz               () const;
	inline int32_t                  get_slot                () const;
	inline java_handle_bytearray_t* get_annotations         () const;
	inline java_handle_bytearray_t* get_parameterAnnotations() const;
	inline java_handle_bytearray_t* get_annotationDefault   () const;
	inline java_handle_t*           get_declaredAnnotations () const;
	inline java_handle_t*           get_m                   () const;

	// Setters.
	inline void set_clazz               (classinfo* value);
	inline void set_name                (java_handle_t* value);
	inline void set_slot                (int32_t value);
	inline void set_annotations         (java_handle_bytearray_t* value);
	inline void set_parameterAnnotations(java_handle_bytearray_t* value);
	inline void set_annotationDefault   (java_handle_bytearray_t* value);
	inline void set_declaredAnnotations (java_handle_t* value);
	inline void set_m                   (java_handle_t* value);

	// Convenience functions.
	inline methodinfo* get_method() const;
};

inline java_lang_reflect_VMMethod::java_lang_reflect_VMMethod(jobject h) : java_lang_Object(h)
{
	java_lang_reflect_VMMethod((java_handle_t*) h);
}

inline java_lang_reflect_VMMethod::java_lang_reflect_VMMethod(java_handle_t* h, methodinfo* m) : java_lang_Object(h)
{
	java_handle_t*           name                 = javastring_intern(javastring_new(m->name));
	int                      slot                 = m - m->clazz->methods;
	java_handle_bytearray_t* annotations          = method_get_annotations(m);
	java_handle_bytearray_t* parameterAnnotations = method_get_parameterannotations(m);
	java_handle_bytearray_t* annotationDefault    = method_get_annotationdefault(m);

	set_clazz(m->clazz);
	set_name(name);
	set_slot(slot);
	set_annotations(annotations);
	set_parameterAnnotations(parameterAnnotations);
	set_annotationDefault(annotationDefault);
}

inline classinfo* java_lang_reflect_VMMethod::get_clazz() const
{
	return get<classinfo*>(_handle, offset_clazz);
}

inline int32_t java_lang_reflect_VMMethod::get_slot() const
{
	return get<int32_t>(_handle, offset_slot);
}

inline java_handle_bytearray_t* java_lang_reflect_VMMethod::get_annotations() const
{
	return get<java_handle_bytearray_t*>(_handle, offset_annotations);
}

inline java_handle_bytearray_t* java_lang_reflect_VMMethod::get_parameterAnnotations() const
{
	return get<java_handle_bytearray_t*>(_handle, offset_parameterAnnotations);
}

inline java_handle_bytearray_t* java_lang_reflect_VMMethod::get_annotationDefault() const
{
	return get<java_handle_bytearray_t*>(_handle, offset_annotationDefault);
}

inline java_handle_t* java_lang_reflect_VMMethod::get_declaredAnnotations() const
{
	return get<java_handle_t*>(_handle, offset_declaredAnnotations);
}

inline java_handle_t* java_lang_reflect_VMMethod::get_m() const
{
	return get<java_handle_t*>(_handle, offset_m);
}

inline void java_lang_reflect_VMMethod::set_clazz(classinfo* value)
{
	set(_handle, offset_clazz, value);
}

inline void java_lang_reflect_VMMethod::set_name(java_handle_t* value)
{
	set(_handle, offset_name, value);
}

inline void java_lang_reflect_VMMethod::set_slot(int32_t value)
{
	set(_handle, offset_slot, value);
}

inline void java_lang_reflect_VMMethod::set_annotations(java_handle_bytearray_t* value)
{
	set(_handle, offset_annotations, value);
}

inline void java_lang_reflect_VMMethod::set_parameterAnnotations(java_handle_bytearray_t* value)
{
	set(_handle, offset_parameterAnnotations, value);
}

inline void java_lang_reflect_VMMethod::set_annotationDefault(java_handle_bytearray_t* value)
{
	set(_handle, offset_annotationDefault, value);
}

inline void java_lang_reflect_VMMethod::set_declaredAnnotations(java_handle_t* value)
{
	set(_handle, offset_declaredAnnotations, value);
}

inline void java_lang_reflect_VMMethod::set_m(java_handle_t* value)
{
	set(_handle, offset_m, value);
}

inline methodinfo* java_lang_reflect_VMMethod::get_method() const
{
	classinfo*  c    = get_clazz();
	int32_t     slot = get_slot();
	methodinfo* m    = &(c->methods[slot]);
	return m;
}


/**
 * GNU Classpath java/nio/Buffer
 *
 * Object layout:
 *
 * 0. object header
 * 1. int                   cap;
 * 2. int                   limit;
 * 3. int                   pos;
 * 4. int                   mark;
 * 5. gnu.classpath.Pointer address;
 */
class java_nio_Buffer : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_cap     = sizeof(java_object_t);
	static const off_t offset_limit   = offset_cap   + sizeof(int32_t);
	static const off_t offset_pos     = offset_limit + sizeof(int32_t);
	static const off_t offset_mark    = offset_pos   + sizeof(int32_t);
	static const off_t offset_address = offset_mark  + sizeof(int32_t);

public:
	java_nio_Buffer(java_handle_t* h) : java_lang_Object(h) {}

	// Getters.
	inline int32_t get_cap() const;
};

inline int32_t java_nio_Buffer::get_cap() const
{
	return get<int32_t>(_handle, offset_cap);
}


/**
 * GNU Classpath java/nio/DirectByteBufferImpl
 *
 * Object layout:
 *
 * 0. object header
 * 1. int                   cap;
 * 2. int                   limit;
 * 3. int                   pos;
 * 4. int                   mark;
 * 5. gnu.classpath.Pointer address;
 * 6. java.nio.ByteOrder    endian;
 * 7. byte[]                backing_buffer;
 * 8. int                   array_offset;
 * 9. java.lang.Object      owner;
 */
class java_nio_DirectByteBufferImpl : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_cap            = sizeof(java_object_t);
	static const off_t offset_limit          = offset_cap            + sizeof(int32_t);
	static const off_t offset_pos            = offset_limit          + sizeof(int32_t);
	static const off_t offset_mark           = offset_pos            + sizeof(int32_t);
	static const off_t offset_address        = offset_mark           + sizeof(int32_t);
	static const off_t offset_endian         = offset_address        + SIZEOF_VOID_P;
	static const off_t offset_backing_buffer = offset_endian         + SIZEOF_VOID_P;
	static const off_t offset_array_offset   = offset_backing_buffer + SIZEOF_VOID_P;
	static const off_t offset_owner          = offset_array_offset   + sizeof(int32_t);

public:
	java_nio_DirectByteBufferImpl(java_handle_t* h) : java_lang_Object(h) {}
	java_nio_DirectByteBufferImpl(jobject h);

	// Getters.
	inline java_handle_t* get_address() const;
};

inline java_nio_DirectByteBufferImpl::java_nio_DirectByteBufferImpl(jobject h) : java_lang_Object(h)
{
	java_nio_DirectByteBufferImpl((java_handle_t*) h);
}

inline java_handle_t* java_nio_DirectByteBufferImpl::get_address() const
{
	return get<java_handle_t*>(_handle, offset_address);
}


/**
 * GNU Classpath gnu/classpath/Pointer
 *
 * Actually there are two classes, gnu.classpath.Pointer32 and
 * gnu.classpath.Pointer64, but we only define the abstract super
 * class and use the int/long field as void* type.
 *
 * Object layout:
 *
 * 0. object header
 * 1. int/long data;
 */
class gnu_classpath_Pointer : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_data = sizeof(java_object_t);

public:
	gnu_classpath_Pointer(java_handle_t* h) : java_lang_Object(h) {}
	gnu_classpath_Pointer(java_handle_t* h, void* data);

	// Setters.
	inline void* get_data() const;

	// Setters.
	inline void set_data(void* value);
};

inline gnu_classpath_Pointer::gnu_classpath_Pointer(java_handle_t* h, void* data) : java_lang_Object(h)
{
	set_data(data);
}

inline void* gnu_classpath_Pointer::get_data() const
{
	return get<void*>(_handle, offset_data);
}

inline void gnu_classpath_Pointer::set_data(void* value)
{
	set(_handle, offset_data, value);
}


# if defined(ENABLE_ANNOTATIONS)
/**
 * GNU Classpath sun/reflect/ConstantPool
 *
 * Object layout:
 *
 * 0. object header
 * 1. java.lang.Object constantPoolOop;
 */
class sun_reflect_ConstantPool : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_constantPoolOop = sizeof(java_object_t);

public:
	sun_reflect_ConstantPool(java_handle_t* h) : java_lang_Object(h) {}

	// Setters.
	inline void set_constantPoolOop(classinfo* value);
	inline void set_constantPoolOop(jclass value);
};

inline void sun_reflect_ConstantPool::set_constantPoolOop(classinfo* value)
{
	set(_handle, offset_constantPoolOop, value);
}

inline void sun_reflect_ConstantPool::set_constantPoolOop(jclass value)
{
	// XXX jclass is a boxed object.
	set_constantPoolOop(LLNI_classinfo_unwrap(value));
}
# endif // ENABLE_ANNOTATIONS

#endif // WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH


#if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)

/**
 * OpenJDK java/lang/reflect/Constructor
 *
 * Object layout:
 *
 * 0. object header
 * 1. boolean                                     flag;
 * 2. gnu.java.lang.reflect.MethodSignatureParser p;
 * 3. java.lang.reflect.VMConstructor             cons;
 */
class java_lang_reflect_Constructor : public java_lang_Object, private FieldAccess {
private:
	// Static offsets of the object's instance fields.
	// TODO These offsets need to be checked on VM startup.
	static const off_t offset_flag = sizeof(java_object_t);
	static const off_t offset_p    = offset_flag + sizeof(int32_t) + 4;
	static const off_t offset_cons = offset_p    + SIZEOF_VOID_P;

public:
	java_lang_reflect_Constructor(java_handle_t* h) : java_lang_Object(h) {}
	java_lang_reflect_Constructor(jobject h);

	static java_handle_t* create(methodinfo* m);
	static java_handle_t* new_instance(methodinfo* m, java_handle_objectarray_t* args, bool override);

	// Getters.
	inline int32_t        get_flag() const;
	inline java_handle_t* get_cons() const;

	// Setters.
	inline void set_cons(java_handle_t* value);
};

inline java_lang_reflect_Constructor::java_lang_reflect_Constructor(jobject h) : java_lang_Object(h)
{
	java_lang_reflect_Constructor((java_handle_t*) h);
}

inline int32_t java_lang_reflect_Constructor::get_flag() const
{
	return get<int32_t>(_handle, offset_flag);
}

inline java_handle_t* java_lang_reflect_Constructor::get_cons() const
{
	return get<java_handle_t*>(_handle, offset_cons);
}

inline void java_lang_reflect_Constructor::set_cons(java_handle_t* value)
{
	set(_handle, offset_cons, value);
}

#endif // WITH_JAVA_RUNTIME_LIBRARY_OPENJDK

#else

// Legacy C interface.
java_handle_t* java_lang_reflect_Constructor_create(methodinfo* m);
java_handle_t* java_lang_reflect_Field_create(fieldinfo* f);
java_handle_t* java_lang_reflect_Method_create(methodinfo* m);

#endif

#endif // _JAVAOBJECTS_HPP


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
