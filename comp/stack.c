/***************************** alpha/gen.c *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	behandeln des Java-Stacks

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/11/14

*******************************************************************************/


static stackinfo *tos;       /* Top of Stack */



/********************* Verwaltungsfunktionen *********************************/

static void stack_init() { tos = NULL; }

static stackinfo *stack_get () { return tos; }

static void stack_restore (stackinfo *s)  { tos = s; }

static bool stack_isempty () { return tos == NULL; }



/********************** Funktion: stack_topslots ******************************

	liefert die Anzahl der JavaVM-slots, die die oberste Pseudovariable am
	Stack belegt (entweder 2 bei LONG und DOUBLE, oder 1 sonst)

******************************************************************************/

static u2 stack_topslots ()
{
	u2 t;
	if (!tos) panic ("Stack is empty on attempt to determine top slots");

	t = var_type (tos->var);
	return ( (t==TYPE_LONG) || (t==TYPE_DOUBLE) ) ? 2 : 1;
}


/********************* Funktion: stack_push ***********************************

	erzeugt eine neues Pseudoregister eines gew"unschten Typs und gibt
	es oben auf den Stack.	
	Ausserdem liefert die Funktion den Zeiger auf dieses Pseudoregister
	
******************************************************************************/

static varinfo *stack_push (u2 type)
{
	stackinfo *s = DNEW (stackinfo);

	s -> prev = tos;
	s -> var = var_create (type);

	tos = s;
	return s -> var;
}


/********************** Funktion: stack_repush ********************************

	Gibt ein bereits vorhandenen Pseudoregister auf den Stack
	
******************************************************************************/

static void stack_repush (varinfo *v) 
{
	stackinfo *s = DNEW (stackinfo);

	s -> prev = tos;
	s -> var = v;

	tos = s;
}


/********************** Funktion: stack_pop ***********************************

	nimmt das oberste Pseudoregister vom Stack, dabei wird aber 
	"uberpr"uft, ob der Typ stimmt.

******************************************************************************/

static varinfo *stack_pop (u2 type)
{
	varinfo *v;
	
	if (!tos) panic ("Stack is empty on attempt to pop");

	v = tos -> var;
	tos = tos -> prev;

	if (var_type (v) != type) panic ("Popped invalid element from stack");
	return v;
}


/********************* Funktion: stack_popany ********************************

	nimmt das oberste Pseudoregister vom stack, ohne Typ"uberpr"ufung
	durchzuf"uhren, wobei aber zumindest die Anzahl der notwendigen
	Slots "ubereinstimmen muss.

******************************************************************************/

static varinfo *stack_popany (u2 slots)
{
	varinfo *v;
	
	if (!tos) panic ("Stack is empty on attempt to pop");
	if (slots != stack_topslots() ) 
		panic ("Pop would tear LONG/DOUBLE-Datatype apart");

	v = tos -> var;
	tos = tos -> prev;

	return v;
}



/********************** Funktion: stack_popmany *******************************

	nimmt vom Stack soviele Pseudoregister, dass die Anzahl der von
	ihnen belegten Slots die gew"unschte Menge ergeben.
	Zeiger auf dieses Pseudoregister werden im Array vars gespeichert, und
	deren tats"achliche Anzahl als Funktionswert zur"uckgeliefert.
	(Haupts"achlich f"ur die Typneutralen DUP/POP.. Operationen)

******************************************************************************/

static u2 stack_popmany (varid *vars, u2 slots)
{
	u2 ts;
	u2 varcount=0;
	
	while (slots>0) {
		ts=stack_topslots();
		if (ts > slots) panic ("POP would tear LONG/DOUBLE-Datatype apart");
		vars[(varcount)++] = stack_popany(ts);
		slots -= ts;				
		}
	return varcount;
}


/********************** Funktion: stack_pushmany ******************************

	Gibt eine Anzahl von Pseudoregister auf den Stack.

******************************************************************************/

static void stack_repushmany (u2 varcount, varid *vars)
{
	u2 i;
	for (i=0; i<varcount; i++) stack_repush(vars[(varcount-1)-i]);
}


/***************** Funktion: stack_addjoincode ********************************

	erzeugt die passenden MOVE-Commandos, sodass alle Pseudoregisterinhalte
	eines Stacks auf die Pseudoregister eines anderen Stacks kopiert
	werden.
	Dann wird der Zielstack als neuer aktueller Stack verwendet.
	(F"ur allem f"ur Verzweigungs- und Sprungbefehle)
	
*******************************************************************************/ 

static void stack_jointail (stackinfo *now, stackinfo *then)
{
	varinfo *v1,*v2;
	u2 t;

	if (now==then) return;
	if ( (now==NULL) || (then==NULL) ) 
		panic ("Stacks of different length on join");

	v1 = now->var;
	v2 = then->var;
	if (v1 != v2) {
		t = var_type (v1);
		if (t != var_type (v2)) 
		    panic ("Mismatching stack types on join of control flow");

		pcmd_move_n_drop (t, v1,v2);
	}

	stack_jointail (now -> prev, then -> prev);
	return;
}

static void stack_addjoincode (stackinfo *targetstack)
{
	stack_jointail (tos, targetstack);
	tos = targetstack;
}	


/******************* Funktion: stack_makesaved ********************************

	Kennzeichnet alle am Stack befindlichen Pseudoregister als
	zu sichernde Register.

*******************************************************************************/

static void stack_makesaved ()
{
	stackinfo *s = tos;
	while (s) {
		var_makesaved (s->var);
		s = s->prev;
		}
}


/******************* Funktion: stack_display **********************************

	Gibt den Inhalt des Stacks aus (also die Pseudoregister, die am 
	Stack liegen)
	(Nur zu Debug-Zwecken)
	
******************************************************************************/

static void stack_display (stackinfo *s)
{
	if (s) {
		stack_display (s->prev);
		var_display (s->var);
		}
}

