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
                        LOGFILE - BEHANDLUNG 
***************************************************************************/

char logtext[MAXLOGTEXT];   /* Musz mit dem gewuenschten Text vor */
                            /* Aufruf von dolog() beschrieben werden */


FILE *logfile = NULL;




void log_init(char *fname)
{
	if (fname) {
		if (fname[0]) {
			logfile = fopen(fname, "w");
			}
		}
}



/*********************** Funktion: dolog ************************************

Gibt den in logtext stehenden Text auf die Protokollierungsdatei
aus (wenn sie offen ist) und auszerdem auf stdout. 

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

/********************* Funktion: log_text ********************************/

void log_text (char *text)
{
	sprintf (logtext, "%s",text);
	dolog();
}


/********************* Funktion: log_cputime ****************************/

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



/************************** Funktion: error *******************************

Wie dolog(), aber das Programm wird auszerdem sofort terminiert.

**************************************************************************/

void error()
{
	if (logfile) {
		fprintf (logfile, "ERROR: %s\n", logtext);
		}   
	printf ("ERROR: %s\n",logtext);
	exit(10);
}


/************************ Funktion: panic (txt) ****************************

  Wie error(), jedoch wird der auszugebende Text als Argument uebergeben

***************************************************************************/

void panic(char *txt)
{
	sprintf (logtext, "%s", txt);
	error();
}


/********************** Funktion: getcputime ********************************

	liefert die verbrauchte CPU-Zeit im Mikrosekunden
	
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

