/****************************** comp/var.c *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	verwaltet die Pseudoregister (die ich manchmal auch einfach nur
	'Variablen'  nenne, daher der Namen des Programmteiles)

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/11/14

*******************************************************************************/


s4 varnum;                /* Anzahl der bereits vergebenen Variablen 
                             (eigentlich nur, um den Variablen forlaufende
                             Nummern zu geben, damit man sie beim Debuggen
                             eindeutig kennzeichnen kann) */

list copiedvars;          /* Liste aller Variablen die nur eine Kopie einer 
                             anderen Variablen sind (w"ahrend der 
                             Parse-Phase notwendig) */
list activevars;          /* Liste aller gerade aktiven Variablen 
						     (f"ur die Registerbelegungs-phase notwendig) */
						      
list vars;                /* Liste aller Variablen, die nicht in einer der
                             obigen Listen sind */


/********************** Funktion: var_init ************************************

	initialisiert die Listen und den Variablen-Index-Z"ahler

******************************************************************************/

static void var_init ()
{
	varnum = maxlocals;
	list_init (&vars, OFFSET (varinfo, linkage) );
	list_init (&copiedvars, OFFSET (varinfo, linkage) );
	list_init (&activevars, OFFSET (varinfo, linkage) );
}


/********************* Funktion: var_type *************************************

	Liefert den JavaVM-Grundtyp einer Variablen

******************************************************************************/

static u2 var_type (varid v)
{
	return v->type;
}


/********************* Funktion: var_create ***********************************

	Erzeugt eine neue Variable des gew"unschten Typs
	
******************************************************************************/

static varid var_create (u2 type)
{
	varinfo *v = DNEW (varinfo);

	list_addlast (&vars, v);
	
	list_init (&(v -> copies), OFFSET (varinfo, copylink) );
	v -> original = v;
	
	v -> type = type;
	v -> number = varnum++;
	v -> active = false;
	v -> globalscope = false;
	v -> saved = false;

	v -> reg = NULL;
	
	return v;
}


/***************** Funktion: var_createwithspecialnumber **********************

	Erzeugt eine neue Variable des gew"unschten Typs, dabei wird aber als
	Kennzeichnungsnummer eine vorgegebene Zahl verwendet (das brauche ich
	um die lokalen Java-Variablen mit der Nummer des ihres Slots zu 
	kennzeichnen).
	
******************************************************************************/

static varid var_createwithspecialnumber(u2 type, u4 num)
{
	varinfo *v = var_create (type);
	varnum--;
	v -> number = num;
	return v;
}


/*************** Funktion: var_makesaved **************************************

	Kennzeichnet eine Variable daf"ur, dass sie bei Methodenaufrufen nicht
	zerst"ort werden darf.
	
******************************************************************************/

static void var_makesaved (varid v)
{
	v -> saved = true;
	v -> reg = NULL;
}


/*************** Funktion: var_proposereg *************************************

	Macht dem Regiserallokator einen Vorschlag, mit welchem Register
	eine Variable belegt werden sollte (bei Methodenaufrufen k"onnen so
	viele Umlade-Befehle in die Argumentregister vermieden werden).
	Aber: Die Anforderung, dass ein Register gesichert sein soll, hat auf
		jeden Fall Priorit"at "uber so einen Vorschlag.
	
******************************************************************************/

static void var_proposereg (varid v, reginfo *r)
{
	if (v -> saved) return;
	
	v -> reg = r;
}


/******************** Funktion: var_makecopy **********************************

	kennzeichnet eine Variable daf"ur, dass sie nur eine Kopie einer anderen
	Variable enth"alt. 

******************************************************************************/

static void var_makecopy (varid original, varid copy)
{
	list_addlast (&(original->copies), copy);
	copy -> original = original;
	
	list_remove (&vars, copy);
	list_addlast (&copiedvars, copy);
}


/******************** Funktion: var_unlinkcopy ********************************

	eine Variable, die bis jetzt in der Liste der Kopien eingetragen
	war, wird wieder auf normalen Zustand gebracht.
	
******************************************************************************/

static void var_unlinkcopy (varid copy)
{
	list_remove (&(copy->original->copies), copy);
	copy -> original = copy;
	
	list_remove (&copiedvars, copy);
	list_addlast (&vars, copy);
}


