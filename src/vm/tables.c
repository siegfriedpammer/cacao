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





/******************************************************************************
********************* Der garbage-collected Heap ******************************
*******************************************************************************

	verwaltet einen Heap mit automatischer Speicherfreigabe.

	(eine ausf"uhrliche Dokumentation findet sich in der Datei: 
	collect.doc)
	
******************************************************************************/


bool collectverbose = false;  /* soll Meldung beim GC ausgegeben werden? */


#define BLOCKSIZE 8         /* Gr"osse eines Speicherblockes */
typedef u8 heapblock;       /* Datentyp mit der Gr"osse eines Speicherblocks */

#if U8_AVAILABLE && SUPPORT_LONG
#define	bitfieldtype u8
#define BITFIELDBITS 64
#else
#define bitfieldtype u4
#define BITFIELDBITS 32
#endif

u4 heapsize;                /* Gr"osse des Heap in Blocks */
u4 topofheap;               /* Bisherige Obergrenze Heaps */
heapblock *heap;            /* Speicher f"ur den Heap selbst */

bitfieldtype *startbits;     /* Bitfeld f"ur Bereichsstartkennung */
bitfieldtype *markbits;      /* Bitfeld f"ur Markierung */
bitfieldtype *referencebits; /* Bitfeld f"ur Folgereferenzenkennung */

u4 heapfillgrade;           /* Menge der Daten im Heap */
u4 collectthreashold;       /* Schwellwert f"ur n"achstes GC */

void **bottom_of_stack;     /* Zeiger auf Untergrenze des C-Stacks */ 
chain *allglobalreferences; /* Liste f"ur alle globalen Zeiger */

typedef struct finalizernode {
	struct finalizernode *next;	
	u4 objstart;
	methodinfo *finalizer;
	} finalizernode;
	
finalizernode *livefinalizees;
finalizernode *deadfinalizees;


typedef struct memarea {    /* Datenstruktur f"ur einen Freispeicherbereich */
	struct memarea *next;
	} memarea;

typedef struct bigmemarea {   /* Datenstruktur f"ur einen */
	struct bigmemarea *next;  /* Freispeicherbereich variabler L"ange */
	u4 size;
	} bigmemarea;
	
#define DIFFERENTSIZES 128         /* Anzahl der 'kleinen' Freispeicherlisten */
memarea *memlist[DIFFERENTSIZES];  /* Die 'kleinen' Freispeicherlisten */
bitfieldtype memlistbits[DIFFERENTSIZES/BITFIELDBITS]; 
                                   /* Bitfeld, in dem jeweils ein Bit gesetzt */
                                   /* ist, wenn eine Liste noch etwas enth"alt */

bigmemarea *bigmemlist;         /* Liste der gr"osseren Freispeicherbereiche */




/**************** Hilfsfunktion: lowest **************************************

liefert als Ergebnis die Nummer des niederwertigsten Bits einer 
Zahl vom Typ bitfieldtype, das 1 ist.
Wenn die ganze Zahl keine 1en enth"alt, dann ist egal, was f"ur einen
Wert die Funktion l"iefert.
z.B.: lowest(1) = 0,   lowest(12) = 2,   lowest(1024) = 10

*****************************************************************************/

static u1 lowesttable[256] = {
	255, 0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	5,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	6,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	5,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	7,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	5,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	6,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	5,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
	4,   0,1,0,2,0,1,0,3,0,1,0,2,0,1,0 };
	
static int lowest(bitfieldtype i)
{
#if BITFIELDBITS==64
	u8 i1 = i<<32;
	if (i1) {
		u8 i11 = i1<<16;
		if (i11) {
			u8 i111 = i11<<8;
			if (i111) return lowesttable[i111>>56];
			else      return 8+lowesttable[i11>>56];
			}
		else {
			u8 i112 = i1<<8;
			if (i112) return 16+lowesttable[i112>>56];
			else      return 24+lowesttable[i1>>56];
			}
		}
	else {
		u8 i12 = i<<16;
		if (i12) {
			u8 i121 = i12<<8;
			if (i121) return 32+lowesttable[i121>>56];
			else      return 40+lowesttable[i12>>56];
			}
		else {
			u8 i122 = i<<8;
			if (i122) return 48+lowesttable[i122>>56];
			else      return 56+lowesttable[i>>56];
			}
		}
#else
	u4 i1 = i<<16;
	if (i1) {
		u4 i11 = i1<<8;
		if (i11) return lowesttable[i11>>24];
		else     return 8+lowesttable[i1>>24];
		}
	else {
		u4 i12 = i<<8;
		if (i12) return 16+lowesttable[i12>>24];
		else     return 24+lowesttable[i>>24];
		}
#endif
}


