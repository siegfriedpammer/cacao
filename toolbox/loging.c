/* toolbox/loging.c - contains loging functions

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

   $Id: loging.c 701 2003-12-07 16:26:58Z edwin $

*/


#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "global.h"
#include "loging.h"


/***************************************************************************
                        LOG FILE HANDLING 
***************************************************************************/

FILE *logfile = NULL;



void log_init(char *fname)
{
	if (fname) {
		if (fname[0]) {
			logfile = fopen(fname, "w");
		}
	}
}


/*********************** Function: dolog ************************************

Writes logtext to the protocol file (if opened) or to stdout.

**************************************************************************/

void dolog(char *txt, ...)
{
	char logtext[MAXLOGTEXT];
	va_list ap;

	va_start(ap, txt);
	vsprintf(logtext, txt, ap);
	va_end(ap);

	if (logfile) {
		fprintf(logfile, "%s\n",logtext);
		fflush(logfile);

	} else {
		fprintf(stdout,"LOG: %s\n",logtext);
		fflush(stdout);
	}
}


/******************** Function: dolog_plain *******************************

Writes logtext to the protocol file (if opened) or to stdout.

**************************************************************************/

void dolog_plain(char *txt, ...)
{
	char logtext[MAXLOGTEXT];
	va_list ap;

	va_start(ap, txt);
	vsprintf(logtext, txt, ap);
	va_end(ap);

	if (logfile) {
		fprintf(logfile, "%s",logtext);
		fflush(logfile);

	} else {
		fprintf(stdout,"%s",logtext);
		fflush(stdout);
	}
}


/********************* Function: log_text ********************************/

void log_text(char *text)
{
	dolog("%s", text);
}


/******************** Function: log_plain *******************************/

void log_plain(char *text)
{
	dolog_plain("%s", text);
}


/****************** Function: get_logfile *******************************/

FILE *get_logfile()
{
	return (logfile) ? logfile : stdout;
}


/********************* Function: log_cputime ****************************/

void log_cputime()
{
   long int t;
   int sec, usec;
   char logtext[MAXLOGTEXT];

   t = getcputime();
   sec = t / 1000000;
   usec = t % 1000000;

   sprintf(logtext, "Total CPU usage: %d seconds and %d milliseconds",
		   sec, usec / 1000);
   dolog(logtext);
}


/************************** Function: error *******************************

Like dolog(), but terminates the program immediately.

**************************************************************************/

void error(char *txt, ...)
{
	char logtext[MAXLOGTEXT];
	va_list ap;

	va_start(ap, txt);
	vsprintf(logtext, txt, ap);
	va_end(ap);

	if (logfile) {
		fprintf(logfile, "ERROR: %s\n", logtext);
	}

	fprintf(stderr, "ERROR: %s\n", logtext);
	exit(10);
}


/************************ Function: panic (txt) ****************************

  Like error(), takes the text to output as an argument

***************************************************************************/

void panic(char *txt)
{
	error("%s", txt);
}


/********************** Function: getcputime ********************************

	Returns the used CPU time in microseconds
	
****************************************************************************/

long int getcputime()
{
	struct rusage ru;
	int sec, usec;

	getrusage(RUSAGE_SELF, &ru);
	sec = ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
	usec = ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;
	return sec * 1000000 + usec;
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
