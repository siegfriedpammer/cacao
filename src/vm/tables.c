/* -*- mode: c; tab-width: 4; c-basic-offset: 4 -*- */
/****************************** tables.c ***************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Enth"alt Supportfunktionen f"ur:
		- Lesen von JavaClass-Files
	    - unicode-Symbole
		- den Heap 
		- zus"atzliche Support-Funktionen

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	Changes: Mark Probst         EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1998/03/24

*******************************************************************************/

#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include "global.h"
#include "tables.h"
#include "asmpart.h"
#include "callargs.h"

#include "threads/thread.h"                  /* schani */
#include "threads/locks.h"


bool runverbose = false;

int count_unicode_len = 0;


/******************************************************************************
************************* Der Dateien-Sauger **********************************
*******************************************************************************

	dient zum Behandeln von Java-ClassFiles ("offnen, schlie"sen, 
	einlesen von 8-, 16-, 32-, 64-bit Integers und 32-, 64- bit Floats)

******************************************************************************/

static FILE *classfile = NULL;   /* File-handle der gerade gelesenen Datei */
static char *classpath = "";     /* Suchpfad f"ur die ClassFiles */



/************************** Funktion: suck_init ******************************

	Wird zu Programmstart einmal aufgerufen und setzt den Suchpfad f"ur
	Klassenfiles

******************************************************************************/

void suck_init (char *cpath)
{
	classfile = NULL;
	classpath = cpath;
}


/************************** Funktion: suck_start ******************************

	"Offnet die Datei f"ur die Klasse des gegebenen Namens zum Lesen.
	Dabei werden alle im Suchpfad angegebenen Verzeichnisse durchsucht,
	bis eine entsprechende Datei  ( <classname>.class) gefunden wird. 
	
******************************************************************************/

bool suck_start (unicode *classname)
{
#define MAXFILENAME 1000 	   /* Maximale Langes des Dateinamens plus Pfad */

	char filename[MAXFILENAME+10];   /* Platz fuer '.class' */
	u2 filenamelen;
	char *pathpos;
	u2 i,c;


	pathpos = classpath;

	while (*pathpos) {
		while ( *pathpos == ':' ) pathpos++;
 
		filenamelen=0;
		while ( (*pathpos) && (*pathpos!=':') ) {
		    PANICIF (filenamelen >= MAXFILENAME, "Filename too long") ;
			
			filename[filenamelen++] = *(pathpos++);
			}

		filename[filenamelen++] = '/';
   
		for (i=0; i < classname -> length; i++) {
			PANICIF (filenamelen >= MAXFILENAME, "Filename too long");
			
			c = classname -> text [i];
			if (c=='/') c = '/';     /* Slashes im Namen passen zu UNIX */
			else {
				if ( c<=' ' || c>'z') {
					c = '?';
					}
				}
			
			filename[filenamelen++] = c;	
			}
      
		strcpy (filename+filenamelen, ".class");

		classfile = fopen(filename, "r");
		if (classfile) {
			return true;
			}

		
		}
		   
	sprintf (logtext,"Can not open class file '%s'", filename);
	error();
	return false;
}


/************************** Funktion: suck_stop *******************************

	Schlie"st die offene Datei wieder.
	
******************************************************************************/

void suck_stop ()
{
	u4 rest=0;
	u1 dummy;
	
	while ( fread (&dummy, 1,1, classfile) > 0) rest++;
	if (rest) {
		sprintf (logtext,"There are %d access bytes at end of classfile",
		                 (int) rest);
		dolog();
		}
			
	fclose (classfile);
	classfile = NULL;
}
      


/************************** Lesefunktionen ***********************************

	Lesen von der Datei in verschieden grossen Paketen
	(8,16,32,64-bit Integer oder Float)

*****************************************************************************/

void suck_nbytes (u1 *buffer, u4 len)
{
	if ( fread (buffer, 1, len, classfile) != len) panic ("Unexpected EOF");
}


void skip_nbytes (u4 len)
{
	u4 i;
	for (i=0; i<len; i++) suck_u1 ();
}


u1 suck_u1 ()
{
	u1 b;
	if ( fread (&b, 1,1, classfile) != 1) panic ("Unexpected EOF");
	return b;
}

