/**************************** comp/local.c *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	behandeln der lokalen Java-Variablen

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/11/14

*******************************************************************************/

	/* es werden f"ur die lokalen Java-Variablen 5 Tabellen angelegt,
	   f"ur jeden m"oglichen Typ eine, weil mehrere Java-Variablen 
	   verschiendenen Typs die selben Slots belegen k"onnen */

varid *locals[5];


/*********************** Funktion: local_get **********************************

	erzeugt eine neue Pseudo-Variable f"ur einen JavaVM-Slot und einen
	gew"unschten Typ, oder eine bereits vorhandene Variable.

******************************************************************************/

static varid local_get (int slot, int type)
{
	varid v;
	int   secondslot;
	
	secondslot = ((type==TYPE_DOUBLE) || (type==TYPE_LONG)) ? 1 : 0;

	if (slot+secondslot>=maxlocals) 
		panic ("Local-variable access out of bounds");
	v = locals[type][slot];

	if (v==NULL) {
		v = var_createwithspecialnumber (type, slot);
		v -> globalscope = true;
		locals[type][slot] = v;
		}

	return v;
}




/************************* Funktion: local_init *******************************
	
	legt die 5 Tabellen an, und erzeugt auch gleich die Belegungen
	f"ur die Methodenparameter.

******************************************************************************/

static void local_init()
{
	int t, i, slot;
	
	if (TYPE_INT != 0 || TYPE_ADDRESS != 4) 
	  panic ("JAVA-Basictypes have been changed");
		
	for (t=TYPE_INT; t<=TYPE_ADDRESS; t++) {
		locals[t] = DMNEW (varid, maxlocals);
		for (i=0; i<maxlocals; i++) locals[t][i] = NULL;
		}


	slot =0;
	for (i=0; i<mparamnum; i++) {
		t = mparamtypes[i];
		mparamvars[i] = local_get (slot, t);
		slot += ((t==TYPE_DOUBLE || t==TYPE_LONG) ? 2 : 1);
		}
}


/************************ Funktion: local_regalloc ****************************

	f"uhrt die Registerbelegung f"ur die lokalen Java-Variablen durch
	
******************************************************************************/
	

static void local_regalloc ()
{
	int   s, t;
	varid v;
	
	for (s=0; s<maxlocals; s++) 
		for (t=TYPE_INT; t<=TYPE_ADDRESS; t++) {
			v = locals[t][s];
			if (v) {
				if (! isleafmethod)
					var_makesaved (v);
				if (v->reg == NULL)
					v->reg = reg_allocate (v->type, v->saved, true);
				}
			}
}

