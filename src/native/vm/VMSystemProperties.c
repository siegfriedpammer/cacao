/* src/native/vm/VMSystemProperties.c - gnu/classpath/VMSystemProperties

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
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

   Contact: cacao@cacaojvm.org

   Authors: Christian Thalinger

   Changes:

   $Id: VMSystemProperties.c 4839 2006-04-25 17:42:08Z edwin $

*/


#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_util_Properties.h"
#include "toolbox/logging.h"
#include "toolbox/util.h"
#include "vm/exceptions.h"
#include "vm/builtin.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/properties.h"
#include "vm/suck.h"


/*
 * Class:     gnu/classpath/VMSystemProperties
 * Method:    preInit
 * Signature: (Ljava/util/Properties;)V
 */
JNIEXPORT void JNICALL Java_gnu_classpath_VMSystemProperties_preInit(JNIEnv *env, jclass clazz, java_util_Properties *p)
{
	char       *cwd;
	char       *java_home;
	char       *user;
	char       *home;
	char       *extdirs;
	s4          extdirslen;
	char       *locale;
	char       *lang;
	char       *country;
	struct utsname utsnamebuf;

#if !defined(WITH_STATIC_CLASSPATH)
	char *ld_library_path;
	char *libpath;
	s4    libpathlen;
#endif

	if (p == NULL) {
		exceptions_throw_nullpointerexception();
		return;
	}

	/* get properties from system */

	cwd = _Jv_getcwd();
	java_home = getenv("JAVA_HOME");
	user = getenv("USER");
	home = getenv("HOME");
	uname(&utsnamebuf);

	/* post-initialize the properties */

	if (!properties_postinit(p))
		return;

	/* set JAVA_HOME to default prefix if not defined */

	if (java_home == NULL)
		java_home = CACAO_PREFIX;

	properties_system_add("java.version", JAVA_VERSION);
	properties_system_add("java.vendor", "GNU Classpath");
	properties_system_add("java.vendor.url", "http://www.gnu.org/software/classpath/");
	properties_system_add("java.home", java_home);
	properties_system_add("java.vm.specification.version", "1.0");
	properties_system_add("java.vm.specification.vendor", "Sun Microsystems Inc.");
	properties_system_add("java.vm.specification.name", "Java Virtual Machine Specification");
	properties_system_add("java.vm.version", VERSION);
	properties_system_add("java.vm.vendor", "CACAO Team");
	properties_system_add("java.vm.name", "CACAO");
	properties_system_add("java.specification.version", "1.4");
	properties_system_add("java.specification.vendor", "Sun Microsystems Inc.");
	properties_system_add("java.specification.name", "Java Platform API Specification");
	properties_system_add("java.class.version", CLASS_VERSION);
	properties_system_add("java.class.path", classpath);

	properties_system_add("java.runtime.version", VERSION);
	properties_system_add("java.runtime.name", "CACAO");

	/* Set bootclasspath properties. One for GNU classpath and the
	   other for compatibility with Sun (required by most
	   applications). */

	properties_system_add("java.boot.class.path", bootclasspath);
	properties_system_add("sun.boot.class.path", bootclasspath);

#if defined(WITH_STATIC_CLASSPATH)
	properties_system_add("gnu.classpath.boot.library.path", ".");
	properties_system_add("java.library.path" , ".");
#else /* defined(WITH_STATIC_CLASSPATH) */
	/* fill gnu.classpath.boot.library.path with GNU Classpath library
       path */

	libpathlen = strlen(CLASSPATH_LIBRARY_PATH) + strlen("0");

	libpath = MNEW(char, libpathlen);

	strcpy(libpath, CLASSPATH_LIBRARY_PATH);

	properties_system_add("gnu.classpath.boot.library.path", libpath);

	MFREE(libpath, char, libpathlen);


	/* now fill java.library.path */

	ld_library_path = getenv("LD_LIBRARY_PATH");

	if (ld_library_path) {
		libpathlen = strlen(ld_library_path) + strlen("0");

		libpath = MNEW(char, libpathlen);

		strcpy(libpath, ld_library_path);

		properties_system_add("java.library.path", libpath);

		MFREE(libpath, char, libpathlen);

	} else {
		properties_system_add("java.library.path", "");
	}
#endif /* defined(WITH_STATIC_CLASSPATH) */

	properties_system_add("java.io.tmpdir", "/tmp");

#if defined(ENABLE_INTRP)
	if (opt_intrp) {
		/* XXX We don't support java.lang.Compiler */
/*  		properties_system_add("java.compiler", "cacao.intrp"); */
		properties_system_add("java.vm.info", "interpreted mode");
	} else
#endif
	{
		/* XXX We don't support java.lang.Compiler */
/*  		properties_system_add("java.compiler", "cacao.jit"); */
		properties_system_add("java.vm.info", "JIT mode");
	}

	/* set the java.ext.dirs property */

	extdirslen = strlen(java_home) + strlen("/jre/lib/ext") + strlen("0");

	extdirs = MNEW(char, extdirslen);

	strcpy(extdirs, java_home);
	strcat(extdirs, "/jre/lib/ext");

	properties_system_add("java.ext.dirs", extdirs);

	MFREE(extdirs, char, extdirslen);


#if defined(DISABLE_GC)
	/* When we disable the GC, we mmap the whole heap to a specific
	   address, so we can compare call traces. For this reason we have
	   to add the same properties on different machines, otherwise
	   more memory may be allocated (e.g. strlen("i386")
	   vs. strlen("alpha"). */

	properties_system_add("os.arch", "unknown");
 	properties_system_add("os.name", "unknown");
	properties_system_add("os.version", "unknown");
#else
	/* We need to set the os.arch hardcoded to be compatible with SUN. */

#if defined(__I386__)
	/* map all x86 architectures (i386, i486, i686) to i386 */

	properties_system_add("os.arch", "i386");
#elif defined(__POWERPC__)
	properties_system_add("os.arch", "ppc");
#elif defined(__X86_64__)
	properties_system_add("os.arch", "amd64");
#else
	/* default to what uname returns */

	properties_system_add("os.arch", utsnamebuf.machine);
#endif

 	properties_system_add("os.name", utsnamebuf.sysname);
	properties_system_add("os.version", utsnamebuf.release);
#endif

	properties_system_add("file.separator", "/");
	properties_system_add("path.separator", ":");
	properties_system_add("line.separator", "\n");
	properties_system_add("user.name", user ? user : "null");
	properties_system_add("user.home", home ? home : "null");
	properties_system_add("user.dir", cwd ? cwd : "null");

#if defined(WITH_STATIC_CLASSPATH)
	/* This is just for debugging purposes and can cause troubles in
       GNU Classpath. */

	properties_system_add("gnu.cpu.endian", "unknown");
#else
# if WORDS_BIGENDIAN == 1
	properties_system_add("gnu.cpu.endian", "big");
# else
	properties_system_add("gnu.cpu.endian", "little");
# endif
#endif

	/* get locale */

	locale = getenv("LANG");

	if (locale != NULL) {
		/* get the local stuff from the environment */

		if (strlen(locale) <= 2) {
			properties_system_add("user.language", locale);

		} else {
			if ((locale[2] == '_') && (strlen(locale) >= 5)) {
				lang = MNEW(char, 3);
				strncpy(lang, (char *) &locale[0], 2);
				lang[2] = '\0';

				country = MNEW(char, 3);
				strncpy(country, (char *) &locale[3], 2);
				country[2] = '\0';

				properties_system_add("user.language", lang);
				properties_system_add("user.country", country);
			}
		}

	} else {
		/* if no default locale was specified, use `en_US' */

		properties_system_add("user.language", "en");
		properties_system_add("user.country", "US");
	}
	

	/* Add remaining properties defined on commandline to the Java
	   system properties. */

	properties_system_add_all();

	/* clean up */

	MFREE(cwd, char, 0);
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
