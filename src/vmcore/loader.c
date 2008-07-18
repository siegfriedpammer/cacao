/* src/vmcore/loader.c - class loader functions

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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "native/llni.h"

#include "threads/lock-common.h"

#include "toolbox/hashtable.h"
#include "toolbox/logging.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/package.hpp"
#include "vm/primitive.hpp"
#include "vm/resolve.h"
#include "vm/stringlocal.h"
#include "vm/vm.hpp"

#include "vm/jit_interface.h"

#if defined(ENABLE_JAVASE)
# include "vmcore/annotation.h"
# include "vmcore/stackmap.h"
#endif

#include "vmcore/classcache.h"
#include "vmcore/field.h"
#include "vmcore/globals.hpp"
#include "vmcore/linker.h"
#include "vmcore/loader.h"
#include "vmcore/method.h"
#include "vmcore/options.h"
#include "vmcore/rt-timing.h"

#if defined(ENABLE_STATISTICS)
# include "vmcore/statistics.h"
#endif

#include "vmcore/suck.h"

#if defined(ENABLE_ZLIB)
# include "vmcore/zip.h"
#endif

#if defined(ENABLE_JVMTI)
# include "native/jvmti/cacaodbg.h"
#endif


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
#if defined(ENABLE_THREADS)
	list_classpath_entry *lce;
#endif

	TRACESUBSYSTEMINITIALIZATION("loader_preinit");

#if defined(ENABLE_THREADS)
	/* Initialize the monitor pointer for zip/jar file locking. */

	for (lce = list_first(list_classpath_entries); lce != NULL;
		 lce = list_next(list_classpath_entries, lce)) {
		if (lce->type == CLASSPATH_ARCHIVE)
			LOCK_INIT_OBJECT_LOCK(lce);
	}
#endif

	/* initialize classloader hashtable, 10 entries should be enough */

	hashtable_classloader = NEW(hashtable);
	hashtable_create(hashtable_classloader, 10);

	/* Load the most basic classes. */

	assert(VM_is_initializing() == true);

	class_java_lang_Object     = load_class_bootstrap(utf_java_lang_Object);

#if defined(ENABLE_JAVASE)
	class_java_lang_Cloneable  = load_class_bootstrap(utf_java_lang_Cloneable);
	class_java_io_Serializable = load_class_bootstrap(utf_java_io_Serializable);
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

	assert(VM_is_initializing() == true);

#if defined(ENABLE_JAVASE)
	class_java_lang_Void       = load_class_bootstrap(utf_java_lang_Void);
#endif

	class_java_lang_Boolean    = load_class_bootstrap(utf_java_lang_Boolean);
	class_java_lang_Byte       = load_class_bootstrap(utf_java_lang_Byte);
	class_java_lang_Character  = load_class_bootstrap(utf_java_lang_Character);
	class_java_lang_Short      = load_class_bootstrap(utf_java_lang_Short);
	class_java_lang_Integer    = load_class_bootstrap(utf_java_lang_Integer);
	class_java_lang_Long       = load_class_bootstrap(utf_java_lang_Long);
	class_java_lang_Float      = load_class_bootstrap(utf_java_lang_Float);
	class_java_lang_Double     = load_class_bootstrap(utf_java_lang_Double);

	/* Load important system classes. */

	class_java_lang_Class      = load_class_bootstrap(utf_java_lang_Class);
	class_java_lang_String     = load_class_bootstrap(utf_java_lang_String);

#if defined(ENABLE_JAVASE)
	class_java_lang_ClassLoader =
		load_class_bootstrap(utf_java_lang_ClassLoader);

	class_java_lang_SecurityManager =
		load_class_bootstrap(utf_java_lang_SecurityManager);
#endif

	class_java_lang_System     =
		load_class_bootstrap(utf_new_char("java/lang/System"));

	class_java_lang_Thread     =
		load_class_bootstrap(utf_new_char("java/lang/Thread"));

#if defined(ENABLE_JAVASE)
	class_java_lang_ThreadGroup =
		load_class_bootstrap(utf_java_lang_ThreadGroup);
#endif

	class_java_lang_Throwable  = load_class_bootstrap(utf_java_lang_Throwable);

#if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
	class_java_lang_VMSystem   =
		load_class_bootstrap(utf_new_char("java/lang/VMSystem"));

	class_java_lang_VMThread   =
		load_class_bootstrap(utf_new_char("java/lang/VMThread"));

	class_java_lang_VMThrowable =
		load_class_bootstrap(utf_new_char("java/lang/VMThrowable"));
#endif

	/* Important system exceptions. */

	class_java_lang_Exception  = load_class_bootstrap(utf_java_lang_Exception);

	class_java_lang_ClassNotFoundException =
		load_class_bootstrap(utf_java_lang_ClassNotFoundException);

	class_java_lang_RuntimeException =
		load_class_bootstrap(utf_java_lang_RuntimeException);

	/* Some classes which may be used often. */

