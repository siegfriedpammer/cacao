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
#include "../global.h"
#include "../tables.h"
#include "../asmpart.h"
#include "../callargs.h"

#include "../threads/thread.h"                  /* schani */
#include "../threads/locks.h"
#include "../sysdep/threads.h"



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

	/*	fprintf(stderr, "mark: marking object at 0x%lx\n", obj); */
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

#ifdef USE_THREADS
    thread *aThread;

	if (currentThread == NULL) {
		void **top_of_stack = &dummy;
				
		if (top_of_stack > bottom_of_stack)
			markreferences(bottom_of_stack, top_of_stack);
		else
			markreferences(top_of_stack, bottom_of_stack);
	}
	else {
		for (aThread = liveThreads; aThread != 0;
			 aThread = CONTEXT(aThread).nextlive) {
			mark((heapblock*)aThread);
			if (aThread == currentThread) {
				void **top_of_stack = &dummy;
				
				if (top_of_stack > (void**)CONTEXT(aThread).stackEnd)
					markreferences((void**)CONTEXT(aThread).stackEnd, top_of_stack);
				else 	
					markreferences(top_of_stack, (void**)CONTEXT(aThread).stackEnd);
			}
			else {
				if (CONTEXT(aThread).usedStackTop > CONTEXT(aThread).stackEnd)
					markreferences((void**)CONTEXT(aThread).stackEnd,
								   (void**)CONTEXT(aThread).usedStackTop);
				else 	
					markreferences((void**)CONTEXT(aThread).usedStackTop,
								   (void**)CONTEXT(aThread).stackEnd);
			}
	    }

		markreferences((void**)&threadQhead[0],
					   (void**)&threadQhead[MAX_THREAD_PRIO]);
	}
#else
    void **top_of_stack = &dummy;

	/*	fprintf(stderr, "marking stack\n"); */

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

	/*   	fprintf(stderr, "marking references\n"); */
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

#if 0
		if (freeend==topofheap) {
			topofheap = freestart;
   			heapfreemem += (freeend-freestart);
			goto freememfinished;
		} else {
			fprintf(stderr, "%lx -- %lx\n", heap + freestart, heap + freeend);
#endif

			/* Freien Bereich in Freispeicherliste einh"angen */
			memlist_addrange (freestart, freeend-freestart);

			/* Menge des freien Speichers mitnotieren */
			heapfreemem += (freeend-freestart);
#if 0
		}
#endif
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

  Initializes anything that must be initialized to call the gc on the right
  stack.

******************************************************************************/

void
gc_init (void)
{
}

/************************** Function: gc_call ********************************

  Calls the garbage collector. The garbage collector should always be called
  using this function since it ensures that enough stack space is available.

******************************************************************************/

void
gc_call (void)
{
#ifdef USE_THREADS
	assert(blockInts == 0);

	intsDisable();
	if (currentThread == NULL || currentThread == mainThread)
		heap_docollect();
	else
		asm_switchstackandcall(CONTEXT(mainThread).usedStackTop, heap_docollect);
	intsRestore();
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
	while (livefinalizees) {
		finalizernode *n = livefinalizees->next;
		asm_calljavamethod (livefinalizees->finalizer, 
							heap+livefinalizees->objstart, 
							NULL,NULL,NULL);
		FREE (livefinalizees, finalizernode);
		livefinalizees = n;
	}

#ifndef TRACECALLARGS
	MFREE (heap, heapblock, heapsize);
#endif
	MFREE (startbits, bitfieldtype, heapsize/BITFIELDBITS);
	MFREE (markbits, bitfieldtype, heapsize/BITFIELDBITS);
	MFREE (referencebits, bitfieldtype, heapsize/BITFIELDBITS);
	chain_free (allglobalreferences);
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
	
	/*	fprintf(stderr, "heap_allocate: 0x%lx (%ld) requested, 0x%lx (%ld) aligned\n", bytelength, bytelength, length * BLOCKSIZE, length * BLOCKSIZE); */

	/*	heap_docollect(); */

	intsDisable();                        /* schani */

	memlist_getsuitable (&freestart, &freelength, length);

onemoretry:
	if (!freelength) {
		if ((topofheap+length > collectthreashold) ||
		    (topofheap+length > heapsize)) {

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
