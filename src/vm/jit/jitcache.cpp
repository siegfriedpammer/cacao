/* src/vm/jit/jitcache.c - JIT caching stuff

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

#include "config.h"

#if defined(ENABLE_JITCACHE)

/* for mkdir() */
#include <sys/stat.h>

#include <assert.h>
#include <stdint.h>

#include "md.h"

#include "toolbox/list.h"
#include "toolbox/logging.h"

#include "mm/memory.h"
#include "mm/codememory.h"

#include "vm/builtin.h"
#include "vm/method.h"
#include "vm/options.h"
#include "vm/resolve.h"
#include "vm/stringl.hpp"
#include "vm/types.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/code.h"
#include "vm/jit/patcher-common.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/linenumbertable.h"
#include "vm/jit/exceptiontable.h"
#include "vm/jit/methodtree.h"

#include "vm/references.h"
#include "vm/os.hpp"
#include "vm/field.h"
#include "vm/utf8.h"

#include "vm/jit/jitcache.hpp"

#include "threads/thread.hpp"

/* TODO: Wrap this in vm/system.h" */
#include "unistd.h"

#define CACHEROOT "/tmp/cacao-jitcache/"

/* small grained helper functions */
char *get_dest_dir(methodinfo *);
char *get_dest_file(methodinfo *);

int mkdir_hier(char *, mode_t);
int open_to_read(char *);
int open_to_write(char *);

u1 *to_abs(u1 *, u1 *);
u1 *to_offset(u1 *, u1 *);

void store_utf(int, utf *);
void store_classinfo(int, classinfo *);
void store_builtin(int, builtintable_entry *);
void store_string(int, java_object_t *);
void store_methodinfo(int, methodinfo *);
void store_fieldinfo(int, fieldinfo *);
void store_cachedref(int, cachedref_t *);

void load_utf(utf **, int);
void load_classinfo(classinfo **, int, methodinfo *);
void load_builtin(builtintable_entry **, int);
void load_string(java_object_t **, int);
void load_methodinfo(methodinfo **, int, methodinfo *);
void load_fieldinfo(fieldinfo **, int, methodinfo *);
void load_cachedref(cachedref_t **, int, codeinfo *);

/* medium grained helper functions */
void load_from_file_patchers(codeinfo *, int);
void load_from_file_cachedrefs(codeinfo *, int);
void load_from_file_exceptiontable(codeinfo *, int);
void load_from_file_linenumbertable(codeinfo *, int);

void store_to_file_patchers(int, codeinfo *);
void store_to_file_cachedrefs(int, codeinfo *);
void store_to_file_linenumbertable(int, codeinfo *);
void store_to_file_exceptiontable(int, codeinfo *);

/* file handling functions */
void update_method_table(methodinfo *, int);
int seek_method_table(methodinfo *, int);
int get_cache_file_readable(methodinfo *);
int get_cache_file_writable(methodinfo *);

/* serializer forward declarations */
void s_dummy(int, patchref_t *, methodinfo *);
void s_unresolved_class(int, patchref_t *, methodinfo *);
void s_unresolved_field(int, patchref_t *, methodinfo *);
void s_unresolved_method(int, patchref_t *, methodinfo *);
void s_constant_classref(int, patchref_t *, methodinfo *);
void s_classinfo(int, patchref_t *, methodinfo *);
void s_methodinfo(int, patchref_t *, methodinfo *);
void s_fieldinfo(int, patchref_t *, methodinfo *);
void s_string(int, patchref_t *, methodinfo *);

/* deserializer forward declarations */
void d_dummy(patchref_t *, int, methodinfo *);
void d_unresolved_class(patchref_t *, int, methodinfo *);
void d_unresolved_field(patchref_t *, int, methodinfo *);
void d_unresolved_method(patchref_t *, int, methodinfo *);
void d_constant_classref(patchref_t *, int, methodinfo *);
void d_classinfo(patchref_t *, int, methodinfo *);
void d_methodinfo(patchref_t *, int, methodinfo *);
void d_fieldinfo(patchref_t *, int, methodinfo *);
void d_string(patchref_t *, int, methodinfo *);

/* The order of entries follows the order of
 * declarations in patcher-common.h
 */

static jitcache_patcher_function_list_t patcher_functions[] = {
		{ PATCHER_resolve_class, s_unresolved_class, d_unresolved_class },
		{ PATCHER_initialize_class, s_classinfo, d_classinfo },
		{ PATCHER_resolve_classref_to_classinfo, s_constant_classref, d_constant_classref },
		{ PATCHER_resolve_classref_to_vftbl, s_constant_classref, d_constant_classref },
		{ PATCHER_resolve_classref_to_index, s_constant_classref, d_constant_classref },
		{ PATCHER_resolve_classref_to_flags, s_constant_classref, d_constant_classref },
		{ PATCHER_resolve_native_function, s_methodinfo, d_methodinfo },

		/* old patcher functions */
		{ PATCHER_get_putstatic, s_unresolved_field, d_unresolved_field },

#if defined(__I386__)
		{ PATCHER_getfield, s_unresolved_field, d_unresolved_field },
		{ PATCHER_putfield, s_unresolved_field, d_unresolved_field },
#else
		{ PATCHER_get_putfield, s_unresolved_field, d_unresolved_field },
#endif

#if defined(__I386__) || defined(__X86_64__)
		{ PATCHER_putfieldconst, s_unresolved_field, d_unresolved_field }, /* 10 */
#endif

		{ PATCHER_invokestatic_special, s_unresolved_method, d_unresolved_method },
		{ PATCHER_invokevirtual, s_unresolved_method, d_unresolved_method },
		{ PATCHER_invokeinterface, s_unresolved_method, d_unresolved_method },

#if defined(__ALPHA__) || defined(__I386__) || defined(__MIPS__) || defined(__POWERPC__) || defined(__POWERPC64__) || defined(__S390__) || defined(__X86_64__) || defined(__M68K__)
		{ PATCHER_checkcast_interface, s_constant_classref, d_constant_classref },
		{ PATCHER_instanceof_interface, s_constant_classref, d_constant_classref },
#endif

#if defined(__S390__)
		{ PATCHER_checkcast_instanceof_interface, s_dummy, d_dummy },
#endif

#if defined(__I386__)
		{ PATCHER_aconst, s_constant_classref, d_constant_classref }, /* 16 */
		{ PATCHER_builtin_multianewarray, s_constant_classref, d_constant_classref },
		{ PATCHER_builtin_arraycheckcast, s_constant_classref, d_constant_classref },
		{ PATCHER_checkcast_instanceof_flags, s_constant_classref, d_constant_classref },
		{ PATCHER_checkcast_class, s_constant_classref, d_constant_classref },
		{ PATCHER_instanceof_class, s_constant_classref, d_constant_classref },
#endif

		{ NULL, s_dummy, d_dummy }
};

#define JC_MRU_SIZE 100
static classinfo *jc_mru_list[JC_MRU_SIZE];
int mru_last_free = 0;
int mru_start = 0;

/* jitcache_mru_add ************************************************************

   Adds a classinfo to the most recently used (MRU) list. The MRU uses a simple
   strategy: it will store entries until it is full, further candidates replace
   the 

*******************************************************************************/

void jitcache_mru_add(classinfo *c, int fd)
{
	classinfo *old_c;
	assert(!c->cache_file_fd);

	c->cache_file_fd = fd;

	if (mru_last_free < JC_MRU_SIZE) {
		jc_mru_list[mru_last_free] = c;

		mru_last_free++;

		return;
	}
	else {
		mru_start--;
		if (mru_start < 0)
			mru_start = JC_MRU_SIZE - 1;

		old_c = jc_mru_list[mru_start];

		assert (old_c);
		assert (old_c->cache_file_fd);
		os::close(old_c->cache_file_fd);
		old_c->cache_file_fd = 0;

		jc_mru_list[mru_start] = c;
	}
	
}

void jitcache_mru_remove(classinfo *c)
{
	int i, j;

	for (i = 0; i < JC_MRU_SIZE; i++)
		if (jc_mru_list[i] == c) {
			jc_mru_list[j] = NULL;

			for (j = i; i < mru_last_free - 1; j++)
				jc_mru_list[j] = jc_mru_list[j+1];

			mru_last_free--;
		}

	assert (0);
}

/* jitcache_list_create ********************************************************

   Creates an empty cached reference list for the given codeinfo.

*******************************************************************************/

void jitcache_list_create(codeinfo *code)
{
	code->cachedrefs = list_create(OFFSET(cachedref_t, linkage));
}