/******** Funktionen zum Setzen und L"oschen von Bits in Bitfeldern ***********/

static void setbit(bitfieldtype *bitfield, u4 bitnumber)
{
	bitfield[bitnumber/BITFIELDBITS] |= 
	  ((bitfieldtype) 1) << (bitnumber%BITFIELDBITS);
}

static void clearbit(bitfieldtype *bitfield, u4 bitnumber)
{
	bitfield[bitnumber/BITFIELDBITS] &= 
	   ~((bitfieldtype) 1) << (bitnumber%BITFIELDBITS);
}

static bool isbitset(bitfieldtype *bitfield, u4 bitnumber)
{
	return ( bitfield[bitnumber/BITFIELDBITS] & 
	   ( ((bitfieldtype) 1) << (bitnumber%BITFIELDBITS) ) ) != 0;
}

static bool isbitclear(bitfieldtype *bitfield, u4 bitnumber)
{
	return ( bitfield[bitnumber/BITFIELDBITS] & 
	    ( ((bitfieldtype) 1) << (bitnumber%BITFIELDBITS) ) ) == 0;
}


/***************** Funktion: clearbitfield ************************************
	
	l"oscht ein ganzes Bitfeld
	
******************************************************************************/

static void clearbitfield(bitfieldtype *bitfield, u4 fieldsize)
{ 
	u4 i,t;
	t = ALIGN(fieldsize, BITFIELDBITS) / BITFIELDBITS;
	for (i=0; i<t; i++) bitfield[i] = 0;
}

/************** Funktion: maskfieldwithfield **********************************
	
	Verkn"upft zwei Bitfelder bitweise mit UND, das erste Bitfeld
	wird mit dem Ergebnis "uberschrieben 
	
******************************************************************************/
	
static void maskfieldwithfield (bitfieldtype* bitfield, 
                                bitfieldtype *maskfield, u4 fieldsize)
{
	u4 i,t;
	t = ALIGN(fieldsize, BITFIELDBITS) / BITFIELDBITS;
	for (i=0; i<t; i++) bitfield[i] &= maskfield[i];
}


/************** Funktion: findnextsetbit **************************************

	Sucht in einem Bitfeld ab einer Stelle das n"achste gesetzte Bit
	und liefert die Nummer dieses Bits.
	Wenn kein Bit mehr zwischen 'bitnumber' und 'fieldsize' gesetzt
	ist, dann wird fieldsize zur"uckgeliefte.

******************************************************************************/

static u4 findnextsetbit(bitfieldtype* bitfield, u4 fieldsize, u4 bitnumber)
{
	bitfieldtype pattern;

	for (;;) {
		if (bitnumber >= fieldsize) return fieldsize;
		
		pattern = bitfield[bitnumber/BITFIELDBITS];
		pattern >>= (bitnumber%BITFIELDBITS);
		if (pattern) return bitnumber + lowest(pattern);

		bitnumber = ((bitnumber + BITFIELDBITS) / BITFIELDBITS) * BITFIELDBITS;
		}
}


/************ Funktion: findnextcombination_set_unset *************************

	funktioniert wie findnextsetbit, allerdings sucht diese Funktion
	nach einer Stelle, an der gleichzeitig ein Bit im ersten
	Bitfeld gesetzt ist und im zweiten Bitfeld das entsprechende
	Bit nicht gesetzt ist.

******************************************************************************/

static u4 findnextcombination_set_unset 
	(bitfieldtype *bitfield1, bitfieldtype* bitfield2, u4 fieldsize, u4 bitnumber)
{
	bitfieldtype pattern;

	for (;;) {
		if (bitnumber >= fieldsize) return fieldsize;
		
		pattern =     bitfield1[bitnumber/BITFIELDBITS] 
		          & (~bitfield2[bitnumber/BITFIELDBITS]);
		pattern >>= (bitnumber%BITFIELDBITS);
		if (pattern) return bitnumber + lowest(pattern);

		bitnumber = ((bitnumber + BITFIELDBITS) / BITFIELDBITS) * BITFIELDBITS;
		}
}



/************** Funktion: memlist_init ****************************************

	initialisiert die Freispeicherlisten (zum nachfolgenden Eintragen 
	mit memlist_addrange).

******************************************************************************/

