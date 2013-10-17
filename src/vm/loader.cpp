/* src/vm/loader.cpp - class loader functions

   Copyright (C) 1996-2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
   Copyright (C) 2009 Theobroma Systems Ltd.

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


#include "vm/loader.hpp"
#include "config.h"

#include <cassert>
#include <cstdlib>
#include <cstddef>                      // for size_t
#include <cstdio>                       // for printf, fprintf
#include <cstring>                      // for memset, strstr, strlen, etc
#include <list>                         // for _List_iterator, etc

#include "mm/dumpmemory.hpp"            // for DumpMemory, DumpMemoryArea
#include "mm/gc.hpp"                    // for heap_hashcode

#include "native/llni.hpp"

#include "threads/mutex.hpp"            // for Mutex

#include "toolbox/hashtable.hpp"        // for hashtable, hashtable_create
#include "toolbox/list.hpp"             // for DumpList
#include "toolbox/logging.hpp"          // for log_message_class, etc

#include "vm/class.hpp"                 // for classinfo, etc
#include "vm/classcache.hpp"            // for classcache_store, etc
#include "vm/descriptor.hpp"            // for DescriptorPool, methoddesc, etc
#include "vm/exceptions.hpp"
#include "vm/descriptor.hpp"
#include "vm/field.hpp"                 // for fieldinfo, field_load
#include "vm/global.hpp"
#include "vm/globals.hpp"               // for class_java_io_Serializable, etc
#include "vm/hook.hpp"                  // for class_loaded
#include "vm/javaobjects.hpp"           // for java_lang_ClassLoader
#include "vm/linker.hpp"
#include "vm/method.hpp"                // for methodinfo, etc
#include "vm/options.hpp"               // for opt_verify, loadverbose, etc
#include "vm/package.hpp"               // for Package
#include "vm/primitive.hpp"             // for Primitive
#include "vm/references.hpp"            // for constant_FMIref, etc
#include "vm/resolve.hpp"
#include "vm/rt-timing.hpp"
#include "vm/statistics.hpp"
#include "vm/string.hpp"                // for JavaString
#include "vm/suck.hpp"                  // for suck_check_classbuffer_size, etc
#include "vm/types.hpp"                 // for u2, u1, u4, s4
#include "vm/vm.hpp"                    // for VM, vm_call_method
#include "vm/zip.hpp"                   // for hashtable_zipfile_entry

#include "vm/jit/builtin.hpp"
#include "vm/jit/stubs.hpp"             // for NativeStub

#if defined(ENABLE_JAVASE)
# include "vm/annotation.hpp"
# include "vm/stackmap.hpp"
#endif

using namespace cacao;

struct classinfo;


STAT_REGISTER_VAR(int,count_class_loads,0,"class loads","Number of class loads")

STAT_DECLARE_GROUP(memory_stat)
STAT_REGISTER_SUM_SUBGROUP(info_struct_stat,"info structs","info struct usage",memory_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,size_classinfo,0,"size classinfo","classinfo",info_struct_stat) // sizeof(classinfo)?
STAT_REGISTER_GROUP_VAR(int,size_fieldinfo,0,"size fieldinfo","fieldinfo",info_struct_stat) // sizeof(fieldinfo)?
STAT_REGISTER_GROUP_VAR(int,size_methodinfo,0,"size methodinfo","methodinfo",info_struct_stat) // sizeof(methodinfo)?


STAT_REGISTER_SUM_SUBGROUP(misc_code_stat,"misc structs","misc struct usage",memory_stat)
STAT_REGISTER_GROUP_VAR(int,count_const_pool_len,0,"const pool len","constant pool",misc_code_stat)
STAT_REGISTER_GROUP_VAR(int,count_classref_len,0,"classref len","classref",misc_code_stat)
STAT_REGISTER_GROUP_VAR(int,count_parsed_desc_len,0,"parsed desc len","parsed descriptors",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,count_vftbl_len,0,"vftbl len","vftbl",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,count_cstub_len,0,"cstub len","compiler stubs",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,size_stub_native,0,"stub native","native stubs",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,count_utf_len,0,"utf len","utf",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,count_vmcode_len,0,"vmcode len","vmcode",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,size_stack_map,0,"stack map","stack map",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,size_string,0,"string","string",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,size_threadobject,0,"threadobject","threadobject",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,size_thread_index_t,0,"size_thread_index_t","thread index",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,size_stacksize,0,"size_stacksize","stack size",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,size_lock_record,0,"size_lock_record","lock record",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,size_lock_hashtable,0,"size_lock_hashtable","lock hashtable",misc_code_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(int,size_lock_waiter,0,"size_lock_waiter","lock waiter",misc_code_stat)


/* global variables ***********************************************************/

static hashtable *hashtable_classloader;


/* loader_preinit **************************************************************

   Initializes the classpath list and loads classes required for the
   primitive table.

   NOTE: Exceptions thrown during VM initialization are caught in the
         exception functions themselves.

*******************************************************************************/

void loader_preinit(void)
{
	TRACESUBSYSTEMINITIALIZATION("loader_preinit");

	// Get current list of classpath entries.
	SuckClasspath& suckclasspath = VM::get_current()->get_suckclasspath();

	/* Initialize the monitor pointer for zip/jar file locking. */

	for (SuckClasspath::iterator it = suckclasspath.begin(); it != suckclasspath.end(); it++) {
		list_classpath_entry* lce = *it;

		if (lce->type == CLASSPATH_ARCHIVE)
			lce->mutex = new Mutex();
	}

	/* initialize classloader hashtable, 10 entries should be enough */

	hashtable_classloader = NEW(hashtable);
	hashtable_create(hashtable_classloader, 10);

	/* Load the most basic classes. */

	assert(VM::get_current()->is_initializing() == true);

	class_java_lang_Object     = load_class_bootstrap(utf8::java_lang_Object);

#if defined(ENABLE_JAVASE)
	class_java_lang_Cloneable  = load_class_bootstrap(utf8::java_lang_Cloneable);
	class_java_io_Serializable = load_class_bootstrap(utf8::java_io_Serializable);
#endif
}


/* loader_init *****************************************************************

   Loads all classes required in the VM.

   NOTE: Exceptions thrown during VM initialization are caught in the
         exception functions themselves.

*******************************************************************************/