/* jitcache_list_reset **********************************************************

   Resets the cached reference list inside a codeinfo.

*******************************************************************************/

void jitcache_list_reset(codeinfo *code)
{
	cachedref_t *pr;

	/* free all elements of the list */

	while((pr = list_first(code->cachedrefs)) != NULL) {
		list_remove(code->cachedrefs, pr);

		FREE(pr, cachedref_t);

#if defined(ENABLE_STATISTICS)
		if (opt_stat)
			size_cachedref -= sizeof(cachedref_t);
#endif
	}
}


/* jitcache_list_free ***********************************************************

   Frees the cached reference list and all its entries for the given codeinfo.

*******************************************************************************/

void jitcache_list_free(codeinfo *code)
{
	/* free all elements of the list */

	jitcache_list_reset(code);

	/* free the list itself */

	FREE(code->cachedrefs, list_t);
}


/* jitcache_list_find ***********************************************************

   Find an entry inside the cached reference list for the given codeinfo
   by specifying the displacement in the code/data segment.

   NOTE: Caller should hold the patcher list lock or maintain
   exclusive access otherwise.

*******************************************************************************/

static cachedref_t *jitcache_list_find(codeinfo *code, s4 disp)
{
	cachedref_t *cr;

	/* walk through all cached references for the given codeinfo */

	cr = list_first(code->cachedrefs);
	while (cr) {

		if (cr->disp == disp)
			return cr;

		cr = list_next(code->cachedrefs, cr);
	}

	return NULL;
}


/* jitcache_new_cachedref ******************************************************

   Creates and initializes a new cachedref

*******************************************************************************/

cachedref_t *jitcache_new_cached_ref(cachedreftype type, s4 md_patch, void* ref, s4 disp)
{
	cachedref_t *cr;

	/* allocate cachedref on heap (at least freed together with codeinfo) */

	cr = NEW(cachedref_t);

#if defined(ENABLE_STATISTICS)
	if (opt_stat)
		size_cachedref += sizeof(cachedref_t);
#endif

	/* set reference information */

	cr->type    = type;
	cr->md_patch= md_patch;
	cr->disp    = disp;
	cr->ref     = ref;

	return cr;
}
/* jitcache_add_cachedref_jd ***************************************************

   Creates a new cached ref appends it to the list in the codeinfo structure
   *or* attaches it to the *last* patchref_t if it overlaps with the address
   of the cached reference.

*******************************************************************************/

void jitcache_add_cached_ref_intern(codeinfo *code, cachedref_t *cachedref)
{
	cachedref_t *list_cr;

	list_cr = (cachedref_t *) list_first(code->cachedrefs);

	while (list_cr)
	{
		if (list_cr->disp == cachedref->disp)
		{
			assert(list_cr->type == cachedref->type);
			assert(list_cr->ref == cachedref->ref);

			/* Cachedref for already existing object found. No need to store
			 * it.
             */
			return;
		}

		list_cr = list_next(code->cachedrefs, list_cr);
	}

	list_add_first(code->cachedrefs, cachedref);
}

/* jitcache_add_cachedref_jd ***************************************************

   Creates a new cached ref appends it to the list in the codeinfo structure
   *or* attaches it to the *last* patchref_t if it overlaps with the address
   of the cached reference.

*******************************************************************************/

void jitcache_add_cached_ref_jd(jitdata *jd, cachedreftype type, void* ref)
{
	jitcache_add_cached_ref_md_jd(jd, type, 0, ref);
}


/* jitcache_add_cachedref_md_jd ************************************************

   Creates a new cached ref appends it to the list in the codeinfo structure
   *or* attaches it to the *last* patchref_t if it overlaps with the address
   of the cached reference.

*******************************************************************************/

void jitcache_add_cached_ref_md_jd(jitdata *jd, cachedreftype type, s4 md_patch, void* ref)
{
	patchref_t	 *patchref;
	codegendata	 *cd;
	s4			 disp;
	cachedref_t *cachedref;

	if (type >= CRT_OBJECT_HEADER && !ref)
		return;

	cd = jd->cd;

	disp = (s4) (cd->mcodeptr - cd->mcodebase) - SIZEOF_VOID_P;
	cachedref = jitcache_new_cached_ref(type, md_patch, ref, disp);

	patchref = (patchref_t *) list_first(jd->code->patchers);

	if (patchref
		&& (patchref->mpc) <= disp
		&& (patchref->mpc + sizeof(patchref->mcode)) >= disp)
	{
		/* patchers and cachedref overlap: cached ref must
		 * be handled after the patcher.
         */

		if (opt_DebugJitCache)
		{
			log_message_method("cached ref overlaps with patchref: ", jd->m);
		}

		/* There can be only one cached ref per patcher currently.
         * If the need arises to handle more cached refs a list can
         * be used.
         */
		assert(!patchref->attached_ref);

		patchref->attached_ref = cachedref;
	}
	else
		jitcache_add_cached_ref_intern(jd->code, cachedref);
}


/* jitcache_add_cachedref ******************************************************

   Creates a new cached references and appends it to the list.

*******************************************************************************/

void jitcache_add_cached_ref(codeinfo *code, cachedreftype type, void* ref, s4 disp)
{
	cachedref_t *cr;

	/* allocate cachedref on heap (at least freed together with codeinfo) */
	cr = jitcache_new_cached_ref(type, 0, ref,disp);

	jitcache_add_cached_ref_intern(code, cr);
}



/* jitcache_handle_cached_ref **************************************************

   Creates a new cached references and appends it to the list.

*******************************************************************************/

void jitcache_handle_cached_ref(cachedref_t *cr, codeinfo *code)
{
	u1 **location;

	location = (u1 **) (code->entrypoint + cr->disp);

	/* Write the restored reference into the code. */
	if (cr->md_patch)
		patch_md(cr->md_patch, (ptrint) location, cr->ref);
	else
		*location = cr->ref;

	md_cacheflush(location, SIZEOF_VOID_P);

	FREE(cr, cachedref_t);
}

#include <stdlib.h>
#include <stdio.h>

