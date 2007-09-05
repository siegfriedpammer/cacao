/* src/vm/properties.c - handling commandline properties

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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


#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>

#if defined(WITH_JRE_LAYOUT)
# include <libgen.h>
#endif

#include "vm/types.h"

#include "mm/memory.h"

#include "native/jni.h"
#include "native/llni.h"

#include "vm/global.h"                      /* required by java_lang_String.h */
#include "native/include/java_lang_String.h"

#include "toolbox/list.h"
#include "toolbox/util.h"

#include "vm/properties.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vm/jit/asmpart.h"

#include "vmcore/class.h"
#include "vmcore/method.h"
#include "vmcore/options.h"


/* internal property structure ************************************************/

typedef struct list_properties_entry_t list_properties_entry_t;

struct list_properties_entry_t {
	char       *key;
	char       *value;
	listnode_t  linkage;
};


/* global variables ***********************************************************/

static list_t *list_properties = NULL;


/* properties_init *************************************************************

   Initialize the properties list and fill the list with default
   values.

*******************************************************************************/

void properties_init(void)
{
	list_properties = list_create(OFFSET(list_properties_entry_t, linkage));
}


/* properties_set **************************************************************

   Fill the properties list with default values.

*******************************************************************************/

void properties_set(void)
{
	int             len;
	char           *p;

	char           *java_home;
	char           *boot_class_path;

#if defined(ENABLE_JAVASE)
	char           *class_path;
	char           *boot_library_path;

# if defined(WITH_CLASSPATH_GNU)
	char           *cwd;
	char           *env_user;
	char           *env_home;
	char           *env_lang;
	char           *extdirs;
	char           *lang;
	char           *country;
	struct utsname *utsnamebuf;

	char           *java_library_path;
# endif
#endif

#if defined(WITH_JRE_LAYOUT)
	/* SUN also uses a buffer of 4096-bytes (strace is your friend). */

	p = MNEW(char, 4096);

	if (readlink("/proc/self/exe", p, 4095) == -1)
		vm_abort("properties_set: readlink failed: %s\n", strerror(errno));

	/* Get the path of the current executable. */

	p = dirname(p);

# if defined(WITH_CLASSPATH_GNU)

	/* Set java.home. */

	len = strlen(path) + strlen("/..") + strlen("0");

	java_home = MNEW(char, len);

	strcpy(java_home, p);
	strcat(java_home, "/..");

	/* Set the path to Java core native libraries. */

	len = strlen(cacao_prefix) + strlen("/lib/classpath") + strlen("0");

	boot_library_path = MNEW(char, len);

	strcpy(boot_library_path, java_home);
	strcat(boot_library_path, "/lib/classpath");

# elif defined(WITH_CLASSPATH_SUN)

	/* Find correct java.home.  We check if there is a JRE
	   co-located. */

	/* NOTE: We use the server VM here as it should be available on
	   all architectures. */

	len =
		strlen(p) +
		strlen("/../jre/lib/"JAVA_ARCH"/server/libjvm.so") +
		strlen("0");

	java_home = MNEW(char, len);

	strcpy(java_home, p);
	strcat(java_home, "/../jre/lib/"JAVA_ARCH"/server/libjvm.so");

	/* Check if that libjvm.so exists. */

	if (access(java_home, F_OK) == 0) {
		/* Yes, we add /jre to java.home. */

		strcpy(java_home, p);
		strcat(java_home, "/../jre");
	}
	else {
		/* No, java.home is parent directory. */

		strcpy(java_home, p);
		strcat(java_home, "/..");
	}

	/* Set the path to Java core native libraries. */

	len = strlen(java_home) + strlen("/lib/"JAVA_ARCH) + strlen("0");

	boot_library_path = MNEW(char, len);

	strcpy(boot_library_path, java_home);
	strcat(boot_library_path, "/lib/"JAVA_ARCH);

# else
#  error unknown classpath configuration
# endif

	/* Free path. */

	MFREE(p, char, len);

#else
	java_home         = CACAO_PREFIX;

# if defined(WITH_CLASSPATH_GNU)

	boot_library_path = CLASSPATH_LIBDIR"/classpath";

# elif defined(WITH_CLASSPATH_SUN)

	boot_library_path = CLASSPATH_LIBDIR;

# elif defined(WITH_CLASSPATH_CLDC1_1)

	/* No boot_library_path required. */

# else
#  error unknown classpath configuration
# endif
#endif

	properties_add("java.home", java_home);

	/* Set the bootclasspath. */

	p = getenv("BOOTCLASSPATH");

	if (p != NULL) {
		boot_class_path = MNEW(char, strlen(p) + strlen("0"));
		strcpy(boot_class_path, p);
	}
	else {
#if defined(WITH_JRE_LAYOUT)
# if defined(WITH_CLASSPATH_GNU)

		len =
			strlen(java_home) + strlen("/share/cacao/vm.zip:") +
			strlen(java_home) + strlen("/share/classpath/glibj.zip") +
			strlen("0");

		boot_class_path = MNEW(char, len);

		strcpy(boot_class_path, java_home);
		strcat(boot_class_path, "/share/cacao/vm.zip");
		strcat(boot_class_path, ":");
		strcat(boot_class_path, java_home);
		strcat(boot_class_path, "/share/classpath/glibj.zip");

# elif defined(WITH_CLASSPATH_SUN)

		/* This is the bootclasspath taken from HotSpot (see
		   hotspot/src/share/vm/runtime/os.cpp
		   (os::set_boot_path)). */

		len =
			strlen(java_home) + strlen("/lib/resources.jar:") +
			strlen(java_home) + strlen("/lib/rt.jar:") +
			strlen(java_home) + strlen("/lib/sunrsasign.jar:") +
			strlen(java_home) + strlen("/lib/jsse.jar:") +
			strlen(java_home) + strlen("/lib/jce.jar:") +
			strlen(java_home) + strlen("/lib/charsets.jar:") +
			strlen(java_home) + strlen("/classes") +
			strlen("0");

		boot_class_path = MNEW(char, len);

		strcpy(boot_class_path, java_home);
		strcat(boot_class_path, "/lib/resources.jar:");
		strcat(boot_class_path, java_home);
		strcat(boot_class_path, "/lib/rt.jar:");
		strcat(boot_class_path, java_home);
		strcat(boot_class_path, "/lib/sunrsasign.jar:");
		strcat(boot_class_path, java_home);
		strcat(boot_class_path, "/lib/jsse.jar:");
		strcat(boot_class_path, java_home);
		strcat(boot_class_path, "/lib/jce.jar:");
		strcat(boot_class_path, java_home);
		strcat(boot_class_path, "/lib/charsets.jar:");
		strcat(boot_class_path, java_home);
		strcat(boot_class_path, "/classes");

# else
#  error unknown classpath configuration
# endif
#else
# if defined(WITH_CLASSPATH_GNU)

		len =
			strlen(CACAO_VM_ZIP) +
			strlen(":") +
			strlen(CLASSPATH_CLASSES) +
			strlen("0");

		boot_class_path = MNEW(char, len);

		strcpy(boot_class_path, CACAO_VM_ZIP);
		strcat(boot_class_path, ":");
		strcat(boot_class_path, CLASSPATH_CLASSES);

# elif defined(WITH_CLASSPATH_SUN)

		/* This is the bootclasspath taken from HotSpot (see
		   hotspot/src/share/vm/runtime/os.cpp
		   (os::set_boot_path)). */

		len =
			strlen(CLASSPATH_PREFIX"/lib/resources.jar:") +
			strlen(CLASSPATH_PREFIX"/lib/rt.jar:") +
			strlen(CLASSPATH_PREFIX"/lib/sunrsasign.jar:") +
			strlen(CLASSPATH_PREFIX"/lib/jsse.jar:") +
			strlen(CLASSPATH_PREFIX"/lib/jce.jar:") +
			strlen(CLASSPATH_PREFIX"/lib/charsets.jar:") +
			strlen(CLASSPATH_PREFIX"/classes") +
			strlen("0");

		boot_class_path = MNEW(char, len);

		strcpy(boot_class_path, CLASSPATH_PREFIX"/lib/resources.jar:");
		strcat(boot_class_path, CLASSPATH_PREFIX"/lib/rt.jar:");
		strcat(boot_class_path, CLASSPATH_PREFIX"/lib/sunrsasign.jar:");
		strcat(boot_class_path, CLASSPATH_PREFIX"/lib/jsse.jar:");
		strcat(boot_class_path, CLASSPATH_PREFIX"/lib/jce.jar:");
		strcat(boot_class_path, CLASSPATH_PREFIX"/lib/charsets.jar:");
		strcat(boot_class_path, CLASSPATH_PREFIX"/classes");

# elif defined(WITH_CLASSPATH_CLDC1_1)

		len =
			strlen(CLASSPATH_CLASSES) +
			strlen("0");

		boot_class_path = MNEW(char, len);

		strcpy(boot_class_path, CLASSPATH_CLASSES);

# else
#  error unknown classpath configuration
# endif
#endif
	}

	properties_add("sun.boot.class.path", boot_class_path);
	properties_add("java.boot.class.path", boot_class_path);

#if defined(ENABLE_JAVASE)

	/* Set the classpath. */

	p = getenv("CLASSPATH");

	if (p != NULL) {
		class_path = MNEW(char, strlen(p) + strlen("0"));
		strcpy(class_path, p);
	}
	else {
		class_path = MNEW(char, strlen(".") + strlen("0"));
		strcpy(class_path, ".");
	}

	properties_add("java.class.path", class_path);

	/* Add java.vm properties. */

	properties_add("java.vm.specification.version", "1.0");
	properties_add("java.vm.specification.vendor", "Sun Microsystems Inc.");
	properties_add("java.vm.specification.name", "Java Virtual Machine Specification");
	properties_add("java.vm.version", VERSION);
	properties_add("java.vm.vendor", "CACAO Team");
	properties_add("java.vm.name", "CACAO");

# if defined(ENABLE_INTRP)
	if (opt_intrp) {
		/* XXX We don't support java.lang.Compiler */
/*  		properties_add("java.compiler", "cacao.intrp"); */
		properties_add("java.vm.info", "interpreted mode");
	}
	else
# endif
	{
		/* XXX We don't support java.lang.Compiler */
/*  		properties_add("java.compiler", "cacao.jit"); */
		properties_add("java.vm.info", "JIT mode");
	}

# if defined(WITH_CLASSPATH_GNU)

	/* Get properties from system. */

	cwd      = _Jv_getcwd();

	env_user = getenv("USER");
	env_home = getenv("HOME");
	env_lang = getenv("LANG");

	utsnamebuf = NEW(struct utsname);

	uname(utsnamebuf);

	properties_add("java.runtime.version", VERSION);
	properties_add("java.runtime.name", "CACAO");

	properties_add("java.specification.version", "1.5");
	properties_add("java.specification.vendor", "Sun Microsystems Inc.");
	properties_add("java.specification.name", "Java Platform API Specification");

	properties_add("java.version", JAVA_VERSION);
	properties_add("java.vendor", "GNU Classpath");
	properties_add("java.vendor.url", "http://www.gnu.org/software/classpath/");

	properties_add("java.class.version", CLASS_VERSION);

	properties_add("gnu.classpath.boot.library.path", boot_library_path);

	/* Get and set java.library.path. */

	java_library_path = getenv("LD_LIBRARY_PATH");

	if (java_library_path == NULL)
		java_library_path = "";

	properties_add("java.library.path", java_library_path);

	properties_add("java.io.tmpdir", "/tmp");

#  if defined(ENABLE_INTRP)
	if (opt_intrp) {
		properties_add("gnu.java.compiler.name", "cacao.intrp");
	}
	else
#  endif
	{
		properties_add("gnu.java.compiler.name", "cacao.jit");
	}

	/* set the java.ext.dirs property */

	len = strlen(java_home) + strlen("/jre/lib/ext") + strlen("0");

	extdirs = MNEW(char, len);

	strcpy(extdirs, java_home);
	strcat(extdirs, "/jre/lib/ext");

	properties_add("java.ext.dirs", extdirs);

	properties_add("java.endorsed.dirs", ""CACAO_PREFIX"/jre/lib/endorsed");

#  if defined(DISABLE_GC)
	/* When we disable the GC, we mmap the whole heap to a specific
	   address, so we can compare call traces. For this reason we have
	   to add the same properties on different machines, otherwise
	   more memory may be allocated (e.g. strlen("i386")
	   vs. strlen("alpha"). */

	properties_add("os.arch", "unknown");
 	properties_add("os.name", "unknown");
	properties_add("os.version", "unknown");
#  else
	properties_add("os.arch", JAVA_ARCH);
 	properties_add("os.name", utsnamebuf->sysname);
	properties_add("os.version", utsnamebuf->release);
#  endif

#  if WORDS_BIGENDIAN == 1
	properties_add("gnu.cpu.endian", "big");
#  else
	properties_add("gnu.cpu.endian", "little");
#  endif

	properties_add("file.separator", "/");
	properties_add("path.separator", ":");
	properties_add("line.separator", "\n");

	properties_add("user.name", env_user ? env_user : "null");
	properties_add("user.home", env_home ? env_home : "null");
	properties_add("user.dir", cwd ? cwd : "null");

	/* get locale */

	if (env_lang != NULL) {
		/* get the local stuff from the environment */

		if (strlen(env_lang) <= 2) {
			properties_add("user.language", env_lang);
		}
		else {
			if ((env_lang[2] == '_') && (strlen(env_lang) >= 5)) {
				lang = MNEW(char, 3);
				strncpy(lang, (char *) &env_lang[0], 2);
				lang[2] = '\0';

				country = MNEW(char, 3);
				strncpy(country, (char *) &env_lang[3], 2);
				country[2] = '\0';

				properties_add("user.language", lang);
				properties_add("user.country", country);
			}
		}
	}
	else {
		/* if no default locale was specified, use `en_US' */

		properties_add("user.language", "en");
		properties_add("user.country", "US");
	}

# elif defined(WITH_CLASSPATH_SUN)

	/* Actually this property is set by OpenJDK, but we need it in
	   nativevm_preinit(). */

	properties_add("sun.boot.library.path", boot_library_path);

# else

#  error unknown classpath configuration

# endif

#elif defined(ENABLE_JAVAME_CLDC1_1)

    properties_add("microedition.configuration", "CLDC-1.1");
    properties_add("microedition.platform", "generic");
    properties_add("microedition.encoding", "ISO8859_1");
    properties_add("microedition.profiles", "");

#else

# error unknown Java configuration

#endif
}