s1 suck_s1 ()
{
	s1 b;
	if ( fread (&b, 1,1, classfile) != 1) panic ("Unexpected EOF");
	return b;
}


u2 suck_u2 ()
{
	u1 b[2];
	if ( fread (b, 1,2, classfile) != 2) panic ("Unexpected EOF");
	return (b[0]<<8) + b[1];
}

s2 suck_s2 ()
{
	return suck_u2 ();
}


u4 suck_u4 ()
{
	u1 b[4];
	u4 v;
	if ( fread (b, 1,4, classfile) != 4) panic ("Unexpected EOF");
	v = ( ((u4)b[0]) <<24) + ( ((u4)b[1])<<16) + ( ((u4)b[2])<<8) + ((u4)b[3]);
	return v;
}

s4 suck_s4 ()
{
	s4 v = suck_u4 ();
	return v;
}

u8 suck_u8 ()
{
#if U8_AVAILABLE
	u8 lo,hi;
	hi = suck_u4();
	lo = suck_u4();
	return (hi<<32) + lo;
#else
	u8 v;
	v.high = suck_u4();
	v.low = suck_u4();
	return v;
#endif
}

s8 suck_s8 ()
{
	return suck_u8 ();
}
	

float suck_float ()
{
	float f;

#if !WORDS_BIGENDIAN 
		u1 buffer[4];
		u2 i;
		for (i=0; i<4; i++) buffer[3-i] = suck_u1 ();
		memcpy ( (u1*) (&f), buffer, 4);
#else 
		suck_nbytes ( (u1*) (&f), 4 );
#endif

	PANICIF (sizeof(float) != 4, "Incompatible float-format");
	
	return f;
}


double suck_double ()
{
	double d;

#if !WORDS_BIGENDIAN 
		u1 buffer[8];
		u2 i;	
		for (i=0; i<8; i++) buffer[7-i] = suck_u1 ();
		memcpy ( (u1*) (&d), buffer, 8);
#else 
		suck_nbytes ( (u1*) (&d), 8 );
#endif

	PANICIF (sizeof(double) != 8, "Incompatible double-format" );
	
	return d;
}




/******************************************************************************
******************** Der Unicode-Symbol-Verwalter *****************************
*******************************************************************************

	legt eine Hashtabelle f"ur unicode-Symbole an und verwaltet
	das Eintragen neuer Symbole
	
******************************************************************************/



#define UNICODESTART  2187      /* Startgr"osse: moeglichst gross und prim */

static u4 unicodeentries;       /* Anzahl der Eintr"age in der Tabelle */
static u4 unicodehashsize;      /* Gr"osse der Tabelle */
static unicode ** unicodehash;  /* Zeiger auf die Tabelle selbst */


/*********************** Funktion: unicode_init ******************************

	Initialisiert die unicode-Symboltabelle (muss zu Systemstart einmal
	aufgerufen werden)
	
*****************************************************************************/

void unicode_init ()
{
	u4 i;
	
#ifdef STATISTICS
	count_unicode_len += sizeof(unicode*) * unicodehashsize;
#endif

	unicodeentries = 0;
	unicodehashsize = UNICODESTART;
	unicodehash = MNEW (unicode*, unicodehashsize);
	for (i=0; i<unicodehashsize; i++) unicodehash[i] = NULL;
}


/*********************** Funktion: unicode_close *****************************

	Gibt allen Speicher der Symboltabellen frei.
	Parameter: Ein Zeiger auf eine Funktion, die dazu n"otig ist, 
	           Stringkonstanten (die mit 'unicode_setstringlink' 
	           Unicode-Symbole gebunden wurden) wieder freizugeben
	
*****************************************************************************/

void unicode_close (stringdeleter del)
{
	unicode *u;
	u4 i;
	
	for (i=0; i<unicodehashsize; i++) {
		u = unicodehash[i];
		while (u) {
			unicode *nextu = u->hashlink;

			if (u->string) del (u->string);
			
			MFREE (u->text, u2, u->length);
			FREE (u, unicode);
			u = nextu;
			}	
		}
	MFREE (unicodehash, unicode*, unicodehashsize);
}


