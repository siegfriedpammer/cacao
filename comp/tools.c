/***************************** comp/tools.c ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Ein paar zus"atzlich notwendige Funktionen, die sonst nirgends 
	hinpassen.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/11/14

*******************************************************************************/


/***************** Funktion: compiler_addinitclass ****************************

	zum Eintragen einer Klasse in die Liste der noch zu initialisierenden 
	Klassen
	
******************************************************************************/
                                
void compiler_addinitclass (classinfo *c)
{
	classinfo *cl;

	if (c->initialized) return;
	
	cl = chain_first(uninitializedclasses);
	if (cl == c)
		return;
	
	if (cl == class)
		cl = chain_next(uninitializedclasses);
	for (;;) {
		if (cl == c)
			return;
		if (cl == NULL) {
			if (runverbose) {
				sprintf(logtext, "compiler_addinitclass: ");
				unicode_sprint(logtext+strlen(logtext), c->name);
				dolog();
				}
			chain_addlast(uninitializedclasses, c);
			return;
			}
		if (c < cl) {
			if (runverbose) {
				sprintf(logtext, "compiler_addinitclass: ");
				unicode_sprint(logtext+strlen(logtext), c->name);
				dolog();
				}
			chain_addbefore(uninitializedclasses, c);
			return;
			}
		cl = chain_next(uninitializedclasses);
		}
}
                                


/***************** Hilfsfunktionen zum Decodieren des Bytecodes ***************

	lesen ein Datum des gew"unschten Typs aus dem Bytecode an der
	angegebenen Stelle

******************************************************************************/

static u1 code_get_u1 (u4 pos)
{
	return jcode[pos];
}

static s1 code_get_s1 (u4 pos)
{
	return code_get_u1 (pos);
}

static u2 code_get_u2 (u4 pos)
{
	return ( ((u2) jcode[pos]) << 8 ) + jcode[pos+1];
}

static s2 code_get_s2 (u4 pos)
{
	return code_get_u2 (pos);
}

static u4 code_get_u4 (u4 pos)
{
	return    ( ((u4) jcode[pos])   << 24 ) 
	        + ( ((u4) jcode[pos+1]) << 16 )
	        + ( ((u4) jcode[pos+2]) << 8 )
	        + ( jcode[pos+3] );
}

static s4 code_get_s4 (u4 pos)
{
	return code_get_u4 (pos);
}



/******************** Funktion: descriptor2types *****************************

	Decodiert einen Methoddescriptor.
	Beim Aufruf dieser Funktion MUSS (!!!) der Descriptor ein
	gueltiges Format haben (wird eh vorher vom loader ueberprueft).
	
	Die Funktion erzeugt ein Array von integers (u2), in das die 
	Parametertypen eingetragen werden, und liefert einen Zeiger auf
	das Array in einem Referenzparameter ('paramtypes') zur"uck.
	Die L"ange dieses Arrays und der Methodenr"uckgabewert werden ebenfalls
	in Referenzparametern zur"uckgeliefert.
	
	Der Parameter 'isstatic' gibt an (wenn true), dass kein zus"atzlicher
	erster Eintrag f"ur den this-Zeiger in das Array eingetragen
	werden soll (sonst wird er n"amlich automatisch erzeugt, mit dem
	Typ TYPE_ADDRESS).
	
******************************************************************************/		

static void descriptor2types (unicode *desc, bool isstatic,
                 s4 *paramnum, u1 **paramtypes, s4 *returntype)
{
	u2 *text = desc->text;
	s4 pos;
	u1 *types;
	s4 tnum;
	
	tnum = (isstatic) ? 0 : 1; 
	pos=1;
	while (text[pos] != ')') {
	   repeatcounting:	
		
		switch (text[pos]) {
		case '[':  pos++;
		           goto repeatcounting;
		case 'L':  while (text[pos]!=';') pos++;
	               break;
	    }
		pos++;
		tnum++;
		}
	
	types = DMNEW (u1, tnum);
	
	if (isstatic) tnum=0;
	else {
		types[0] = TYPE_ADDRESS;
		tnum = 1;
		}
	pos=1;
	while (text[pos] != ')') {
		switch (text[pos]) {
		case 'B':
		case 'C':
		case 'I':
		case 'S':
		case 'Z':  types[tnum++] = TYPE_INT;
		           break;
		case 'J':  types[tnum++] = TYPE_LONG;
		           break;
		case 'F':  types[tnum++] = TYPE_FLOAT;
		           break;
		case 'D':  types[tnum++] = TYPE_DOUBLE;
		           break;
		case 'L':  types[tnum++] = TYPE_ADDRESS;
		           while (text[pos] != ';') pos++;
		           break;
		case '[':  types[tnum++] = TYPE_ADDRESS;
		           while (text[pos] == '[') pos++;
		           if (text[pos] == 'L') while (text[pos] != ';') pos++;
		           break;
		default:   panic ("Ill formed methodtype-descriptor");
		}
		pos++;
		}
		
	pos++;	/* ueberlesen von ')' */

	switch (text[pos]) {
		case 'B':
		case 'C':
		case 'I':
		case 'S':
		case 'Z':  *returntype = TYPE_INT;
		           break;
		case 'J':  *returntype = TYPE_LONG;
		           break;
		case 'F':  *returntype = TYPE_FLOAT;
		           break;
		case 'D':  *returntype = TYPE_DOUBLE;
		           break;
		case '[':
		case 'L':  *returntype = TYPE_ADDRESS;
		           break;
		case 'V':  *returntype = TYPE_VOID;
		           break;
	
		default:   panic ("Ill formed methodtype-descriptor");
		}

	*paramnum = tnum;
	*paramtypes = types;
}

