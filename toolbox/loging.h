/************************* toolbox/loging.h ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Provides logging functions

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: $Id: loging.h 455 2003-09-15 18:48:46Z stefan $

*******************************************************************************/

#define PANICIF(when,txt)  if(when)panic(txt)
#define panic cacaopanic

#define MAXLOGTEXT 500
extern char logtext[MAXLOGTEXT];

void log_init(char *fname);
void log_text(char *txt);

void log_cputime();

void dolog();
void error();
void panic(char *txt);

long int getcputime();


/*

-------------------------- interface description -------------------------

log_init .... initialize the log file system 
               fname ....... filename for the protocol file
               keepfile .... 1 to keep old file (don't overwrite)
               echostdout .. 1 for output to stdout additionally
               
log_text .... print a text to log file
log_cputime . print used cpu time
dolog ....... print contents of logtext to log file
error ....... print contents of logtext to log file, stop the system
panic ....... print a text to log file

logtext ..... this global character array needs to be filled before using the
			  functions 'log' or 'error'

getcputimew . returns the number of microseconds used by the program

*/