static void memlist_init ()
{
	u4 i;
	for (i=0; i<DIFFERENTSIZES; i++) memlist[i] = NULL;
	clearbitfield (memlistbits, DIFFERENTSIZES);
	bigmemlist = NULL;
}


/************** Funktion: memlist_addrange ************************************

	f"ugt einen Bereich von Heap-Bl"ocken zur Freispeicherliste 
	hinzu (in die freien Heap-Bl"ocke werden dabei verschiedene 
	Verkettungszeiger hineingeschrieben).

******************************************************************************/

static void memlist_addrange (u4 freestart, u4 freesize)
{
	if (freesize>=DIFFERENTSIZES) {
		bigmemarea *m = (bigmemarea*) (heap+freestart);
		m -> next = bigmemlist;
		m -> size = freesize;
		bigmemlist = m;
		}
	else {
		if (freesize*BLOCKSIZE>=sizeof(memarea)) {
			memarea *m = (memarea*) (heap+freestart);
			m -> next = memlist[freesize];
			memlist[freesize] = m;
			setbit (memlistbits, freesize);
			}
		}
}


/************** Funktion: memlist_getsuitable *********************************

	sucht in der Freispeicherliste einen Speicherbereich, der m"oglichst
	genau die gew"unschte L"ange hat.
	Der Bereich wird dabei auf jeden Fall aus der Liste ausgetragen.
	(Wenn der Bereich zu lang sein sollte, dann kann der Aufrufer den
	Rest wieder mit 'memlist_addrange' in die Liste einh"angen)
	
	Return (in Referenzparametern): 
		*freestart: Anfang des freien Speichers
		*freelength: L"ange dieses Speicherbereiches

	Wenn kein passender Speicherblock mehr gefunden werden kann, dann 
	wird in '*freelength' 0 hineingeschrieben.
	
******************************************************************************/		

static void memlist_getsuitable (u4 *freestart, u4 *freelength, u4 length)
{
	bigmemarea *m;
	bigmemarea *prevm = NULL;
	
	if (length<DIFFERENTSIZES) {
		u4 firstfreelength;

		if (memlist[length]) {
			firstfreelength = length;
			}
		else {
			firstfreelength = findnextsetbit 
				(memlistbits, DIFFERENTSIZES, length+3); 
				/* wenn kein passender Block da ist, dann gleich nach */
				/* einem etwas gr"osseren suchen, damit keine kleinen */
				/* St"uckchen "ubrigbleiben */
			}
			
		if (firstfreelength<DIFFERENTSIZES) {
			memarea *m = memlist[firstfreelength];
			memlist[firstfreelength] = m->next;		
			if (!m->next) clearbit (memlistbits, firstfreelength);
			*freestart = ((heapblock*) m) - heap;
			*freelength = firstfreelength;
			return;
			}
		}

	m = bigmemlist;
	while (m) {
		if (m->size >= length) {
			if (prevm) prevm->next = m->next;					
				else   bigmemlist = m->next;
			
			*freestart = ((heapblock*) m) - heap;
			*freelength = m->size;
			return ;			
			}

		prevm = m;
		m = m->next;
		}
	
	*freelength=0;
}





/******************* Funktion: mark *******************************************

	Markiert ein m"oglicherweise g"ultiges Objekt.
	Sollte sich herausstellen, dass der Zeiger gar nicht auf ein Objekt
	am Heap zeigt, dann wird eben gar nichts markiert.

******************************************************************************/

static void markreferences (void **rstart, void **rend);

static void mark (heapblock *obj)
{
	u4 blocknum,objectend;

	if ((long) obj & (BLOCKSIZE-1)) return;
	
	if (obj<heap) return;
	if (obj>=(heap+topofheap) ) return;

	blocknum = obj-heap;
	if ( isbitclear(startbits, blocknum) ) return;
	if ( isbitset(markbits, blocknum) ) return;

	setbit (markbits, blocknum);
	
	if ( isbitclear(referencebits, blocknum) ) return;

	objectend = findnextsetbit (startbits, topofheap, blocknum+1);
	markreferences ((void**)obj, (void**) (heap+objectend) );
}


/******************** Funktion: markreferences ********************************

	Geht einen Speicherbereich durch, und markiert alle Objekte, auf
	die von diesem Bereich aus irgendeine Referenz existiert.

******************************************************************************/

static void markreferences (void **rstart, void **rend)
{
	void **ptr;
	
	for (ptr=rstart; ptr<rend; ptr++) mark (*ptr);
}