void loader_init(void)
{
	TRACESUBSYSTEMINITIALIZATION("loader_init");

	/* Load primitive-type wrapping classes. */

	assert(VM::get_current()->is_initializing() == true);

#if defined(ENABLE_JAVASE)
	class_java_lang_Void       = load_class_bootstrap(utf8::java_lang_Void);
#endif

	class_java_lang_Boolean    = load_class_bootstrap(utf8::java_lang_Boolean);
	class_java_lang_Byte       = load_class_bootstrap(utf8::java_lang_Byte);
	class_java_lang_Character  = load_class_bootstrap(utf8::java_lang_Character);
	class_java_lang_Short      = load_class_bootstrap(utf8::java_lang_Short);
	class_java_lang_Integer    = load_class_bootstrap(utf8::java_lang_Integer);
	class_java_lang_Long       = load_class_bootstrap(utf8::java_lang_Long);
	class_java_lang_Float      = load_class_bootstrap(utf8::java_lang_Float);
	class_java_lang_Double     = load_class_bootstrap(utf8::java_lang_Double);

	/* Load important system classes. */

	class_java_lang_Class      = load_class_bootstrap(utf8::java_lang_Class);
	class_java_lang_String     = load_class_bootstrap(utf8::java_lang_String);

#if defined(ENABLE_JAVASE)
	class_java_lang_ClassLoader =
		load_class_bootstrap(utf8::java_lang_ClassLoader);

	class_java_lang_SecurityManager =
		load_class_bootstrap(utf8::java_lang_SecurityManager);
#endif

	class_java_lang_System     =
		load_class_bootstrap(Utf8String::from_utf8("java/lang/System"));

	class_java_lang_Thread     =
		load_class_bootstrap(Utf8String::from_utf8("java/lang/Thread"));

#if defined(ENABLE_JAVASE)
	class_java_lang_ThreadGroup =
		load_class_bootstrap(utf8::java_lang_ThreadGroup);
#endif

	class_java_lang_Throwable  = load_class_bootstrap(utf8::java_lang_Throwable);

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
	class_java_lang_VMSystem   =
		load_class_bootstrap(Utf8String::from_utf8("java/lang/VMSystem"));

	class_java_lang_VMThread   =
		load_class_bootstrap(Utf8String::from_utf8("java/lang/VMThread"));

	class_java_lang_VMThrowable =
		load_class_bootstrap(Utf8String::from_utf8("java/lang/VMThrowable"));
#endif

	/* Important system exceptions. */

	class_java_lang_Exception  = load_class_bootstrap(utf8::java_lang_Exception);

	class_java_lang_ClassNotFoundException =
		load_class_bootstrap(utf8::java_lang_ClassNotFoundException);

	class_java_lang_RuntimeException =
		load_class_bootstrap(utf8::java_lang_RuntimeException);

	/* Some classes which may be used often. */

#if defined(ENABLE_JAVASE)
	class_java_lang_StackTraceElement      = load_class_bootstrap(utf8::java_lang_StackTraceElement);

	class_java_lang_reflect_Constructor    = load_class_bootstrap(utf8::java_lang_reflect_Constructor);
	class_java_lang_reflect_Field          = load_class_bootstrap(utf8::java_lang_reflect_Field);
	class_java_lang_reflect_Method         = load_class_bootstrap(utf8::java_lang_reflect_Method);

# if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
	class_java_lang_reflect_VMConstructor  = load_class_bootstrap(utf8::java_lang_reflect_VMConstructor);
	class_java_lang_reflect_VMField        = load_class_bootstrap(utf8::java_lang_reflect_VMField);
	class_java_lang_reflect_VMMethod       = load_class_bootstrap(utf8::java_lang_reflect_VMMethod);
# endif

	class_java_security_PrivilegedAction   = load_class_bootstrap(Utf8String::from_utf8("java/security/PrivilegedAction"));

	class_java_util_HashMap                = load_class_bootstrap(Utf8String::from_utf8("java/util/HashMap"));
	class_java_util_Vector                 = load_class_bootstrap(utf8::java_util_Vector);

# if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
	class_sun_misc_Signal                     = load_class_bootstrap(Utf8String::from_utf8("sun/misc/Signal"));
	class_sun_reflect_MagicAccessorImpl       = load_class_bootstrap(Utf8String::from_utf8("sun/reflect/MagicAccessorImpl"));
	class_sun_reflect_MethodAccessorImpl      = load_class_bootstrap(Utf8String::from_utf8("sun/reflect/MethodAccessorImpl"));
	class_sun_reflect_ConstructorAccessorImpl = load_class_bootstrap(Utf8String::from_utf8("sun/reflect/ConstructorAccessorImpl"));
# endif

	arrayclass_java_lang_Object =
		load_class_bootstrap(Utf8String::from_utf8("[Ljava/lang/Object;"));

# if defined(ENABLE_ANNOTATIONS)
	/* needed by annotation support */
	class_sun_reflect_ConstantPool =
		load_class_bootstrap(Utf8String::from_utf8("sun/reflect/ConstantPool"));

#  if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
	/* needed by GNU Classpaths annotation support */
	class_sun_reflect_annotation_AnnotationParser =
		load_class_bootstrap(Utf8String::from_utf8("sun/reflect/annotation/AnnotationParser"));
#  endif
# endif
#endif
}


/* loader_hashtable_classloader_add ********************************************

   Adds an entry to the classloader hashtable.

   REMEMBER: Also use this to register native loaders!

*******************************************************************************/

classloader_t *loader_hashtable_classloader_add(java_handle_t *cl)
{
	hashtable_classloader_entry *cle;
	u4   key;
	u4   slot;

	if (cl == NULL)
		return NULL;

	hashtable_classloader->mutex->lock();

	LLNI_CRITICAL_START;

	/* key for entry is the hashcode of the classloader;
	   aligned to 16-byte boundaries */

	key  = heap_hashcode(LLNI_DIRECT(cl)) >> 4;
	slot = key & (hashtable_classloader->size - 1);
	cle  = (hashtable_classloader_entry*) hashtable_classloader->ptr[slot];

	/* search hashchain for existing entry */

	while (cle) {
		if (cle->object == LLNI_DIRECT(cl))
			break;

		cle = cle->hashlink;
	}

	LLNI_CRITICAL_END;

	/* if no classloader was found, we create a new entry here */

	if (cle == NULL) {
		cle = NEW(hashtable_classloader_entry);

#if defined(ENABLE_GC_CACAO)
		/* register the classloader object with the GC */

		gc_reference_register(&(cle->object), GC_REFTYPE_CLASSLOADER);
#endif

		LLNI_CRITICAL_START;

		cle->object = LLNI_DIRECT(cl);

		LLNI_CRITICAL_END;

/*#define LOADER_DEBUG_CLASSLOADER*/
#ifdef LOADER_DEBUG_CLASSLOADER
		printf("CLASSLOADER: adding new classloader entry %p for %p: ", cle, cl);
		class_print(LLNI_vftbl_direct(cl)->class);
		printf("\n");
		fflush(stdout);
#endif

		/* insert entry into hashtable */

		cle->hashlink = (hashtable_classloader_entry*) hashtable_classloader->ptr[slot];
		hashtable_classloader->ptr[slot] = cle;

		/* update number of entries */

		hashtable_classloader->entries++;
	}

	hashtable_classloader->mutex->unlock();

#if defined(ENABLE_HANDLES)
	return cle;
#else
	return cl;
#endif
}


/* loader_hashtable_classloader_find *******************************************

   Find an entry in the classloader hashtable.

*******************************************************************************/

classloader_t *loader_hashtable_classloader_find(java_handle_t *cl)
{
	hashtable_classloader_entry *cle;
	u4   key;
	u4   slot;

	if (cl == NULL)
		return NULL;

	LLNI_CRITICAL_START;

	/* key for entry is the hashcode of the classloader;
	   aligned to 16-byte boundaries */

	key  = heap_hashcode(LLNI_DIRECT(cl)) >> 4;
	slot = key & (hashtable_classloader->size - 1);
	cle  = (hashtable_classloader_entry*) hashtable_classloader->ptr[slot];

	/* search hashchain for existing entry */

	while (cle) {
		if (cle->object == LLNI_DIRECT(cl))
			break;

		cle = cle->hashlink;
	}

#ifdef LOADER_DEBUG_CLASSLOADER
	if (cle == NULL) {
		printf("CLASSLOADER: unable to find classloader entry for %p: ", cl);
		class_print(LLNI_vftbl_direct(cl)->class);
		printf("\n");
		fflush(stdout);
	}
#endif

	LLNI_CRITICAL_END;

#if defined(ENABLE_HANDLES)
	return cle;
#else
	return cl;
#endif
}


/* loader_load_all_classes *****************************************************

   Loads all classes specified in the BOOTCLASSPATH.

*******************************************************************************/