bool filter(utf *classname)
{
	static bool printed = false;
	char *buf = NULL;
	int i = 0;
	int max_index = 30000;
	const char *classes[] = {
		"Test$FooBar",
		"Test",
		"FieldTest",
		"UnresolvedClass",
		"java/lang/ThreadGroup",
		"java/lang/Object",
		"java/util/Vector",
		"java/util/Vector$1",
		"java/util/AbstractList", 
		"java/util/AbstractCollection",
		"java/lang/Thread",
		"java/lang/String",
		"java/lang/ClassLoader",
		"java/lang/VMClassLoader",
		"java/util/HashMap",
		"java/util/HashSet",
		"java/util/AbstractSet",
		"gnu/classpath/SystemProperties",
		"java/util/Properties",
		"java/util/Hashtable",
		"java/util/Dictionary",
		"java/util/Hashtable$HashEntry",
		"java/util/AbstractMap$SimpleEntry",
		"java/lang/StringBuilder",
		"java/lang/AbstractStringBuffer",
		"java/util/Collections$SynchronizedSet",
		"java/util/Collections$SynchronizedCollection",
		"java/util/Hashtable$3",
		"java/util/Hashtable$EntryIterator",
		"java/util/Collections$SynchronizedIterator",
		"java/lang/Boolean",
		"java/util/Collections",
		"java/util/Collections$EmptySet",
		"java/util/Collections$EmptyList",
		"java/util/Collections$EmptyMap",
		"java/util/Collections$ReverseComparator",
		"java/util/Collections$UnmodifiableMap",
		"java/io/File",
		"java/util/StringTokenizer",
		"java/util/ArrayList",
		"java/io/VMFile",
		"java/lang/System",
		"java/lang/VMSystem",
		"java/io/FileDescriptor",
		"gnu/java/io/FileChannelImpl",
		"java/lang/Runtime",
		"gnu/classpath/VMStackWalker",
		"gnu/java/io/VMChannel",
		"gnu/java/io/VMChannel$State",
		"gnu/java/io/VMChannel$Kind",
		"java/nio/channels/FileChannelImpl",
		"java/nio/channels/spi/AbstractInterruptibleChannel",
		"java/lang/Number",
		"java/io/FileInputStream",
		"java/io/InputStream",
		"java/io/BufferedInputStream",
		"java/io/FilterInputStream",
		"java/io/PrintStream",
		"java/io/OutputStream",
		"java/io/BufferedOutputStream",
		"java/io/FilterOutputStream",
		"java/net/URL",
		"java/util/Locale",
		"java/lang/Math",
		"gnu/java/lang/CharData",
		"java/lang/Character",
		"java/net/URL$1",
		"java/security/VMAccessController",
		"java/lang/ThreadLocal",
		"java/security/CodeSource",
		"**placeholder**",
		"java/security/PermissionCollection",
		"Test$1",
		"java/security/ProtectionDomain",
		"java/security/AccessControlContext",
		"gnu/java/util/WeakIdentityHashMap",
		"gnu/java/util/WeakIdentityHashMap$WeakEntrySet",
		"java/lang/ref/ReferenceQueue",
		"java/util/LinkedList",
		"java/util/AbstractSequentialList",
		"gnu/java/util/WeakIdentityHashMap$WeakBucket$WeakEntry",
		"java/util/LinkedList$Entry",
		"java/lang/Class",
		"java/lang/reflect/VMConstructor",
		"java/lang/reflect/Constructor",
		"java/lang/reflect/Modifier",
		"gnu/java/net/protocol/file/Handler",
		"java/net/URLStreamHandler",
		"java/util/ArrayList",
		"java/security/SecureClassLoader",
		"java/lang/Exception",
		"java/lang/Throwable",
		"java/lang/VMThrowable",
		"gnu/java/net/loader/FileURLLoader",
		"gnu/java/net/loader/URLLoader",
		"java/security/Policy",
		"gnu/java/security/provider/DefaultPolicy",
		"gnu/java/net/loader/Resource",
		"gnu/java/net/loader/FileResource", 
		"java/io/FileInputStream", 
		"gnu/java/nio/FileChannelImpl",
		"java/nio/ByteBuffer",
		"java/nio/Buffer",
		"java/nio/ByteOrder",
		"java/security/Permissions$PermissionsHash",
		"java/nio/charset/Charset",
		"gnu/java/nio/charset/Provider",
		"gnu/java/nio/charset/Provider$1",
		"gnu/java/nio/charset/US_ASCII",
		"java/util/Collections$UnmodifiableSet/",
		"java/util/Collections$UnmodifiableCollection",
		"java/util/Collections$UnmodifiableIterator",
		"gnu/java/nio/charset/ISO_8859_1",
		"gnu/java/nio/charset/UTF_8",
		"gnu/java/nio/charset/UTF_16BE",
		"gnu/java/nio/charset/UTF_16LE",
		"gnu/java/nio/charset/UTF_16",
		"gnu/java/nio/charset/UnicodeLittle",
		"gnu/java/nio/charset/Windows1250",
		"gnu/java/nio/charset/Windows1250",
		"gnu/java/nio/charset/ByteCharset",
		"gnu/java/nio/charset/Windows1251",
		"gnu/java/nio/charset/Windows1252",
		"gnu/java/nio/charset/Windows1253",
		"gnu/java/nio/charset/Windows1254",
		"gnu/java/nio/charset/Windows1257",
		"gnu/java/nio/charset/ISO_8859_2",
		"gnu/java/nio/charset/ISO_8859_4",
		"gnu/java/nio/charset/ISO_8859_5",
		"gnu/java/nio/charset/ISO_8859_7",
		"gnu/java/nio/charset/ISO_8859_9",
		"gnu/java/nio/charset/ISO_8859_13",
		"gnu/java/nio/charset/ISO_8859_15",
		"gnu/java/nio/charset/KOI_8",
		"gnu/java/nio/charset/ISO_8859_1$Encoder",
		"java/nio/charset/CharsetEncoder",
		"gnu/java/nio/charset/ByteEncodeLoopHelper",
		"java/nio/charset/CodingErrorAction",
		"java/nio/CharBuffer",
		"java/nio/CharBufferImpl",
		"gnu/java/nio/charset/ByteEncodeLoopHelper",
		"java/nio/charset/CoderResult",
		"java/nio/charset/CoderResult$1",
		"java/nio/charset/CoderResult$2",
		"java/nio/charset/CoderResult$Cache",
		"java/awt/Toolkit",
		"gnu/java/awt/peer/gtk/GtkToolkit",
		"gnu/java/awt/peer/gtk/GtkGenericPeer",
		"java/util/WeakHashMap",
		"java/util/WeakHashMap$1", 
		"java/util/WeakHashMap$WeakEntrySet",
		"gnu/java/awt/peer/gtk/GtkWindowPeer",
		"gnu/java/awt/peer/gtk/GtkCheckboxPeer",
		"gnu/java/awt/peer/gtk/GtkFileDialogPeer",
		"gnu/java/awt/peer/gtk/GtkMainThread",
		"java/security/AccessController",
		"java/security/Permission",
		"java/lang/ClassLoader$StaticData",
		"java/lang/VMString",
		"gnu/java/lang/CPStringBuilder",
		"gnu/java/lang/VMCPStringBuilder",
		"java/io/FileOutputStream",
		"gnu/java/nio/VMChannel",
		"gnu/java/nio/VMChannel$State",
		"gnu/java/nio/VMChannel$Kind",
		"java/nio/channels/FileChannel",
		"gnu/classpath/VMSystemProperties",
		"java/lang/StringBuffer",
		"java/lang/VMRuntime",
		"java/lang/VMObject",
		"java/lang/VMThread",
		"java/lang/VMClass",
		"java/lang/InheritableThreadLocal",
		"java/lang/ClassNotFoundException",
		"java/net/URLClassLoader",
		"java/lang/ClassLoader$1",
		"java/nio/ByteBufferImpl",
		"java/io/FilePermission",
		"gnu/java/nio/charset/ISO_8859_1$Encoder$1",
		"java/nio/charset/spi/CharsetProvider",
		"gnu/java/net/loader/URLStreamHandlerCache",
		"java/util/HashMap$HashIterator",
		"java/util/HashMap$HashEntry",
		"java/util/AbstractMap",
		"java/util/AbstractMap$1",
		"java/lang/RuntimeException",
		"java/util/Collections$UnmodifiableSet",
		"java/lang/ref/Reference",
		"java/lang/ref/WeakReference",
		"gnu/java/util/WeakIdentityHashMap$WeakBucket",
		"gnu/java/util/WeakIdentityHashMap$WeakEntrySet$1",
		"java/lang/String$CaseInsensitiveComparator",
		"java/lang/Throwable$StaticData",
		"java/lang/StackTraceElement",
		"java/lang/VMMath",
		"java/lang/Double",
		"java/lang/VMDouble",
		"java/lang/Long",
		"java/lang/Float",
		"java/lang/VMFloat",
		"java/security/Permissions", /* 200 */
		"java/security/AllPermission", 
		"java/security/AllPermission$AllPermissionCollection",
		"java/util/AbstractMap$1$1", /* 203 */
		"java/lang/Integer",
		
		NULL };

	if (getenv("FILTER_VERBOSE") && !printed)
	{
		i = 0;
		while (classes[i])
		{
			log_println("[%d] - %s", i, classes[i]);
			i++;
		}

		printed = true;
	}

	buf = getenv("INDEX");
	if (buf)
		sscanf(buf, "%d", &max_index);

	i = 0;
	while (classes[i] && i <= max_index)
	{

		if (!strcmp(classes[i], classname->text))
			return true;

		i++;
	}

	if ((buf = getenv("FILTER_VERBOSE")))
	{
		log_println("filtered: %s", classname->text);
	}

	return false;
}

void filter_match()
{
	/* Wohoo! */
}

/**
 * Causes filter_match() on which one can set a breakpoint in the debugger
 * in the following conditions:
 *
 * If the environment variable NO_FILTER is set, then filter_match() is not
 * called at all. This disables filter capabilities.
 *
 * If environment variable TEST_CLASS is set and the method belong to this
 * class and there is no variable TEST_METHOD then filter_match() is called.
 *
 * If TEST_CLASS and TEST_METHOD match the methodinfo and TEST_DESCRIPTOR is
 * not set.
 *
 * If TEST_CLASS, TEST_METHOD and TEST_DESCRIPTOR match the methodinfo's values.
 */
void filter_single(methodinfo *m)
{
	char *buf = NULL;

	buf = getenv("NO_FILTER");
	if (buf)
		return;

	buf = getenv("TEST_CLASS");
	if (!buf || strcmp(m->clazz->name->text, buf))
		return;

	buf = getenv("TEST_METHOD");
	if (!buf)
	{
		filter_match();
		return;
	}
	else if (strcmp(m->name->text, buf))
		return;

	buf = getenv("TEST_DESCRIPTOR");
	if (!buf)
	{
		filter_match();
		return;
	}
	else if (strcmp(m->descriptor->text, buf))
		return;

	filter_match();
}

Mutex *jitcache_lock;