/******************* Funktion: markstack **************************************

        Marks all objects that are referenced by the (C-)stacks of
        all live threads.

	(The stack-bottom is to be specified in the call to heap_init).

******************************************************************************/

static void markstack ()                   /* schani */
{
	void *dummy;
	void **top_of_stack = &dummy;

#ifdef USE_THREADS
	thread *aThread;

	for (aThread = liveThreads; aThread != 0; aThread = CONTEXT(aThread).nextlive) {
		mark((heapblock*)aThread);
		if (aThread == currentThread) {
			if (top_of_stack > (void**)CONTEXT(aThread).stackEnd)
				markreferences ((void**)CONTEXT(aThread).stackEnd, top_of_stack);
			else 	
				markreferences (top_of_stack, (void**)CONTEXT(aThread).stackEnd);
			}
		else {
			if (CONTEXT(aThread).usedStackTop > CONTEXT(aThread).stackEnd)
				markreferences ((void**)CONTEXT(aThread).stackEnd,
				                (void**)CONTEXT(aThread).usedStackTop + 16);
			else 	
				markreferences ((void**)CONTEXT(aThread).usedStackTop - 16,
				                (void**)CONTEXT(aThread).stackEnd);
			}
		}

	markreferences((void**)&threadQhead[0], (void**)&threadQhead[MAX_THREAD_PRIO]);
#else
	if (top_of_stack > bottom_of_stack)
		markreferences(bottom_of_stack, top_of_stack);
	else
		markreferences(top_of_stack, bottom_of_stack);
#endif
}



/**************** Funktion: searchlivefinalizees *****************************

	geht die Liste aller Objekte durch, die noch finalisiert werden m"ussen
	(livefinalizees), und tr"agt alle nicht mehr markierten in die
	Liste deadfinalizess ein (allerdings werden sie vorher noch als
	erreicht markiert, damit sie nicht jetzt gleich gel"oscht werden).
	
*****************************************************************************/ 

static void searchlivefinalizees ()
{
	finalizernode *fn = livefinalizees;
	finalizernode *lastlive = NULL;
		
	while (fn) {
	
		/* alle zu finalisierenden Objekte, die nicht mehr markiert sind: */
		if (isbitclear (markbits, fn->objstart)) {
			finalizernode *nextfn = fn->next;

			mark (heap + fn->objstart); 

			if (lastlive) lastlive -> next = nextfn;
				       else livefinalizees = nextfn;

			fn -> next = deadfinalizees;
			deadfinalizees = fn;
			
			fn = nextfn;
			}
		else {	
			lastlive = fn;
			fn = fn->next;
			}
		}
}



/********************** Funktion: finalizedead *******************************

	ruft die 'finalize'-Methode aller Objekte in der 'deadfinalizees'-
	Liste auf.
	Achtung: Es kann hier eventuell zu neuerlichen Speicheranforderungen
	(mit potentiell notwendigem GC) kommen, deshalb m"ussen manche
	globalen Variablen in lokale Variblen kopiert werden 
	(Reentrant-F"ahigkeit!!!)
	 
******************************************************************************/

static void finalizedead()
{
	finalizernode *fn = deadfinalizees;
	deadfinalizees = NULL;
	
	while (fn) {
		finalizernode *nextfn = fn->next;
		
		asm_calljavamethod (fn->finalizer, heap+fn->objstart, NULL,NULL,NULL);
		FREE (fn, finalizernode);
		
		fn = nextfn;
		}

}



/****************** Funktion: heap_docollect **********************************
	
	F"uhrt eine vollst"andige Garbage Collection durch
	
******************************************************************************/

