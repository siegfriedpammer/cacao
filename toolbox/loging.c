/************************* toolbox/loging.c ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Not documented, see loging.h.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/10/03

*******************************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "loging.h"

/***************************************************************************
                        LOG FILE HANDLING 
***************************************************************************/

char logtext[MAXLOGTEXT];   /* Needs to be filled with desired text before */
                            /* call to dolog() */


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

Writes the contents of logtext to both the protocol file (if opened) and to
stdout.

**************************************************************************/

void dolog()
{
	if (logfile) {
		fprintf (logfile, "%s\n",logtext);
		fflush (logfile);
		}
	else {
		printf ("LOG: %s\n",logtext);
		fflush (stdout);
		}
}

/********************* Function: log_text ********************************/

void log_text (char *text)
{
	sprintf (logtext, "%s",text);
	dolog();
}


/********************* Function: log_cputime ****************************/

void log_cputime ()
{
   long int t;
   int sec,usec;

   t = getcputime();
   sec = t/1000000;
   usec = t%1000000;

   sprintf (logtext, "Total CPU usage: %d seconds and %d milliseconds",
            sec,usec/1000);
   dolog();
}



/************************** Function: error *******************************

Like dolog(), but terminates the program immediately.

**************************************************************************/

void error()
{
	if (logfile) {
		fprintf (logfile, "ERROR: %s\n", logtext);
		}   
	printf ("ERROR: %s\n",logtext);
	exit(10);
}


/************************ Function: panic (txt) ****************************

  Like error(), takes the text to output as an argument

***************************************************************************/

void panic(char *txt)
{
	sprintf (logtext, "%s", txt);
	error();
}


/********************** Function: getcputime ********************************

	Returns the used CPU time in microseconds
	
****************************************************************************/

long int getcputime()
{
   struct rusage ru;
   int sec,usec;

   getrusage (RUSAGE_SELF, &ru);
   sec = ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
   usec = ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;
   return sec*1000000 + usec;
}

