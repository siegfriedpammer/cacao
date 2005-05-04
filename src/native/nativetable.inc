/* src/native/nativetable.inc - table of native functions CACAO implements

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

   Authors: Christian Thalinger

   Changes:

   $Id: nativetable.inc 2437 2005-05-04 15:06:30Z twisti $

*/


#include "native/include/java_lang_Cloneable.h"
#include "native/include/java_util_Properties.h"
#include "native/include/java_io_InputStream.h"
#include "native/include/java_io_PrintStream.h"
#include "native/include/gnu_classpath_VMStackWalker.h"
#include "native/include/gnu_classpath_VMSystemProperties.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_Object.h"
#include "native/include/java_lang_VMClass.h"
#include "native/include/java_lang_VMClassLoader.h"
#include "native/include/java_lang_VMObject.h"
#include "native/include/java_lang_VMRuntime.h"
#include "native/include/java_lang_VMString.h"
#include "native/include/java_lang_VMSystem.h"
#include "native/include/java_lang_VMThread.h"
#include "native/include/java_lang_VMThrowable.h"
#include "native/include/java_lang_reflect_Constructor.h"
#include "native/include/java_lang_reflect_Field.h"
#include "native/include/java_lang_reflect_Proxy.h"
#include "native/include/java_lang_reflect_Method.h"


#if defined(STATIC_CLASSPATH)

/* Include header files for native functions implemented in GNU classpath */

#include "native/include/java_lang_SecurityManager.h"
#include "native/include/java_net_DatagramPacket.h"
#include "native/include/java_net_InetAddress.h"
#include "native/include/java_net_SocketImpl.h"

#include "native/include/gnu_java_net_PlainDatagramSocketImpl.h"
#include "native/include/gnu_java_net_PlainSocketImpl.h"
#include "native/include/gnu_java_nio_channels_FileChannelImpl.h"
#include "native/include/java_io_ObjectInputStream.h"
#include "native/include/java_io_VMFile.h"
#include "native/include/java_io_VMObjectStreamClass.h"
#include "native/include/java_lang_Double.h"
#include "native/include/java_lang_Math.h"
#include "native/include/java_lang_VMDouble.h"
#include "native/include/java_lang_VMFloat.h"
#include "native/include/java_lang_reflect_Array.h"
#include "native/include/java_net_NetworkInterface.h"
#include "native/include/java_nio_DirectByteBufferImpl.h"
#include "native/include/java_util_VMTimeZone.h"