static void heap_docollect ()
{
	u4 freestart,freeend;
	void **p;
	bitfieldtype *dummy;
	u4 heapfreemem;
	u4 oldfillgrade;


	if (runverbose) {
		fprintf(stderr, "doing garbage collection\n");
		}
		/* alle Markierungsbits l"oschen */
	clearbitfield (markbits, topofheap);

		/* Alle vom Stack referenzierten Objekte markieren */
	asm_dumpregistersandcall (markstack);

		/* Alle von globalen Variablen erreichbaren Objekte markieren */
	p = chain_first (allglobalreferences);
	while (p) {
		mark (*p);
		p = chain_next (allglobalreferences);	
		}

		/* alle Objekte durchsehen, die eine finalizer-Methode haben */
	searchlivefinalizees();

		/* alle Reference-Bits l"oschen, deren Objekte nicht */
		/* mehr erreichbar sind */
	maskfieldwithfield (referencebits, markbits, topofheap);


		/* Freispeicherliste initialisieren */
	memlist_init ();
	
	
		/* Heap schrittweise durchgehen, und alle freien Bl"ocke merken */

	heapfreemem = 0;
	freeend=0;
	for (;;) {
			/* Anfang des n"achsten freien Bereiches suchen */
		freestart = findnextcombination_set_unset 
			(startbits, markbits, topofheap, freeend);

			/* Wenn es keinen freien Bereich mehr gibt -> fertig */
		if (freestart>=topofheap) goto freememfinished;

			/* Anfang des freien Bereiches markieren */
		setbit (markbits, freestart);

			/* Ende des freien Bereiches suchen */		
		freeend = findnextsetbit (markbits, topofheap, freestart+1);

			/* Freien Bereich in Freispeicherliste einh"angen */
		memlist_addrange (freestart, freeend-freestart);

			/* Menge des freien Speichers mitnotieren */
		heapfreemem += (freeend-freestart);
		}

	freememfinished:

		/* Die Rollen von markbits und startbits vertauschen */	
	dummy = markbits;
	markbits = startbits;
	startbits = dummy;


		/* Threashold-Wert f"ur n"achstes Collect */
	oldfillgrade = heapfillgrade;
	heapfillgrade = topofheap - heapfreemem;
	collectthreashold = heapfillgrade*3;   /* eine nette Heuristik */

		/* Eventuell eine Meldung ausgeben */
	if (collectverbose) {
		sprintf (logtext, "Garbage Collection:  previous/now = %d / %d ",
		  (int) (oldfillgrade * BLOCKSIZE), 
		  (int) (heapfillgrade * BLOCKSIZE) );
		dolog ();
		}


		
		/* alle gerade unerreichbar gewordenen Objekte mit 
		   finalize-Methoden jetzt finalisieren.
		   Achtung: Diese Funktion kann zu neuerlichem GC f"uhren!! */
	finalizedead ();
	
}

/************************* Function: gc_init **********************************

        Initializes the garbage collection thread mechanism.

******************************************************************************/

#ifdef USE_THREADS
iMux gcStartMutex;
iMux gcThreadMutex;
iCv gcConditionStart;
iCv gcConditionDone;

void
gc_init (void)
{
    gcStartMutex.holder = 0;
    gcStartMutex.count = 0;
    gcStartMutex.muxWaiters = 0;

    gcThreadMutex.holder = 0;
    gcThreadMutex.count = 0;
    gcThreadMutex.muxWaiters = 0;

    gcConditionStart.cvWaiters = 0;
    gcConditionStart.mux = 0;

    gcConditionDone.cvWaiters = 0;
    gcConditionDone.mux = 0;
}    
#else
void
gc_init (void)
{
}
#endif

/************************* Function: gc_thread ********************************

        In an endless loop waits for a condition to be posted, then
	garbage collects the heap.

******************************************************************************/

#ifdef USE_THREADS
void
gc_thread (void)
{
    intsRestore();           /* all threads start with interrupts disabled */

	assert(blockInts == 0);

    lock_mutex(&gcThreadMutex);

    for (;;)
    {
		wait_cond(&gcThreadMutex, &gcConditionStart, 0);

		intsDisable();
		heap_docollect();
		intsRestore();

		signal_cond(&gcConditionDone);
    }
}
#endif

void
gc_call (void)
{
#ifdef USE_THREADS
    lock_mutex(&gcThreadMutex);

    signal_cond(&gcConditionStart);
    wait_cond(&gcThreadMutex, &gcConditionDone, 0);

    unlock_mutex(&gcThreadMutex);
#else
    heap_docollect();
#endif
}


/************************* Funktion: heap_init ********************************

	Initialisiert den Garbage Collector.
	Parameter: 
		heapbytesize:      Maximale Gr"osse des Heap (in Bytes)
		heapbytestartsize: Gr"osse des Heaps, bei dem zum ersten mal eine
		                   GC durchgef"uhrt werden soll.
		stackbottom:       Ein Zeiger auf die Untergrenze des Stacks, im dem
	                       Referenzen auf Objekte stehen k"onnen.		       
	                                   
******************************************************************************/

