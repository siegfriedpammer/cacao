/***************************** ncomp/ntools.c **********************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Ein paar zus"atzlich notwendige Funktionen, die sonst nirgends 
	hinpassen.

	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/11/03

*******************************************************************************/


/***************** Funktion: compiler_addinitclass ****************************

	zum Eintragen einer Klasse in die Liste der noch zu initialisierenden 
	Klassen
	
******************************************************************************/
                                
static void compiler_addinitclass (classinfo *c)
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

#define code_get_u1(pos)    jcode[pos]
#define code_get_s1(pos)    ((s1)jcode[pos])
#define code_get_u2(pos)    ((((u2)jcode[pos])<<8)+jcode[pos+1])
#define code_get_s2(pos)    ((s2)((((u2)jcode[pos])<<8)+jcode[pos+1]))
#define code_get_u4(pos)    ((((u4)jcode[pos])<<24)+(((u4)jcode[pos+1])<<16)+\
                             (((u4)jcode[pos+2])<<8)+jcode[pos+3])
#define code_get_s4(pos)    ((s4)((((u4)jcode[pos])<<24)+(((u4)jcode[pos+1])<<16)+\
                             (((u4)jcode[pos+2])<<8)+jcode[pos+3]))


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
	Typ TYPE_ADR).
	
******************************************************************************/		

static void descriptor2types (methodinfo *m)
{
	u1 *types, *tptr;
	int pcount, c;
	u2 *cptr;

	pcount = 0;
	types = DMNEW (u1, m->descriptor->length);
	
	tptr = types;
	if (!(m->flags & ACC_STATIC)) {
		*tptr++ = TYPE_ADR;
		pcount++;
		}

	cptr = m->descriptor->text;
	cptr++;
	while ((c = *cptr++) != ')') {
		pcount++;
		switch (c) {
			case 'B':
			case 'C':
			case 'I':
			case 'S':
			case 'Z':  *tptr++ = TYPE_INT;
			           break;
			case 'J':  *tptr++ = TYPE_LNG;
			           break;
			case 'F':  *tptr++ = TYPE_FLT;
			           break;
			case 'D':  *tptr++ = TYPE_DBL;
			           break;
			case 'L':  *tptr++ = TYPE_ADR;
			           while (*cptr++ != ';');
			           break;
			case '[':  *tptr++ = TYPE_ADR;
			           while (c == '[')
			               c = *cptr++;
			           if (c == 'L')
			               while (*cptr++ != ';') /* skip */;
			           break;
			default:   panic ("Ill formed methodtype-descriptor");
			}
		}
		
	switch (*cptr) {
		case 'B':
		case 'C':
		case 'I':
		case 'S':
		case 'Z':  m->returntype = TYPE_INT;
		           break;
		case 'J':  m->returntype = TYPE_LNG;
		           break;
		case 'F':  m->returntype = TYPE_FLT;
		           break;
		case 'D':  m->returntype = TYPE_DBL;
		           break;
		case '[':
		case 'L':  m->returntype = TYPE_ADR;
		           break;
		case 'V':  m->returntype = TYPE_VOID;
		           break;
	
		default:   panic ("Ill formed methodtype-descriptor");
		}

	m->paramcount = pcount;
	m->paramtypes = types;
}