void loader_load_all_classes(void)
{
	// Get current list of classpath entries.
	SuckClasspath& suckclasspath = VM::get_current()->get_suckclasspath();

	for (SuckClasspath::iterator it = suckclasspath.begin(); it != suckclasspath.end(); it++) {
		list_classpath_entry* lce = *it;

#if defined(ENABLE_ZLIB)
		if (lce->type == CLASSPATH_ARCHIVE) {
			/* get the classes hashtable */

			ZipFile *zip = lce->zip;

			for (ZipFile::Iterator it = zip->begin(), end = zip->end(); it != end; ++it) {
				const ZipFileEntry& entry = *it;

				Utf8String filename = entry.filename;

				// skip all entries in META-INF, .properties and .png files

				if (!strncmp(filename.begin(), "META-INF", strlen("META-INF")) ||
				    strstr(filename.begin(), ".properties") ||
				    strstr(filename.begin(), ".png"))
					continue;

				// load class from bootstrap classloader

				if (!load_class_bootstrap(filename)) {
					err() << "Error loading class: '" << filename << "'" << nl;

#ifndef NDEBUG
					// print out exception and cause

					exceptions_print_current_exception();
#endif
				}
			}

		} else {
#endif
#if defined(ENABLE_ZLIB)
		}
#endif
	}
}


/* loader_skip_attribute_body **************************************************

   Skips an attribute the attribute_name_index has already been read.

   attribute_info {
       u2 attribute_name_index;
       u4 attribute_length;
       u1 info[attribute_length];
   }

*******************************************************************************/

bool loader_skip_attribute_body(classbuffer *cb)
{
	u4 attribute_length;

	if (!suck_check_classbuffer_size(cb, 4))
		return false;

	attribute_length = suck_u4(cb);

	if (!suck_check_classbuffer_size(cb, attribute_length))
		return false;

	suck_skip_nbytes(cb, attribute_length);

	return true;
}


/* load_constantpool ***********************************************************

   Loads the constantpool of a class, the entries are transformed into
   a simpler format by resolving references (a detailed overview of
   the compact structures can be found in global.h).

*******************************************************************************/

// The following structures are used to save information which cannot be
// processed during the first pass. After the complete constantpool has
// been traversed the references can be resolved (only in specific order).

/* CONSTANT_Class entries */
typedef struct forward_class {
	u2 thisindex;
	u2 name_index;
} forward_class;

/* CONSTANT_String */
typedef struct forward_string {
	u2 thisindex;
	u2 string_index;
} forward_string;

/* CONSTANT_NameAndType */
typedef struct forward_nameandtype {
	u2 thisindex;
	u2 name_index;
	u2 sig_index;
} forward_nameandtype;

/* CONSTANT_Fieldref, CONSTANT_Methodref or CONSTANT_InterfaceMethodref */
typedef struct forward_fieldmethint {
	u2 thisindex;
	u1 tag;
	u2 class_index;
	u2 nameandtype_index;
} forward_fieldmethint;

static bool load_constantpool(classbuffer *cb, DescriptorPool& descpool)
{
	classinfo *c;
	u4 idx;

	DumpList<forward_class>        forward_classes;
	DumpList<forward_string>       forward_strings;
	DumpList<forward_nameandtype>  forward_nameandtypes;
	DumpList<forward_fieldmethint> forward_fieldmethints;

	forward_class nfc;
	forward_string nfs;
	forward_nameandtype nfn;
	forward_fieldmethint nff;

	u4 cpcount;
	u1 *cptags;
	void** cpinfos;

	c = cb->clazz;

	/* number of entries in the constant_pool table plus one */
	if (!suck_check_classbuffer_size(cb, 2))
		return false;

	cpcount = c->cpcount = suck_u2(cb);

	/* allocate memory */
	cptags  = c->cptags  = MNEW(u1, cpcount);
	cpinfos = c->cpinfos = MNEW(void*, cpcount);

	if (cpcount < 1) {
		exceptions_throw_classformaterror(c, "Illegal constant pool size");
		return false;
	}

	STATISTICS(count_const_pool_len += (sizeof(u1) + sizeof(void*)) * cpcount);

	/* initialize constantpool */
	for (idx = 0; idx < cpcount; idx++) {
		cptags[idx] = CONSTANT_UNUSED;
		cpinfos[idx] = NULL;
	}


	/******* first pass *******/
	// entries which cannot be resolved now are written into
	// temporary structures and traversed again later

	idx = 1;
	while (idx < cpcount) {
		u4 t;

		/* get constant type */
		if (!suck_check_classbuffer_size(cb, 1))
			return false;

		t = suck_u1(cb);

		switch (t) {
		case CONSTANT_Class:
			nfc.thisindex = idx;

			/* reference to CONSTANT_NameAndType */
			if (!suck_check_classbuffer_size(cb, 2))
				return false;

			nfc.name_index = suck_u2(cb);

			forward_classes.push_front(nfc);

			idx++;
			break;

		case CONSTANT_String:
			nfs.thisindex = idx;

			/* reference to CONSTANT_Utf8_info with string characters */
			if (!suck_check_classbuffer_size(cb, 2))
				return false;

			nfs.string_index = suck_u2(cb);

			forward_strings.push_front(nfs);

			idx++;
			break;

		case CONSTANT_NameAndType:
			nfn.thisindex = idx;

			if (!suck_check_classbuffer_size(cb, 2 + 2))
				return false;

			/* reference to CONSTANT_Utf8_info containing simple name */
			nfn.name_index = suck_u2(cb);

			/* reference to CONSTANT_Utf8_info containing field or method
			   descriptor */
			nfn.sig_index = suck_u2(cb);

			forward_nameandtypes.push_front(nfn);

			idx++;
			break;

		case CONSTANT_Fieldref:
		case CONSTANT_Methodref:
		case CONSTANT_InterfaceMethodref:
			nff.thisindex = idx;
			/* constant type */
			nff.tag = t;

			if (!suck_check_classbuffer_size(cb, 2 + 2))
				return false;

			/* class or interface type that contains the declaration of the
			   field or method */
			nff.class_index = suck_u2(cb);

			/* name and descriptor of the field or method */
			nff.nameandtype_index = suck_u2(cb);

			forward_fieldmethints.push_front(nff);

			idx++;
			break;

		case CONSTANT_Integer: {
			constant_integer *ci = NEW(constant_integer);

			STATISTICS(count_const_pool_len += sizeof(constant_integer));

			if (!suck_check_classbuffer_size(cb, 4))
				return false;

			ci->value = suck_s4(cb);
			cptags[idx] = CONSTANT_Integer;
			cpinfos[idx] = ci;

			idx++;
			break;
		}

		case CONSTANT_Float: {
			constant_float *cf = NEW(constant_float);

			STATISTICS(count_const_pool_len += sizeof(constant_float));

			if (!suck_check_classbuffer_size(cb, 4))
				return false;

			cf->value = suck_float(cb);
			cptags[idx] = CONSTANT_Float;
			cpinfos[idx] = cf;

			idx++;
			break;
		}

		case CONSTANT_Long: {
			constant_long *cl = NEW(constant_long);

			STATISTICS(count_const_pool_len += sizeof(constant_long));

			if (!suck_check_classbuffer_size(cb, 8))
				return false;

			cl->value = suck_s8(cb);
			cptags[idx] = CONSTANT_Long;
			cpinfos[idx] = cl;
			idx += 2;
			if (idx > cpcount) {
				exceptions_throw_classformaterror(c, "Invalid constant pool entry");
				return false;
			}
			break;
		}

		case CONSTANT_Double: {
			constant_double *cd = NEW(constant_double);

			STATISTICS(count_const_pool_len += sizeof(constant_double));

			if (!suck_check_classbuffer_size(cb, 8))
				return false;

			cd->value = suck_double(cb);
			cptags[idx] = CONSTANT_Double;
			cpinfos[idx] = cd;
			idx += 2;
			if (idx > cpcount) {
				exceptions_throw_classformaterror(c, "Invalid constant pool entry");
				return false;
			}
			break;
		}

		case CONSTANT_Utf8: {
			u4 length;

			/* number of bytes in the bytes array (not string-length) */
			if (!suck_check_classbuffer_size(cb, 2))
				return false;

			length = suck_u2(cb);
			cptags[idx] = CONSTANT_Utf8;

			/* validate the string */
			if (!suck_check_classbuffer_size(cb, length))
				return false;

			Utf8String u = Utf8String::from_utf8((char *) cb->pos, length);
#ifdef ENABLE_VERIFIER

			if (opt_verify && u == NULL)
			{
				exceptions_throw_classformaterror(c, "Invalid UTF-8 string");
				return false;
			}
#endif /* ENABLE_VERIFIER */
			/* insert utf-string into the utf-symboltable */
			cpinfos[idx] = u.c_ptr();

			/* skip bytes of the string (buffer size check above) */
			suck_skip_nbytes(cb, length);
			idx++;
			break;
		}

		default:
			exceptions_throw_classformaterror(c, "Illegal constant pool type");
			return false;
		}  /* end switch */
	} /* end while */


	/* resolve entries in temporary structures */

	for (DumpList<forward_class>::iterator it = forward_classes.begin();
			it != forward_classes.end(); ++it) {

		Utf8String name = (utf*) class_getconstant(c, it->name_index, CONSTANT_Utf8);
		if (!name)
			return false;

#ifdef ENABLE_VERIFIER
		if (opt_verify && !Utf8String(name).is_valid_name()) {
			exceptions_throw_classformaterror(c, "Class reference with invalid name");
			return false;
		}
#endif /* ENABLE_VERIFIER */

		/* add all class references to the descriptor_pool */

		if (!descpool.add_class(name))
			return false;

		cptags[it->thisindex] = CONSTANT_Class;

		/* the classref is created later */
		cpinfos[it->thisindex] = name;
	}

	for (DumpList<forward_string>::iterator it = forward_strings.begin();
			it != forward_strings.end(); ++it) {

		Utf8String text = (utf*) class_getconstant(c, it->string_index, CONSTANT_Utf8);

		if (!text)
			return false;

		/* resolve utf-string */
		cptags[it->thisindex] = CONSTANT_String;
		cpinfos[it->thisindex] = text;
	}

	for (DumpList<forward_nameandtype>::iterator it = forward_nameandtypes.begin();
			it != forward_nameandtypes.end(); ++it) {

		constant_nameandtype *cn = NEW(constant_nameandtype);

		STATISTICS(count_const_pool_len += sizeof(constant_nameandtype));

		/* resolve simple name and descriptor */
		cn->name = (utf*) class_getconstant(c,
											it->name_index,
											CONSTANT_Utf8);
		if (!cn->name)
			return false;

		cn->descriptor = (utf*) class_getconstant(c,
												  it->sig_index,
												  CONSTANT_Utf8);
		if (!cn->descriptor)
			return false;

#ifdef ENABLE_VERIFIER
		if (opt_verify) {
			/* check name */
			if (!Utf8String(cn->name).is_valid_name()) {
				exceptions_throw_classformaterror(c,
				                                  "Illegal Field name \"%s\"",
				                                  cn->name.begin());

				return false;
			}

			/* disallow referencing <clinit> among others */
			if (cn->name[0] == '<' && cn->name != utf8::init) {
				exceptions_throw_classformaterror(c, "Illegal reference to special method");
				return false;
			}
		}
#endif /* ENABLE_VERIFIER */

		cptags[it->thisindex] = CONSTANT_NameAndType;
		cpinfos[it->thisindex] = cn;
	}

	for (DumpList<forward_fieldmethint>::iterator it = forward_fieldmethints.begin();
			it != forward_fieldmethints.end(); ++it) {

		constant_nameandtype *nat;
		constant_FMIref *fmi = NEW(constant_FMIref);

		STATISTICS(count_const_pool_len += sizeof(constant_FMIref));
		/* resolve simple name and descriptor */

		nat = (constant_nameandtype*) class_getconstant(c,
														it->nameandtype_index,
														CONSTANT_NameAndType);

		if (!nat)
			return false;

		/* add all descriptors in {Field,Method}ref to the descriptor_pool */

		switch (it->tag) {
		case CONSTANT_Fieldref:
			if (!descpool.add_field(nat->descriptor))
				return false;
			break;
		case CONSTANT_Methodref:
		case CONSTANT_InterfaceMethodref:
			if (descpool.add_method(nat->descriptor) == -1)
				return false;
			break;
		default:
			assert(false);
			break;
		}

		/* the classref is created later */

		fmi->p.index    = it->class_index;
		fmi->name       = nat->name;
		fmi->descriptor = nat->descriptor;

		cptags[it->thisindex]  = it->tag;
		cpinfos[it->thisindex] = fmi;
	}

	/* everything was ok */

	return true;
}