/********************* Funktion: unicode_display ******************************
	
	Gibt ein unicode-Symbol auf stdout aus (zu Debugzwecken)

******************************************************************************/

void unicode_display (unicode *u)
{
	u2 i,c;
	for (i=0; i < u->length; i++) {
		c = u->text[i];
		if (c>=32 && c<=127) printf ("%c",c);
		                else printf ("?");
		}
	fflush (stdout);
}


/********************* Funktion: unicode_sprint ******************************
	
	Schreibt ein unicode-Symbol in einen C-String

******************************************************************************/ 

void unicode_sprint (char *buffer, unicode *u)
{
	u2 i;
	for (i=0; i < u->length; i++) buffer[i] = u->text[i];
	buffer[i] = '\0';
}


/********************* Funktion: unicode_fprint ******************************
	
	Schreibt ein unicode-Symbol auf eine Datei aus

******************************************************************************/ 

void unicode_fprint (FILE *file, unicode *u)
{
	u2 i;
	for (i=0; i < u->length; i++) putc (u->text[i], file);
} 


/****************** interne Funktion: u_hashkey ******************************/

static u4 u_hashkey (u2 *text, u2 length)
{
	u4 k = 0;
	u2 i,sh=0;
	
	for (i=0; i<length; i++) {
		k ^=  ( ((u4) (text[i])) << sh );
		if (sh<16) sh++;
		     else  sh=0;
		}
		
	return k;
}

/*************** interne Funktion: u_reorganizehash **************************/

static void u_reorganizehash ()
{
	u4 i;
	unicode *u;

	u4 newhashsize = unicodehashsize*2;
	unicode **newhash = MNEW (unicode*, newhashsize);

#ifdef STATISTICS
	count_unicode_len += sizeof(unicode*) * unicodehashsize;
#endif

	for (i=0; i<newhashsize; i++) newhash[i] = NULL;

	for (i=0; i<unicodehashsize; i++) {
		u = unicodehash[i];
		while (u) {
			unicode *nextu = u -> hashlink;
			u4 slot = (u->key) % newhashsize;
						
			u->hashlink = newhash[slot];
			newhash[slot] = u;

			u = nextu;
			}
		}
	
	MFREE (unicodehash, unicode*, unicodehashsize);
	unicodehash = newhash;
	unicodehashsize = newhashsize;
}


/****************** Funktion: unicode_new_u2 **********************************

	Legt ein neues unicode-Symbol an. Der Text des Symbols wird dieser
	Funktion als u2-Array "ubergeben

******************************************************************************/

unicode *unicode_new_u2 (u2 *text, u2 length)
{
	u4 key = u_hashkey (text, length);
	u4 slot = key % unicodehashsize;
	unicode *u = unicodehash[slot];
	u2 i;

	while (u) {
		if (u->key == key) {
			if (u->length == length) {
				for (i=0; i<length; i++) {
					if (text[i] != u->text[i]) goto nomatch;
					}	
					return u;
				}
			}
		nomatch:
		u = u->hashlink;
		}

#ifdef STATISTICS
	count_unicode_len += sizeof(unicode) + 2 * length;
#endif

	u = NEW (unicode);
	u->key = key;
	u->length = length;
	u->text = MNEW (u2, length);
	u->class = NULL;
	u->string = NULL;
	u->hashlink = unicodehash[slot];
	unicodehash[slot] = u;
	for (i=0; i<length; i++) u->text[i] = text[i];

	unicodeentries++;
	
	if ( unicodeentries > (unicodehashsize/2)) u_reorganizehash();
	
	return u;
}


/********************* Funktion: unicode_new_char *****************************

	Legt ein neues unicode-Symbol an. Der Text des Symbols wird dieser
	Funktion als C-String ( = char* ) "ubergeben

******************************************************************************/

unicode *unicode_new_char (char *text)
{
#define MAXNEWCHAR 500
	u2 buffer[MAXNEWCHAR];
	u2 length = 0;
	u1 c;
	
	while ( (c = *text) != '\0' ) {
		if (length>=MAXNEWCHAR) panic ("Text too long in unicode_new_char");
		buffer[length++] = c;
		text ++;
		}
	return unicode_new_u2 (buffer, length);
}


