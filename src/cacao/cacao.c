/* src/cacao/cacao.c - contains main() of cacao

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, J. Wenninger, Institut f. Computersprachen - TU Wien

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

   Authors: Reinhard Grafl

   Changes: Andi Krall
            Mark Probst
            Philipp Tomsich
            Christian Thalinger

   $Id: cacao.c 4552 2006-03-04 17:15:44Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "vm/types.h"

#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/jni.h"
#include "native/include/java_lang_String.h"

#if defined(ENABLE_JVMTI)
#include "native/jvmti/jvmti.h"
#include "native/jvmti/dbg.h"
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#endif

#include "toolbox/logging.h"
#include "vm/classcache.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/stringlocal.h"
#include "vm/suck.h"
#include "vm/vm.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"

#ifdef TYPEINFO_DEBUG_TEST
#include "vm/jit/verify/typeinfo.h"
#endif


#ifdef TYPECHECK_STATISTICS
void typecheck_print_statistics(FILE *file);
#endif

/* setup_debugger_process *****************************************************

   Helper function to start JDWP threads

*******************************************************************************/
#if defined(ENABLE_JVMTI)

static void setup_debugger_process(char* transport) {
	java_objectheader *o;
	methodinfo *m;
	java_lang_String  *s;

	/* new gnu.classpath.jdwp.Jdwp() */
	mainclass = 
		load_class_from_sysloader(utf_new_char("gnu.classpath.jdwp.Jdwp"));
	if (!mainclass)
		throw_main_exception_exit();

	o = builtin_new(mainclass);

	if (!o)
		throw_main_exception_exit();
	
	m = class_resolveclassmethod(mainclass,
								 utf_init, 
								 utf_java_lang_String__void,
								 class_java_lang_Object,
								 true);
	if (!m)
		throw_main_exception_exit();

	(void) vm_call_method_intern(m, o, NULL, NULL, NULL);

	/* configure(transport,NULL) */
	m = class_resolveclassmethod(
		mainclass, utf_new_char("configure"), 
		utf_new_char("(Ljava/lang/String;Ljava/lang/Thread;)V"),
		class_java_lang_Object,
		false);


	s = javastring_new_char(transport);

	(void) vm_call_method_intern(m, o, s, NULL, NULL);

	if (!m)
		throw_main_exception_exit();

	/* _doInitialization */
	m = class_resolveclassmethod(mainclass,
								 utf_new_char("_doInitialization"), 
								 utf_new_char("()V"),
								 mainclass,
								 false);
	
	if (!m)
		throw_main_exception_exit();

	(void) vm_call_method_intern(m, o, NULL, NULL, NULL);
}
#endif


/* getmainclassfromjar *********************************************************

   Gets the name of the main class form a JAR's manifest file.

*******************************************************************************/

static char *getmainclassnamefromjar(char *mainstring)
{
	classinfo         *c;
	java_objectheader *o;
	methodinfo        *m;
	java_lang_String  *s;

	c = load_class_from_sysloader(utf_new_char("java/util/jar/JarFile"));

	if (!c)
		throw_main_exception_exit();
	
	/* create JarFile object */

	o = builtin_new(c);

	if (!o)
		throw_main_exception_exit();


	m = class_resolveclassmethod(c,
								 utf_init, 
								 utf_java_lang_String__void,
								 class_java_lang_Object,
								 true);

	if (!m)
		throw_main_exception_exit();

	s = javastring_new_char(mainstring);

	(void) vm_call_method_intern(m, o, s, NULL, NULL);

	if (*exceptionptr)
		throw_main_exception_exit();

	/* get manifest object */

	m = class_resolveclassmethod(c,
								 utf_new_char("getManifest"), 
								 utf_new_char("()Ljava/util/jar/Manifest;"),
								 class_java_lang_Object,
								 true);

	if (!m)
		throw_main_exception_exit();

	o = vm_call_method_intern(m, o, NULL, NULL, NULL);

	if (o == NULL) {
		fprintf(stderr, "Could not get manifest from %s (invalid or corrupt jarfile?)\n", mainstring);
		vm_exit(1);
	}


	/* get Main Attributes */

	m = class_resolveclassmethod(o->vftbl->class,
								 utf_new_char("getMainAttributes"), 
								 utf_new_char("()Ljava/util/jar/Attributes;"),
								 class_java_lang_Object,
								 true);

	if (!m)
		throw_main_exception_exit();

	o = vm_call_method_intern(m, o, NULL, NULL, NULL);

	if (o == NULL) {
		fprintf(stderr, "Could not get main attributes from %s (invalid or corrupt jarfile?)\n", mainstring);
		vm_exit(1);
	}


	/* get property Main-Class */

	m = class_resolveclassmethod(o->vftbl->class,
								 utf_new_char("getValue"), 
								 utf_new_char("(Ljava/lang/String;)Ljava/lang/String;"),
								 class_java_lang_Object,
								 true);

	if (!m)
		throw_main_exception_exit();

	s = javastring_new_char("Main-Class");

	o = vm_call_method_intern(m, o, s, NULL, NULL);

	if (!o)
		throw_main_exception_exit();

	return javastring_tochar(o);
}