/* loader_load_attribute_signature *********************************************

   Signature_attribute {
       u2 attribute_name_index;
	   u4 atrribute_length;
	   u2 signature_index;
   }

*******************************************************************************/

#if defined(ENABLE_JAVASE)
bool loader_load_attribute_signature(classbuffer *cb, Utf8String& signature)
{
	/* get classinfo */

	classinfo *c = cb->clazz;

	/* destination must be NULL */

	if (signature != NULL) {
		exceptions_throw_classformaterror(c, "Multiple Signature attributes");
		return false;
	}

	/* check remaining bytecode */

	if (!suck_check_classbuffer_size(cb, 4 + 2))
		return false;

	/* check attribute length */

	u4 attribute_length = suck_u4(cb);

	if (attribute_length != 2) {
		exceptions_throw_classformaterror(c, "Wrong size for VALUE attribute");
		return false;
	}

	/* get signature */

	u2 signature_index = suck_u2(cb);

	signature = (utf*) class_getconstant(c, signature_index, CONSTANT_Utf8);

	return signature != NULL;
}
#endif /* defined(ENABLE_JAVASE) */


/* load_class_from_sysloader ***************************************************

   Load the class with the given name using the system class loader

   IN:
       name.............the classname

   RETURN VALUE:
       the loaded class, or
	   NULL if an exception has been thrown

*******************************************************************************/

#if defined(ENABLE_JAVASE)
classinfo *load_class_from_sysloader(Utf8String name)
{
	classloader_t *cl;
	classinfo     *c;

	cl = java_lang_ClassLoader::invoke_getSystemClassLoader();

	if (cl == NULL)
		return NULL;

	c = load_class_from_classloader(name, cl);

	return c;
}
#endif /* defined(ENABLE_JAVASE) */


// register loader real-time group
RT_REGISTER_GROUP(linker_group,"linker","linker group")

// register real-time timers
RT_REGISTER_GROUP_TIMER(checks_timer,"load","initial checks",linker_group)
RT_REGISTER_GROUP_TIMER(ndpool_timer,"load","new descriptor pool",linker_group)
RT_REGISTER_GROUP_TIMER(cpool_timer,"load","load constant pool",linker_group)
RT_REGISTER_GROUP_TIMER(setup_timer,"load","class setup",linker_group)
RT_REGISTER_GROUP_TIMER(fields_timer,"load","load fields",linker_group)
RT_REGISTER_GROUP_TIMER(methods_timer,"load","load methods",linker_group)
RT_REGISTER_GROUP_TIMER(classrefs_timer,"load","create classrefs",linker_group)
RT_REGISTER_GROUP_TIMER(descs_timer,"load","allocate descriptors",linker_group)
RT_REGISTER_GROUP_TIMER(setrefs_timer,"load","set classrefs",linker_group)
RT_REGISTER_GROUP_TIMER(parsefds_timer,"load","parse field descriptors",linker_group)
RT_REGISTER_GROUP_TIMER(parsemds_timer,"load","parse method descriptors",linker_group)
RT_REGISTER_GROUP_TIMER(parsecpool_timer,"load","parse descriptors in constant pool",linker_group)
RT_REGISTER_GROUP_TIMER(verify_timer,"load","verifier checks",linker_group)
RT_REGISTER_GROUP_TIMER(attrs_timer,"load","load attributes",linker_group)

