/************************* toolbox/loging.h ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Stellt Funktionen f"urs Logging zur Verf"ugung

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: $Id: loging.h 66 1998-11-11 21:15:48Z phil $

*******************************************************************************/

#define PANICIF(when,txt)  if(when)panic(txt)

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

-------------------------- Schnittstellenbeschreibung -------------------------

log_init .... Initialisiert das Logfile-System 
               fname ....... Dateiname f"ur die Protokollierungsdatei
               keepfile .... 1, wenn die alte Datei nicht gel"oscht werden soll
               echostdout .. 1, wenn auch auf stdout ausgegeben werden soll
               
log_text .... Gibt einen Text auf das Logfile aus
log_cputime . Gibt eine Information "uber die verbrauchte CPU-Zeit aus
dolog ....... Gibt den Inhalt von logtext aus
error ....... Gibt den Inhalt von logtext aus, und stoppt das System
panic ....... Gibt eine Text auf das Logfile aus

logtext ..... dieses globale Array mu"s vor Benutzung der Funktionen 'log' 
              oder 'error' mit dem auszugebenen Text bef"ullt werden.

getcputimew . gibt die vom Programm verbrauchte CPU-Zeit in
              Mikrosekunden zur"uck

*/
