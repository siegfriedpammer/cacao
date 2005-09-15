/* src/toolbox/logging.c - contains logging functions

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

   Authors: Reinhard Grafl

   Changes: Christian Thalinger

   $Id: logging.c 3183 2005-09-15 20:07:37Z twisti $

*/


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "mm/memory.h"
#include "toolbox/logging.h"
#include "toolbox/util.h"
#include "vm/global.h"
#include "vm/tables.h"
#include "vm/statistics.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# endif
#endif


/***************************************************************************
                        LOG FILE HANDLING 
***************************************************************************/

FILE *logfile = NULL;


void log_init(const char *fname)
{
	if (fname) {
		if (fname[0]) {
			logfile = fopen(fname, "w");
		}
	}
}


/* dolog ***********************************************************************

   Writes logtext to the protocol file (if opened) or to stdout.

*******************************************************************************/

void dolog(const char *text, ...)
{
	va_list ap;

	if (logfile) {
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
		fprintf(logfile, "[%p] ", (void *) THREADOBJECT);
#endif

		va_start(ap, text);
		vfprintf(logfile, text, ap);
		va_end(ap);

		fflush(logfile);

	} else {
#if defined(USE_THREADS) && defined(NATIVE_THREADS)
		fprintf(stdout, "LOG: [%p] ", (void *) THREADOBJECT);
#else
		fputs("LOG: ", stdout);
#endif

		va_start(ap, text);
		vfprintf(stdout, text, ap);
		va_end(ap);

		fprintf(stdout, "\n");

		fflush(stdout);
	}
}


/******************** Function: dolog_plain *******************************

Writes logtext to the protocol file (if opened) or to stdout.

**************************************************************************/

void dolog_plain(const char *txt, ...)
{
	char logtext[MAXLOGTEXT];
	va_list ap;

	va_start(ap, txt);
	vsprintf(logtext, txt, ap);
	va_end(ap);

	if (logfile) {
		fprintf(logfile, "%s", logtext);
		fflush(logfile);

	} else {
		fprintf(stdout,"%s", logtext);
		fflush(stdout);
	}
}


/********************* Function: log_text ********************************/

void log_text(const char *text)
{
	dolog("%s", text);
}


/******************** Function: log_plain *******************************/

void log_plain(const char *text)
{
	dolog_plain("%s", text);
}


/****************** Function: get_logfile *******************************/

FILE *get_logfile(void)
{
	return (logfile) ? logfile : stdout;
}


/****************** Function: log_flush *********************************/

void log_flush(void)
{
	fflush(get_logfile());
}


/********************* Function: log_nl *********************************/

void log_nl(void)
{
	log_plain("\n");
	fflush(get_logfile());
}


/* log_message_utf *************************************************************

   Outputs log text like this:

   LOG: Creating class: java/lang/Object

*******************************************************************************/

void log_message_utf(const char *msg, utf *u)
{
	char *buf;
	s4    len;

	len = strlen(msg) + utf_strlen(u) + strlen("0");

	buf = MNEW(char, len);

	strcpy(buf, msg);
	utf_strcat(buf, u);

	log_text(buf);

	MFREE(buf, char, len);
}


/* log_message_class ***********************************************************

   Outputs log text like this:

   LOG: Loading class: java/lang/Object

*******************************************************************************/

void log_message_class(const char *msg, classinfo *c)
{
	log_message_utf(msg, c->name);
}


/* log_message_class_message_class *********************************************

   Outputs log text like this:

   LOG: Initialize super class java/lang/Object from java/lang/VMThread

*******************************************************************************/

void log_message_class_message_class(const char *msg1, classinfo *c1,
									 const char *msg2, classinfo *c2)
{
	char *buf;
	s4    len;

	len =
		strlen(msg1) + utf_strlen(c1->name) +
		strlen(msg2) + utf_strlen(c2->name) + strlen("0");

	buf = MNEW(char, len);

	strcpy(buf, msg1);
	utf_strcat(buf, c1->name);
	strcat(buf, msg2);
	utf_strcat(buf, c2->name);

	log_text(buf);

	MFREE(buf, char, len);
}


/* log_message_method **********************************************************

   Outputs log text like this:

   LOG: Compiling: java.lang.Object.clone()Ljava/lang/Object;

*******************************************************************************/

void log_message_method(const char *msg, methodinfo *m)
{
	char *buf;
	s4    len;

	len = strlen(msg) + utf_strlen(m->class->name) + strlen(".") +
		utf_strlen(m->name) + utf_strlen(m->descriptor) + strlen("0");

	buf = MNEW(char, len);

	strcpy(buf, msg);
	utf_strcat_classname(buf, m->class->name);
	strcat(buf, ".");
	utf_strcat(buf, m->name);
	utf_strcat(buf, m->descriptor);

	log_text(buf);

	MFREE(buf, char, len);
}


/* log_utf *********************************************************************

   Log utf symbol.

*******************************************************************************/

void log_utf(utf *u)
{
	char buf[MAXLOGTEXT];
	utf_sprint(buf, u);
	dolog("%s", buf);
}


/* log_plain_utf ***************************************************************

   Log utf symbol (without printing "LOG: " and newline).

*******************************************************************************/

void log_plain_utf(utf *u)
{
	char buf[MAXLOGTEXT];
	utf_sprint(buf, u);
	dolog_plain("%s", buf);
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
