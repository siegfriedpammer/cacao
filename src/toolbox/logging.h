/* src/toolbox/logging.h - contains logging functions

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

   Authors: Reinhard Grafl

   Changes: Christan Thalinger

   $Id: logging.h 4357 2006-01-22 23:33:38Z twisti $

*/


#ifndef _LOGGING_H
#define _LOGGING_H

#include <stdio.h>

#include "config.h"
#include "vm/class.h"
#include "vm/method.h"
#include "vm/utf8.h"


/*500 is to small for eclipse traces, (builtin_trace_args, perhaps the
buffer should be created there dynamically, if the text is longer,
instead of setting the size for all invocations that big*/

#define MAXLOGTEXT  16383 

/* function prototypes ********************************************************/

void log_init(const char *fname);
void log_text(const char *txt);

/* same as log_text without "LOG: " and newline */
void log_plain(const char *txt);

/* fflush logfile */
void log_flush(void);

/* newline and fflush */
void log_nl(void);

/* log message functions */
void log_message_utf(const char *msg, utf *u);
void log_message_class(const char *msg, classinfo *c);
void log_message_class_message_class(const char *msg1, classinfo *c1,
									 const char *msg2, classinfo *c2);
void log_message_method(const char *msg, methodinfo *m);

void dolog(const char *txt, ...);

/* same as dolog without "LOG: " and newline*/
void dolog_plain(const char *txt, ...);

FILE *get_logfile(void);                        /* return the current logfile */

/* write utf symbol to logfile/stdout */
void log_utf(utf *u);
void log_plain_utf(utf *u);

#endif /* _LOGGING_H */


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