static nativeref nativetable[] = {
	/* These are implemented in CACAO *****************************************/

	{ "gnu/classpath/VMStackWalker",
	  "getClassContext",
	  "()[Ljava/lang/Class;",
	  true,
	  (functionptr) Java_gnu_classpath_VMStackWalker_getClassContext
	},
	{ "gnu/classpath/VMSystemProperties",
	  "preInit",
	  "(Ljava/util/Properties;)V",
	  true,
	  (functionptr) Java_gnu_classpath_VMSystemProperties_preInit
	},
	{ "java/lang/VMClass",
	  "isInstance",
	  "(Ljava/lang/Class;Ljava/lang/Object;)Z",
	  true,
	  (functionptr) Java_java_lang_VMClass_isInstance
	},
	{ "java/lang/VMClass",
	  "isAssignableFrom",
	  "(Ljava/lang/Class;Ljava/lang/Class;)Z",
	  true,
	  (functionptr) Java_java_lang_VMClass_isAssignableFrom
	},
	{ "java/lang/VMClass",
	  "isInterface",
	  "(Ljava/lang/Class;)Z",
	  true,
	  (functionptr) Java_java_lang_VMClass_isInterface
	},
	{ "java/lang/VMClass",
	  "isPrimitive",
	  "(Ljava/lang/Class;)Z",
	  true,
	  (functionptr) Java_java_lang_VMClass_isPrimitive
	},
	{ "java/lang/VMClass",
	  "getName",
	  "(Ljava/lang/Class;)Ljava/lang/String;",
	  true,
	  (functionptr) Java_java_lang_VMClass_getName
	},
	{ "java/lang/VMClass",
	  "getSuperclass",
	  "(Ljava/lang/Class;)Ljava/lang/Class;",
	  true,
	  (functionptr) Java_java_lang_VMClass_getSuperclass
	},
	{ "java/lang/VMClass",
	  "getInterfaces",
	  "(Ljava/lang/Class;)[Ljava/lang/Class;",
	  true,
	  (functionptr) Java_java_lang_VMClass_getInterfaces
	},
	{ "java/lang/VMClass",
	  "getComponentType",
	  "(Ljava/lang/Class;)Ljava/lang/Class;",
	  true,
	  (functionptr) Java_java_lang_VMClass_getComponentType
	},
	{ "java/lang/VMClass",
	  "getModifiers",
	  "(Ljava/lang/Class;)I",
	  true,
	  (functionptr) Java_java_lang_VMClass_getModifiers
	},
	{ "java/lang/VMClass",
	  "getDeclaringClass",
	  "(Ljava/lang/Class;)Ljava/lang/Class;",
	  true,
	  (functionptr) Java_java_lang_VMClass_getDeclaringClass
	},
	{ "java/lang/VMClass",
	  "getDeclaredClasses",
	  "(Ljava/lang/Class;Z)[Ljava/lang/Class;",
	  true,
	  (functionptr) Java_java_lang_VMClass_getDeclaredClasses
	},
	{ "java/lang/VMClass",
	  "getDeclaredFields",
	  "(Ljava/lang/Class;Z)[Ljava/lang/reflect/Field;",
	  true,
	  (functionptr) Java_java_lang_VMClass_getDeclaredFields
	},
	{ "java/lang/VMClass",
	  "getDeclaredMethods",
	  "(Ljava/lang/Class;Z)[Ljava/lang/reflect/Method;",
	  true,
	  (functionptr) Java_java_lang_VMClass_getDeclaredMethods
	},
	{ "java/lang/VMClass",
	  "getDeclaredConstructors",
	  "(Ljava/lang/Class;Z)[Ljava/lang/reflect/Constructor;",
	  true,
	  (functionptr) Java_java_lang_VMClass_getDeclaredConstructors
	},
	{ "java/lang/VMClass",
	  "getClassLoader",
	  "(Ljava/lang/Class;)Ljava/lang/ClassLoader;",
	  true,
	  (functionptr) Java_java_lang_VMClass_getClassLoader
	},
	{ "java/lang/VMClass",
	  "forName",
	  "(Ljava/lang/String;)Ljava/lang/Class;",
	  true,
	  (functionptr) Java_java_lang_VMClass_forName
	},
	{ "java/lang/VMClass",
	  "isArray",
	  "(Ljava/lang/Class;)Z",
	  true,
	  (functionptr) Java_java_lang_VMClass_isArray
	},
	{ "java/lang/VMClass",
	  "initialize",
	  "(Ljava/lang/Class;)V",
	  true,
	  (functionptr) Java_java_lang_VMClass_initialize
	},
	{ "java/lang/VMClass",
	  "loadArrayClass",
	  "(Ljava/lang/String;Ljava/lang/ClassLoader;)Ljava/lang/Class;",
	  true,
	  (functionptr) Java_java_lang_VMClass_loadArrayClass
	},
	{ "java/lang/VMClass",
	  "throwException",
	  "(Ljava/lang/Throwable;)V",
	  true,
	  (functionptr) Java_java_lang_VMClass_throwException
	},
	{ "java/lang/VMClassLoader",
	  "defineClass",
	  "(Ljava/lang/ClassLoader;Ljava/lang/String;[BIILjava/security/ProtectionDomain;)Ljava/lang/Class;",
	  true,
	  (functionptr) Java_java_lang_VMClassLoader_defineClass
	},
	{ "java/lang/VMClassLoader",
	  "resolveClass",
	  "(Ljava/lang/Class;)V",
	  true,
	  (functionptr) Java_java_lang_VMClassLoader_resolveClass
	},
	{ "java/lang/VMClassLoader",
	  "loadClass",
	  "(Ljava/lang/String;Z)Ljava/lang/Class;",
	  true,
	  (functionptr) Java_java_lang_VMClassLoader_loadClass
	},
	{ "java/lang/VMClassLoader",
	  "nativeGetResources",
	  "(Ljava/lang/String;)Ljava/util/Vector;",
	  true,
 	  (functionptr) Java_java_lang_VMClassLoader_nativeGetResources
	},
	{ "java/lang/VMClassLoader",
	  "getPrimitiveClass",
	  "(Ljava/lang/String;)Ljava/lang/Class;",
	  true,
	  (functionptr) Java_java_lang_VMClassLoader_getPrimitiveClass
	},
	{ "java/lang/VMObject",
	  "clone",
	  "(Ljava/lang/Cloneable;)Ljava/lang/Object;",
	  true,
	  (functionptr) Java_java_lang_VMObject_clone
	},
	{ "java/lang/VMObject",
	  "notify",
	  "(Ljava/lang/Object;)V",
	  true,
	  (functionptr) Java_java_lang_VMObject_notify
	},
	{ "java/lang/VMObject",
	  "notifyAll",
	  "(Ljava/lang/Object;)V",
	  true,
	  (functionptr) Java_java_lang_VMObject_notifyAll
	},
	{ "java/lang/VMObject",
	  "wait",
	  "(Ljava/lang/Object;JI)V",
	  true,
	  (functionptr) Java_java_lang_VMObject_wait
	},
	{ "java/lang/VMRuntime",
	  "availableProcessors",
	  "()I",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_availableProcessors
	},
	{ "java/lang/VMRuntime",
	  "freeMemory",
	  "()J",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_freeMemory
	},
	{ "java/lang/VMRuntime",
	  "totalMemory",
	  "()J",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_totalMemory
	},
	{ "java/lang/VMRuntime",
	  "maxMemory",
	  "()J",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_maxMemory
	},
	{ "java/lang/VMRuntime",
	  "gc",
	  "()V",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_gc
	},
	{ "java/lang/VMRuntime",
	  "runFinalization",
	  "()V",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_runFinalization
	},
	{ "java/lang/VMRuntime",
	  "runFinalizationForExit",
	  "()V",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_runFinalizationForExit
	},
	{ "java/lang/VMRuntime",
	  "traceInstructions",
	  "(Z)V",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_traceInstructions
	},
	{ "java/lang/VMRuntime",
	  "traceMethodCalls",
	  "(Z)V",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_traceMethodCalls
	},
	{ "java/lang/VMRuntime",
	  "runFinalizersOnExit",
	  "(Z)V",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_runFinalizersOnExit
	},
	{ "java/lang/VMRuntime",
	  "exit",
	  "(I)V",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_exit
	},
	{ "java/lang/VMRuntime",
	  "nativeLoad",
	  "(Ljava/lang/String;Ljava/lang/ClassLoader;)I",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_nativeLoad
	},
	{ "java/lang/VMRuntime",
	  "mapLibraryName",
	  "(Ljava/lang/String;)Ljava/lang/String;",
	  true,
	  (functionptr) Java_java_lang_VMRuntime_mapLibraryName
	},
	{ "java/lang/VMString",
	  "intern",
	  "(Ljava/lang/String;)Ljava/lang/String;",
	  true,
	  (functionptr) Java_java_lang_VMString_intern
	},
	{ "java/lang/VMSystem",
	  "arraycopy",
	  "(Ljava/lang/Object;ILjava/lang/Object;II)V",
	  true,
	  (functionptr) Java_java_lang_VMSystem_arraycopy
	},
	{ "java/lang/VMSystem",
	  "identityHashCode",
	  "(Ljava/lang/Object;)I",
	  true,
	  (functionptr) Java_java_lang_VMSystem_identityHashCode
	},
	{ "java/lang/VMThread",
	  "start",
	  "(J)V",
	  false,
	  (functionptr) Java_java_lang_VMThread_start
	},
	{ "java/lang/VMThread",
	  "interrupt",
	  "()V",
	  false,
	  (functionptr) Java_java_lang_VMThread_interrupt
	},
	{ "java/lang/VMThread",
	  "isInterrupted",
	  "()Z",
	  false,
	  (functionptr) Java_java_lang_VMThread_isInterrupted
	},
	{ "java/lang/VMThread",
	  "suspend",
	  "()V",
	  false,
	  (functionptr) Java_java_lang_VMThread_suspend
	},
	{ "java/lang/VMThread",
	  "resume",
	  "()V",
	  false,
	  (functionptr) Java_java_lang_VMThread_resume
	},
	{ "java/lang/VMThread",
	  "nativeSetPriority",
	  "(I)V",
	  false,
	  (functionptr) Java_java_lang_VMThread_nativeSetPriority
	},
	{ "java/lang/VMThread",
	  "nativeStop",
	  "(Ljava/lang/Throwable;)V",
	  false,
	  (functionptr) Java_java_lang_VMThread_nativeStop
	},
	{ "java/lang/VMThread",
	  "currentThread",
	  "()Ljava/lang/Thread;",
	  true,
	  (functionptr) Java_java_lang_VMThread_currentThread
	},
	{ "java/lang/VMThread",
	  "yield",
	  "()V",
	  true,
	  (functionptr) Java_java_lang_VMThread_yield
	},
	{ "java/lang/VMThread",
	  "interrupted",
	  "()Z",
	  true,
	  (functionptr) Java_java_lang_VMThread_interrupted
	},
	{ "java/lang/VMThread",
	  "holdsLock",
	  "(Ljava/lang/Object;)Z",
	  true,
	  (functionptr) Java_java_lang_VMThread_holdsLock
	},
	{ "java/lang/VMThrowable",
	  "fillInStackTrace",
	  "(Ljava/lang/Throwable;)Ljava/lang/VMThrowable;",
	  true,
	  (functionptr) Java_java_lang_VMThrowable_fillInStackTrace
	},
	{ "java/lang/VMThrowable",
	  "getStackTrace",
	  "(Ljava/lang/Throwable;)[Ljava/lang/StackTraceElement;",
	  false,
	  (functionptr) Java_java_lang_VMThrowable_getStackTrace
	},
	{ "java/lang/reflect/Constructor",
	  "getModifiers",
	  "()I",
	  false,
	  (functionptr) Java_java_lang_reflect_Constructor_getModifiers
	},
	{ "java/lang/reflect/Constructor",
	  "constructNative",
	  "([Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;",
	  false,
	  (functionptr) Java_java_lang_reflect_Constructor_constructNative
	},
	{ "java/lang/reflect/Field",
	  "getModifiers",
	  "()I",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_getModifiers
	},
	{ "java/lang/reflect/Field",
	  "getType",
	  "()Ljava/lang/Class;",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_getType
	},
	{ "java/lang/reflect/Field",
	  "get",
	  "(Ljava/lang/Object;)Ljava/lang/Object;",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_get
	},
	{ "java/lang/reflect/Field",
	  "getBoolean",
	  "(Ljava/lang/Object;)Z",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_getBoolean
	},
	{ "java/lang/reflect/Field",
	  "getByte",
	  "(Ljava/lang/Object;)B",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_getByte
	},
	{ "java/lang/reflect/Field",
	  "getChar",
	  "(Ljava/lang/Object;)C",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_getChar
	},
	{ "java/lang/reflect/Field",
	  "getShort",
	  "(Ljava/lang/Object;)S",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_getShort
	},
	{ "java/lang/reflect/Field",
	  "getInt",
	  "(Ljava/lang/Object;)I",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_getInt
	},
	{ "java/lang/reflect/Field",
	  "getLong",
	  "(Ljava/lang/Object;)J",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_getLong
	},
	{ "java/lang/reflect/Field",
	  "getFloat",
	  "(Ljava/lang/Object;)F",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_getFloat
	},
	{ "java/lang/reflect/Field",
	  "getDouble",
	  "(Ljava/lang/Object;)D",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_getDouble
	},
	{ "java/lang/reflect/Field",
	  "set",
	  "(Ljava/lang/Object;Ljava/lang/Object;)V",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_set
	},
	{ "java/lang/reflect/Field",
	  "setBoolean",
	  "(Ljava/lang/Object;Z)V",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_setBoolean
	},
	{ "java/lang/reflect/Field",
	  "setByte",
	  "(Ljava/lang/Object;B)V",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_setByte
	},
	{ "java/lang/reflect/Field",
	  "setChar",
	  "(Ljava/lang/Object;C)V",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_setChar
	},
	{ "java/lang/reflect/Field",
	  "setShort",
	  "(Ljava/lang/Object;S)V",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_setShort
	},
	{ "java/lang/reflect/Field",
	  "setInt",
	  "(Ljava/lang/Object;I)V",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_setInt
	},
	{ "java/lang/reflect/Field",
	  "setLong",
	  "(Ljava/lang/Object;J)V",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_setLong
	},
	{ "java/lang/reflect/Field",
	  "setFloat",
	  "(Ljava/lang/Object;F)V",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_setFloat
	},
	{ "java/lang/reflect/Field",
	  "setDouble",
	  "(Ljava/lang/Object;D)V",
	  false,
	  (functionptr) Java_java_lang_reflect_Field_setDouble
	},
	{ "java/lang/reflect/Proxy",
	  "getProxyClass0",
	  "(Ljava/lang/ClassLoader;[Ljava/lang/Class;)Ljava/lang/Class;",
	  true,
	  (functionptr) Java_java_lang_reflect_Proxy_getProxyClass0
	},
	{ "java/lang/reflect/Proxy",
	  "getProxyData0",
	  "(Ljava/lang/ClassLoader;[Ljava/lang/Class;)Ljava/lang/reflect/Proxy$ProxyData;",
	  true,
	  (functionptr) Java_java_lang_reflect_Proxy_getProxyData0
	},
	{ "java/lang/reflect/Proxy",
	  "generateProxyClass0",
	  "(Ljava/lang/ClassLoader;Ljava/lang/reflect/Proxy$ProxyData;)Ljava/lang/Class;",
	  true,
	  (functionptr) Java_java_lang_reflect_Proxy_generateProxyClass0
	},
	{ "java/lang/reflect/Method",
	  "getModifiers",
	  "()I",
	  false,
	  (functionptr) Java_java_lang_reflect_Method_getModifiers
	},
	{ "java/lang/reflect/Method",
	  "getReturnType",
	  "()Ljava/lang/Class;",
	  false,
	  (functionptr) Java_java_lang_reflect_Method_getReturnType
	},
	{ "java/lang/reflect/Method",
	  "getParameterTypes",
	  "()[Ljava/lang/Class;",
	  false,
	  (functionptr) Java_java_lang_reflect_Method_getParameterTypes
	},
	{ "java/lang/reflect/Method",
	  "getExceptionTypes",
	  "()[Ljava/lang/Class;",
	  false,
	  (functionptr) Java_java_lang_reflect_Method_getExceptionTypes
	},
	{ "java/lang/reflect/Method",
	  "invokeNative",
	  "(Ljava/lang/Object;[Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;",
	  false,
	  (functionptr) Java_java_lang_reflect_Method_invokeNative
	},


	/* These are implemented in GNU classpath *********************************/

	{ "java/lang/Object",
	  "getClass",
	  "()Ljava/lang/Class;",
	  false,
	  (functionptr) Java_java_lang_Object_getClass
	},
	{ "java/net/InetAddress",
	  "getLocalHostname",
	  "()Ljava/lang/String;",
	  true,
	  (functionptr) Java_java_net_InetAddress_getLocalHostname
	},
	{ "java/net/InetAddress",
	  "lookupInaddrAny",
	  "()[B",
	  true,
	  (functionptr) Java_java_net_InetAddress_lookupInaddrAny
	},
	{ "java/net/InetAddress",
	  "getHostByAddr",
	  "([B)Ljava/lang/String;",
	  true,
	  (functionptr) Java_java_net_InetAddress_getHostByAddr
	},
	{ "java/net/InetAddress",
	  "getHostByName",
	  "(Ljava/lang/String;)[[B",
	  true,
	  (functionptr) Java_java_net_InetAddress_getHostByName
	},
	{ "gnu/java/net/PlainDatagramSocketImpl",
	  "bind",
	  "(ILjava/net/InetAddress;)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainDatagramSocketImpl_bind
	},
	{ "gnu/java/net/PlainDatagramSocketImpl",
	  "create",
	  "()V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainDatagramSocketImpl_create
	},
	{ "gnu/java/net/PlainDatagramSocketImpl",
	  "sendto",
	  "(Ljava/net/InetAddress;I[BII)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainDatagramSocketImpl_sendto
	},
	{ "gnu/java/net/PlainDatagramSocketImpl",
	  "receive0",
	  "(Ljava/net/DatagramPacket;)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainDatagramSocketImpl_receive0
	},
	{ "gnu/java/net/PlainDatagramSocketImpl",
	  "setOption",
	  "(ILjava/lang/Object;)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainDatagramSocketImpl_setOption
	},
	{ "gnu/java/net/PlainDatagramSocketImpl",
	  "getOption",
	  "(I)Ljava/lang/Object;",
	  false,
	  (functionptr) Java_gnu_java_net_PlainDatagramSocketImpl_getOption
	},
	{ "gnu/java/net/PlainDatagramSocketImpl",
	  "close",
	  "()V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainDatagramSocketImpl_close
	},
	{ "gnu/java/net/PlainDatagramSocketImpl",
	  "join",
	  "(Ljava/net/InetAddress;)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainDatagramSocketImpl_join
	},
	{ "gnu/java/net/PlainDatagramSocketImpl",
	  "leave",
	  "(Ljava/net/InetAddress;)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainDatagramSocketImpl_leave
	},
	{ "gnu/java/net/PlainSocketImpl",
	  "setOption",
	  "(ILjava/lang/Object;)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainSocketImpl_setOption
	},
	{ "gnu/java/net/PlainSocketImpl",
	  "getOption",
	  "(I)Ljava/lang/Object;",
	  false,
	  (functionptr) Java_gnu_java_net_PlainSocketImpl_getOption
	},
	{ "gnu/java/net/PlainSocketImpl",
	  "create",
	  "(Z)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainSocketImpl_create
	},
	{ "gnu/java/net/PlainSocketImpl",
	  "connect",
	  "(Ljava/net/InetAddress;I)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainSocketImpl_connect
	},
	{ "gnu/java/net/PlainSocketImpl",
	  "bind",
	  "(Ljava/net/InetAddress;I)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainSocketImpl_bind
	},
	{ "gnu/java/net/PlainSocketImpl",
	  "listen",
	  "(I)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainSocketImpl_listen
	},
	{ "gnu/java/net/PlainSocketImpl",
	  "accept",
	  "(Ljava/net/SocketImpl;)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainSocketImpl_accept
	},
	{ "gnu/java/net/PlainSocketImpl",
	  "available",
	  "()I",
	  false,
	  (functionptr) Java_gnu_java_net_PlainSocketImpl_available
	},
	{ "gnu/java/net/PlainSocketImpl",
	  "close",
	  "()V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainSocketImpl_close
	},
	{ "gnu/java/net/PlainSocketImpl",
	  "read",
	  "([BII)I",
	  false,
	  (functionptr) Java_gnu_java_net_PlainSocketImpl_read
	},
	{ "gnu/java/net/PlainSocketImpl",
	  "write",
	  "([BII)V",
	  false,
	  (functionptr) Java_gnu_java_net_PlainSocketImpl_write
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "init",
	  "()V",
	  true,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_init
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "open",
	  "(Ljava/lang/String;I)I",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_open
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "available",
	  "()I",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_available
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "implPosition",
	  "()J",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_implPosition
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "seek",
	  "(J)V",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_seek
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "implTruncate",
	  "(J)V",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_implTruncate
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "unlock",
	  "(JJ)V",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_unlock
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "size",
	  "()J",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_size
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "implCloseChannel",
	  "()V",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_implCloseChannel
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "read",
	  "()I",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_read__
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "read",
	  "([BII)I",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_read___3BII
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "write",
	  "([BII)V",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_write___3BII
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "write",
	  "(I)V",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_write__I
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "mapImpl",
	  "(CJI)Ljava/nio/MappedByteBuffer;",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_mapImpl
	},
	{ "gnu/java/nio/channels/FileChannelImpl",
	  "lock",
	  "(JJZZ)Z",
	  false,
	  (functionptr) Java_gnu_java_nio_channels_FileChannelImpl_lock
	},
	{ "java/io/ObjectInputStream",
	  "currentClassLoader",
	  "(Ljava/lang/SecurityManager;)Ljava/lang/ClassLoader;",
	  true,
	  (functionptr) Java_java_io_ObjectInputStream_currentClassLoader
	},
	{ "java/io/ObjectInputStream",
	  "allocateObject",
	  "(Ljava/lang/Class;)Ljava/lang/Object;",
	  false,
	  (functionptr) Java_java_io_ObjectInputStream_allocateObject
	},
	{ "java/io/VMFile",
	  "lastModified",
	  "(Ljava/lang/String;)J",
	  true,
	  (functionptr) Java_java_io_VMFile_lastModified
	},
	{ "java/io/VMFile",
	  "setReadOnly",
	  "(Ljava/lang/String;)Z",
	  true,
	  (functionptr) Java_java_io_VMFile_setReadOnly
	},
	{ "java/io/VMFile",
	  "create",
	  "(Ljava/lang/String;)Z",
	  true,
	  (functionptr) Java_java_io_VMFile_create
	},
	{ "java/io/VMFile",
	  "list",
	  "(Ljava/lang/String;)[Ljava/lang/String;",
	  true,
	  (functionptr) Java_java_io_VMFile_list
	},
	{ "java/io/VMFile",
	  "renameTo",
	  "(Ljava/lang/String;Ljava/lang/String;)Z",
	  true,
	  (functionptr) Java_java_io_VMFile_renameTo
	},
	{ "java/io/VMFile",
	  "length",
	  "(Ljava/lang/String;)J",
	  true,
	  (functionptr) Java_java_io_VMFile_length
	},
	{ "java/io/VMFile",
	  "exists",
	  "(Ljava/lang/String;)Z",
	  true,
	  (functionptr) Java_java_io_VMFile_exists
	},
	{ "java/io/VMFile",
	  "delete",
	  "(Ljava/lang/String;)Z",
	  true,
	  (functionptr) Java_java_io_VMFile_delete
	},
	{ "java/io/VMFile",
	  "setLastModified",
	  "(Ljava/lang/String;J)Z",
	  true,
	  (functionptr) Java_java_io_VMFile_setLastModified
	},
	{ "java/io/VMFile",
	  "mkdir",
	  "(Ljava/lang/String;)Z",
	  true,
	  (functionptr) Java_java_io_VMFile_mkdir
	},
	{ "java/io/VMFile",
	  "isFile",
	  "(Ljava/lang/String;)Z",
	  true,
	  (functionptr) Java_java_io_VMFile_isFile
	},
	{ "java/io/VMFile",
	  "canWrite",
	  "(Ljava/lang/String;)Z",
	  true,
	  (functionptr) Java_java_io_VMFile_canWrite
	},
	{ "java/io/VMFile",
	  "canRead",
	  "(Ljava/lang/String;)Z",
	  true,
	  (functionptr) Java_java_io_VMFile_canRead
	},
	{ "java/io/VMFile",
	  "isDirectory",
	  "(Ljava/lang/String;)Z",
	  true,
	  (functionptr) Java_java_io_VMFile_isDirectory
	},
	{ "java/io/VMObjectStreamClass",
	  "hasClassInitializer",
	  "(Ljava/lang/Class;)Z",
	  true,
	  (functionptr) Java_java_io_VMObjectStreamClass_hasClassInitializer
	},
	{ "java/io/VMObjectStreamClass",
	  "setDoubleNative",
	  "(Ljava/lang/reflect/Field;Ljava/lang/Object;D)V",
	  true,
	  (functionptr) Java_java_io_VMObjectStreamClass_setDoubleNative
	},
	{ "java/io/VMObjectStreamClass",
	  "setFloatNative",
	  "(Ljava/lang/reflect/Field;Ljava/lang/Object;F)V",
	  true,
	  (functionptr) Java_java_io_VMObjectStreamClass_setFloatNative
	},
	{ "java/io/VMObjectStreamClass",
	  "setLongNative",
	  "(Ljava/lang/reflect/Field;Ljava/lang/Object;J)V",
	  true,
	  (functionptr) Java_java_io_VMObjectStreamClass_setLongNative
	},
	{ "java/io/VMObjectStreamClass",
	  "setIntNative",
	  "(Ljava/lang/reflect/Field;Ljava/lang/Object;I)V",
	  true,
	  (functionptr) Java_java_io_VMObjectStreamClass_setIntNative
	},
	{ "java/io/VMObjectStreamClass",
	  "setShortNative",
	  "(Ljava/lang/reflect/Field;Ljava/lang/Object;S)V",
	  true,
	  (functionptr) Java_java_io_VMObjectStreamClass_setShortNative
	},
	{ "java/io/VMObjectStreamClass",
	  "setCharNative",
	  "(Ljava/lang/reflect/Field;Ljava/lang/Object;C)V",
	  true,
	  (functionptr) Java_java_io_VMObjectStreamClass_setCharNative
	},
	{ "java/io/VMObjectStreamClass",
	  "setByteNative",
	  "(Ljava/lang/reflect/Field;Ljava/lang/Object;B)V",
	  true,
	  (functionptr) Java_java_io_VMObjectStreamClass_setByteNative
	},
	{ "java/io/VMObjectStreamClass",
	  "setBooleanNative",
	  "(Ljava/lang/reflect/Field;Ljava/lang/Object;Z)V",
	  true,
	  (functionptr) Java_java_io_VMObjectStreamClass_setBooleanNative
	},
	{ "java/io/VMObjectStreamClass",
	  "setObjectNative",
	  "(Ljava/lang/reflect/Field;Ljava/lang/Object;Ljava/lang/Object;)V",
	  true,
	  (functionptr) Java_java_io_VMObjectStreamClass_setObjectNative
	},
	{ "java/lang/Double",
	  "parseDouble",
	  "(Ljava/lang/String;)D",
	  true,
	  (functionptr) Java_java_lang_Double_parseDouble
	},
	{ "java/lang/Double",
	  "toString",
	  "(DZ)Ljava/lang/String;",
	  true,
	  (functionptr) Java_java_lang_Double_toString
	},
	{ "java/lang/Double",
	  "initIDs",
	  "()V",
	  true,
	  (functionptr) Java_java_lang_Double_initIDs
	},
	{ "java/lang/Math",
	  "sin",
	  "(D)D",
	  true,
	  (functionptr) Java_java_lang_Math_sin
	},
	{ "java/lang/Math",
	  "cos",
	  "(D)D",
	  true,
	  (functionptr) Java_java_lang_Math_cos
	},
	{ "java/lang/Math",
	  "tan",
	  "(D)D",
	  true,
	  (functionptr) Java_java_lang_Math_tan
	},
	{ "java/lang/Math",
	  "asin",
	  "(D)D",
	  true,
	  (functionptr) Java_java_lang_Math_asin
	},
	{ "java/lang/Math",
	  "acos",
	  "(D)D",
	  true,
	  (functionptr) Java_java_lang_Math_acos
	},
	{ "java/lang/Math",
	  "atan",
	  "(D)D",
	  true,
	  (functionptr) Java_java_lang_Math_atan
	},
	{ "java/lang/Math",
	  "atan2",
	  "(DD)D",
	  true,
	  (functionptr) Java_java_lang_Math_atan2
	},
	{ "java/lang/Math",
	  "exp",
	  "(D)D",
	  true,
	  (functionptr) Java_java_lang_Math_exp
	},
	{ "java/lang/Math",
	  "log",
	  "(D)D",
	  true,
	  (functionptr) Java_java_lang_Math_log
	},
	{ "java/lang/Math",
	  "sqrt",
	  "(D)D",
	  true,
	  (functionptr) Java_java_lang_Math_sqrt
	},
	{ "java/lang/Math",
	  "pow",
	  "(DD)D",
	  true,
	  (functionptr) Java_java_lang_Math_pow
	},
	{ "java/lang/Math",
	  "IEEEremainder",
	  "(DD)D",
	  true,
	  (functionptr) Java_java_lang_Math_IEEEremainder
	},
	{ "java/lang/Math",
	  "ceil",
	  "(D)D",
	  true,
	  (functionptr) Java_java_lang_Math_ceil
	},
	{ "java/lang/Math",
	  "floor",
	  "(D)D",
	  true,
	  (functionptr) Java_java_lang_Math_floor
	},
	{ "java/lang/Math",
	  "rint",
	  "(D)D",
	  true,
	  (functionptr) Java_java_lang_Math_rint
	},
	{ "java/lang/VMDouble",
	  "doubleToLongBits",
	  "(D)J",
	  true,
	  (functionptr) Java_java_lang_VMDouble_doubleToLongBits
	},
	{ "java/lang/VMDouble",
	  "doubleToRawLongBits",
	  "(D)J",
	  true,
	  (functionptr) Java_java_lang_VMDouble_doubleToRawLongBits
	},
	{ "java/lang/VMDouble",
	  "longBitsToDouble",
	  "(J)D",
	  true,
	  (functionptr) Java_java_lang_VMDouble_longBitsToDouble
	},
	{ "java/lang/VMFloat",
	  "floatToIntBits",
	  "(F)I",
	  true,
	  (functionptr) Java_java_lang_VMFloat_floatToIntBits
	},
	{ "java/lang/VMFloat",
	  "floatToRawIntBits",
	  "(F)I",
	  true,
	  (functionptr) Java_java_lang_VMFloat_floatToRawIntBits
	},
	{ "java/lang/VMFloat",
	  "intBitsToFloat",
	  "(I)F",
	  true,
	  (functionptr) Java_java_lang_VMFloat_intBitsToFloat
	},
	{ "java/lang/VMSystem",
	  "setIn",
	  "(Ljava/io/InputStream;)V",
	  true,
	  (functionptr) Java_java_lang_VMSystem_setIn
	},
	{ "java/lang/VMSystem",
	  "setOut",
	  "(Ljava/io/PrintStream;)V",
	  true,
	  (functionptr) Java_java_lang_VMSystem_setOut
	},
	{ "java/lang/VMSystem",
	  "setErr",
	  "(Ljava/io/PrintStream;)V",
	  true,
	  (functionptr) Java_java_lang_VMSystem_setErr
	},
	{ "java/lang/VMSystem",
	  "currentTimeMillis",
	  "()J",
	  true,
	  (functionptr) Java_java_lang_VMSystem_currentTimeMillis
	},
	{ "java/lang/VMSystem",
	  "getenv",
	  "(Ljava/lang/String;)Ljava/lang/String;",
	  true,
	  (functionptr) Java_java_lang_VMSystem_getenv
	},
	{ "java/lang/reflect/Array",
	  "createObjectArray",
	  "(Ljava/lang/Class;I)Ljava/lang/Object;",
	  true,
	  (functionptr) Java_java_lang_reflect_Array_createObjectArray
	},
	{ "java/net/NetworkInterface",
	  "getRealNetworkInterfaces",
	  "()Ljava/util/Vector;",
	  true,
	  (functionptr) Java_java_net_NetworkInterface_getRealNetworkInterfaces
	},
	{ "java/util/VMTimeZone",
	  "getSystemTimeZoneId",
	  "()Ljava/lang/String;",
	  true,
	  (functionptr) Java_java_util_VMTimeZone_getSystemTimeZoneId
	},
};