// register classloader real-time group
RT_REGISTER_GROUP(cl_group,"classloader","classloader")

// register real-time timers
RT_REGISTER_GROUP_TIMER(cllookup_timer,"classloader","lookup in classcache",cl_group)
RT_REGISTER_GROUP_TIMER(prepare_timer,"classloader","prepare loader call",cl_group)
RT_REGISTER_GROUP_TIMER(java_timer,"classloader","loader Java code",cl_group)
RT_REGISTER_GROUP_TIMER(clcache_timer,"classloader","store in classcache",cl_group)

/* load_class_from_classloader *************************************************

   Load the class with the given name using the given user-defined class loader.

   IN:
       name.............the classname
	   cl...............user-defined class loader

   RETURN VALUE:
       the loaded class, or
	   NULL if an exception has been thrown

*******************************************************************************/

classinfo *load_class_from_classloader(Utf8String name, classloader_t *cl)
{
	java_handle_t *o;
	classinfo     *c;
	classinfo     *tmpc;
	java_handle_t *string;

	RT_TIMER_START(cllookup_timer);

	assert(name);

	/* lookup if this class has already been loaded */

	c = classcache_lookup(cl, name);

	RT_TIMER_STOPSTART(cllookup_timer,prepare_timer);

	if (c != NULL)
		return c;

	/* if other class loader than bootstrap, call it */

	if (cl != NULL) {
		methodinfo *lc;
		const char *text;
		s4          namelen;

		text    = name.begin();
		namelen = name.size();

		/* handle array classes */
		if (text[0] == '[') {
			classinfo *comp;
			Utf8String u;

			switch (text[1]) {
			case 'L':
				/* check for cases like `[L;' or `[L[I;' or `[Ljava.lang.Object' */
				if (namelen < 4 || text[2] == '[' || text[namelen - 1] != ';') {
					exceptions_throw_classnotfoundexception(name);
					return NULL;
				}

				u = Utf8String::from_utf8(text + 2, namelen - 3);

				if (!(comp = load_class_from_classloader(u, cl)))
					return NULL;

				/* create the array class */

				c = class_array_of(comp, false);

				tmpc = classcache_store(cl, c, true);

				if (tmpc == NULL) {
					/* exception, free the loaded class */
					c->state &= ~CLASS_LOADING;
					class_free(c);
				}

				return tmpc;

			case '[':
				/* load the component class */

				u = Utf8String::from_utf8(text + 1, namelen - 1);

				if (!(comp = load_class_from_classloader(u, cl)))
					return NULL;

				/* create the array class */

				c = class_array_of(comp, false);

				tmpc = classcache_store(cl, c, true);

				if (tmpc == NULL) {
					/* exception, free the loaded class */
					c->state &= ~CLASS_LOADING;
					class_free(c);
				}

				return tmpc;

			default:
				/* primitive array classes are loaded by the bootstrap loader */

				c = load_class_bootstrap(name);

				return c;
			}
		}

		LLNI_class_get(cl, c);

#if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
		/* OpenJDK uses this internal function because it's
		   synchronized. */

		lc = class_resolveclassmethod(c,
									  utf8::loadClassInternal,
									  utf8::java_lang_String__java_lang_Class,
									  NULL,
									  true);
#else
		lc = class_resolveclassmethod(c,
									  utf8::loadClass,
									  utf8::java_lang_String__java_lang_Class,
									  NULL,
									  true);
#endif

		if (lc == NULL)
			return NULL; /* exception */

		/* move return value into `o' and cast it afterwards to a classinfo* */

		string = JavaString::from_utf8_slash_to_dot(name);

		RT_TIMER_STOPSTART(prepare_timer,java_timer);

		o = vm_call_method(lc, (java_handle_t *) cl, string);

		RT_TIMER_STOPSTART(java_timer,clcache_timer);

		c = LLNI_classinfo_unwrap(o);

		if (c != NULL) {
			/* Store this class in the loaded class cache. If another
			   class with the same (initloader,name) pair has been
			   stored earlier it will be returned by classcache_store
			   In this case classcache_store may not free the class
			   because it has already been exposed to Java code which
			   may have kept references to that class. */

		    tmpc = classcache_store(cl, c, false);

			if (tmpc == NULL) {
				/* exception, free the loaded class */
				c->state &= ~CLASS_LOADING;
				class_free(c);
			}

			c = tmpc;
		}
		else {
			// Expected behavior for the classloader is to throw an exception
			// and never return NULL. If the classloader shows a different
			// behavior, we are correcting it here (see PR126).
			if (exceptions_get_exception() == NULL) {
#if !defined(NDEBUG)
				if (opt_PrintWarnings)
					log_message_utf("load_class_from_classloader: Correcting faulty classloader behavior (PR126) for ", name);
#endif
				exceptions_throw_classnotfoundexception(name);
			}
		}

		RT_TIMER_STOP(clcache_timer);


		/* SUN compatible -verbose:class output */

		if (opt_verboseclass && (c != NULL) && (c->classloader == cl)) {
			printf("[Loaded ");
			utf_display_printable_ascii_classname(name);
			printf("]\n");
		}

		return c;
	}

	c = load_class_bootstrap(name);

	return c;
}


// register boot real-time group
RT_REGISTER_GROUP(boot_group,"boot","boot group")

// register real-time timers
RT_REGISTER_GROUP_TIMER(lookup_timer,"boot","lookup in classcache",boot_group)
RT_REGISTER_GROUP_TIMER(array_timer,"boot","load array classes",boot_group)
RT_REGISTER_GROUP_TIMER(suck_timer,"boot","suck class files",boot_group)
RT_REGISTER_GROUP_TIMER(load_timer,"boot","load from class buffer",boot_group)
RT_REGISTER_GROUP_TIMER(cache_timer,"boot","store in classcache",boot_group)

/* load_class_bootstrap ********************************************************

   Load the class with the given name using the bootstrap class loader.

   IN:
       name.............the classname

   RETURN VALUE:
       loaded classinfo, or
	   NULL if an exception has been thrown

   SYNCHRONIZATION:
       load_class_bootstrap is synchronized. It can be treated as an
	   atomic operation.

*******************************************************************************/