/* properties_add **************************************************************

   Adds a property entry to the internal property list.  If there's
   already an entry with the same key, replace it.

*******************************************************************************/

void properties_add(char *key, char *value)
{
	list_properties_entry_t *pe;

	/* search for the entry */

	for (pe = list_first_unsynced(list_properties); pe != NULL;
		 pe = list_next_unsynced(list_properties, pe)) {
		if (strcmp(pe->key, key) == 0) {
			/* entry was found, replace the value */

			pe->value = value;

			return;
		}
	}

	/* entry was not found, insert a new one */

	pe = NEW(list_properties_entry_t);

	pe->key   = key;
	pe->value = value;

	list_add_last_unsynced(list_properties, pe);
}


/* properties_get **************************************************************

   Get a property entry from the internal property list.

*******************************************************************************/

char *properties_get(char *key)
{
	list_properties_entry_t *pe;

	for (pe = list_first_unsynced(list_properties); pe != NULL;
		 pe = list_next_unsynced(list_properties, pe)) {
		if (strcmp(pe->key, key) == 0)
			return pe->value;
	}

	return NULL;
}


/* properties_system_add *******************************************************

   Adds a given property to the Java system properties.

*******************************************************************************/

void properties_system_add(java_handle_t *p, char *key, char *value)
{
	classinfo     *c;
	methodinfo    *m;
	java_handle_t *k;
	java_handle_t *v;

	/* search for method to add properties */

	LLNI_class_get(p, c);

	m = class_resolveclassmethod(c,
								 utf_put,
								 utf_new_char("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"),
								 NULL,
								 true);

	if (m == NULL)
		return;

	/* add to the Java system properties */

	k = javastring_new_from_utf_string(key);
	v = javastring_new_from_utf_string(value);

	(void) vm_call_method(m, p, k, v);
}


/* properties_system_add_all ***************************************************

   Adds all properties from the properties list to the Java system
   properties.

   ARGUMENTS:
       p.... is actually a java_util_Properties structure

*******************************************************************************/

#if defined(ENABLE_JAVASE)
void properties_system_add_all(java_handle_t *p)
{
	list_properties_entry_t *pe;
	classinfo               *c;
	methodinfo              *m;
	java_handle_t           *key;
	java_handle_t           *value;

	/* search for method to add properties */

	LLNI_class_get(p, c);

	m = class_resolveclassmethod(c,
								 utf_put,
								 utf_new_char("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"),
								 NULL,
								 true);

	if (m == NULL)
		return;

	/* process all properties stored in the internal table */

	for (pe = list_first(list_properties); pe != NULL;
		 pe = list_next(list_properties, pe)) {
		/* add to the Java system properties */

		key   = javastring_new_from_utf_string(pe->key);
		value = javastring_new_from_utf_string(pe->value);

		(void) vm_call_method(m, (java_handle_t *) p, key, value);
	}
}
#endif /* defined(ENABLE_JAVASE) */


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