/********************** Funktion: unicode_setclasslink ************************

	H"angt einen Verweis auf eine Klasse an ein unicode-Symbol an.

******************************************************************************/

void unicode_setclasslink (unicode *u, classinfo *class)
{
	PANICIF (u->class, "Attempt to attach class to already attached symbol");
	u->class = class;
}

/********************** Funktion: unicode_getclasslink ************************

	Sucht den Verweis von einem unicode-Symbol auf eine Klasse.
	Wenn keine solche Klasse existiert, dann wird ein Fehler
	ausgegeben.

******************************************************************************/

classinfo *unicode_getclasslink (unicode *u)
{
	PANICIF (!u->class, "Attempt to get unknown class-reference");
	return u->class;
}



/********************* Funktion: unicode_unlinkclass *************************

	Entfernt den Verweis auf eine Klasse wieder von einem Symbol
	
******************************************************************************/

void unicode_unlinkclass (unicode *u)
{
	PANICIF (!u->class, "Attempt to unlink not yet linked symbol");
	u -> class = NULL;
}



/******************* Funktion> unicode_setstringlink *********************

	H"angt einen Verweis auf einen konstanten String an ein 
	Unicode-Symbol
	
*************************************************************************/

void unicode_setstringlink (unicode *u, java_objectheader *str)
{
	PANICIF (u->string, "Attempt to attach string to already attached symbol");
	u->string = str;
}


/********************* Funktion: unicode_unlinkstring *************************

	Entfernt den Verweis auf einen String wieder von einem Symbol
	
******************************************************************************/

void unicode_unlinkstring (unicode *u)
{
	PANICIF (!u->class, "Attempt to unlink not yet linked symbol");
	u -> string = NULL;
}



/*********************** Funktion: unicode_show ******************************

	gibt eine Aufstellung aller Symbol im unicode-hash auf stdout aus.
	(nur f"ur Debug-Zwecke)
	
*****************************************************************************/

void unicode_show ()
{
	unicode *u;
	u4 i;

	printf ("UNICODE-HASH: %d slots for %d entries\n", 
	         (int) unicodehashsize, (int) unicodeentries );
	          
	for (i=0; i<unicodehashsize; i++) {
		u = unicodehash[i];
		if (u) {
			printf ("SLOT %d: ", (int) i);
			while (u) {
				printf ("'");
				unicode_display (u);
				printf ("' ");
				if (u->string) printf ("(string)  ");
				u = u->hashlink;
				}	
			printf ("\n");
			}
		
		}
}



/******************************************************************************
*********************** Diverse Support-Funktionen ****************************
******************************************************************************/


/******************** Funktion: desc_to_type **********************************

	Findet zu einem gegebenen Typdescriptor den entsprechenden 
	Java-Grunddatentyp.
	
******************************************************************************/

u2 desc_to_type (unicode *descriptor)
{
	if (descriptor->length < 1) panic ("Type-Descriptor is empty string");
	
	switch (descriptor->text[0]) {
	case 'B': 
	case 'C':
	case 'I':
	case 'S':  
	case 'Z':  return TYPE_INT;
	case 'D':  return TYPE_DOUBLE;
	case 'F':  return TYPE_FLOAT;
	case 'J':  return TYPE_LONG;
	case 'L':
	case '[':  return TYPE_ADDRESS;
	}
			
	sprintf (logtext, "Invalid Type-Descriptor: "); 
	unicode_sprint (logtext+strlen(logtext), descriptor);
	error (); 
	return 0;
}


/********************** Funktion: desc_typesize *******************************

	Berechnet die L"ange (in Byte) eines Datenelements gegebenen Typs,
	der durch den Typdescriptor gegeben ist.
	
******************************************************************************/

u2 desc_typesize (unicode *descriptor)
{
	switch (desc_to_type(descriptor)) {
	case TYPE_INT:     return 4;
	case TYPE_LONG:    return 8;
	case TYPE_FLOAT:   return 4;
	case TYPE_DOUBLE:  return 8;
	case TYPE_ADDRESS: return sizeof(voidptr);
	default:           return 0;
	}
}