/* jitcache_store **************************************************************

   Saves the generated machine code to disk.

*******************************************************************************/
void jitcache_store (methodinfo *m)
{
	static int init_lock = true;
	u1 *temp;
	int fd;

	if (init_lock)
	{
		jitcache_lock = Mutex_new();
		init_lock = false;
	}

/*
	filter_single(m);

	if (!filter(m->clazz->name))
		return;
*/

	/* Never try to store native method stubs because those include a reference
	 * a dynamically resolved function.
     *
     * TODO: Handle those, too.
	 */
	if (m->flags & ACC_NATIVE)
		return;

	fd = get_cache_file_writable(m);
	if (!fd)
	{
		if (opt_DebugJitCache)
	      log_message_method("[jitcache] store: got no file descriptor for ", m);

      return;
	}

	/* Write (and some read) file operations beyond this point.
	 * Acquire lock first because another thread may try to load a different
     * method from this class.
     */
/*	Mutex_lock(&m->clazz->cache_file_lock);*/
	Mutex_lock(jitcache_lock);

	if (opt_DebugJitCache)
      log_message_method("[jitcache] store: ", m);

	update_method_table(m, fd);

	/* flags, optlevel, basicblockcount, synchronizedoffset, stackframesize, entrypoint, mcodelength, mcode
    */
	os::write(fd, (const void *) &m->code->flags, sizeof(m->code->flags));

	os::write(fd, (const void *) &m->code->optlevel, sizeof(m->code->optlevel));
	os::write(fd, (const void *) &m->code->basicblockcount, sizeof(m->code->basicblockcount));

	os::write(fd, (const void *)  &m->code->synchronizedoffset, sizeof(m->code->synchronizedoffset));

	os::write(fd, (const void *)  &m->code->stackframesize, sizeof(m->code->stackframesize));

	temp = to_offset(m->code->mcode, m->code->entrypoint);
	os::write(fd, (const void *) &temp, sizeof(temp));

	os::write(fd, (const void *) &m->code->mcodelength, sizeof(m->code->mcodelength));

	os::write(fd, (const void *) m->code->mcode, m->code->mcodelength);

	store_to_file_exceptiontable(fd, m->code);

	store_to_file_linenumbertable(fd, m->code);

	store_to_file_patchers(fd, m->code);

	store_to_file_cachedrefs(fd, m->code);

/*	Mutex_unlock(&m->clazz->cache_file_lock);*/
	Mutex_unlock(jitcache_lock);

}

/* jitcache_load ***************************************************************

   Try to load previously generated machine code from disk.

   Returns non-zero if successfull.

*******************************************************************************/

u1 jitcache_load (methodinfo *m)
{
	codeinfo *code;
	u1 *endpc;
	int fd;

/*
	if (!filter(m->clazz->name))
		return false;
*/

	/* Never try to store native method stubs because those include a reference
	 * a dynamically resolved function.
	 */
	if (m->flags & ACC_NATIVE)
		return false;

	fd = get_cache_file_readable(m);
	if (fd <= 0)
	{
		if (opt_DebugJitCache)
	      log_message_method("[jitcache] load: got no file descriptor for ", m);

      return false;
	}

	if(!seek_method_table(m, fd))
	{
		os::close(fd);

		return false;
	}

	if (opt_DebugJitCache)
		log_message_method("[jitcache] load: ", m);

	code = code_codeinfo_new(m);
	m->code = code;

	/* flags, optlevel, basicblockcount, synchronizedoffset, stackframesize, entrypoint, mcodelength, mcode
    */
	os::read(fd, (void *) &code->flags, sizeof(code->flags));

	os::read(fd, (void *) &code->optlevel, sizeof(code->optlevel));
	os::read(fd, (void *) &code->basicblockcount, sizeof(code->basicblockcount));

	os::read(fd, (void *) &code->synchronizedoffset, sizeof(code->synchronizedoffset));

	os::read(fd, (void *) &code->stackframesize, sizeof(code->stackframesize));

	os::read(fd, (void *) &code->entrypoint, sizeof(code->entrypoint));

	os::read(fd, (void *) &code->mcodelength, sizeof(code->mcodelength));

	code->mcode = CNEW(u1, code->mcodelength);
	os::read(fd, (void *) code->mcode, code->mcodelength);
	code->entrypoint = to_abs(code->mcode, code->entrypoint);

	load_from_file_exceptiontable(code, fd);

	load_from_file_linenumbertable(code, fd);

	load_from_file_patchers(code, fd);

	load_from_file_cachedrefs(code, fd);

	os::close(fd);

	endpc = (u1 *) ((ptrint) code->mcode) + code->mcodelength;

	/* Insert method into methodtree to find the entrypoint. */

	methodtree_insert(code->entrypoint, endpc);

	/* flush the instruction and data caches */

	md_cacheflush(code->mcode, code->mcodelength);

	if (opt_DebugJitCache)
	{
		log_println("[jitcache] load - registered method: %x -> %x", code->entrypoint, endpc);
	}

	return true;
}

void jitcache_quit()
{
	int i;

	for (i = 0; i < mru_last_free; i++)
	{
		os::close(jc_mru_list[i]->cache_file_fd);
		jc_mru_list[i]->cache_file_fd = 0;
		jc_mru_list[i] = 0;
	}

	mru_last_free = 0;

	/* Closes all open file descriptors. */
}

void jitcache_freeclass(classinfo *c)
{
	if (c->cache_file_fd)
		jitcache_mru_remove(c);
}

/* Helper functions */
void update_method_table(methodinfo *m, int fd)
{
	int state = 0, i, temp, offset = 0;

	os::lseek(fd, 0, SEEK_SET);

	os::read(fd, (void *) &state, sizeof(state));

	/* table does not exist yet and needs to be created first */
	if (state != 1)
	{
		os::lseek(fd, 0, SEEK_SET);
		state = 1;
		os::write(fd, &state, sizeof(state));

		temp = -1;
		for (i = 0; i < m->clazz->methodscount; i++)
			os::write(fd, &temp, sizeof(temp));
	}

	/* get last offset in file */
	offset = os::lseek(fd, 0, SEEK_END);

	/* find out the index in the methods array */
	temp = -1;
	for (i = 0; i < m->clazz->methodscount; i++)
		if (&m->clazz->methods[i] == m)
		{
			temp = i;
			break;
		}
	assert(temp != -1);

	/* seek to the method's entry in the table */
	os::lseek(fd, temp * sizeof(int) + sizeof(int), SEEK_SET);

	/* enter the location */
	os::write(fd, &offset, sizeof(offset));

	os::lseek(fd, offset, SEEK_SET);
}

int seek_method_table(methodinfo *m, int fd)
{
	int state = 0, i, temp, offset;

	os::lseek(fd, 0, SEEK_SET);

	os::read(fd, (void *) &state, sizeof(state));

	/* if table does not exist, we cannot load any machine code from this file */
	if (state != 1)
		return 0;

	/* find out the index in the methods array */
	temp = -1;
	for (i = 0; i < m->clazz->methodscount; i++)
		if (&m->clazz->methods[i] == m)
		{
			temp = i;
			break;
		}
	assert(temp != -1);

	/* seek to the method's entry in the table */
	os::lseek(fd, temp * sizeof(int) + sizeof(int), SEEK_SET);

	/* get the location */
	os::read(fd, &offset, sizeof(offset));

	if (offset > 0)
	{
		os::lseek(fd, offset, SEEK_SET);
		return offset;
	}

	return 0;
}

int get_cache_file_readable(methodinfo *m)
{
	char *dest_file;
	int fd;

	if (m->clazz->cache_file_fd)
		return dup(m->clazz->cache_file_fd);


	/* load from filesystem */
	dest_file = get_dest_file(m);

	if (os::access(dest_file, F_OK) != 0)
	{
		if (opt_DebugJitCache)
			log_message_method("[jitcache] no cache file found for ", m);

		os::free(dest_file);

		return 0;
	}

/*
	filter_single(m);
*/

	fd = open_to_read(dest_file);

	os::free(dest_file);

/*
	if (fd > 0)
		jitcache_mru_add(m->clazz, fd);
*/
	return fd;
}