void heap_init (u4 heapbytesize, u4 heapbytestartsize, void **stackbottom)
{

#ifdef TRACECALLARGS

	heapsize = ALIGN (heapbytesize, getpagesize());
	heapsize = heapsize/BLOCKSIZE;
	heap = (void*) mmap ((void*) 0x10000000, (size_t) (heapsize * BLOCKSIZE),
	       PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, (off_t) 0);
	if (heap != (void *) 0x10000000) {
		perror("mmap");
		printf("address = %p\n", heap);
		panic("mmap failed\n");
		}

#else

	heapsize = ALIGN (heapbytesize, 1024);
	heapsize = heapsize/BLOCKSIZE;
	heap = MNEW (heapblock, heapsize);

#endif

	topofheap = 0;
	startbits = MNEW (bitfieldtype, heapsize/BITFIELDBITS);
	markbits = MNEW (bitfieldtype, heapsize/BITFIELDBITS);
	referencebits = MNEW (bitfieldtype, heapsize/BITFIELDBITS);
	clearbitfield (startbits, heapsize);
	clearbitfield (markbits, heapsize);
	clearbitfield (referencebits, heapsize);

	heapfillgrade = 0;
	collectthreashold = heapbytestartsize/BLOCKSIZE;
	allglobalreferences = chain_new ();
	bottom_of_stack = stackbottom;

	livefinalizees = NULL;
	deadfinalizees = NULL;
}


/****************** Funktion: heap_close **************************************

	Gibt allen ben"otigten Speicher frei

******************************************************************************/

void heap_close ()
{
#ifndef TRACECALLARGS
	MFREE (heap, heapblock, heapsize); */
#endif
	MFREE (startbits, bitfieldtype, heapsize/BITFIELDBITS);
	MFREE (markbits, bitfieldtype, heapsize/BITFIELDBITS);
	MFREE (referencebits, bitfieldtype, heapsize/BITFIELDBITS);
	chain_free (allglobalreferences);

	while (livefinalizees) {
		finalizernode *n = livefinalizees->next;
		FREE (livefinalizees, finalizernode);
		livefinalizees = n;
		}
}


/***************** Funktion: heap_addreference ********************************

	Teilt dem GC eine Speicherstelle mit, an der eine globale Referenz
	auf Objekte gespeichert sein kann.

******************************************************************************/

void heap_addreference (void **reflocation)
{
	intsDisable();                     /* schani */

	chain_addlast (allglobalreferences, reflocation);

	intsRestore();                    /* schani */
}



/***************** Funktion: heap_allocate ************************************

	Fordert einen Speicher vom Heap an, der eine gew"unschte Gr"osse
	(in Byte angegeben) haben muss.
	Wenn kein Speicher mehr vorhanden ist (auch nicht nach einem
	Garbage Collection-Durchlauf), dann wird NULL zur"uckgegeben.
	Zus"atzlich wird durch den Parameter 'references' angegeben, ob
	in das angelegte Objekt Referenzen auf andere Objekte eingetragen
	werden k"onnten. 
	(Wichtig wegen Beschleunigung des Mark&Sweep-Durchlauf)
	Im Zweifelsfalle immer references=true verwenden.

******************************************************************************/

void *heap_allocate (u4 bytelength, bool references, methodinfo *finalizer)
{
	u4 freestart,freelength;
	u4 length = ALIGN(bytelength, BLOCKSIZE) / BLOCKSIZE;
	
	intsDisable();                        /* schani */

	memlist_getsuitable (&freestart, &freelength, length);

onemoretry:
	if (!freelength) {
		if (    (topofheap+length > collectthreashold) 
		     || (topofheap+length > heapsize) ) {

		        intsRestore();
		        gc_call();
			intsDisable();
			
			memlist_getsuitable (&freestart, &freelength, length);
			if (freelength) goto onemoretry;
			}
		
		if (topofheap+length > heapsize) {
			sprintf (logtext, "Heap-Allocation failed for %d bytes", 
			    (int) bytelength);
			dolog();
			intsRestore();            /* schani */
			return NULL;
			}
			
		freestart = topofheap;
		freelength = length;
		setbit (startbits, topofheap);
		topofheap += length;
		}
	else {
		if (freelength>length) {
			setbit (startbits, freestart+length);
			memlist_addrange (freestart+length, freelength-length);
			}
		}
		
	if (references) setbit (referencebits, freestart);

	heapfillgrade += length;

	if (finalizer) {
		finalizernode *n = NEW(finalizernode);
		n -> next = livefinalizees;
		n -> objstart = freestart;
		n -> finalizer = finalizer;
		livefinalizees = n;
		}
	
	intsRestore();                   /* schani */

	if (runverbose) {
		sprintf (logtext, "new returns: %lx", (long) (heap + freestart));
		dolog ();
		}

	return (void*) (heap + freestart);
}




