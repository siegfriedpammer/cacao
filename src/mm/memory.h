/************************* toolbox/memory.h ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Headerfiles und Makros f"ur die Speicherverwaltung.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/10/03

*******************************************************************************/

#define CODEMMAP

#define ALIGN(pos,size)       ( ( ((pos)+(size)-1) / (size))*(size) )
#define PADDING(pos,size)     ( ALIGN((pos),(size)) - (pos) )
#define OFFSET(s,el)          ( (int) ( (size_t) &( ((s*)0) -> el ) ) )


#define NEW(type)             ((type*) mem_alloc ( sizeof(type) ))
#define FREE(ptr,type)        mem_free (ptr, sizeof(type) )

#define LNEW(type)             ((type*) lit_mem_alloc ( sizeof(type) ))
#define LFREE(ptr,type)        lit_mem_free (ptr, sizeof(type) )

#define MNEW(type,num)        ((type*) mem_alloc ( sizeof(type) * (num) ))
#define MFREE(ptr,type,num)   mem_free (ptr, sizeof(type) * (num) )
#define MREALLOC(ptr,type,num1,num2) mem_realloc (ptr, sizeof(type) * (num1), \
                                                       sizeof(type) * (num2) )

#define DNEW(type)            ((type*) dump_alloc ( sizeof(type) ))
#define DMNEW(type,num)       ((type*) dump_alloc ( sizeof(type) * (num) ))
#define DMREALLOC(ptr,type,num1,num2)  dump_realloc (ptr, sizeof(type)*(num1),\
                                                       sizeof(type) * (num2) )

#define MCOPY(dest,src,type,num)  memcpy (dest,src, sizeof(type)* (num) )

#ifdef CODEMMAP
#define CNEW(type,num)        ((type*) mem_mmap ( sizeof(type) * (num) ))
#define CFREE(ptr,num)
#else
#define CNEW(type,num)        ((type*) mem_alloc ( sizeof(type) * (num) ))
#define CFREE(ptr,num)        mem_free (ptr, num)
#endif

void *mem_alloc(int length);
void *mem_mmap(int length);
void *lit_mem_alloc(int length);
void mem_free(void *m, int length);
void lit_mem_free(void *m, int length);
void *mem_realloc(void *m, int len1, int len2);
long int mem_usage();

void *dump_alloc(int length);
void *dump_realloc(void *m, int len1, int len2);
long int dump_size();
void dump_release(long int size);

void mem_usagelog(int givewarnings);
 
 
 
/* 
---------------------------- Schnittstellenbeschreibung -----------------------

Der Speicherverwalter hat zwei m"ogliche Arten Speicher zu reservieren
und freizugeben:

	1.   explizites Anfordern / Freigeben

			mem_alloc ..... Anfordern eines Speicherblocks 
			mem_free ...... Freigeben eines Speicherblocks
			mem_realloc ... Vergr"o"sern eines Speicherblocks (wobei 
			                der Inhalt eventuell an eine neue Position kommt)
			mem_usage ..... Menge des bereits belegten Speichers


	2.   explizites Anfordern und automatisches Freigeben
	
			dump_alloc .... Anfordern eines Speicherblocks vom
			                (wie ich es nenne) DUMP-Speicher
			dump_realloc .. Vergr"o"sern eines Speicherblocks
			dump_size ..... Merkt sich eine Freigabemarke am Dump
			dump_release .. Gibt allen Speicher, der nach der Marke angelegt 
			                worden ist, wieder frei.
			                
	
Es gibt f"ur diese Funktionen ein paar praktische Makros:

	NEW (type) ....... legt Speicher f"ur ein Element des Typs `type` an.
	FREE (ptr,type) .. gibt Speicher zur"uck
	
	MNEW (type,num) .. legt Speicher f"ur ein ganzes Array an
	MFREE (ptr,type,num) .. gibt den Speicher wieder her
	
	MREALLOC (ptr,type,num1,num2) .. vergr"o"sert den Speicher f"ur das Array
	                                 auf die Gr"o"se num2
	                                 
Die meisten der Makros gibt es auch f"ur den DUMP-Speicher, na"mlich mit
gleichem Namen, nur mit vorangestelltem 'D', also:	
	
	DNEW,  DMNEW, DMREALLOC   (DFREE gibt es nat"urlich keines)


-------------------------------------------------------------------------------

Die restlichen Makros:

	ALIGN (pos, size) ... Rundet den Wert von 'pos' auf die n"achste durch
	                      'size' teilbare Zahl auf.
	                      
	
	OFFSET (s,el) ....... Berechnet den Offset (in Bytes) des Elementes 'el'   
	                      in der Struktur 's'.
	                      
	MCOPY (dest,src,type,num) ... Kopiert 'num' Elemente vom Typ 'type'.
	

*/