classinfo *load_class_bootstrap(Utf8String name)
{
	classbuffer *cb;
	classinfo   *c;
	classinfo   *r;

	RT_TIMER_START(lookup_timer);

	/* for debugging */

	assert(name);

	/* lookup if this class has already been loaded */

	r = classcache_lookup(NULL, name);

	if (r != NULL) {
		RT_TIMER_STOP(lookup_timer);

		return r;
	}

	RT_TIMER_STOP(lookup_timer);

	/* create the classinfo */

	c = class_create_classinfo(name);

	/* handle array classes */

	if (name[0] == '[') {
		c = load_newly_created_array(c, NULL);
		RT_TIMER_START(array_timer);

		if (c == NULL)
			return NULL;

		assert(c->state & CLASS_LOADED);

		RT_TIMER_STOP(array_timer);

		return c;
	}
	RT_TIMER_START(suck_timer);

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_stop();

	if (opt_getloadingtime)
		loadingtime_start();
#endif

	/* load classdata, throw exception on error */

	cb = suck_start(c);

	if (cb == NULL) {
		exceptions_throw_classnotfoundexception(name);
		return NULL;
	}

	RT_TIMER_STOPSTART(suck_timer,load_timer);

	/* load the class from the buffer */

	r = load_class_from_classbuffer(cb);

	RT_TIMER_STOPSTART(load_timer,cache_timer);

	if (r == NULL) {
		/* the class could not be loaded, free the classinfo struct */

		class_free(c);
	}
	else {
		/* Store this class in the loaded class cache this step also
		   checks the loading constraints. If the class has been
		   loaded before, the earlier loaded class is returned. */

	   	classinfo *res = classcache_store(NULL, c, true);

		if (res == NULL) {
			/* exception */
			class_free(c);
		}
		else {
			// Add the package name to the boot packages.
			Package::add(c->packagename);
		}

		r = res;
	}

	RT_TIMER_STOP(cache_timer);

	/* SUN compatible -verbose:class output */

	if (opt_verboseclass && r) {
		printf("[Loaded ");
		utf_display_printable_ascii_classname(name);
		printf(" from %s]\n", cb->path);
	}

	/* free memory */

	suck_stop(cb);

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getloadingtime)
		loadingtime_stop();

	if (opt_getcompilingtime)
		compilingtime_start();
#endif

	return r;
}


/* load_class_from_classbuffer_intern ******************************************

   Loads a class from a classbuffer into a given classinfo structure.
   Super-classes are also loaded at this point and some verfication
   checks are done.

   SYNCHRONIZATION:
       This function is NOT synchronized!

*******************************************************************************/