int get_cache_file_writable(methodinfo *m)
{
	char *dest_file, *dest_dir;
	int fd;

	if (m->clazz->cache_file_fd)
		return m->clazz->cache_file_fd;

	/* try to get the file first */
	dest_file = get_dest_file(m);
	fd = open_to_write(dest_file);

	/* file does not exist. We need to create it and possibly
	 * the directory hierarchy as well.
	 */
	if (fd <= 0) {
		dest_dir = get_dest_dir(m);

		if (os::access(dest_dir, F_OK) != 0)
		{
			if (mkdir_hier(dest_dir, S_IRWXU | S_IRWXG) != 0)
			{
				if (opt_DebugJitCache)
					log_println("[jitcache] unable to create cache directory: %s", dest_dir);

				os::free(dest_dir);
				os::free(dest_file);

				return 0;
			}
		}

		os::free(dest_dir);

		/* try to open the file again. */
		fd = open_to_write(dest_file);
		os::free(dest_file);

		if (fd <= 0)
			return 0;
	}

	os::free(dest_file);

	jitcache_mru_add(m->clazz, fd);

	return fd;
}

/* mkdir_hier ******************************************************************

   Creates a directory hierarchy on the filesystem.

*******************************************************************************/
int mkdir_hier(char *path, mode_t mode)
{
	int index;
	int length = os::strlen(path);

	for (index = 0; index < length; index++)
	{
		if (path[index] == '/')
		{
			path[index] = 0;
			mkdir(path, mode);

			path[index] = '/';
		}
	}

	return mkdir(path, mode);
}

/* get_dest_file ****************************************************************

   Returns a string denoting the file in which the method's machine code
   (along with the other data) is stored.

*******************************************************************************/

char *get_dest_file(methodinfo *m)
{
	int len_cacheroot = os::strlen(CACHEROOT);
	int len_classname = utf_bytes(m->clazz->name);

	char *dest_file = os::calloc(sizeof(u1),
								   len_cacheroot
	                               + len_classname
	                               + 2);

	strcat(dest_file, CACHEROOT);
	utf_cat(dest_file, m->clazz->name);

	return dest_file;
}

/* get_dest_dir ****************************************************************

   Returns a string denoting the directory in which the method's machine code
   (along with the other data) is stored.

*******************************************************************************/

char *get_dest_dir(methodinfo *m)
{
	int len_cacheroot = os::strlen(CACHEROOT);
	int len_packagename = utf_bytes(m->clazz->packagename);

	char *dest_dir = os::calloc(sizeof(u1),
								   len_cacheroot
	                               + len_packagename + 2);

	strcat(dest_dir, CACHEROOT);
	utf_cat(dest_dir, m->clazz->packagename);

	/* Make trailing slash from package name to 0 */
	dest_dir[len_cacheroot + len_packagename + 2 - 1] = 0;

	return dest_dir;
}

/* to_abs **********************************************************************

   Generates an absolute pointer from an offset. You need this after loading
   a value from the disk which is absolute at runtime.

*******************************************************************************/

u1 *to_abs(u1 *base, u1 *offset)
{
	return (u1 *) ((ptrint) base + (ptrint) offset);
}

/* to_offset *******************************************************************

   Generates an offset from an absolute pointer. This has to be done to each
   absolute pointer before storing it to disk.

*******************************************************************************/

u1 *to_offset(u1 *base, u1 *abs)
{
	return (u1 *) ((ptrint) abs - (ptrint) base);
}

/* open_to_read ****************************************************************

   Open a file for reading.

*******************************************************************************/

