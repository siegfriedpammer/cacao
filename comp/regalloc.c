/************************** comp/regalloc.c ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	The register-allocator.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/03/05

*******************************************************************************/


/******************** Funktion: regalloc_doalloc ******************************

	versucht f"ur ein Pseudoregister ein tats"achliches CPU-Register zu 
	belegen.
	Wenn noch keine Belegung stattgefunden hat, dann wird ein passendes
	gerade freies Register angefordert.
	Wenn das Pseudoregsiter bereits vorher schon einmal ein Belegung
	hatte, dann wird versucht, das selbe CPU-Register wie vorher wieder
	anzufordern. Wenn es nicht gelingt (weil es schon wieder anderwertig
	verwendet wird), wann wird eben ein ganz neues Register angefordert.
	
******************************************************************************/

static void regalloc_doalloc (varinfo *v)
{
	if (v->reg) {
		if (! reg_reallocate (v->reg)) {
			v->reg = reg_allocate (v->type, v->saved, true);
			}
		}
	else {
		v->reg = reg_allocate (v->type, v->saved, false);
		}
}



/********************* Funktion: regalloc_activate ****************************

	setzt ein Pseudoregister auf aktiv (wenn es nicht schon 
	bereits aktiv ist), und fordert gegebenenfalls gleich ein passendes
	CPU-Register an
	Diese Operation wird aber nur bei Variablen mit lokalen Scope
	gemacht.
	
******************************************************************************/

static void regalloc_activate (varinfo *v)
{
	if (! v->globalscope) {
		if (! var_isactive(v) ) {
			regalloc_doalloc (v);
			var_activate (v);
			}
		}
}


/******************** Funktion: regalloc_deactivate ***************************

	setzt ein Pseudoregister auf inaktiv (wenn es nicht schon inaktiv war)

******************************************************************************/

static void regalloc_deactivate (varinfo *v)
{
	if (! v->globalscope) {
		if (var_isactive(v) ) {
			var_deactivate (v);
			if (v->reg) reg_free (v->reg);
			}
		}
}


/******************** Funktion: regalloc_cmd **********************************

	f"uhrt f"ur ein Pseudo-Command die Registerbelegung durch.
	Wird von regalloc in einer Schleife aufgerufen.
	
******************************************************************************/ 
	
static void regalloc_cmd (pcmd *c)
{
	switch (c->tag) {
		case TAG_DROP:
			regalloc_deactivate (c->dest);
			break;
		case TAG_METHOD:
			if (c->u.method.exceptionvar) 
				regalloc_activate (c->u.method.exceptionvar);
			if (c->dest) 
				regalloc_activate (c->dest);
			break;
		default:
			if (c->dest) regalloc_activate (c->dest);
			break;
		}
}


/******************** Funktion: regalloc **************************************

	f"uhrt f"ur einen ganzen Block die Registerbelegung durch.

******************************************************************************/	

static void regalloc (basicblock *b)
{
	stackinfo *tos;
	pcmd *c;
	varinfo *v; 
	
		/* alle Pseudoregister am Stack, die schon eine Belegung haben, 
		   wieder aktivieren */
	tos = b->stack;
	while (tos) {
		if (tos -> var -> reg) regalloc_activate(tos->var);
		tos = tos->prev;
		}
	
		/* alle anderen Pseudoregister am Stack belegen */
	tos = b->stack;
	while (tos) {
		regalloc_activate(tos->var);
		tos = tos->prev;
		}
	
	
		/* alle Befehle abarbeiten und Registerbelegung machen */
	c = list_first (&b->pcmdlist);
	while (c) {
		regalloc_cmd (c);
		c = list_next (&b->pcmdlist, c);			
		} 


		/* alle noch aktiven Pseudoregister deaktivieren */
	while ( (v = var_nextactive ()) != NULL) {
		regalloc_deactivate (v);
		}
}