#else /* defined(STATIC_CLASSPATH) */

/* Ensure that symbols for functions implemented within cacao are used and    */
/* exported to dlopen.                                                        */

static functionptr dummynativetable[] = {
	(functionptr) Java_gnu_classpath_VMStackWalker_getClassContext,
	(functionptr) Java_gnu_classpath_VMSystemProperties_preInit,
	(functionptr) Java_java_lang_VMClass_isInstance,
	(functionptr) Java_java_lang_VMClass_isAssignableFrom,
	(functionptr) Java_java_lang_VMClass_isInterface,
	(functionptr) Java_java_lang_VMClass_isPrimitive,
	(functionptr) Java_java_lang_VMClass_getName,
	(functionptr) Java_java_lang_VMClass_getSuperclass,
	(functionptr) Java_java_lang_VMClass_getInterfaces,
	(functionptr) Java_java_lang_VMClass_getComponentType,
	(functionptr) Java_java_lang_VMClass_getModifiers,
	(functionptr) Java_java_lang_VMClass_getDeclaringClass,
	(functionptr) Java_java_lang_VMClass_getDeclaredClasses,
	(functionptr) Java_java_lang_VMClass_getDeclaredFields,
	(functionptr) Java_java_lang_VMClass_getDeclaredMethods,
	(functionptr) Java_java_lang_VMClass_getDeclaredConstructors,
	(functionptr) Java_java_lang_VMClass_getClassLoader,
	(functionptr) Java_java_lang_VMClass_forName,
	(functionptr) Java_java_lang_VMClass_isArray,
	(functionptr) Java_java_lang_VMClass_initialize,
	(functionptr) Java_java_lang_VMClass_loadArrayClass,
	(functionptr) Java_java_lang_VMClass_throwException,
	(functionptr) Java_java_lang_VMClassLoader_defineClass,
	(functionptr) Java_java_lang_VMClassLoader_resolveClass,
	(functionptr) Java_java_lang_VMClassLoader_loadClass,
	(functionptr) Java_java_lang_VMClassLoader_getPrimitiveClass,
	(functionptr) Java_java_lang_VMClassLoader_nativeGetResources,
	(functionptr) Java_java_lang_VMObject_clone,
	(functionptr) Java_java_lang_VMObject_notify,
	(functionptr) Java_java_lang_VMObject_notifyAll,
	(functionptr) Java_java_lang_VMObject_wait,
	(functionptr) Java_java_lang_VMRuntime_availableProcessors,
	(functionptr) Java_java_lang_VMRuntime_freeMemory,
	(functionptr) Java_java_lang_VMRuntime_totalMemory,
	(functionptr) Java_java_lang_VMRuntime_maxMemory,
	(functionptr) Java_java_lang_VMRuntime_gc,
	(functionptr) Java_java_lang_VMRuntime_runFinalization,
	(functionptr) Java_java_lang_VMRuntime_runFinalizationForExit,
	(functionptr) Java_java_lang_VMRuntime_traceInstructions,
	(functionptr) Java_java_lang_VMRuntime_traceMethodCalls,
	(functionptr) Java_java_lang_VMRuntime_runFinalizersOnExit,
	(functionptr) Java_java_lang_VMRuntime_exit,
	(functionptr) Java_java_lang_VMRuntime_nativeLoad,
	(functionptr) Java_java_lang_VMRuntime_mapLibraryName,
	(functionptr) Java_java_lang_VMString_intern,
	(functionptr) Java_java_lang_VMSystem_arraycopy,
	(functionptr) Java_java_lang_VMSystem_identityHashCode,
	(functionptr) Java_java_lang_VMThread_start,
	(functionptr) Java_java_lang_VMThread_interrupt,
	(functionptr) Java_java_lang_VMThread_isInterrupted,
	(functionptr) Java_java_lang_VMThread_suspend,
	(functionptr) Java_java_lang_VMThread_resume,
	(functionptr) Java_java_lang_VMThread_nativeSetPriority,
	(functionptr) Java_java_lang_VMThread_nativeStop,
	(functionptr) Java_java_lang_VMThread_currentThread,
	(functionptr) Java_java_lang_VMThread_yield,
	(functionptr) Java_java_lang_VMThread_interrupted,
	(functionptr) Java_java_lang_VMThread_holdsLock,
	(functionptr) Java_java_lang_VMThrowable_fillInStackTrace,
	(functionptr) Java_java_lang_VMThrowable_getStackTrace,
	(functionptr) Java_java_lang_reflect_Constructor_getModifiers,
	(functionptr) Java_java_lang_reflect_Constructor_constructNative,
	(functionptr) Java_java_lang_reflect_Field_getModifiers,
	(functionptr) Java_java_lang_reflect_Field_getType,
	(functionptr) Java_java_lang_reflect_Field_get,
	(functionptr) Java_java_lang_reflect_Field_getBoolean,
	(functionptr) Java_java_lang_reflect_Field_getByte,
	(functionptr) Java_java_lang_reflect_Field_getChar,
	(functionptr) Java_java_lang_reflect_Field_getShort,
	(functionptr) Java_java_lang_reflect_Field_getInt,
	(functionptr) Java_java_lang_reflect_Field_getLong,
	(functionptr) Java_java_lang_reflect_Field_getFloat,
	(functionptr) Java_java_lang_reflect_Field_getDouble,
	(functionptr) Java_java_lang_reflect_Field_set,
	(functionptr) Java_java_lang_reflect_Field_setBoolean,
	(functionptr) Java_java_lang_reflect_Field_setByte,
	(functionptr) Java_java_lang_reflect_Field_setChar,
	(functionptr) Java_java_lang_reflect_Field_setShort,
	(functionptr) Java_java_lang_reflect_Field_setInt,
	(functionptr) Java_java_lang_reflect_Field_setLong,
	(functionptr) Java_java_lang_reflect_Field_setFloat,
	(functionptr) Java_java_lang_reflect_Field_setDouble,
	(functionptr) Java_java_lang_reflect_Proxy_getProxyClass0,
	(functionptr) Java_java_lang_reflect_Proxy_getProxyData0,
	(functionptr) Java_java_lang_reflect_Proxy_generateProxyClass0,
	(functionptr) Java_java_lang_reflect_Method_getModifiers,
	(functionptr) Java_java_lang_reflect_Method_getReturnType,
	(functionptr) Java_java_lang_reflect_Method_getParameterTypes,
	(functionptr) Java_java_lang_reflect_Method_getExceptionTypes,
	(functionptr) Java_java_lang_reflect_Method_invokeNative,
};

#endif /* defined(STATIC_CLASSPATH) */


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