int open_to_read(char *dest_file)
{
	int fd;
/*
	fd = os::open(dest_file, O_RDONLY, 0);
*/
	fd = os::open(dest_file,
					 O_RDWR,
					 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

	return fd;
}

/* open_to_write ***************************************************************

   Open a file for writing.

*******************************************************************************/

int open_to_write(char *dest_file)
{
	int fd;

/*	fd = os::open(filename,
					 O_CREAT | O_WRONLY,
					 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
*/
	fd = os::open(dest_file,
					 O_CREAT | O_RDWR,
					 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

	return fd;
}

/* store_utf *******************************************************************

   Writes a utf object to disk (via filedescriptor).

*******************************************************************************/
void store_utf(int fd, utf *s)
{
	int len;

	if (!s)
	{
		len = -1;
		os::write(fd, (const void *) &len, sizeof(len));
	}
	else
	{
		len = utf_bytes(s);
		os::write(fd, (const void *) &len, sizeof(len));
		os::write(fd, s->text, len);
	}
}

/* load_utf ********************************************************************

   Loads a UTF8 constant from the given filedescriptor and initializes
   the given pointer with it.
   In case the stored constant's length is -1 the returned string is NULL.

*******************************************************************************/
void load_utf(utf **s, int fd)
{
	int len = 0;
	char *tmp;

	os::read(fd, (void *) &len, sizeof(len));

	if (len == -1)
		*s = NULL;
	else
	{
		tmp = os::calloc(sizeof(char), len);

		os::read(fd, tmp, len);

		*s = utf_new(tmp, len);

/*		os::free(tmp);*/
	}
}


/* store_to_file_patchers ******************************************************

   Writes the patchers structure of a codeinfo to disk.

*******************************************************************************/
void store_to_file_patchers(int fd, codeinfo *code)
{
	int temp = 0;
	u1 *temp_ptr;
	patchref_t *pr;
	int j;

	list_t *patchers = code->patchers;

	/* serialize patchers list */
	os::write(fd, (const void *) &patchers->size, sizeof(patchers->size));
	if (opt_DebugJitCache)
		log_println("store_to_file_patchers - patchers size %d", patchers->size);

	for (pr = (patchref_t *) list_first(patchers); pr != NULL; pr = (patchref_t *) list_next(patchers, pr))
	{
		temp_ptr = to_offset(code->mcode, (u1 *) pr->mpc);
		os::write(fd, (const void *) &temp_ptr, sizeof(temp_ptr));

		temp_ptr = to_offset(code->mcode, (u1 *) pr->datap);
		os::write(fd, (const void *) &temp_ptr, sizeof(temp_ptr));

		os::write(fd, (const void *) &pr->disp, sizeof(pr->disp));

		temp = -1;
		j = 0;
		while (patcher_functions[j].patcher)
		{
			if (patcher_functions[j].patcher == pr->patcher)
			{
				temp = j;
				os::write(fd, (const void *) &j, sizeof(j));

				(*patcher_functions[j].serializer)(fd, pr, code->m);

				if (patcher_functions[j].serializer == s_dummy)
					log_println("store_to_file_patchers: unhandled patcher function for %d", j);
				break;
			}
			j++;
		}
		
		if (temp == -1)
		{
			log_println("warning! unknown patcher function stored!");
			os::write(fd, (const void *) &temp, sizeof(temp));
		}

		os::write(fd, (const void *) &pr->attached_ref, sizeof(pr->attached_ref));

		if (pr->attached_ref)
		{
			store_cachedref(fd, pr->attached_ref);

			/* Release the cached reference now because it should not be used
			 * in the current Cacao process.
			 */
			FREE(pr->attached_ref, cachedref_t);
			pr->attached_ref = NULL;
		}

		os::write(fd, (const void *) &pr->mcode, sizeof(pr->mcode));
	}
}


/* store_to_file_cachedrefs *****************************************************

   Writes the cachedrefs structure of a codeinfo to disk.

*******************************************************************************/
void store_to_file_cachedrefs(int fd, codeinfo *code)
{
	cachedref_t *cr;

	list_t *cachedrefs = code->cachedrefs;
	if (opt_DebugJitCache)
		log_println("store_to_file_cachedrefs - cachedrefs size %d", cachedrefs->size);

	/* serialize cachedrefs list */
	os::write(fd, (const void *) &cachedrefs->size, sizeof(cachedrefs->size));

	for (cr = (cachedref_t *) list_first(cachedrefs);
		 cr != NULL;
		 cr = (cachedref_t *) list_next(cachedrefs, cr))
		store_cachedref(fd, cr);
}

/* store_cachedref *************************************************************

   Stores a single cachedref_t instance to disk.

*******************************************************************************/

void store_cachedref(int fd, cachedref_t *cr)
{
	os::write(fd, (const void *) &cr->type, sizeof(cr->type));
	os::write(fd, (const void *) &cr->md_patch, sizeof(cr->md_patch));
	os::write(fd, (const void *) &cr->disp, sizeof(cr->disp));

	switch (cr->type) {
		case CRT_CODEINFO:
		case CRT_ENTRYPOINT:
		case CRT_CODEGEN_FINISH_NATIVE_CALL:
		case CRT_ASM_HANDLE_EXCEPTION:
		case CRT_ASM_HANDLE_NAT_EXCEPTION:
			/* Nothing to store. */
			break;
		case CRT_NUM:
			os::write(fd, (const void *) &cr->ref, sizeof(s4));
			break;
		case CRT_OBJECT_HEADER:
		case CRT_CLASSINFO:
		case CRT_CLASSINFO_INDEX:
		case CRT_CLASSINFO_INTERFACETABLE:
		case CRT_CLASSINFO_VFTBL:
			/* Store classinfo */
			store_classinfo(fd, (classinfo *) cr->ref);
			break;
		case CRT_BUILTIN:
		case CRT_BUILTIN_FP:
			store_builtin(fd, (builtintable_entry *) cr->ref);
			break;
		case CRT_STRING:
			store_string(fd, (java_object_t *) cr->ref);
			break;
		case CRT_METHODINFO_STUBROUTINE:
		case CRT_METHODINFO_TABLE:
		case CRT_METHODINFO_INTERFACETABLE:
		case CRT_METHODINFO_METHODOFFSET:
			store_methodinfo(fd, (methodinfo *) cr->ref);
			break;
		case CRT_FIELDINFO_VALUE:
		case CRT_FIELDINFO_OFFSET:
		case CRT_FIELDINFO_OFFSET_HIGH:
			store_fieldinfo(fd, (fieldinfo *) cr->ref);
			break;
		case CRT_JUMPREFERENCE:
			os::write(fd, (const void *) &cr->ref, sizeof(cr->ref));
			
			break;
		default:
			log_println("store_cachedref: Invalid cachedref type: %d", cr->type);
			assert(0);
	}
}


/* store_to_file_exceptiontable ************************************************

   Writes the exceptiontable structure of a codeinfo to disk.

*******************************************************************************/
void store_to_file_exceptiontable(int fd, codeinfo *code)
{
	int count = 0;
	u1 *temp_ptr;
	int i;
	utf *name;

	/* serialize exceptiontable */

	/* temp will contain the amount of exceptiontable entries or zero
	 * if none exists.
     */
	if (code->exceptiontable)
		count = code->exceptiontable->length;

	os::write(fd, (const void *) &count, sizeof(count));
	if (opt_DebugJitCache)
		log_println("store_exceptiontable - exceptiontable size %d", count);

	for (i = 0; i < count; i++)
	{
		exceptiontable_entry_t *entry = &code->exceptiontable->entries[i];

		temp_ptr = to_offset(code->mcode, entry->endpc);
		os::write(fd, (const void *) &temp_ptr, sizeof(temp_ptr));

		temp_ptr = to_offset(code->mcode, entry->startpc);
		os::write(fd, (const void *) &temp_ptr, sizeof(temp_ptr));

		temp_ptr = to_offset(code->mcode, entry->handlerpc);
		os::write(fd, (const void *) &temp_ptr, sizeof(temp_ptr));

		/* store class name of entry->catchtype */
		if (entry->catchtype.any)
		{
			name = CLASSREF_OR_CLASSINFO_NAME(entry->catchtype);
			store_utf(fd, name);
		}
		else
			store_utf(fd, NULL);

	}

}


/* store_to_file_linenumbertable ***********************************************

   Writes the linenumbertable structure of a codeinfo to disk.

*******************************************************************************/
void store_to_file_linenumbertable(int fd, codeinfo *code)
{
	void *temp_ptr;
	linenumbertable_entry_t *lte;
	int count = 0;
	int i;
	linenumbertable_t *linenumbertable;

	linenumbertable = code->linenumbertable;

	if (code->linenumbertable)
		count = code->linenumbertable->length;

	/* serialize patchers list */
	os::write(fd, (const void *) &count, sizeof(count));

	if (opt_DebugJitCache)
		log_println("store_to_file_linenumbertable - linenumbertable size %d", count);

	if (count)
	{
		lte = linenumbertable->entries;
		for (i = 0; i < count; i++)
		{
			os::write(fd, (const void *) &lte->linenumber, sizeof(lte->linenumber));
	
			temp_ptr = to_offset(code->entrypoint, lte->pc);
			os::write(fd, (const void *) &temp_ptr, sizeof(temp_ptr));

			lte++;
		}
	}

}


/* load_from_file_patchers *****************************************************

   Loads the patchers structure of codeinfo from a file.

*******************************************************************************/
void load_from_file_patchers(codeinfo *code, int fd)
{
	int temp = 0;
	u1 *temp_ptr;
	int count = 0;
	int i;

	/* serialize patchers list */
	os::read(fd, (void *) &count, sizeof(count));

	if (opt_DebugJitCache	/* Insert method into methodtree to find the entrypoint. */
)
		log_println("load_from_file_patchers - patcher size %d", count);

	patcher_list_create(code);

	for (i = 0;i < count; i++)
	{
		patchref_t *pr = NEW(patchref_t);

		os::read(fd, (void *) &temp_ptr, sizeof(temp_ptr));
		pr->mpc = (ptrint) to_abs(code->mcode, temp_ptr);

		os::read(fd, (void *) &temp_ptr, sizeof(temp_ptr));
		pr->datap = (ptrint) to_abs(code->mcode, temp_ptr);

		os::read(fd, (void *) &pr->disp, sizeof(pr->disp));

		os::read(fd, (void *) &temp, sizeof(temp));
		if (temp == -1)
		{
			vm_abort("Invalid patcher function index loaded!");
			temp = 0;
		}
		pr->patcher = patcher_functions[temp].patcher;

		(*patcher_functions[temp].deserializer)(pr, fd, code->m);

		/* Load the pointer value to decide whether a cached reference must
         * be loaded or not. */
		os::read(fd, (void *) &pr->attached_ref, sizeof(pr->attached_ref));

		if (pr->attached_ref)
		{
			pr->attached_ref = NULL;
			load_cachedref(&pr->attached_ref, fd, code);
		}

		os::read(fd, (void *) &pr->mcode, sizeof(pr->mcode));

		pr->done = false;

		list_add_first(code->patchers, pr);
	}
}

/* load_from_file_cachedrefs ***************************************************

   Loads the cachedrefs structure of codeinfo from a file.

   Note: code->entrypoint *must* be valid at this point!

   Binary format:
   int - number of cachedref_t instances in the file
   cachedref_t - see load_cachedref

*******************************************************************************/
void load_from_file_cachedrefs(codeinfo *code, int fd)
{
	cachedref_t *cr;
	int count = 0;
	int i;

	/* serialize cachedrefs list */
	os::read(fd, (void *) &count, sizeof(count));

	if (opt_DebugJitCache)
		log_println("load_from_file_cachedrefs - cachedrefs size %d", count);

	jitcache_list_reset(code);

	cr = NEW(cachedref_t);

	for (i = 0;i < count; i++)
	{
		load_cachedref(&cr, fd, code);

		/* Write the restored reference into the code. */
		if (cr->md_patch)
			patch_md(cr->md_patch, ((ptrint) code->entrypoint) + cr->disp, cr->ref);
		else
		{
		  *((u1 **) (code->entrypoint + cr->disp)) = cr->ref;
		}

	}

	FREE(cr, cachedref_t);
}


/* load_cachedref **************************************************************

	Loads a cached reference from disk and 

	Binary format:
     s4 - disp value
     cachedreftype - type value
     * - cached ref specific (depends on type)

*******************************************************************************/

void load_cachedref(cachedref_t **result_cr, int fd, codeinfo *code)
{
	cachedref_t			*cr;
	classinfo			*ci;
	methodinfo			*mi;
	fieldinfo			*fi;
	builtintable_entry  *bte;
	java_object_t		*h;

	if (*result_cr)
		cr = *result_cr;
	else
		*result_cr = cr = NEW(cachedref_t);

	os::read(fd, (void *) &cr->type, sizeof(cr->type));
	os::read(fd, (void *) &cr->md_patch, sizeof(cr->md_patch));
	os::read(fd, (void *) &cr->disp, sizeof(cr->disp));

	switch (cr->type) {
		case CRT_CODEINFO:
			/* Just set the current codeinfo. */
			cr->ref = (void*) code;
			break;
		case CRT_NUM:
			os::read(fd, (void *) &cr->ref, sizeof(s4));
			break;
		case CRT_ENTRYPOINT:
			/* Just set the current entrypoint. */
			cr->ref = (void*) code->entrypoint;
			break;
		case CRT_CODEGEN_FINISH_NATIVE_CALL:
			/* Just set the pointer to codegen_finish_native_call. */
			cr->ref = (void*) (ptrint) codegen_finish_native_call;
			break;
		case CRT_ASM_HANDLE_EXCEPTION:
			/* Just set the pointer to asm_handle_exception. */
			cr->ref = (void*) (ptrint) asm_handle_exception;
			break;
		case CRT_ASM_HANDLE_NAT_EXCEPTION:
			/* Just put the pointer to asm_handle_nat_exception. */
			cr->ref = (void*) (ptrint) asm_handle_nat_exception;
			break;
		case CRT_OBJECT_HEADER:
			/* Load classinfo */
			load_classinfo(&ci, fd, code->m);
			cr->ref = &ci->object.header;
			break;
		case CRT_BUILTIN:
			load_builtin(&bte, fd);
			/* TODO: For the time being prefer the stub if it exists, otherwise
			 * use the function pointer directlty.
			 * This should go away with a moving garbage collector.
             */
			cr->ref = (void*) (bte->stub == NULL ? (ptrint) bte->fp : (ptrint) bte->stub);

			break;
		case CRT_BUILTIN_FP:
			load_builtin(&bte, fd);
			cr->ref = (void*) (ptrint) bte->fp;

			break;
		case CRT_STRING:
			load_string(&h, fd);
			cr->ref = (void*) h;
			break;
		case CRT_CLASSINFO:
			/* Load classinfo */
			load_classinfo(&ci, fd, code->m);
			cr->ref = (void*) ci;

			break;
		case CRT_CLASSINFO_INDEX:
			/* Load classinfo */
			load_classinfo(&ci, fd, code->m);
			cr->ref = (void*) ci->index;
			break;
		case CRT_CLASSINFO_INTERFACETABLE:
			/* Load classinfo */
			load_classinfo(&ci, fd, code->m);
			cr->ref = (void*) (OFFSET(vftbl_t, interfacetable[0]) -
						ci->index * sizeof(methodptr*));
			break;
		case CRT_CLASSINFO_VFTBL:
			/* Load classinfo */
			load_classinfo(&ci, fd, code->m);
			cr->ref = (void*) ci->vftbl;
			break;
		case CRT_METHODINFO_STUBROUTINE:
			load_methodinfo(&mi, fd, code->m);
			cr->ref = (void*) mi->stubroutine;
			break;
		case CRT_METHODINFO_TABLE:
			load_methodinfo(&mi, fd, code->m);
			cr->ref = (void*) ((OFFSET(vftbl_t, table[0]) +
							   sizeof(methodptr) * mi->vftblindex));
			break;
		case CRT_METHODINFO_INTERFACETABLE:
			load_methodinfo(&mi, fd, code->m);
			cr->ref = (void*) (OFFSET(vftbl_t, interfacetable[0]) -
					sizeof(methodptr) * mi->clazz->index);
			break;
		case CRT_METHODINFO_METHODOFFSET:
			load_methodinfo(&mi, fd, code->m);
			cr->ref = (void*) ((sizeof(methodptr) * (mi - mi->clazz->methods)));
			break;
		case CRT_FIELDINFO_VALUE:
			load_fieldinfo(&fi, fd, code->m);

			cr->ref = (void*) fi->value;
			break;
		case CRT_FIELDINFO_OFFSET:
			load_fieldinfo(&fi, fd, code->m);

			cr->ref = (void*) fi->offset;
			break;
		case CRT_FIELDINFO_OFFSET_HIGH:
			/* Should be used on 32 bit archs only. */
			load_fieldinfo(&fi, fd, code->m);

			cr->ref = (void*) fi->offset + 4;
			break;
		case CRT_JUMPREFERENCE:
			os::read(fd, (void *) &cr->ref, sizeof(cr->ref));

			cr->ref = (void*) ((ptrint) cr->ref + (ptrint) code->entrypoint);
			break;
		default:
			log_println("Invalid (or unhandled) cachedreference type: %d", cr->type);
			assert(0);
			break;
	}


	if (opt_DebugJitCache)
	{
		if (cr->md_patch)
			log_println("[%X, %d]: replace (md) %X with %X", code->entrypoint + cr->disp, cr->type, 0xFFF & (u4) (*(u1 **) (code->entrypoint + cr->disp)), cr->ref);
		else
		{
		log_println("[%X, %d]: replace %X with %X", code->entrypoint + cr->disp, cr->type, *((u1 **) (code->entrypoint + cr->disp)), cr->ref);
		if ((cr->type == CRT_BUILTIN || cr->type == CRT_BUILTIN_FP) && (void*) (*((u1 **) (code->entrypoint + cr->disp))) != cr->ref)
			log_println("[!!!] differing builtin function pointer: %s", bte->cname);
		}
	}


}

/* load_from_file_exceptiontable ***********************************************

   Loads the exceptiontable structure of codeinfo from a file.

*******************************************************************************/
void load_from_file_exceptiontable(codeinfo *code, int fd)
{
	int i;
	u1 *temp_ptr;
	utf *classname;
	constant_classref *classref;
	exceptiontable_entry_t *ete;

	code->exceptiontable = NEW(exceptiontable_t);

	os::read(fd, (void *) &code->exceptiontable->length, sizeof(code->exceptiontable->length));

	if (opt_DebugJitCache)
		log_println("load_exceptiontable - exceptiontable size %d", code->exceptiontable->length);


	ete = MNEW(exceptiontable_entry_t, code->exceptiontable->length);
	code->exceptiontable->entries = ete;

	for (i = 0; i < code->exceptiontable->length; i++)
	{
		os::read(fd, (void *) &temp_ptr, sizeof(temp_ptr));
		ete->endpc = to_abs(code->mcode, temp_ptr);

		os::read(fd, (void *) &temp_ptr, sizeof(temp_ptr));
		ete->startpc = to_abs(code->mcode, temp_ptr);

		os::read(fd, (void *) &temp_ptr, sizeof(temp_ptr));
		ete->handlerpc = to_abs(code->mcode, temp_ptr);

		/* load class name of entry->catchtype */
		load_utf(&classname, fd);

		if (classname)
		{
			classref = NEW(constant_classref);
			CLASSREF_INIT(*classref, code->m->clazz, classname);

			ete->catchtype = CLASSREF_OR_CLASSINFO(classref);
		}
		else
			ete->catchtype.any = NULL;

		ete++;
	}

}


/* load_from_file_linenumbertable **********************************************

   Loads the linenumbertable structure of codeinfo from a file.

*******************************************************************************/
void load_from_file_linenumbertable(codeinfo *code, int fd)
{
	linenumbertable_entry_t *lte;
	void *temp_ptr;
	int i;

	code->linenumbertable = NEW(linenumbertable_t);

	os::read(fd, (void *) &code->linenumbertable->length, sizeof(code->linenumbertable->length));

	if (opt_DebugJitCache)
		log_println("load_linenumbertable - linenumbertable size %d", code->linenumbertable->length);

	lte = MNEW(linenumbertable_entry_t, code->linenumbertable->length);
	code->linenumbertable->entries = lte;

	for (i = 0;i < code->linenumbertable->length; i++)
	{
		os::read(fd, (void *) &lte->linenumber, sizeof(lte->linenumber));

		os::read(fd, (void *) &temp_ptr, sizeof(temp_ptr));
		lte->pc = to_abs(code->entrypoint, temp_ptr);

		lte++;
	}
}


/* s_dummy *********************************************************************

   Patcher serialization function which does nothing and can therefore be used
   as a placeholder for not yet written serializers.

*******************************************************************************/
void s_dummy(int fd, patchref_t *pr, methodinfo *m)
{
  /* Intentionally does nothing. */
}

/* s_unresolved_class **********************************************************

   Serializes a unresolved_class reference.

   Binary format:
   - utf string - classname

*******************************************************************************/
void s_unresolved_class(int fd, patchref_t *pr, methodinfo *m)
{
	unresolved_class *uc;

	uc = (unresolved_class *) pr->ref;

	/* Store the class name ... */
	store_utf(fd, uc->classref->name);

/*
	log_println("s_unresolved_class:");
	log_message_utf("class:", uc->classref->name);
*/
}

/* s_unresolved_field **********************************************************

	Serializes a unresolved_field reference.

	Binary format:
	s4  - unresolved_field.flags
	int - index into class' cpinfo that denotes the unresolved_field's
		  constant_FMIref

*******************************************************************************/
void s_unresolved_field(int fd, patchref_t *pr, methodinfo *m)
{
	int i;

	unresolved_field *ref = (unresolved_field *) pr->ref;

/*
	log_println("s_unresolved_field:");
	log_message_utf("field name: ", ref->fieldref->name);
	log_message_utf("field desc: ", ref->fieldref->descriptor);
	log_message_utf("field's class: ", FIELDREF_CLASSNAME(ref->fieldref));
*/
	os::write(fd, (const void *) &ref->flags, sizeof(ref->flags));

	for (i = 0; i < m->clazz->cpcount; i++)
	{
		if (m->clazz->cpinfos[i] == (void*) ref->fieldref)
		{
			os::write(fd, (const void *) &i, sizeof(i));

			return;
		}
	}
	/* We should be out at this point. */

	vm_abort("fieldref not found");
}

/* s_unresolved_method **********************************************************

   Serializes a unresolved_method reference.

   Binary format:
   undecided

*******************************************************************************/
void s_unresolved_method(int fd, patchref_t *pr, methodinfo *m)
{
	int i;

	unresolved_method *ref = (unresolved_method *) pr->ref;

	os::write(fd, (const void *) &ref->flags, sizeof(ref->flags));

	for (i = 0; i < m->clazz->cpcount; i++)
	{
		if (m->clazz->cpinfos[i] == (void*) ref->methodref)
		{
			os::write(fd, (const void *) &i, sizeof(i));

			return;
		}
	}
	/* We should be out at this point. */

	vm_abort("methodref not found");
}

/* s_classinfo *****************************************************************

   Serializes a classinfo reference.

*******************************************************************************/
void s_classinfo(int fd, patchref_t *pr, methodinfo *m)
{
	classinfo *ci;

	ci = (classinfo *) pr->ref;

	store_classinfo(fd, ci);
}

/* s_methodinfo ****************************************************************

	Serializes a methodinfo reference.

*******************************************************************************/
void s_methodinfo(int fd, patchref_t *pr, methodinfo *m)
{
	methodinfo *mi;

	mi = (methodinfo *) pr->ref;

	store_methodinfo(fd, mi);
}

/* store_methodinfo ************************************************************

	Serializes a methodinfo reference.

	Binary format:
	utf - method name
	utf - method descriptor
	utf - class to which method belongs

*******************************************************************************/
void store_methodinfo(int fd, methodinfo *mi)
{
	store_utf(fd, mi->name);
	store_utf(fd, mi->descriptor);
	store_utf(fd, mi->clazz->name);
}

/* s_fieldinfo ****************************************************************

	Serializes a fieldinfo reference.

	Binary format:
	utf - field name
	utf - field descriptor
	utf - class to which field belongs

*******************************************************************************/
void s_fieldinfo(int fd, patchref_t *pr, methodinfo *m)
{
	fieldinfo *fi;

	fi = (fieldinfo *) pr->ref;

	store_fieldinfo(fd, fi);
}

void store_fieldinfo(int fd, fieldinfo *fi)
{
	store_utf(fd, fi->name);
	store_utf(fd, fi->descriptor);
	store_utf(fd, fi->clazz->name);
}

/* s_constant_classref *********************************************************

   Serializes a constant_classref reference.

   Binary format:
   - utf string - constant_classref's classname

*******************************************************************************/
void s_constant_classref(int fd, patchref_t *pr, methodinfo *m)
{
	constant_classref *cr = (constant_classref *) pr->ref;

	store_utf(fd, cr->name);
}


/* store_builtin *******************************************************************

   Serializes a constant_classref reference.

   Binary format:
   - s4 - key from builtintable_get_key()

*******************************************************************************/
void store_builtin(int fd, builtintable_entry *bte)
{
	s4					key;

	key = builtintable_get_key(bte);

	os::write(fd, (const void *) &key, sizeof(key));
}

/* store_string ****************************************************************

   Serializes a java_object_t reference which denotes a string.

   Binary format:
   - utf - utf bytes of the string instance

*******************************************************************************/
void store_string(int fd, java_object_t *h)
{
	utf				*string;

	string = javastring_toutf((java_handle_t *) h, false);

	store_utf(fd, string);
}


/* d_dummy *********************************************************************

   Patcher deserialization function which does nothing and can therefore be used
   as a placeholder for not yet written deserializers.

*******************************************************************************/
void d_dummy(patchref_t *pr, int fd, methodinfo *m)
{
  /* Intentionally do nothing. */
}

/*
 * Loads UTF8 classname and creates an unresolved_class for it
 * using the class to which the patchref_t belongs as the referer.
 */
void d_unresolved_class(patchref_t *pr, int fd, methodinfo *m)
{
	utf *classname;
	constant_classref *classref;
	unresolved_class *uc;

	classref = NEW(constant_classref);

	load_utf(&classname, fd);

	CLASSREF_INIT(*classref, m->clazz, classname);

	uc = create_unresolved_class(m, classref, NULL);

	pr->ref = (void*) uc;

/*	FREE(classref, constant_classref);*/

/*	os::free(classname); */
}

void d_unresolved_field(patchref_t *pr, int fd, methodinfo *m)
{
	int i;

	unresolved_field *ref = NEW(unresolved_field);

	os::read(fd, (void *) &ref->flags, sizeof(ref->flags));

	os::read(fd, (void *) &i, sizeof(i));
	ref->fieldref = (constant_FMIref *) m->clazz->cpinfos[i];

	ref->referermethod = m;

	UNRESOLVED_SUBTYPE_SET_EMTPY(ref->valueconstraints);

	pr->ref = (void*) ref;
}

void d_unresolved_method(patchref_t *pr, int fd, methodinfo *m)
{
	int i;

	unresolved_method *ref = NEW(unresolved_method);

	os::read(fd, (void *) &ref->flags, sizeof(ref->flags));

	os::read(fd, (void *) &i, sizeof(i));
	ref->methodref = (constant_FMIref *) m->clazz->cpinfos[i];

	ref->referermethod = m;
	ref->paramconstraints = NULL;
	UNRESOLVED_SUBTYPE_SET_EMTPY(ref->instancetypes);

	pr->ref = (void*) ref;
}

void d_classinfo(patchref_t *pr, int fd, methodinfo *m)
{
	classinfo *ci;

	load_classinfo(&ci, fd, m);

	pr->ref = (void*) ci;
}

void d_methodinfo(patchref_t *pr, int fd, methodinfo *m)
{
	methodinfo *lm;

	load_methodinfo(&lm, fd, m);

	pr->ref = (void*) lm;
}

void load_methodinfo(methodinfo **lm, int fd, methodinfo *m)
{
	utf *m_name;
	utf *m_desc;
	utf *classname;
	classinfo *class;
	constant_classref ref;

	load_utf(&m_name, fd);
	load_utf(&m_desc, fd);
	load_utf(&classname, fd);

	CLASSREF_INIT(ref, m->clazz, classname);

	class = resolve_classref_eager(&ref);

	*lm = class_findmethod(class, m_name, m_desc);
}

void d_fieldinfo(patchref_t *pr, int fd, methodinfo *m)
{
	fieldinfo *fi;

	load_fieldinfo(&fi, fd, m);
	
	pr->ref = (void*) fi;
}

void load_fieldinfo(fieldinfo **fi, int fd, methodinfo *m)
{
	utf *f_name;
	utf *f_desc;
	utf *classname;
	classinfo *class;
	constant_classref ref;

	load_utf(&f_name, fd);
	load_utf(&f_desc, fd);
	load_utf(&classname, fd);

	CLASSREF_INIT(ref, m->clazz, classname);

	class = resolve_classref_eager(&ref);
/*
	if (!(class->state & CLASS_INITIALIZED))
		if (!initialize_class(class))
*/
	*fi = class_findfield(class, f_name, f_desc);
}

/*
 * Loads UTF8 classname and initializes a constant_classref for it
 * using the class to which the patchref_t belongs as the referer.
 */
void d_constant_classref(patchref_t *pr, int fd, methodinfo *m)
{
	utf *classname;
	constant_classref *cr = NEW(constant_classref);

	load_utf(&classname, fd);

	CLASSREF_INIT(*cr, m->clazz, classname);

	pr->ref = (void*) cr;

/*	os::free(classname);*/
}

void load_builtin(builtintable_entry **bte, int fd)
{
	s4					key;

	os::read(fd, (void *) &key, sizeof(key));

	*bte = builtintable_get_by_key(key);
}

void load_string(java_object_t **h, int fd)
{
	utf *string;

	load_utf(&string, fd);

/*	*h = javastring_new(string);*/
	*h = literalstring_new(string);
}

/* store_classinfo *************************************************************

   Serializes a classinfo reference.

   Binary format:
   - utf string - classinfo's classname

*******************************************************************************/
void store_classinfo(int fd, classinfo *ci)
{
	/* Store the class name ... */
	store_utf(fd, ci->name);
}


/* load_classinfo *************************************************************

   Deserializes a classinfo reference.

   Binary format: see store_classinfo

*******************************************************************************/
void load_classinfo(classinfo **ci, int fd, methodinfo *m)
{
	utf *classname;
	constant_classref *classref;

	classref = NEW(constant_classref);

	load_utf(&classname, fd);

	CLASSREF_INIT(*classref, m->clazz, classname);

	*ci = resolve_classref_eager(classref);
}
#endif

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
