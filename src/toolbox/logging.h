/* toolbox/logging.h - contains logging functions

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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

   $Id: logging.h 1086 2004-05-26 21:22:05Z twisti $

*/


#ifndef _LOGGING_H
#define _LOGGING_H

#include <stdio.h>

/* This is to bring in the conflicting panic from darwin */
#include <sys/mman.h>
/*  #define panic cacao_panic */

#define PANICIF(when,txt)  if(when)panic(txt)

#define MAXLOGTEXT 500

/* function prototypes */

void log_init(char *fname);
void log_text(char *txt);
void log_plain(char *txt); /* same as log_text without "LOG: " and newline */
void log_flush();          /* fflush logfile */
void log_nl();             /* newline and fflush */

void log_cputime();

void log_message_class(char *msg, classinfo *c);
void log_message_method(char *msg, methodinfo *m);

void dolog(char *txt, ...);
void dolog_plain(char *txt, ...); /* same as dolog without "LOG: " and newline */
void error(char *txt, ...);
void panic(char *txt);

FILE *get_logfile(); /* return the current logfile */

s8 getcputime();

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