#if defined(ENABLE_JAVASE)
	class_java_lang_StackTraceElement      = load_class_bootstrap(utf_java_lang_StackTraceElement);

	class_java_lang_reflect_Constructor    = load_class_bootstrap(utf_java_lang_reflect_Constructor);
	class_java_lang_reflect_Field          = load_class_bootstrap(utf_java_lang_reflect_Field);
	class_java_lang_reflect_Method         = load_class_bootstrap(utf_java_lang_reflect_Method);

# if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
	class_java_lang_reflect_VMConstructor  = load_class_bootstrap(utf_java_lang_reflect_VMConstructor);
	class_java_lang_reflect_VMField        = load_class_bootstrap(utf_java_lang_reflect_VMField);
	class_java_lang_reflect_VMMethod       = load_class_bootstrap(utf_java_lang_reflect_VMMethod);
# endif

	class_java_security_PrivilegedAction   = load_class_bootstrap(utf_new_char("java/security/PrivilegedAction"));

	class_java_util_HashMap                = load_class_bootstrap(utf_new_char("java/util/HashMap"));
	class_java_util_Vector                 = load_class_bootstrap(utf_java_util_Vector);

# if defined(WITH_JAVA_RUNTIME_LIBRARY_OPENJDK)
	class_sun_reflect_MagicAccessorImpl =
		load_class_bootstrap(utf_new_char("sun/reflect/MagicAccessorImpl"));
# endif

	arrayclass_java_lang_Object =
		load_class_bootstrap(utf_new_char("[Ljava/lang/Object;"));

# if defined(ENABLE_ANNOTATIONS)
	/* needed by annotation support */
	class_sun_reflect_ConstantPool =
		load_class_bootstrap(utf_new_char("sun/reflect/ConstantPool"));

#  if defined(WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH)
	/* needed by GNU Classpaths annotation support */
	class_sun_reflect_annotation_AnnotationParser =
		load_class_bootstrap(utf_new_char("sun/reflect/annotation/AnnotationParser"));
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

	LOCK_MONITOR_ENTER(hashtable_classloader->header);

	LLNI_CRITICAL_START;

	/* key for entry is the hashcode of the classloader;
	   aligned to 16-byte boundaries */

	key  = heap_hashcode(LLNI_DIRECT(cl)) >> 4;
	slot = key & (hashtable_classloader->size - 1);
	cle  = hashtable_classloader->ptr[slot];

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

		cle->hashlink = hashtable_classloader->ptr[slot];
		hashtable_classloader->ptr[slot] = cle;

		/* update number of entries */

		hashtable_classloader->entries++;
	}


	LOCK_MONITOR_EXIT(hashtable_classloader->header);

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
	cle  = hashtable_classloader->ptr[slot];

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
	list_classpath_entry    *lce;
#if defined(ENABLE_ZLIB)
	hashtable               *ht;
	s4                       slot;
	hashtable_zipfile_entry *htzfe;
	utf                     *u;
