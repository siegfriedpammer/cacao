/* src/native/vm/VMSystemProperties.c - gnu/classpath/VMSystemProperties

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

   $Id: VMSystemProperties.c 4126 2006-01-10 20:55:41Z twisti $

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
	char       *locale;
	char       *lang;
	char       *region;
	struct utsname utsnamebuf;

	/* endianess union */
	union {
		u4 i;
		u1 c[4];
	} u;

#if !defined(ENABLE_STATICVM)
	char *ld_library_path;
	char *libpath;
	s4    libpathlen;
#endif

	if (!p) {
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

	properties_system_add("java.version", JAVA_VERSION);
	properties_system_add("java.vendor", "GNU Classpath");
	properties_system_add("java.vendor.url", "http://www.gnu.org/software/classpath/");
	properties_system_add("java.home", java_home ? java_home : CACAO_INSTALL_PREFIX);
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

	/* Set bootclasspath properties. One for GNU classpath and the other for  */
	/* compatibility with Sun (required by most applications).                */
	properties_system_add("java.boot.class.path", bootclasspath);
	properties_system_add("sun.boot.class.path", bootclasspath);

#if defined(ENABLE_STATICVM)
	properties_system_add("gnu.classpath.boot.library.path", ".");
	properties_system_add("java.library.path" , ".");
#else /* defined(ENABLE_STATICVM) */
	/* fill gnu.classpath.boot.library.path with GNU classpath library path */

	libpathlen = strlen(CLASSPATH_INSTALL_DIR) +
		strlen(CLASSPATH_LIBRARY_PATH) + strlen("0");

	libpath = MNEW(char, libpathlen);

	strcat(libpath, CLASSPATH_INSTALL_DIR);
	strcat(libpath, CLASSPATH_LIBRARY_PATH);

	properties_system_add("gnu.classpath.boot.library.path", libpath);

	MFREE(libpath, char, libpathlen);


	/* now fill java.library.path */

	ld_library_path = getenv("LD_LIBRARY_PATH");

	if (ld_library_path) {
		libpathlen = strlen(".:") + strlen(ld_library_path) + strlen("0");

		libpath = MNEW(char, libpathlen);

		strcpy(libpath, ".:");
		strcat(libpath, ld_library_path);

		properties_system_add("java.library.path", libpath);

		MFREE(libpath, char, libpathlen);

	} else {
		properties_system_add("java.library.path", "");
	}
#endif /* defined(ENABLE_STATICVM) */

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

	properties_system_add("java.ext.dirs", "");

#if defined(DISABLE_GC)
	/* When we disable the GC, we mmap the whole heap to a specific
	   address, so we can compare call traces. For this reason we have
	   to add the same properties on different machines, otherwise
	   more memory may be allocated (e.g. strlen("i386")
	   vs. strlen("powerpc"). */

 	properties_system_add("os.name", "unknown");
	properties_system_add("os.arch", "unknown");
	properties_system_add("os.version", "unknown");
#else
 	properties_system_add("os.name", utsnamebuf.sysname);
	properties_system_add("os.arch", utsnamebuf.machine);
	properties_system_add("os.version", utsnamebuf.release);
#endif

	properties_system_add("file.separator", "/");
	/* properties_system_add("file.encoding", "null"); -- this must be set properly */
	properties_system_add("path.separator", ":");
	properties_system_add("line.separator", "\n");
	properties_system_add("user.name", user ? user : "null");
	properties_system_add("user.home", home ? home : "null");
	properties_system_add("user.dir", cwd ? cwd : "null");

	/* Are we little or big endian? */

	u.i = 1;
	properties_system_add("gnu.cpu.endian", u.c[0] ? "little" : "big");

	/* get locales */

	locale = getenv("LANG");
	if (locale != NULL) { /* gnu classpath is going to set en as language */
		if (strlen(locale) <= 2) {
			properties_system_add("user.language", locale);
		} else {
			if ((locale[2]=='_')&&(strlen(locale)>=5)) {
				lang = MNEW(char, 3);
				strncpy(lang, (char*)&locale[0], 2);
				lang[2]='\0';
				region = MNEW(char, 3);
				strncpy(region, (char*)&locale[3], 2);
				region[2]='\0';
				properties_system_add("user.language", lang);
				properties_system_add("user.region", region);
			}
		}
	}
	
	
#if 0
	/* how do we get them? */
	{ "user.country", "US" },
	{ "user.timezone", "Europe/Vienna" },

	/* XXX do we need this one? */
	{ "java.protocol.handler.pkgs", "gnu.java.net.protocol"}
#endif
	properties_system_add("java.protocol.handler.pkgs", "gnu.java.net.protocol");

	/* add remaining properties defined on commandline to the Java
	   system properties */

	properties_system_add_all();

	/* clean up */

	MFREE(cwd, char, 0);

	return;
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
