/************************** toolbox/list.h *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Verwaltung von doppelt verketteten Listen. 

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/10/03

*******************************************************************************/

#ifndef LIST_H
#define LIST_H

typedef struct listnode {           /* Struktur f"ur ein Listenelement */
	struct listnode *next,*prev;
	} listnode;

typedef struct list {               /* Struktur f"ur den Listenkopf */
	listnode *first,*last;
	int nodeoffset;
	} list;


void list_init (list *l, int nodeoffset);

void list_addlast (list *l, void *element);
void list_addfirst (list *l, void *element);

void list_remove (list *l, void *element);
 
void *list_first (list *l);
void *list_last (list *l);

void *list_next (list *l, void *element);
void *list_prev (list *l, void *element);


/*
---------------------- Schnittstellenbeschreibung -----------------------------

Die Listenverwaltung mit diesem Modul geht so vor sich:
	
	- jede Struktur, die in die Liste eingeh"angt werden soll, mu"s 
	  eine Komponente vom Typ 'listnode' haben.
	  
	- es mu"s ein Struktur vom Typ 'list' bereitgestellt werden.
	
	- die Funktion list_init (l, nodeoffset) initialisiert diese Struktur,
	  dabei gibt der nodeoffset den Offset der 'listnode'-Komponente in
	  den Knotenstrukturen an.
	  
	- Einf"ugen, Aush"angen und Suchen von Elementen der Liste geht mit
	  den "ubrigen Funktionen.
	  
Zum besseren Verst"andnis ein kleines Beispiel:


	void bsp() {
		struct node {
			listnode linkage;
			int value;
			} a,b,c, *el;
			
		list l;
		
		a.value = 7;
		b.value = 9;
		c.value = 11;
		
		list_init (&l, OFFSET(struct node,linkage) );
		list_addlast (&l, a);
		list_addlast (&l, b);
		list_addlast (&l, c);
		
		e = list_first (&l);
		while (e) {
			printf ("Element: %d\n", e->value);
			e = list_next (&l,e);
			}
	}
	
	
	Dieses Programm w"urde also folgendes ausgeben:
		7
		9
		11



Der Grund, warum beim Initialisieren der Liste der Offset mitangegeben
werden mu"s, ist der da"s ein und das selbe Datenelement gleichzeitig
in verschiedenen Listen eingeh"angt werden kann (f"ur jede Liste mu"s
also eine Komponente vom Typ 'listnode' im Element enthalten sein).

*/

#endif