static bool load_class_from_classbuffer_intern(classbuffer *cb)
{
	classinfo          *tc;
	Utf8String          name;
	Utf8String          supername;
	Utf8String         *interfacesnames;
	int16_t             index;

	// Create new dump memory area.
	DumpMemoryArea dma;

	RT_TIMER_START(checks_timer);

	/* Get the classbuffer's class. */

	classinfo *c = cb->clazz;

	if (!suck_check_classbuffer_size(cb, 4 + 2 + 2))
		return false;

	/* check signature */

	if (suck_u4(cb) != MAGIC) {
		exceptions_throw_classformaterror(c, "Bad magic number");
		return false;
	}

	/* check version */

	u4 mi = suck_u2(cb);
	u4 ma = suck_u2(cb);

	if (!(ma < MAJOR_VERSION || (ma == MAJOR_VERSION && mi <= MINOR_VERSION))) {
		exceptions_throw_unsupportedclassversionerror(c, ma, mi);
		return false;
	}

	RT_TIMER_STOPSTART(checks_timer,ndpool_timer);

	/* create a new descriptor pool */

	DescriptorPool descpool(c);

	RT_TIMER_STOPSTART(ndpool_timer,cpool_timer);

	/* load the constant pool */

	if (!load_constantpool(cb, descpool))
		return false;

	RT_TIMER_STOPSTART(cpool_timer,setup_timer);

	/* ACC flags */

	if (!suck_check_classbuffer_size(cb, 2))
		return false;

	/* We OR the flags here, as we set already some flags in
	   class_create_classinfo. */

	c->flags |= suck_u2(cb);

	/* check ACC flags consistency */

	if (c->flags & ACC_INTERFACE) {
		if (!(c->flags & ACC_ABSTRACT)) {
			/* We work around this because interfaces in JDK 1.1 are
			 * not declared abstract. */

			c->flags |= ACC_ABSTRACT;
		}

		if (c->flags & ACC_FINAL) {
			exceptions_throw_classformaterror(c,
											  "Illegal class modifiers: 0x%X",
											  c->flags);
			return false;
		}

		if (c->flags & ACC_SUPER) {
			c->flags &= ~ACC_SUPER; /* kjc seems to set this on interfaces */
		}
	}

	if ((c->flags & (ACC_ABSTRACT | ACC_FINAL)) == (ACC_ABSTRACT | ACC_FINAL)) {
		exceptions_throw_classformaterror(c,
										  "Illegal class modifiers: 0x%X",
										  c->flags);
		return false;
	}

	if (!suck_check_classbuffer_size(cb, 2 + 2))
		return false;

	/* This class. */

	index = suck_u2(cb);

	name = (utf *) class_getconstant(c, index, CONSTANT_Class);

	if (name == NULL)
		return false;

	if (c->name == utf8::not_named_yet) {
		/* we finally have a name for this class */
		c->name = name;
		class_set_packagename(c);
	}
	else if (name != c->name) {
		exceptions_throw_noclassdeffounderror_wrong_name(c, name);
		return false;
	}

	/* Retrieve superclass. */

	c->super = NULL;

	index = suck_u2(cb);

	if (index == 0) {
		supername = NULL;

		/* This is only allowed for java.lang.Object. */

		if (c->name != utf8::java_lang_Object) {
			exceptions_throw_classformaterror(c, "Bad superclass index");
			return false;
		}
	}
	else {
		supername = (utf *) class_getconstant(c, index, CONSTANT_Class);

		if (supername == NULL)
			return false;

		/* java.lang.Object may not have a super class. */

		if (c->name == utf8::java_lang_Object) {
			exceptions_throw_classformaterror(NULL, "java.lang.Object with superclass");
			return false;
		}

		/* Detect circularity. */

		if (supername == c->name) {
			exceptions_throw_classcircularityerror(c);
			return false;
		}

		/* Interfaces must have java.lang.Object as super class. */

		if ((c->flags & ACC_INTERFACE) && (supername != utf8::java_lang_Object)) {
			exceptions_throw_classformaterror(c, "Interfaces must have java.lang.Object as superclass");
			return false;
		}
	}

	/* Parse the super interfaces. */

	if (!suck_check_classbuffer_size(cb, 2))
		return false;

	c->interfacescount = suck_u2(cb);

	if (!suck_check_classbuffer_size(cb, 2 * c->interfacescount))
		return false;

	c->interfaces = MNEW(classinfo*, c->interfacescount);

	/* Get the names of the super interfaces. */

	interfacesnames = (Utf8String*) DumpMemory::allocate(sizeof(Utf8String) * c->interfacescount);

	for (int32_t i = 0; i < c->interfacescount; i++) {
		index = suck_u2(cb);

		Utf8String u = (utf *) class_getconstant(c, index, CONSTANT_Class);

		if (u == NULL)
			return false;

		interfacesnames[i] = u;
	}

	RT_TIMER_STOPSTART(setup_timer,fields_timer);

	/* Parse fields. */

	if (!suck_check_classbuffer_size(cb, 2))
		return false;

	c->fieldscount = suck_u2(cb);
  	c->fields      = MNEW(fieldinfo, c->fieldscount);

	MZERO(c->fields, fieldinfo, c->fieldscount);

	for (int32_t i = 0; i < c->fieldscount; i++) {
		if (!field_load(cb, &(c->fields[i]), descpool))
			return false;
	}

	RT_TIMER_STOPSTART(fields_timer,methods_timer);

	/* Parse methods. */

	if (!suck_check_classbuffer_size(cb, 2))
		return false;

	c->methodscount = suck_u2(cb);
	c->methods      = MNEW(methodinfo, c->methodscount);

	MZERO(c->methods, methodinfo, c->methodscount);

	for (int32_t i = 0; i < c->methodscount; i++) {
		if (!method_load(cb, &(c->methods[i]), descpool))
			return false;
	}

	RT_TIMER_STOPSTART(methods_timer,classrefs_timer);

	/* create the class reference table */

	c->classrefs = descpool.create_classrefs(&(c->classrefcount));

	RT_TIMER_STOPSTART(classrefs_timer,descs_timer);

	/* allocate space for the parsed descriptors */

	descpool.alloc_parsed_descriptors();

#if defined(ENABLE_STATISTICS)
	size_t classrefsize, descsize;

	descpool.get_sizes(&classrefsize, &descsize);
	count_classref_len    += classrefsize;
	count_parsed_desc_len += descsize;
#endif

	RT_TIMER_STOPSTART(descs_timer,setrefs_timer);

	/* put the classrefs in the constant pool */

	for (int32_t i = 0; i < c->cpcount; i++) {
		if (c->cptags[i] == CONSTANT_Class) {
			Utf8String name = (utf *) c->cpinfos[i];
			c->cpinfos[i] = descpool.lookup_classref(name);
		}
	}

	/* Resolve the super class. */

	if (supername != NULL) {
		constant_classref *cr = descpool.lookup_classref(supername);

		if (cr == NULL)
			return false;

		/* XXX This should be done better. */
		tc = resolve_classref_or_classinfo_eager(CLASSREF_OR_CLASSINFO(cr), false);

		if (tc == NULL) {
			resolve_handle_pending_exception(true);
			return false;
		}

		/* Interfaces are not allowed as super classes. */

		if (tc->flags & ACC_INTERFACE) {
			exceptions_throw_incompatibleclasschangeerror(c, "class %s has interface %s as super class");
			return false;
		}

		/* Don't allow extending final classes */

		if (tc->flags & ACC_FINAL) {
			exceptions_throw_verifyerror(NULL,
										 "Cannot inherit from final class");
			return false;
		}

		/* Store the super class. */

		c->super = tc;
	}

	/* Resolve the super interfaces. */

	for (int32_t i = 0; i < c->interfacescount; i++) {
		Utf8String         u  = interfacesnames[i];
		constant_classref *cr = descpool.lookup_classref(u);

		if (cr == NULL)
			return false;

		/* XXX This should be done better. */
		tc = resolve_classref_or_classinfo_eager(CLASSREF_OR_CLASSINFO(cr), false);

		if (tc == NULL) {
			resolve_handle_pending_exception(true);
			return false;
		}

		/* Detect circularity. */

		if (tc == c) {
			exceptions_throw_classcircularityerror(c);
			return false;
		}

		if (!(tc->flags & ACC_INTERFACE)) {
			exceptions_throw_incompatibleclasschangeerror(tc,
														  "Implementing class");
			return false;
		}

		/* Store the super interface. */

		c->interfaces[i] = tc;
	}

	RT_TIMER_STOPSTART(setrefs_timer,parsefds_timer);

	/* Parse the field descriptors. */

	for (int32_t i = 0; i < c->fieldscount; i++) {
		c->fields[i].parseddesc = descpool.parse_field_descriptor(c->fields[i].descriptor);
		if (!c->fields[i].parseddesc)
			return false;
	}

	RT_TIMER_STOPSTART(parsefds_timer,parsemds_timer);

	/* parse method descriptors */

	for (int32_t i = 0; i < c->methodscount; i++) {
		methodinfo *m = &c->methods[i];
		m->parseddesc = descpool.parse_method_descriptor(m->descriptor, m->flags, class_get_self_classref(m->clazz));
		if (!m->parseddesc)
			return false;

		for (int32_t j = 0; j < m->rawexceptiontablelength; j++) {
			if (!m->rawexceptiontable[j].catchtype.any)
				continue;

			if ((m->rawexceptiontable[j].catchtype.ref =
				descpool.lookup_classref((utf *) m->rawexceptiontable[j].catchtype.any)) == NULL)
				return false;
		}

		for (int32_t j = 0; j < m->thrownexceptionscount; j++) {
			if (!m->thrownexceptions[j].any)
				continue;

			if ((m->thrownexceptions[j].ref =
				descpool.lookup_classref((utf *) m->thrownexceptions[j].any)) == NULL)
				return false;
		}
	}

	RT_TIMER_STOPSTART(parsemds_timer,parsecpool_timer);

	/* parse the loaded descriptors */

	for (int32_t i = 0; i < c->cpcount; i++) {
		constant_FMIref *fmi;
		s4               index;

		switch (c->cptags[i]) {
		case CONSTANT_Fieldref:
			fmi = (constant_FMIref *) c->cpinfos[i];
			fmi->parseddesc.fd = descpool.parse_field_descriptor(fmi->descriptor);
			if (!fmi->parseddesc.fd)
				return false;

			index = fmi->p.index;
			fmi->p.classref =
				(constant_classref *) class_getconstant(c, index,
														CONSTANT_Class);
			if (!fmi->p.classref)
				return false;
			break;
		case CONSTANT_Methodref:
		case CONSTANT_InterfaceMethodref:
			fmi = (constant_FMIref *) c->cpinfos[i];
			index = fmi->p.index;
			fmi->p.classref =
				(constant_classref *) class_getconstant(c, index,
														CONSTANT_Class);
			if (!fmi->p.classref)
				return false;
			fmi->parseddesc.md = descpool.parse_method_descriptor(fmi->descriptor,
			                                                      ACC_UNDEF,
			                                                      fmi->p.classref);
			if (!fmi->parseddesc.md)
				return false;
			break;
		}
	}

	RT_TIMER_STOPSTART(parsecpool_timer,verify_timer);

#ifdef ENABLE_VERIFIER
	/* Check if all fields and methods can be uniquely
	 * identified by (name,descriptor). */

	if (opt_verify) {
		/* We use a hash table here to avoid making the
		 * average case quadratic in # of methods, fields.
		 */
		static int shift = 0;
		u2 *hashtab;
		u2 *next; /* for chaining colliding hash entries */
		int32_t len;
		int32_t hashlen;
		u2 index;
		u2 old;

		/* Allocate hashtable */
		len = c->methodscount;
		if (len < c->fieldscount) len = c->fieldscount;
		hashlen = 5 * len;
		hashtab = MNEW(u2,(hashlen + len));
		next = hashtab + hashlen;

		/* Determine bitshift (to get good hash values) */
		if (!shift) {
			len = Utf8String::sizeof_utf;
			while (len) {
				len >>= 1;
				shift++;
			}
		}

		/* Check fields */
		memset(hashtab, 0, sizeof(u2) * (hashlen + len));

		for (int32_t i = 0; i < c->fieldscount; ++i) {
			fieldinfo *fi = c->fields + i;

			/* It's ok if we lose bits here */
			index = ((((size_t) fi->name.c_ptr()) +
					  ((size_t) fi->descriptor.c_ptr())) >> shift) % hashlen;

			if ((old = hashtab[index])) {
				old--;
				next[i] = old;
				do {
					if (c->fields[old].name == fi->name &&
						c->fields[old].descriptor == fi->descriptor) {
						exceptions_throw_classformaterror(c, "Repetitive field name/signature");
						return false;
					}
				} while ((old = next[old]));
			}
			hashtab[index] = i + 1;
		}

		/* Check methods */
		memset(hashtab, 0, sizeof(u2) * (hashlen + hashlen/5));

		for (int32_t i = 0; i < c->methodscount; ++i) {
			methodinfo *mi = c->methods + i;

			/* It's ok if we lose bits here */
			index = ((((size_t) mi->name.c_ptr()) +
					  ((size_t) mi->descriptor.c_ptr())) >> shift) % hashlen;

			if ((old = hashtab[index])) {
				old--;
				next[i] = old;
				do {
					if (c->methods[old].name == mi->name &&
						c->methods[old].descriptor == mi->descriptor) {
						exceptions_throw_classformaterror(c, "Repetitive method name/signature");
						return false;
					}
				} while ((old = next[old]));
			}
			hashtab[index] = i + 1;
		}

		MFREE(hashtab, u2, (hashlen + len));
	}
#endif /* ENABLE_VERIFIER */

	RT_TIMER_STOPSTART(verify_timer,attrs_timer);

	STATISTICS(size_classinfo  += sizeof(classinfo*) * c->interfacescount);
	STATISTICS(size_fieldinfo  += sizeof(fieldinfo)  * c->fieldscount);
	STATISTICS(size_methodinfo += sizeof(methodinfo) * c->methodscount);

	/* load attribute structures */

	if (!class_load_attributes(cb))
		return false;

	/* Pre Java 1.5 version don't check this. This implementation is
	   like Java 1.5 do it: for class file version 45.3 we don't check
	   it, older versions are checked. */

	if (((ma == 45) && (mi > 3)) || (ma > 45)) {
		/* check if all data has been read */
		s4 classdata_left = ((cb->data + cb->size) - cb->pos);

		if (classdata_left > 0) {
			exceptions_throw_classformaterror(c, "Extra bytes at the end of class file");
			return false;
		}
	}

	RT_TIMER_STOP(attrs_timer);

	return true;
}