/******************* Funktion: var_isoriginal *********************************

	Liefert true, wenn die Variable selber das Original ist, und keine
	Kopie einer anderen Variablen (das heisst, wenn sie nicht in der Liste
	der Kopien eingetragen ist)
	
******************************************************************************/

static bool var_isoriginal (varid copy)
{
	return (copy -> original == copy) ? true : false;
}


/******************* Funktion: var_findoriginal *******************************

	Sucht zu einer Variablen das Original (wenn die Variable eine Kopie ist),
	oder gibt die Variable selbst zur"uck (im anderen Fall).
	
******************************************************************************/

static varid var_findoriginal (varid v)
{
	return v->original;
}


/******************* Funktion: var_nextcopy ***********************************

	Gibt die erste noch eingetragene Kopie einer Variablen zur"uck.
	(oder NILL, wenn die Variable keine Kopien hat)
	
******************************************************************************/

static varid var_nextcopy (varid original) 
{
	return list_first (&original->copies);
}


/******************* Funktion: var_nextcopiedvar ******************************

	Gibt die erste "uberhaupt noch vorhandene Variable zu"uck, die eine
	Kopie irgendeiner anderen Variablen ist.
	
******************************************************************************/

static varid var_nextcopiedvar ()
{
	return list_first (&copiedvars);
}



/*********************** Funktion: var_isactive *******************************

	Lieftert true, wenn die Variable gerade aktiviert ist (d.h., wenn 
	irgendwann vorher 'var_activate' aufgerufen wurde) 

******************************************************************************/

static bool var_isactive (varinfo *v)
{
	return v->active;
}


/******************** Funktion: var_activate **********************************

	Aktiviert eine Variable, d.h. sie wird in die Liste der aktiven
	Variablen eingetragen.
	
******************************************************************************/
	
static void var_activate (varinfo *v)
{
	list_remove (&vars, v);
	list_addlast (&activevars, v);	
	v -> active = true;
}


/******************** Funktion: var_deactivate ********************************

	Deaktiviert eine Variable (Gegenst"uck zu var_activate)
	
******************************************************************************/

static void var_deactivate (varinfo *v)
{
	list_remove (&activevars, v);
	list_addlast (&vars, v);
	v -> active = false;
}


/****************** Funktion: var_nextactive **********************************

	Liefert die erste noch aktivierte Variable 

******************************************************************************/

static varinfo *var_nextactive ()
{
	return list_first (&activevars);
}



/**************************** Funktion: var_display **************************

	Gibt eine abdruckbare Darstellung einer Variablen aus.
	(nur zu Debug-Zwecken)
	
*****************************************************************************/

static void var_display (varinfo *v)
{
	if (v==NOVAR) {
		printf ("_ ");
		return;
		}
		
	switch (v->type) {
	case TYPE_INT: printf ("I"); break;
	case TYPE_LONG: printf ("L"); break;
	case TYPE_FLOAT: printf ("F"); break;
	case TYPE_DOUBLE: printf ("D"); break;
	case TYPE_ADDRESS: printf ("A"); break;
	default: printf ("?");
	}
	printf ("%d", v->number);

	if (v->reg) {
		printf ("(="); 
		reg_display (v->reg);
		printf (")");
		}
	printf (" ");
}


/************************ Funktion: var_displayall **************************

	Gibt eine abdruckbare Darstellung aller Variablen aus.
	(nur zu Debug-Zwecken)
	
*****************************************************************************/

void var_displayall ()
{
	varid v;
	int num=0;
	varid *sorted = DMNEW (varid, varnum);

	printf ("\n   Types of all pseudo-variables:\n");
	
	for (num=0; num<varnum; num++) sorted[num] = NULL;
	v = list_first (&vars);
	while (v) {
		if (sorted[v->number]) {
			printf ("  Local variable overlay:  "); 
			if (v->saved) printf ("* ");
			       else   printf ("  ");
			
			var_display (v);
			printf ("\n");
			} 
		else sorted[v->number] = v;
		
		v = list_next (&vars, v);
		}

	
	for (num=0; num<varnum; num++) {
		v = sorted[num];
		if (v) {
			if (v->saved) printf ("* ");
			       else   printf ("  ");

			var_display (v);
			if (!v->reg) printf ("   ");
			
			if ( (num%5) == 4 ) printf ("\n");
			else printf ("\t");
			}
		}
	
	printf ("\n");
}


