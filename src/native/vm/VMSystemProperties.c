/* native/vm/VMSystemProperties.c - gnu/classpath/VMSystemProperties

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

   $Id: VMSystemProperties.c 1882 2005-01-24 16:53:38Z twisti $

*/


#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "cacao/cacao.h"
#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_util_Properties.h"
#include "toolbox/logging.h"
#include "vm/exceptions.h"
#include "vm/builtin.h"
#include "vm/loader.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"


/* temporary property structure */

typedef struct property property;

struct property {
	char     *key;
	char     *value;
	property *next;
};

static property *properties = NULL;


/* create_property *************************************************************

   Create a property entry for a command line property definition.

*******************************************************************************/

void create_property(char *key, char *value)
{
	property *p;

	p = NEW(property);
	p->key = key;
	p->value = value;
	p->next = properties;
	properties = p;
}


/* insert_property *************************************************************

   Used for inserting a property into the system's properties table. Method m
   (usually put) and the properties table must be given.

*******************************************************************************/

static void insert_property(methodinfo *m, java_util_Properties *p, char *key,
							char *value)
{
	asm_calljavafunction(m,
						 p,
						 javastring_new_char(key),
						 javastring_new_char(value),
						 NULL);
}


/*
 * Class:     gnu/classpath/VMSystemProperties
 * Method:    preInit
 * Signature: (Ljava/util/Properties;)V
 */
JNIEXPORT void JNICALL Java_gnu_classpath_VMSystemProperties_preInit(JNIEnv *env, jclass clazz, java_util_Properties *p)
{

#define BUFFERSIZE 200
	methodinfo *m;
	char cwd[BUFFERSIZE];
	char *java_home;
	char *user;
	char *home;
	char *locale;
	char *lang;
	char *region;
	struct utsname utsnamebuf;

	union {
		u4 i;
		u1 c[4];
	} u;

#if !defined(STATIC_CLASSPATH)
	char *libpath;
	s4    libpathlen;
#endif

	if (!p) {
		*exceptionptr = new_nullpointerexception();
		return;
	}

	/* get properties from system */

	(void) getcwd(cwd, BUFFERSIZE);
	java_home = getenv("JAVA_HOME");
	user = getenv("USER");
	home = getenv("HOME");
	uname(&utsnamebuf);

	/* search for method to add properties */

	m = class_resolveclassmethod(p->header.vftbl->class,
								 utf_new_char("put"),
								 utf_new_char("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"),
								 clazz,
								 true);

	if (!m)
		return;

	insert_property(m, p, "java.version", VERSION);
	insert_property(m, p, "java.vendor", "CACAO Team");
	insert_property(m, p, "java.vendor.url", "http://www.cacaojvm.org/");
	insert_property(m, p, "java.home", java_home ? java_home : CACAO_INSTALL_PREFIX);
	insert_property(m, p, "java.vm.specification.version", "1.0");
	insert_property(m, p, "java.vm.specification.vendor", "Sun Microsystems Inc.");
	insert_property(m, p, "java.vm.specification.name", "Java Virtual Machine Specification");
	insert_property(m, p, "java.vm.version", VERSION);
	insert_property(m, p, "java.vm.vendor", "CACAO Team");
	insert_property(m, p, "java.vm.name", "CACAO");
	insert_property(m, p, "java.specification.version", "1.4");
	insert_property(m, p, "java.specification.vendor", "Sun Microsystems Inc.");
	insert_property(m, p, "java.specification.name", "Java Platform API Specification");
	insert_property(m, p, "java.class.version", "48.0");
	insert_property(m, p, "java.class.path", classpath);

	/* Set bootclasspath properties. One for GNU classpath and the other for  */
	/* compatibility with Sun (required by most applications).                */
	insert_property(m, p, "java.boot.class.path", bootclasspath);
	insert_property(m, p, "sun.boot.class.path", bootclasspath);

#if defined(STATIC_CLASSPATH)
	insert_property(m, p, "java.library.path" , ".");
#else
	libpathlen = strlen(CACAO_INSTALL_PREFIX) + strlen(CACAO_LIBRARY_PATH) + 1;

	if (getenv("CACAO_LIB_OVERRIDE"))
		libpathlen += strlen(getenv("CACAO_LIB_OVERRIDE")) + 1;

	if (getenv("LD_LIBRARY_PATH"))
		libpathlen += strlen(getenv("LD_LIBRARY_PATH")) + 1;

	libpath = MNEW(char, libpathlen);

	if (getenv("CACAO_LIB_OVERRIDE")) {
		strcat(libpath, getenv("CACAO_LIB_OVERRIDE"));
		strcat(libpath, ":");
	}

	strcat(libpath, CACAO_INSTALL_PREFIX);
	strcat(libpath, CACAO_LIBRARY_PATH);

	if (getenv("LD_LIBRARY_PATH")) {
		strcat(libpath, ":");
		strcat(libpath, getenv("LD_LIBRARY_PATH"));
	}
	insert_property(m, p, "java.library.path", libpath);

	MFREE(libpath, char, libpathlen);
#endif

	insert_property(m, p, "java.io.tmpdir", "/tmp");

	/* XXX We don't support java.lang.Compiler */
/*  	insert_property(m, p, "java.compiler", "cacao.jit"); */

	insert_property(m, p, "java.ext.dirs", CACAO_INSTALL_PREFIX""CACAO_EXT_DIR);
 	insert_property(m, p, "os.name", utsnamebuf.sysname);
	insert_property(m, p, "os.arch", utsnamebuf.machine);
	insert_property(m, p, "os.version", utsnamebuf.release);
	insert_property(m, p, "file.separator", "/");
	/* insert_property(m, p, "file.encoding", "null"); -- this must be set properly */
	insert_property(m, p, "path.separator", ":");
	insert_property(m, p, "line.separator", "\n");
	insert_property(m, p, "user.name", user ? user : "null");
	insert_property(m, p, "user.home", home ? home : "null");
	insert_property(m, p, "user.dir", cwd ? cwd : "null");

	/* Are we little or big endian? */
	u.i = 1;
	insert_property(m, p, "gnu.cpu.endian", u.c[0] ? "little" : "big");

#ifdef USE_GTK
	/* disable gthread-jni's portable native sync due to yet unresolved 
	   threading issues */
	insert_property(m, p, "gnu.classpath.awt.gtk.portable.native.sync", "false");
#endif


	/* get locales */
	locale = getenv("LANG");
	if (locale != NULL) { /* gnu classpath is going to set en as language */
		if (strlen(locale) <= 2) {
			insert_property(m, p, "user.language", locale);
		} else {
			if ((locale[2]=='_')&&(strlen(locale)>=5)) {
				lang = MNEW(char, 3);
				strncpy(lang, (char*)&locale[0], 2);
				lang[2]='\0';
				region = MNEW(char, 3);
				strncpy(region, (char*)&locale[3], 2);
				region[2]='\0';
				insert_property(m, p, "user.language", lang);
				insert_property(m, p, "user.region", region);
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
	insert_property(m, p, "java.protocol.handler.pkgs", "gnu.java.net.protocol");

	/* insert properties defined on commandline */

	while (properties) {
		property *tp;

		insert_property(m, p, properties->key, properties->value);

		tp = properties;
		properties = properties->next;
		FREE(tp, property);
	}

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