/* load_class_from_classbuffer *************************************************

   Convenience wrapper for load_class_from_classbuffer.

   SYNCHRONIZATION:
       This function is NOT synchronized!

*******************************************************************************/

classinfo *load_class_from_classbuffer(classbuffer *cb)
{
	classinfo *c;
	bool       result;

	/* Get the classbuffer's class. */

	c = cb->clazz;

	/* Check if the class is already loaded. */

	if (c->state & CLASS_LOADED)
		return c;

	STATISTICS(count_class_loads++);

#if !defined(NDEBUG)
	if (loadverbose)
		log_message_class("Loading class: ", c);
#endif

	/* Class is currently loading. */

	c->state |= CLASS_LOADING;

	/* Parse the classbuffer. */

	result = load_class_from_classbuffer_intern(cb);

	/* An error occurred. */

	if (result == false) {
		/* Revert loading state. */

		c->state = (c->state & ~CLASS_LOADING);

		return NULL;
	}

	/* Revert loading state and set loaded. */

	c->state = (c->state & ~CLASS_LOADING) | CLASS_LOADED;

#if !defined(NDEBUG)
	if (loadverbose)
		log_message_class("Loading done class: ", c);
#endif

	// Hook point just after a class was loaded.
	Hook::class_loaded(c);

	return c;
}


/* load_newly_created_array ****************************************************

   Load a newly created array class.

	RETURN VALUE:
	    c....................the array class C has been loaded
		other classinfo......the array class was found in the class cache,
		                     C has been freed
	    NULL.................an exception has been thrown

	Note:
		This is an internal function. Do not use it unless you know exactly
		what you are doing!

		Use one of the load_class_... functions for general array class loading.

*******************************************************************************/

classinfo *load_newly_created_array(classinfo *c, classloader_t *loader)
{
	classinfo         *comp = NULL;
	methodinfo        *clone;
	methoddesc        *clonedesc;
	constant_classref *classrefs;
	const char        *text;
	s4                 namelen;
	Utf8String         u;

	Utf8String name = c->name;

	text    = name.begin();
	namelen = name.size();

	/* Check array class name */

	if ((namelen < 2) || (text[0] != '[')) {
		exceptions_throw_classnotfoundexception(c->name);
		return NULL;
	}

	/* Check the element type */

	switch (text[1]) {
	case '[':
		/* c is an array of arrays. We have to create the component class. */

		comp = load_class_from_classloader(name.substring(1), loader);

		if (comp == NULL)
			return NULL;

		assert(comp->state & CLASS_LOADED);

		/* the array's flags are that of the component class */
		c->flags = (comp->flags & ~ACC_INTERFACE) | ACC_FINAL | ACC_ABSTRACT;
		c->classloader = comp->classloader;
		break;

	case 'L':
		/* c is an array of objects. */

		/* check for cases like `[L;' or `[L[I;' or `[Ljava.lang.Object' */
		if ((namelen < 4) || (text[2] == '[') || (text[namelen - 1] != ';')) {
			exceptions_throw_classnotfoundexception(c->name);
			return NULL;
		}

		u  = Utf8String::from_utf8(text + 2, namelen - 3);

		if (!(comp = load_class_from_classloader(u, loader)))
			return NULL;

		assert(comp->state & CLASS_LOADED);

		/* the array's flags are that of the component class */
		c->flags = (comp->flags & ~ACC_INTERFACE) | ACC_FINAL | ACC_ABSTRACT;
		c->classloader = comp->classloader;
		break;

	default:
		/* c is an array of a primitive type */

		/* check for cases like `[II' and whether the character is a
		   valid primitive type */

		if ((namelen > 2) || (Primitive::get_class_by_char(text[1]) == NULL)) {
			exceptions_throw_classnotfoundexception(c->name);
			return NULL;
		}

		/* the accessibility of the array class is public (VM Spec 5.3.3) */
		c->flags = ACC_PUBLIC | ACC_FINAL | ACC_ABSTRACT;
		c->classloader = NULL;
	}

	assert(class_java_lang_Object);
#if defined(ENABLE_JAVASE)
	assert(class_java_lang_Cloneable);
	assert(class_java_io_Serializable);
#endif

	/* Setup the array class. */

	c->super = class_java_lang_Object;

#if defined(ENABLE_JAVASE)

	c->interfacescount = 2;
    c->interfaces      = MNEW(classinfo*, 2);
	c->interfaces[0]   = class_java_lang_Cloneable;
	c->interfaces[1]   = class_java_io_Serializable;

#elif defined(ENABLE_JAVAME_CLDC1_1)

	c->interfacescount = 0;
	c->interfaces      = NULL;

#else
# error unknow Java configuration
#endif

	c->methodscount = 1;
	c->methods      = MNEW(methodinfo, c->methodscount);

	MZERO(c->methods, methodinfo, c->methodscount);

	classrefs = MNEW(constant_classref, 2);

	CLASSREF_INIT(classrefs[0], c, c->name);
	CLASSREF_INIT(classrefs[1], c, utf8::java_lang_Object);

	/* create descriptor for clone method */
	/* we need one paramslot which is reserved for the 'this' parameter */
	clonedesc = NEW(methoddesc);
	clonedesc->returntype.type     = TYPE_ADR;
	clonedesc->returntype.classref = classrefs + 1;
	clonedesc->returntype.arraydim = 0;
	// initialize params to "empty", add real params below in
	// params_from_paramtypes
	clonedesc->paramcount = 0;
	clonedesc->paramslots = 0;
	clonedesc->paramtypes[0].classref = classrefs + 0;
	clonedesc->params = NULL;
	clonedesc->pool_lock = NULL;

	/* create methodinfo */

	clone = c->methods;
	MSET(clone, 0, methodinfo, 1);

	/* ATTENTION: if you delete the ACC_NATIVE below, set
	   clone->maxlocals=1 (interpreter related) */

	clone->mutex      = new Mutex();
	clone->flags      = ACC_PUBLIC | ACC_NATIVE;
	clone->name       = utf8::clone;
	clone->descriptor = utf8::void__java_lang_Object;
	clone->parseddesc = clonedesc;
	clone->clazz      = c;

	/* parse the descriptor to get the register allocation */

	clonedesc->params_from_paramtypes(clone->flags);

	clone->code = NativeStub::generate(clone, BUILTIN_clone);

	/* XXX: field: length? */

	/* array classes are not loaded from class files */

	c->state          |= CLASS_LOADED;
	c->classrefs      = classrefs;
	c->classrefcount  = 1;

	/* insert class into the loaded class cache */
	/* XXX free classinfo if NULL returned? */

	return classcache_store(loader, c, true);
}


/* loader_close ****************************************************************

   Frees all resources.

*******************************************************************************/

void loader_close(void)
{
	/* empty */
}


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
 * vim:noexpandtab:sw=4:ts=4:
 */
