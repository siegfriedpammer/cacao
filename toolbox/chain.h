/************************* toolbox/chain.h *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	dient zur Verwaltung doppelt verketteter Listen mit externer Verkettung

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/10/03

*******************************************************************************/

typedef struct chainlink {          /* Struktur f"ur ein Listenelement */
	struct chainlink *next,*prev;
	void *element;
	} chainlink;

typedef struct chain {	            /* Struktur f"ur eine Liste */
	int  usedump;   

	chainlink *first,*last;
	chainlink *active;
	} chain;


chain *chain_new ();
chain *chain_dnew ();
void chain_free(chain *c);

void chain_addafter(chain *c, void *element);
void chain_addbefore(chain *c, void *element);
void chain_addlast (chain *c, void *element);
void chain_addfirst (chain *c, void *element);

void chain_remove(chain *c);
void *chain_remove_go_prev(chain *c);
void chain_removespecific(chain *c, void *element);

void *chain_next(chain *c);
void *chain_prev(chain *c);
void *chain_this(chain *c);

void *chain_first(chain *c);
void *chain_last(chain *c);


/*
--------------------------- Schnittstellenbeschreibung ------------------------

Bei Verwendung dieser Funktionen f"ur die Listenverwaltung mu"s, im
Gegenstatz zum Modul 'list', keine zus"atzliche Vorbereitung in den 
Element-Strukturen gemacht werden.

Diese Funktionen sind daher auch ein wenig langsamer und brauchen 
mehr Speicher.

Eine neue Liste wird mit 
		  chain_new
	oder  chain_dnew 
angelegt. Der Unterschied ist, da"s bei der zweiten Variante alle
zus"atzlichen Datenstrukturen am DUMP-Speicher angelegt werden (was
schneller geht, und ein explizites Freigeben am Ende der Verarbeitung
unn"otig macht). Dabei mu"s man aber achtgeben, da"s man nicht 
versehentlich Teile dieser Strukturen durch ein verfr"uhtes 'dump_release'
freigibt.

Eine nicht mehr verwendete Liste kann mit
		chain_free
freigegeben werden (nur verwenden, wenn mit 'chain_new' angefordert)


Das Eintragen neuer Elemente geht sehr einfach mit:
	chain_addafter, chain_addlast, chain_addbefore, chain_addfirst		
	
Durchsuchen der Liste mit:
	chain_first, chain_last, chain_prev, chain_next, chain_this
	
L"oschen von Elementen aus der Liste:
	chain_remove, chain_remove_go_prev, chain_removespecific
	
	
ACHTUNG: In den zu verkettenden Elementen gibt es ja (wie oben gesagt) keine
	Referenzen auf die Liste oder irgendwelche Listenknoten, deshalb k"onnen
	die Elemente nicht als Ortsangabe innerhalb der Liste fungieren.
 	Es gibt vielmehr ein Art 'Cursor', der ein Element als das gerade
 	aktuelle festh"alt, und alle Ein-/und Ausf"ugeoperationen geschehen
 	relativ zu diesem Cursor.

*/