#endif

	for (lce = list_first(list_classpath_entries); lce != NULL;
		 lce = list_next(list_classpath_entries, lce)) {
#if defined(ENABLE_ZLIB)
		if (lce->type == CLASSPATH_ARCHIVE) {
			/* get the classes hashtable */

			ht = lce->htclasses;

			for (slot = 0; slot < ht->size; slot++) {
				htzfe = (hashtable_zipfile_entry *) ht->ptr[slot];

				for (; htzfe; htzfe = htzfe->hashlink) {
					u = htzfe->filename;

					/* skip all entries in META-INF and .properties,
                       .png files */

					if (!strncmp(u->text, "META-INF", strlen("META-INF")) ||
						strstr(u->text, ".properties") ||
						strstr(u->text, ".png"))
						continue;

					/* load class from bootstrap classloader */

					if (!load_class_bootstrap(u)) {
						fprintf(stderr, "Error loading: ");
						utf_fprint_printable_ascii_classname(stderr, u);
						fprintf(stderr, "\n");

#if !defined(NDEBUG)
						/* print out exception and cause */

						exceptions_print_current_exception();
#endif
					}
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

static bool load_constantpool(classbuffer *cb, descriptor_pool *descpool)
{

	/* The following structures are used to save information which cannot be 
	   processed during the first pass. After the complete constantpool has 
	   been traversed the references can be resolved. 
	   (only in specific order)						   */
	
	/* CONSTANT_Class entries */
	typedef struct forward_class {
		struct forward_class *next;
		u2 thisindex;
		u2 name_index;
	} forward_class;

	/* CONSTANT_String */
	typedef struct forward_string {
		struct forward_string *next;
		u2 thisindex;
		u2 string_index;
	} forward_string;

	/* CONSTANT_NameAndType */
	typedef struct forward_nameandtype {
		struct forward_nameandtype *next;
		u2 thisindex;
		u2 name_index;
		u2 sig_index;
	} forward_nameandtype;

	/* CONSTANT_Fieldref, CONSTANT_Methodref or CONSTANT_InterfaceMethodref */
	typedef struct forward_fieldmethint {
		struct forward_fieldmethint *next;
		u2 thisindex;
		u1 tag;
		u2 class_index;
		u2 nameandtype_index;
	} forward_fieldmethint;


	classinfo *c;
	u4 idx;

	forward_class *forward_classes = NULL;
	forward_string *forward_strings = NULL;
	forward_nameandtype *forward_nameandtypes = NULL;
	forward_fieldmethint *forward_fieldmethints = NULL;

	forward_class *nfc;
	forward_string *nfs;
	forward_nameandtype *nfn;
	forward_fieldmethint *nff;

	u4 cpcount;
	u1 *cptags;
	voidptr *cpinfos;

	c = cb->clazz;

	/* number of entries in the constant_pool table plus one */
	if (!suck_check_classbuffer_size(cb, 2))
		return false;

	cpcount = c->cpcount = suck_u2(cb);

	/* allocate memory */
	cptags  = c->cptags  = MNEW(u1, cpcount);
	cpinfos = c->cpinfos = MNEW(voidptr, cpcount);

	if (cpcount < 1) {
		exceptions_throw_classformaterror(c, "Illegal constant pool size");
		return false;
	}
	
#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_const_pool_len += (sizeof(u1) + sizeof(voidptr)) * cpcount;
#endif
	
	/* initialize constantpool */
	for (idx = 0; idx < cpcount; idx++) {
		cptags[idx] = CONSTANT_UNUSED;
		cpinfos[idx] = NULL;
	}

			
	/******* first pass *******/
	/* entries which cannot be resolved now are written into 
	   temporary structures and traversed again later        */
		   
	idx = 1;
	while (idx < cpcount) {
		u4 t;

		/* get constant type */
		if (!suck_check_classbuffer_size(cb, 1))
			return false;

		t = suck_u1(cb);

		switch (t) {
		case CONSTANT_Class:
			nfc = DNEW(forward_class);

			nfc->next = forward_classes;
			forward_classes = nfc;

			nfc->thisindex = idx;
			/* reference to CONSTANT_NameAndType */
			if (!suck_check_classbuffer_size(cb, 2))
				return false;

			nfc->name_index = suck_u2(cb);

			idx++;
			break;
			
		case CONSTANT_String:
			nfs = DNEW(forward_string);
				
			nfs->next = forward_strings;
			forward_strings = nfs;
				
			nfs->thisindex = idx;

			/* reference to CONSTANT_Utf8_info with string characters */
			if (!suck_check_classbuffer_size(cb, 2))
				return false;

			nfs->string_index = suck_u2(cb);
				
			idx++;
			break;

		case CONSTANT_NameAndType:
			nfn = DNEW(forward_nameandtype);
				
			nfn->next = forward_nameandtypes;
			forward_nameandtypes = nfn;
				
			nfn->thisindex = idx;

			if (!suck_check_classbuffer_size(cb, 2 + 2))
				return false;

			/* reference to CONSTANT_Utf8_info containing simple name */
			nfn->name_index = suck_u2(cb);

			/* reference to CONSTANT_Utf8_info containing field or method
			   descriptor */
			nfn->sig_index = suck_u2(cb);
				
			idx++;
			break;

		case CONSTANT_Fieldref:
		case CONSTANT_Methodref:
		case CONSTANT_InterfaceMethodref:
			nff = DNEW(forward_fieldmethint);
			
			nff->next = forward_fieldmethints;
			forward_fieldmethints = nff;

			nff->thisindex = idx;
			/* constant type */
			nff->tag = t;

			if (!suck_check_classbuffer_size(cb, 2 + 2))
				return false;

			/* class or interface type that contains the declaration of the
			   field or method */
			nff->class_index = suck_u2(cb);

			/* name and descriptor of the field or method */
			nff->nameandtype_index = suck_u2(cb);

			idx++;
			break;
				
		case CONSTANT_Integer: {
			constant_integer *ci = NEW(constant_integer);

#if defined(ENABLE_STATISTICS)
			if (opt_stat)
				count_const_pool_len += sizeof(constant_integer);
#endif

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

#if defined(ENABLE_STATISTICS)
			if (opt_stat)
				count_const_pool_len += sizeof(constant_float);
#endif

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
					
#if defined(ENABLE_STATISTICS)
			if (opt_stat)
				count_const_pool_len += sizeof(constant_long);
#endif

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
				
#if defined(ENABLE_STATISTICS)
			if (opt_stat)
				count_const_pool_len += sizeof(constant_double);
#endif

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

#ifdef ENABLE_VERIFIER
			if (opt_verify &&
				!is_valid_utf((char *) cb->pos, (char *) (cb->pos + length))) 
			{
				exceptions_throw_classformaterror(c, "Invalid UTF-8 string");
				return false;
			}
#endif /* ENABLE_VERIFIER */
			/* insert utf-string into the utf-symboltable */
			cpinfos[idx] = utf_new((char *) cb->pos, length);

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

	while (forward_classes) {
		utf *name =
			class_getconstant(c, forward_classes->name_index, CONSTANT_Utf8);
		if (!name)
			return false;

#ifdef ENABLE_VERIFIER
		if (opt_verify && !is_valid_name_utf(name)) {
			exceptions_throw_classformaterror(c, "Class reference with invalid name");
			return false;
		}
#endif /* ENABLE_VERIFIER */

		/* add all class references to the descriptor_pool */

		if (!descriptor_pool_add_class(descpool, name))
			return false;

		cptags[forward_classes->thisindex] = CONSTANT_Class;

		/* the classref is created later */
		cpinfos[forward_classes->thisindex] = name;

		nfc = forward_classes;
		forward_classes = forward_classes->next;
	}

	while (forward_strings) {
		utf *text =
			class_getconstant(c, forward_strings->string_index, CONSTANT_Utf8);
		if (!text)
			return false;

		/* resolve utf-string */
		cptags[forward_strings->thisindex] = CONSTANT_String;
		cpinfos[forward_strings->thisindex] = text;
		
		nfs = forward_strings;
		forward_strings = forward_strings->next;
	}

	while (forward_nameandtypes) {
		constant_nameandtype *cn = NEW(constant_nameandtype);	

#if defined(ENABLE_STATISTICS)
		if (opt_stat)
			count_const_pool_len += sizeof(constant_nameandtype);
#endif

		/* resolve simple name and descriptor */
		cn->name = class_getconstant(c,
									 forward_nameandtypes->name_index,
									 CONSTANT_Utf8);
		if (!cn->name)
			return false;

		cn->descriptor = class_getconstant(c,
										   forward_nameandtypes->sig_index,
										   CONSTANT_Utf8);
		if (!cn->descriptor)
			return false;

#ifdef ENABLE_VERIFIER
		if (opt_verify) {
			/* check name */
			if (!is_valid_name_utf(cn->name)) {
				exceptions_throw_classformaterror(c,
												  "Illegal Field name \"%s\"",
												  cn->name->text);

				return false;
			}

			/* disallow referencing <clinit> among others */
			if (cn->name->text[0] == '<' && cn->name != utf_init) {
				exceptions_throw_classformaterror(c, "Illegal reference to special method");
				return false;
			}
		}
#endif /* ENABLE_VERIFIER */

		cptags[forward_nameandtypes->thisindex] = CONSTANT_NameAndType;
		cpinfos[forward_nameandtypes->thisindex] = cn;

		nfn = forward_nameandtypes;
		forward_nameandtypes = forward_nameandtypes->next;
	}

	while (forward_fieldmethints) {
		constant_nameandtype *nat;
		constant_FMIref *fmi = NEW(constant_FMIref);

#if defined(ENABLE_STATISTICS)
		if (opt_stat)
			count_const_pool_len += sizeof(constant_FMIref);
#endif
		/* resolve simple name and descriptor */

		nat = class_getconstant(c,
								forward_fieldmethints->nameandtype_index,
								CONSTANT_NameAndType);
		if (!nat)
			return false;

		/* add all descriptors in {Field,Method}ref to the descriptor_pool */

		if (!descriptor_pool_add(descpool, nat->descriptor, NULL))
			return false;

		/* the classref is created later */

		fmi->p.index = forward_fieldmethints->class_index;
		fmi->name = nat->name;
		fmi->descriptor = nat->descriptor;

		cptags[forward_fieldmethints->thisindex] = forward_fieldmethints->tag;
		cpinfos[forward_fieldmethints->thisindex] = fmi;
	
		nff = forward_fieldmethints;
		forward_fieldmethints = forward_fieldmethints->next;
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
bool loader_load_attribute_signature(classbuffer *cb, utf **signature)
{
	classinfo *c;
	u4         attribute_length;
	u2         signature_index;

	/* get classinfo */

	c = cb->clazz;

	/* check remaining bytecode */

	if (!suck_check_classbuffer_size(cb, 4 + 2))
		return false;

	/* check attribute length */

	attribute_length = suck_u4(cb);

	if (attribute_length != 2) {
		exceptions_throw_classformaterror(c, "Wrong size for VALUE attribute");
		return false;
	}

	if (*signature != NULL) {
		exceptions_throw_classformaterror(c, "Multiple Signature attributes");
		return false;
	}

	/* get signature */

	signature_index = suck_u2(cb);

	if (!(*signature = class_getconstant(c, signature_index, CONSTANT_Utf8)))
		return false;

	return true;
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

classinfo *load_class_from_sysloader(utf *name)
{
	methodinfo    *m;
	java_handle_t *clo;
	classloader_t *cl;
	classinfo     *c;

	assert(class_java_lang_Object);
	assert(class_java_lang_ClassLoader);
	assert(class_java_lang_ClassLoader->state & CLASS_LINKED);
	
	m = class_resolveclassmethod(class_java_lang_ClassLoader,
								 utf_getSystemClassLoader,
								 utf_void__java_lang_ClassLoader,
								 class_java_lang_Object,
								 false);

	if (!m)
		return false;

	clo = vm_call_method(m, NULL);

	if (!clo)
		return false;

	cl = loader_hashtable_classloader_add(clo);

	c = load_class_from_classloader(name, cl);

	return c;
}


/* load_class_from_classloader *************************************************

   Load the class with the given name using the given user-defined class loader.

   IN:
       name.............the classname
	   cl...............user-defined class loader
	   
   RETURN VALUE:
       the loaded class, or
	   NULL if an exception has been thrown

*******************************************************************************/

classinfo *load_class_from_classloader(utf *name, classloader_t *cl)
{
	java_handle_t *o;
	classinfo     *c;
	classinfo     *tmpc;
	java_handle_t *string;
#if defined(ENABLE_RT_TIMING)
	struct timespec time_start, time_lookup, time_prepare, time_java, 
					time_cache;
#endif

	RT_TIMING_GET_TIME(time_start);

	assert(name);

	/* lookup if this class has already been loaded */

	c = classcache_lookup(cl, name);

	RT_TIMING_GET_TIME(time_lookup);
	RT_TIMING_TIME_DIFF(time_start,time_lookup,RT_TIMING_LOAD_CL_LOOKUP);

	if (c != NULL)
		return c;

	/* if other class loader than bootstrap, call it */

	if (cl != NULL) {
		methodinfo *lc;
		char       *text;
		s4          namelen;

		text = name->text;
		namelen = name->blength;

		/* handle array classes */
		if (text[0] == '[') {
			classinfo *comp;
			utf       *u;

			switch (text[1]) {
			case 'L':
				/* check for cases like `[L;' or `[L[I;' or `[Ljava.lang.Object' */
				if (namelen < 4 || text[2] == '[' || text[namelen - 1] != ';') {
					exceptions_throw_classnotfoundexception(name);
					return false;
				}

				u = utf_new(text + 2, namelen - 3);

				if (!(comp = load_class_from_classloader(u, cl)))
					return false;

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

				u = utf_new(text + 1, namelen - 1);

				if (!(comp = load_class_from_classloader(u, cl)))
					return false;

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
									  utf_loadClassInternal,
									  utf_java_lang_String__java_lang_Class,
									  NULL,
									  true);
#else
		lc = class_resolveclassmethod(c,
									  utf_loadClass,
									  utf_java_lang_String__java_lang_Class,
									  NULL,
									  true);
#endif

		if (lc == NULL)
			return false; /* exception */

		/* move return value into `o' and cast it afterwards to a classinfo* */

		string = javastring_new_slash_to_dot(name);

		RT_TIMING_GET_TIME(time_prepare);

		o = vm_call_method(lc, (java_handle_t *) cl, string);

		RT_TIMING_GET_TIME(time_java);

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

		RT_TIMING_GET_TIME(time_cache);

		RT_TIMING_TIME_DIFF(time_lookup , time_prepare, RT_TIMING_LOAD_CL_PREPARE);
		RT_TIMING_TIME_DIFF(time_prepare, time_java   , RT_TIMING_LOAD_CL_JAVA);
		RT_TIMING_TIME_DIFF(time_java   , time_cache  , RT_TIMING_LOAD_CL_CACHE);

		/* SUN compatible -verbose:class output */

		if (opt_verboseclass && (c != NULL) && (c->classloader == cl)) {
			printf("[Loaded ");
			utf_display_printable_ascii_classname(name);
			printf("]\n");
		}

#if defined(ENABLE_JVMTI)
		/* fire Class Load JVMTI event */
		if (jvmti) jvmti_ClassLoadPrepare(false, c);
#endif


		return c;
	} 

	c = load_class_bootstrap(name);

	return c;
}


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

classinfo *load_class_bootstrap(utf *name)
{
	classbuffer *cb;
	classinfo   *c;
	classinfo   *r;
#if defined(ENABLE_RT_TIMING)
	struct timespec time_start, time_lookup, time_array, time_suck, 
					time_load, time_cache;
#endif

	RT_TIMING_GET_TIME(time_start);

	/* for debugging */

	assert(name);

	/* lookup if this class has already been loaded */

	r = classcache_lookup(NULL, name);

	if (r != NULL) {
		RT_TIMING_GET_TIME(time_lookup);
		RT_TIMING_TIME_DIFF(time_start,time_lookup,RT_TIMING_LOAD_BOOT_LOOKUP);
		
		return r;
	}

	RT_TIMING_GET_TIME(time_lookup);
	RT_TIMING_TIME_DIFF(time_start,time_lookup,RT_TIMING_LOAD_BOOT_LOOKUP);
		
	/* create the classinfo */

	c = class_create_classinfo(name);

	/* handle array classes */

	if (name->text[0] == '[') {
		c = load_newly_created_array(c, NULL);

		if (c == NULL)
			return NULL;

		assert(c->state & CLASS_LOADED);

		RT_TIMING_GET_TIME(time_array);
		RT_TIMING_TIME_DIFF(time_start,time_array,RT_TIMING_LOAD_BOOT_ARRAY);
		
		return c;
	}

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

	RT_TIMING_GET_TIME(time_suck);
	
	/* load the class from the buffer */

	r = load_class_from_classbuffer(cb);

	RT_TIMING_GET_TIME(time_load);
	
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
			/* Add the package name to the boot packages. */

			Package_add(c->packagename);
		}

		r = res;
	}

	RT_TIMING_GET_TIME(time_cache);
	
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

	RT_TIMING_TIME_DIFF(time_lookup, time_suck , RT_TIMING_LOAD_BOOT_SUCK);
	RT_TIMING_TIME_DIFF(time_suck  , time_load , RT_TIMING_LOAD_BOOT_LOAD);
	RT_TIMING_TIME_DIFF(time_load  , time_cache, RT_TIMING_LOAD_BOOT_CACHE);
	RT_TIMING_TIME_DIFF(time_lookup, time_cache, RT_TIMING_LOAD_BOOT_TOTAL);

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
	classinfo          *c;
	classinfo          *tc;
	utf                *name;
	utf                *supername;
	utf               **interfacesnames;
	utf                *u;
	constant_classref  *cr;
	int16_t             index;

	u4 i,j;
	u4 ma, mi;
	descriptor_pool *descpool;
#if defined(ENABLE_STATISTICS)
	u4 classrefsize;
	u4 descsize;
#endif
#if defined(ENABLE_RT_TIMING)
	struct timespec time_start, time_checks, time_ndpool, time_cpool,
					time_setup, time_fields, time_methods, time_classrefs,
					time_descs,	time_setrefs, time_parsefds, time_parsemds,
					time_parsecpool, time_verify, time_attrs;
#endif

	RT_TIMING_GET_TIME(time_start);

	/* Get the classbuffer's class. */

	c = cb->clazz;

	if (!suck_check_classbuffer_size(cb, 4 + 2 + 2))
		return false;

	/* check signature */

	if (suck_u4(cb) != MAGIC) {
		exceptions_throw_classformaterror(c, "Bad magic number");
		return false;
	}

	/* check version */

	mi = suck_u2(cb);
	ma = suck_u2(cb);

	if (!(ma < MAJOR_VERSION || (ma == MAJOR_VERSION && mi <= MINOR_VERSION))) {
		exceptions_throw_unsupportedclassversionerror(c, ma, mi);
		return false;
	}

	RT_TIMING_GET_TIME(time_checks);

	/* create a new descriptor pool */

	descpool = descriptor_pool_new(c);

	RT_TIMING_GET_TIME(time_ndpool);

	/* load the constant pool */

	if (!load_constantpool(cb, descpool))
		return false;

	RT_TIMING_GET_TIME(time_cpool);

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

	if (c->name == utf_not_named_yet) {
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

		if (c->name != utf_java_lang_Object) {
			exceptions_throw_classformaterror(c, "Bad superclass index");
			return false;
		}
	}
	else {
		supername = (utf *) class_getconstant(c, index, CONSTANT_Class);

		if (supername == NULL)
			return false;

		/* java.lang.Object may not have a super class. */

		if (c->name == utf_java_lang_Object) {
			exceptions_throw_classformaterror(NULL, "java.lang.Object with superclass");
			return false;
		}

		/* Detect circularity. */

		if (supername == c->name) {
			exceptions_throw_classcircularityerror(c);
			return false;
		}

		/* Interfaces must have java.lang.Object as super class. */

		if ((c->flags & ACC_INTERFACE) && (supername != utf_java_lang_Object)) {
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

	interfacesnames = DMNEW(utf*, c->interfacescount);

	for (i = 0; i < c->interfacescount; i++) {
		index = suck_u2(cb);

		u = (utf *) class_getconstant(c, index, CONSTANT_Class);

		if (u == NULL)
			return false;

		interfacesnames[i] = u;
	}

	RT_TIMING_GET_TIME(time_setup);

	/* Parse fields. */

	if (!suck_check_classbuffer_size(cb, 2))
		return false;

	c->fieldscount = suck_u2(cb);
  	c->fields      = MNEW(fieldinfo, c->fieldscount);

	MZERO(c->fields, fieldinfo, c->fieldscount);

	for (i = 0; i < c->fieldscount; i++) {
		if (!field_load(cb, &(c->fields[i]), descpool))
			return false;
	}

	RT_TIMING_GET_TIME(time_fields);

	/* Parse methods. */

	if (!suck_check_classbuffer_size(cb, 2))
		return false;

	c->methodscount = suck_u2(cb);
	c->methods      = MNEW(methodinfo, c->methodscount);

	MZERO(c->methods, methodinfo, c->methodscount);
	
	for (i = 0; i < c->methodscount; i++) {
		if (!method_load(cb, &(c->methods[i]), descpool))
			return false;
	}

	RT_TIMING_GET_TIME(time_methods);

	/* create the class reference table */

	c->classrefs =
		descriptor_pool_create_classrefs(descpool, &(c->classrefcount));

	RT_TIMING_GET_TIME(time_classrefs);

	/* allocate space for the parsed descriptors */

	descriptor_pool_alloc_parsed_descriptors(descpool);
	c->parseddescs =
		descriptor_pool_get_parsed_descriptors(descpool, &(c->parseddescsize));

#if defined(ENABLE_STATISTICS)
	if (opt_stat) {
		descriptor_pool_get_sizes(descpool, &classrefsize, &descsize);
		count_classref_len += classrefsize;
		count_parsed_desc_len += descsize;
	}
#endif

	RT_TIMING_GET_TIME(time_descs);

	/* put the classrefs in the constant pool */

	for (i = 0; i < c->cpcount; i++) {
		if (c->cptags[i] == CONSTANT_Class) {
			utf *name = (utf *) c->cpinfos[i];
			c->cpinfos[i] = descriptor_pool_lookup_classref(descpool, name);
		}
	}

	/* Resolve the super class. */

	if (supername != NULL) {
		cr = descriptor_pool_lookup_classref(descpool, supername);

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

	for (i = 0; i < c->interfacescount; i++) {
		u  = interfacesnames[i];
		cr = descriptor_pool_lookup_classref(descpool, u);

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

	RT_TIMING_GET_TIME(time_setrefs);

	/* Parse the field descriptors. */

	for (i = 0; i < c->fieldscount; i++) {
		c->fields[i].parseddesc =
			descriptor_pool_parse_field_descriptor(descpool,
												   c->fields[i].descriptor);
		if (!c->fields[i].parseddesc)
			return false;
	}

	RT_TIMING_GET_TIME(time_parsefds);

	/* parse method descriptors */

	for (i = 0; i < c->methodscount; i++) {
		methodinfo *m = &c->methods[i];
		m->parseddesc =
			descriptor_pool_parse_method_descriptor(descpool, m->descriptor,
													m->flags, class_get_self_classref(m->clazz));
		if (!m->parseddesc)
			return false;

		for (j = 0; j < m->rawexceptiontablelength; j++) {
			if (!m->rawexceptiontable[j].catchtype.any)
				continue;

			if ((m->rawexceptiontable[j].catchtype.ref =
				 descriptor_pool_lookup_classref(descpool,
						(utf *) m->rawexceptiontable[j].catchtype.any)) == NULL)
				return false;
		}

		for (j = 0; j < m->thrownexceptionscount; j++) {
			if (!m->thrownexceptions[j].any)
				continue;

			if ((m->thrownexceptions[j].ref = descriptor_pool_lookup_classref(descpool,
						(utf *) m->thrownexceptions[j].any)) == NULL)
				return false;
		}
	}

	RT_TIMING_GET_TIME(time_parsemds);

	/* parse the loaded descriptors */

	for (i = 0; i < c->cpcount; i++) {
		constant_FMIref *fmi;
		s4               index;

		switch (c->cptags[i]) {
		case CONSTANT_Fieldref:
			fmi = (constant_FMIref *) c->cpinfos[i];
			fmi->parseddesc.fd =
				descriptor_pool_parse_field_descriptor(descpool,
													   fmi->descriptor);
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
			fmi->parseddesc.md =
				descriptor_pool_parse_method_descriptor(descpool,
														fmi->descriptor,
														ACC_UNDEF,
														fmi->p.classref);
			if (!fmi->parseddesc.md)
				return false;
			break;
		}
	}

	RT_TIMING_GET_TIME(time_parsecpool);

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
		size_t len;
		size_t hashlen;
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
			len = sizeof(utf);
			while (len) {
				len >>= 1;
				shift++;
			}
		}

		/* Check fields */
		memset(hashtab, 0, sizeof(u2) * (hashlen + len));

		for (i = 0; i < c->fieldscount; ++i) {
			fieldinfo *fi = c->fields + i;

			/* It's ok if we lose bits here */
			index = ((((size_t) fi->name) +
					  ((size_t) fi->descriptor)) >> shift) % hashlen;

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

		for (i = 0; i < c->methodscount; ++i) {
			methodinfo *mi = c->methods + i;

			/* It's ok if we lose bits here */
			index = ((((size_t) mi->name) +
					  ((size_t) mi->descriptor)) >> shift) % hashlen;

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

	RT_TIMING_GET_TIME(time_verify);

#if defined(ENABLE_STATISTICS)
	if (opt_stat) {
		size_classinfo  += sizeof(classinfo*) * c->interfacescount;
		size_fieldinfo  += sizeof(fieldinfo)  * c->fieldscount;
		size_methodinfo += sizeof(methodinfo) * c->methodscount;
	}
#endif

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

	RT_TIMING_GET_TIME(time_attrs);

	RT_TIMING_TIME_DIFF(time_start     , time_checks    , RT_TIMING_LOAD_CHECKS);
	RT_TIMING_TIME_DIFF(time_checks    , time_ndpool    , RT_TIMING_LOAD_NDPOOL);
	RT_TIMING_TIME_DIFF(time_ndpool    , time_cpool     , RT_TIMING_LOAD_CPOOL);
	RT_TIMING_TIME_DIFF(time_cpool     , time_setup     , RT_TIMING_LOAD_SETUP);
	RT_TIMING_TIME_DIFF(time_setup     , time_fields    , RT_TIMING_LOAD_FIELDS);
	RT_TIMING_TIME_DIFF(time_fields    , time_methods   , RT_TIMING_LOAD_METHODS);
	RT_TIMING_TIME_DIFF(time_methods   , time_classrefs , RT_TIMING_LOAD_CLASSREFS);
	RT_TIMING_TIME_DIFF(time_classrefs , time_descs     , RT_TIMING_LOAD_DESCS);
	RT_TIMING_TIME_DIFF(time_descs     , time_setrefs   , RT_TIMING_LOAD_SETREFS);
	RT_TIMING_TIME_DIFF(time_setrefs   , time_parsefds  , RT_TIMING_LOAD_PARSEFDS);
	RT_TIMING_TIME_DIFF(time_parsefds  , time_parsemds  , RT_TIMING_LOAD_PARSEMDS);
	RT_TIMING_TIME_DIFF(time_parsemds  , time_parsecpool, RT_TIMING_LOAD_PARSECP);
	RT_TIMING_TIME_DIFF(time_parsecpool, time_verify    , RT_TIMING_LOAD_VERIFY);
	RT_TIMING_TIME_DIFF(time_verify    , time_attrs     , RT_TIMING_LOAD_ATTRS);
	RT_TIMING_TIME_DIFF(time_start     , time_attrs     , RT_TIMING_LOAD_TOTAL);

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
	int32_t    dumpmarker;

	/* Get the classbuffer's class. */

	c = cb->clazz;

	/* Check if the class is already loaded. */

	if (c->state & CLASS_LOADED)
		return c;

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		count_class_loads++;
#endif

#if !defined(NDEBUG)
	if (loadverbose)
		log_message_class("Loading class: ", c);
#endif

	/* Mark start of dump memory area. */

	DMARKER;

	/* Class is currently loading. */

	c->state |= CLASS_LOADING;

	/* Parse the classbuffer. */

	result = load_class_from_classbuffer_intern(cb);

	/* Release dump area. */

	DRELEASE;

	/* An error occurred. */

	if (result == false) {
		/* Revert loading state. */

		c->state = (c->state & ~CLASS_LOADING);

		return NULL;
	}

	/* Revert loading state and set loaded. */

	c->state = (c->state & ~CLASS_LOADING) | CLASS_LOADED;

#if defined(ENABLE_JVMTI)
	/* fire Class Prepare JVMTI event */

	if (jvmti)
		jvmti_ClassLoadPrepare(true, c);
#endif

#if !defined(NDEBUG)
	if (loadverbose)
		log_message_class("Loading done class: ", c);
#endif

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
	char              *text;
	s4                 namelen;
	utf               *u;

	text    = c->name->text;
	namelen = c->name->blength;

	/* Check array class name */

	if ((namelen < 2) || (text[0] != '[')) {
		exceptions_throw_classnotfoundexception(c->name);
		return NULL;
	}

	/* Check the element type */

	switch (text[1]) {
	case '[':
		/* c is an array of arrays. We have to create the component class. */

		u = utf_new(text + 1, namelen - 1);

		comp = load_class_from_classloader(u, loader);

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

		u = utf_new(text + 2, namelen - 3);

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

		if ((namelen > 2) || (Primitive_get_class_by_char(text[1]) == NULL)) {
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
	CLASSREF_INIT(classrefs[1], c, utf_java_lang_Object);

	/* create descriptor for clone method */
	/* we need one paramslot which is reserved for the 'this' parameter */
	clonedesc = NEW(methoddesc);
	clonedesc->returntype.type = TYPE_ADR;
	clonedesc->returntype.classref = classrefs + 1;
	clonedesc->returntype.arraydim = 0;
	/* initialize params to "empty", add real params below in
	   descriptor_params_from_paramtypes */
	clonedesc->paramcount = 0;
	clonedesc->paramslots = 0;
	clonedesc->paramtypes[0].classref = classrefs + 0;
	clonedesc->params = NULL;

	/* create methodinfo */

	clone = c->methods;
	MSET(clone, 0, methodinfo, 1);

#if defined(ENABLE_THREADS)
	lock_init_object_lock(&clone->header);
#endif

	/* ATTENTION: if you delete the ACC_NATIVE below, set
	   clone->maxlocals=1 (interpreter related) */

	clone->flags      = ACC_PUBLIC | ACC_NATIVE;
	clone->name       = utf_clone;
	clone->descriptor = utf_void__java_lang_Object;
	clone->parseddesc = clonedesc;
	clone->clazz      = c;

	/* parse the descriptor to get the register allocation */

	if (!descriptor_params_from_paramtypes(clonedesc, clone->flags))
		return false;

	clone->code = codegen_generate_stub_native(clone, BUILTIN_clone);

	/* XXX: field: length? */

	/* array classes are not loaded from class files */

	c->state          |= CLASS_LOADED;
	c->parseddescs    = (u1 *) clonedesc;
	c->parseddescsize = sizeof(methodinfo);
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
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