void exit_handler(void);


/* main ************************************************************************

   The main program.
   
*******************************************************************************/

int main(int argc, char **argv)
{
	void *dummy;
	s4 i;
	
	/* local variables ********************************************************/

	JavaVMInitArgs *vm_args;
	JavaVM         *jvm;                /* denotes a Java VM                  */

#if defined(ENABLE_JVMTI)
	bool dbg = false;
	char *transport;
	int waitval;
#endif

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	stackbottom = &dummy;
#endif
	
	if (atexit(vm_exit_handler))
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Unable to register exit_handler");


	/**************************** Program start *****************************/

	if (opt_verbose)
		log_text("CACAO started -------------------------------------------------------");

	/* prepare the options */

	vm_args = options_prepare(argc, argv);
	
	/* load and initialize a Java VM, return a JNI interface pointer in env */

	JNI_CreateJavaVM(&jvm, (void **) &_Jv_env, vm_args);

#if defined(ENABLE_JVMTI)
	set_jvmti_phase(JVMTI_PHASE_START);
#endif

	/* do we have a main class? */

	if (mainstring == NULL)
		usage();


	/* start worker routines **************************************************/

	if (opt_run == true) {
		utf              *mainutf;
		classinfo        *mainclass;
		methodinfo       *m;
		java_objectarray *oa; 
		s4                oalength;
		java_lang_String *s;
		s4                status;

		/* set return value to OK */

		status = 0;

		if (opt_jar == true) {
			/* open jar file with java.util.jar.JarFile */
			mainstring = getmainclassnamefromjar(mainstring);
		}

		/* load the main class */

		mainutf = utf_new_char(mainstring);

		if (!(mainclass = load_class_from_sysloader(mainutf)))
			throw_main_exception_exit();

		/* error loading class, clear exceptionptr for new exception */

		if (*exceptionptr || !mainclass) {
/*  			*exceptionptr = NULL; */

/*  			*exceptionptr = */
/*  				new_exception_message(string_java_lang_NoClassDefFoundError, */
/*  									  mainstring); */
			throw_main_exception_exit();
		}

		if (!link_class(mainclass))
			throw_main_exception_exit();
			
		/* find the `main' method of the main class */

		m = class_resolveclassmethod(mainclass,
									 utf_new_char("main"), 
									 utf_new_char("([Ljava/lang/String;)V"),
									 class_java_lang_Object,
									 false);

		if (*exceptionptr) {
			throw_main_exception_exit();
		}

		/* there is no main method or it isn't static */

		if (!m || !(m->flags & ACC_STATIC)) {
			*exceptionptr = NULL;

			*exceptionptr =
				new_exception_message(string_java_lang_NoSuchMethodError,
									  "main");
			throw_main_exception_exit();
		}

		/* build argument array */

		oalength = vm_args->nOptions - opt_index;

		oa = builtin_anewarray(oalength, class_java_lang_String);

		for (i = 0; i < oalength; i++) {
			s = javastring_new(utf_new_char(vm_args->options[opt_index + i].optionString));
			oa->data[i] = (java_objectheader *) s;
		}

#ifdef TYPEINFO_DEBUG_TEST
		/* test the typeinfo system */
		typeinfo_test();
#endif
		/*class_showmethods(currentThread->group->header.vftbl->class);	*/

#if defined(ENABLE_JVMTI) && defined(NATIVE_THREADS)
		if(dbg) {
			debuggee = fork();
			if (debuggee == (-1)) {
				log_text("fork error");
				exit(1);
			} else {
				if (debuggee == 0) {
					/* child: allow helper process to trace us  */
					if (TRACEME != 0)  exit(0);
					
					/* give parent/helper debugger process control */
					kill(0, SIGTRAP);  /* do we need this at this stage ? */

					/* continue with normal startup */	

				} else {

					/* parent/helper debugger process */
					wait(&waitval);

					remotedbgjvmtienv = new_jvmtienv();
					/* set eventcallbacks */
					if (JVMTI_ERROR_NONE == 
						remotedbgjvmtienv->
						SetEventCallbacks(remotedbgjvmtienv,
										  &jvmti_jdwp_EventCallbacks,
										  sizeof(jvmti_jdwp_EventCallbacks))){
						log_text("unable to setup event callbacks");
						vm_exit(1);
					}

					/* setup listening process (JDWP) */
					setup_debugger_process(transport);

					/* start to be debugged program */
					CONT(debuggee);

                    /* exit debugger process - todo: cleanup */
					joinAllThreads();
					cacao_exit(0);
				}
			}
		}
		else 
			debuggee= -1;
		
#endif
		/* here we go... */

		(void) vm_call_method_intern(m, oa, NULL, NULL, NULL);

		/* exception occurred? */

		if (*exceptionptr) {
			throw_main_exception();
			status = 1;
		}

		/* unload the JavaVM */

		vm_destroy(jvm);

		/* and exit */

		vm_exit(status);
	}

	/************* If requested, compile all methods ********************/

	if (compileall) {
		classinfo *c;
		methodinfo *m;
		u4 slot;
		s4 i;
		classcache_name_entry *nmen;
		classcache_class_entry *clsen;

		/* create all classes found in the bootclasspath */
		/* XXX currently only works with zip/jar's */

		loader_load_all_classes();

		/* link all classes */

		for (slot = 0; slot < hashtable_classcache.size; slot++) {
			nmen = (classcache_name_entry *) hashtable_classcache.ptr[slot];

			for (; nmen; nmen = nmen->hashlink) {
				/* iterate over all class entries */

				for (clsen = nmen->classes; clsen; clsen = clsen->next) {
					c = clsen->classobj;

					if (!c)
						continue;

					if (!(c->state & CLASS_LINKED)) {
						if (!link_class(c)) {
							fprintf(stderr, "Error linking: ");
							utf_fprint_classname(stderr, c->name);
							fprintf(stderr, "\n");

							/* print out exception and cause */

							exceptions_print_exception(*exceptionptr);

							/* goto next class */

							continue;
						}
					}

					/* compile all class methods */

					for (i = 0; i < c->methodscount; i++) {
						m = &(c->methods[i]);

						if (m->jcode) {
							if (!jit_compile(m)) {
								fprintf(stderr, "Error compiling: ");
								utf_fprint_classname(stderr, c->name);
								fprintf(stderr, ".");
								utf_fprint(stderr, m->name);
								utf_fprint(stderr, m->descriptor);
								fprintf(stderr, "\n");

								/* print out exception and cause */

								exceptions_print_exception(*exceptionptr);
							}
						}
					}
				}
			}
		}
	}


	/******** If requested, compile a specific method ***************/

	if (opt_method != NULL) {
		methodinfo *m;

		/* create, load and link the main class */

		if (!(mainclass = load_class_bootstrap(utf_new_char(mainstring))))
			throw_main_exception_exit();

		if (!link_class(mainclass))
			throw_main_exception_exit();

		if (opt_signature != NULL) {
			m = class_resolveclassmethod(mainclass,
										 utf_new_char(opt_method),
										 utf_new_char(opt_signature),
										 mainclass,
										 false);
		} else {
			m = class_resolveclassmethod(mainclass,
										 utf_new_char(opt_method),
										 NULL,
										 mainclass,
										 false);
		}

		if (!m) {
			char message[MAXLOGTEXT];
			sprintf(message, "%s%s", opt_method,
					opt_signature ? opt_signature : "");

			*exceptionptr =
				new_exception_message(string_java_lang_NoSuchMethodException,
									  message);
										 
			throw_main_exception_exit();
		}
		
		jit_compile(m);
	}

	vm_shutdown(0);

	/* keep compiler happy */

	return 0;
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
