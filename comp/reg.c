/******************************* comp/reg.c ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	The register-manager.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/10/23

*******************************************************************************/

/*********************** Structure of a register info *************************/

#define REG_TYPE_FLT        1    /* integer or floating point type       */
#define REG_SIZE_DBL        2    /* int/single or long/double            */
#define REG_INMEMORY        4    /* true if register is in memory        */
#define REG_ISFREE          8    /* true if register is currently free   */
#define REG_ISUNUSED        16   /* true if register never has been used */
#define REG_ISFREEUNUSED    24

#define IS_INT_LNG_REG(a)   (!((a)&REG_TYPE_FLT))
#define IS_FLT_DBL_REG(a)   ((a)&REG_TYPE_FLT)

#define REGTYPE_INT    0
#define REGTYPE_FLT    1
#define REGTYPE_LNG    2
#define REGTYPE_DBL    3

#define REGTYPE(a)	((a) & 3)

typedef struct reginfo {
	int  typeflags;                   /* register type, size and flags        */
	int  num;                         /* register number or stack offset      */
	list *waitlist;                   /* free list for register allocation    */
	listnode linkage;
} reginfo;



static reginfo *intregs = NULL;       /* table for the integer registers      */
static reginfo *floatregs = NULL;     /* table for the float registers        */
static int intregsnum;                /* absolute number of integer registers */
static int floatregsnum;              /* absolute number of float registers   */ 

static int intreg_ret;                /* register to return integer values    */
static int intreg_exc;                /* register to return exception value   */
static int intreg_arg1;               /* register for first integer argument  */
static int intreg_argnum;             /* number of integer argument registers */

static int floatreg_ret;              /* register for return float values     */
static int floatreg_arg1;             /* register for first float argument    */
static int floatreg_argnum;           /* number of float argument registers   */


static list savedintslist;     /* free saved int registers during allocation  */
static list savedfloatslist;   /* free saved float registers during allocation*/
static list scratchintslist;   /* free temp int registers during allocation   */
static list scratchfloatslist; /* free temp float registers during allocation */
static int *freeintregs;       /* free int registers table                    */
static int *freefloatregs;     /* free float registers table                  */

static int savedregs_num;      /* total number of registers to be saved       */
static int localvars_num;      /* total number of variables to spilled        */
static int arguments_num;      /* size of parameter field in the stackframe   */



/****************** function reg_init ******************************************

	initialises the register-allocator
	
*******************************************************************************/

static void reg_init()
{
	int n;
	
	if (!intregs) {
		for (intregsnum=0; regdescint[intregsnum] != REG_END; intregsnum++);
		intregs = MNEW (reginfo, intregsnum);
		freeintregs = MNEW (int, intregsnum + 2);
		*freeintregs++ = 0;
		freeintregs[intregsnum] = 0;

		intreg_arg1 = -1;
   		for (n=0; n<intregsnum; n++) {
			intregs[n].typeflags = 0;
			intregs[n].num = n;
			intregs[n].waitlist = NULL;
			freeintregs[n] = 0;
		    
			switch (regdescint[n]) {
				case REG_RES: break;
				case REG_RET: intreg_ret = n; 
				              break;
				case REG_EXC: intreg_exc = n;
				              break;
				case REG_SAV: intregs[n].waitlist = &(savedintslist);
				              freeintregs[n] = 1;
				              break;
				case REG_TMP: intregs[n].waitlist = &(scratchintslist);
				              freeintregs[n] = 1;
				              break;
				case REG_ARG: if (intreg_arg1 < 0) intreg_arg1 = n;
				              intreg_argnum++;
				              break;
				}
			}
					
		
		for (floatregsnum=0; regdescfloat[floatregsnum] != REG_END; floatregsnum++);
		floatregs = MNEW (reginfo, floatregsnum);
		freefloatregs = MNEW (int, floatregsnum + 2);
		*freefloatregs++ = 0;
		freefloatregs[floatregsnum] = 0;

        floatreg_arg1 = -1;
   		for (n=0; n<floatregsnum; n++) {
			floatregs[n].typeflags = REG_TYPE_FLT;
			floatregs[n].num = n;
			floatregs[n].waitlist = NULL;
			freefloatregs[n] = 0;

			switch (regdescfloat[n]) {
				case REG_RES: break;
				case REG_RET: floatreg_ret = n; 
				              break;
				case REG_EXC: panic ("can not use floating-reg as exception");
				              break;
				case REG_SAV: floatregs[n].waitlist = &(savedfloatslist);
				              freefloatregs[n] = 1;
				              break;
				case REG_TMP: floatregs[n].waitlist = &(scratchfloatslist);
				              freefloatregs[n] = 1;
				              break;
				case REG_ARG: if (floatreg_arg1 < 0) floatreg_arg1 = n;
				              floatreg_argnum++;
				              break;
				}
			}
					
		}
					

	list_init (&savedintslist, OFFSET(reginfo, linkage));
	list_init (&savedfloatslist, OFFSET(reginfo, linkage));
	list_init (&scratchintslist, OFFSET(reginfo, linkage));
	list_init (&scratchfloatslist, OFFSET(reginfo, linkage));
	
	for (n=0; n<intregsnum; n++) {
		intregs[n].typeflags |= REG_ISFREE | REG_ISUNUSED;
		if (intregs[n].waitlist) 
			list_addlast(intregs[n].waitlist, intregs+n);
	}
	for (n=0; n<floatregsnum; n++) {
		floatregs[n].typeflags |= REG_ISFREE | REG_ISUNUSED;
		if (floatregs[n].waitlist) 
			list_addlast(floatregs[n].waitlist, floatregs+n);
		}
	
	
	localvars_num = 0;
	arguments_num = 0;
}


/********************** function reg_close *************************************

	releases all allocated space for registers

*******************************************************************************/

static void reg_close ()
{
	if (intregs) MFREE (intregs, reginfo, intregsnum);
	if (floatregs) MFREE (floatregs, reginfo, floatregsnum);
}


/********************* function reg_free ***************************************

	put back a register which has been allocated by reg_allocate into the
	corresponding free list
	
*******************************************************************************/

static void reg_free (reginfo *ri)
{
	ri -> typeflags |= REG_ISFREE;

	if (ri->waitlist) {
		if (ri->typeflags & REG_INMEMORY)
			list_addlast  (ri->waitlist, ri);
		else {
#if (WORDSIZE == 4)
			reginfo *ri1;

			if (ri->typeflags & REG_TYPE_FLT) {
				if (ri->typeflags & REG_SIZE_DBL) {
					freefloatregs[ri->num] = 0;
					freefloatregs[ri->num + 1] = 0;
					ri1 = &floatregs[ri->num + 1];
					list_addfirst (ri1->waitlist, ri1);
					}
				else
					freefloatregs[ri->num] = 0;
				}
			else {
				if (ri->typeflags & REG_SIZE_DBL) {
					freeintregs[ri->num] = 0;
					freeintregs[ri->num + 1] = 0;
					ri1 = &intregs[ri->num + 1];
					list_addfirst (ri1->waitlist, ri1);
					}
				else
					freeintregs[ri->num] = 0;
				}
#endif
			list_addfirst (ri->waitlist, ri);
			}
		}
}


/******************* internal function reg_suckregister ************************

	searches for the first register of a list which fullfills the requirements:
	if argument isunused is true ri->typeflags&REG_ISUNUSED has to be true too
	if register pairs are required two adjacent register are searched
	if a register can be found it is removed from the free list and the
	fields 'isunused' and 'isfree' are set to false

*******************************************************************************/

static reginfo *reg_remove (list *l, int num) {
	reginfo *ri;

	ri = list_first (l);
	while (ri->num != num)
		ri = list_next (l, ri);
	if (ri->typeflags & REG_TYPE_FLT)
		freefloatregs[num] = 0;
	else
		freeintregs[num] = 0;
	list_remove (l, ri);	
	ri -> typeflags &= ~REG_ISFREEUNUSED;
	return ri;
}


static reginfo *reg_suckregister (list *l, bool isunused)
{
	reginfo *ri;
	
	ri = list_first (l);

	while (ri) {
		if ( (!isunused) || (ri->typeflags & REG_ISUNUSED)) {
#if (WORDSIZE == 4)
			reginfo *retreg = NULL;

			if (ri->typeflags & REG_SIZE_DBL) {
				if (ri->typeflags & REG_TYPE_FLT)
					if (ri->num & 1) {
						if(freefloatregs[ri->num - 1]) {
							freefloatregs[ri->num] = 0;
							retreg = reg_remove(l, ri->num - 1);
							}
						}
					else {
						if (freefloatregs[ri->num + 1]) {
							freefloatregs[ri->num] = 0;
							retreg = ri;
							(void) reg_remove(l, ri->num + 1);
							}
						}
				else if (freeintregs[ri->num + 1]) {
						freeintregs[ri->num] = 0;
						retreg = ri;
						(void) reg_remove(l, ri->num + 1);
						}
					else if(freeintregs[ri->num - 1]) {
						freeintregs[ri->num] = 0;
						retreg = reg_remove(l, ri->num - 1);
						}
				if (retreg) {
					list_remove (l, ri);	
					ri -> typeflags &= ~REG_ISFREEUNUSED;
					return retreg;
					}
				}
			else {
				if (ri->typeflags & REG_TYPE_FLT)
					freefloatregs[ri->num] = 0;
				else
					freeintregs[ri->num] = 0;
				list_remove (l, ri);	
				ri -> typeflags &= ~REG_ISFREEUNUSED;
				return ri;
				}
#else
			list_remove (l, ri);	
			ri -> typeflags &= ~REG_ISFREEUNUSED;
			return ri;
#endif
			}
		ri = list_next (l, ri);
		}
	return NULL;
}


/******************** Funktion: reg_allocate **********************************

	versucht, ein Register zu belegen, das vom richtigen Typ ist, und
	allen gew"unschten Anforderungen entspricht.
	
	Parameter:
		type .... JAVA-Grundtyp (INT,LONG,DOUBLE,FLOAT,ADDRESS)
		saved ... Das Register soll bei Methodenaufrufen nicht zerst"ort werden
		new ..... Das Register soll noch nie vorher verwendet worden sein
		
	Wenn es (aus verschiedenen Gr"unden) kein geeignetes freies Register f"ur
	den Zweck mehr gibt, dann erzeugt diese Funktion einen Verweis auf
	einen Platz am aktuellen Stackframe (diese Stackframe-Eintr"age erf"ullen
	auf jeden Fall alle obigen Forderungen)

*******************************************************************************/

static reginfo *reg_allocate (u2 type, bool saved, bool new)
{
	u2 t;
	reginfo *ri;

	switch (type) {
		case TYPE_LONG:
			t = REG_SIZE_DBL;
			break;
		case TYPE_INT:
			t = 0;
			break;
		case TYPE_FLOAT:
			t = REG_TYPE_FLT;
			break;
		case TYPE_DOUBLE:
			t = REG_TYPE_FLT | REG_SIZE_DBL;
			break;
		default:
			t = 0;
		}

	if (!saved) {
		if (IS_INT_LNG_REG(t))
			ri = reg_suckregister (&scratchintslist, new);
		else
			ri = reg_suckregister (&scratchfloatslist, new);
		if (ri) return ri;
		}

	if (IS_INT_LNG_REG(t))
		ri = reg_suckregister (&savedintslist, new);
	else
		ri = reg_suckregister (&savedfloatslist, new);
	if (ri) return ri;

	ri = DNEW (reginfo);
	ri -> typeflags = t | REG_INMEMORY;
#if (WORDSIZE == 4)
	ri -> num = localvars_num;
	if (t & REG_SIZE_DBL)
		localvars_num += 2;
	else
		localvars_num++;
#else
	ri -> num = (localvars_num++);
#endif
	ri -> waitlist = (IS_INT_LNG_REG(t)) ? &savedintslist : &savedfloatslist;
	return ri;
}


/********************* Funktion: reg_reallocate *******************************

	versucht, ein schon einmal angefordertes (aber in der Zwischenzeit 
	wieder freigegebenens) Register neuerlich anzufordern. 
	Wenn das Register immer noch unbenutzt ist, dann ist alles OK 
	(R"uckgabewert true), sonst wird 'false' zur"uckgeben, und aus der
	neuerlichen Belegung wird nichts. 

******************************************************************************/

static bool reg_reallocate (reginfo *ri)
{
	if (!(ri->typeflags & REG_ISFREE)) return false;

	ri->typeflags &= ~REG_ISFREEUNUSED;

	if (ri->waitlist) list_remove (ri->waitlist, ri);

	return true;
}



/****************** Funktion: reg_parlistresult *******************************

	Erzeugt eine Registerreferenz auf das Register, das vom System zur
	R"uckgabe von Werten aus Methoden verwendet wird.
	Parameter:
		type ... Der JAVA-Grundtyp des R"uckgabewertes

******************************************************************************/

static reginfo *reg_parlistresult (u2 type)
{
	if (type==TYPE_FLOAT || type==TYPE_DOUBLE) return &(floatregs[floatreg_ret]);
										  else return &(intregs[intreg_ret]);
}



/*************** Funktion: reg_parlistexception ******************************
	
	Erzeugt eine Registerreferenz auf das Register, in dem das System die
	Zeiger auf allf"allige Exception-Objekte bei Methodenaufrufen 
	zur"uckliefert.
	
******************************************************************************/

static reginfo *reg_parlistexception ()
{
	return &(intregs[intreg_exc]);
}



/************************ Funktion: reg_parlistpar ****************************

	Erzeugt eine Registerreferenz auf ein Register, in dem das n"achste
	Argument (in der Z"ahlung ab dem Zeitpunkt von Aufruf von 'reg_parlistinit')
	bei Methodenaufrufen eingetragen wird. 
	Wenn es in Summe mehr als die m"oglichen Parameter-Register werden, dann 
	wird auch noch Platz am Stack f"ur die "uberz"ahligen Werte reserviert

******************************************************************************/

static int usedintpar, usedfloatpar;
static int usedparoverflow;

static reginfo *reg_parlistpar (u2 type)
{
	reginfo *f;
	
	if (type == TYPE_FLOAT || type == TYPE_DOUBLE) {
		usedfloatpar++;
		if (reg_parammode == PARAMMODE_NUMBERED) usedintpar++;
		
		if (usedfloatpar <= floatreg_argnum) {
			f = &floatregs[floatreg_arg1 + (usedfloatpar - 1) ];
			f->typeflags &= ~REG_ISUNUSED;
			return f;
			}
		else goto overflow;
		}
	else {
		usedintpar++;
		if (reg_parammode == PARAMMODE_NUMBERED) usedfloatpar++;
		
		if (usedintpar <= intreg_argnum) {
			f = &intregs[intreg_arg1 + (usedintpar - 1) ];
			f->typeflags &= ~REG_ISUNUSED;
			return f;
			}
		else goto overflow;
	}



overflow:
	usedparoverflow++;
	if (usedparoverflow > arguments_num) arguments_num = usedparoverflow;
	return NULL;
}	


/****************** Funktion: reg_parlistinit *********************************

	initialisiert die Z"ahlung der Parameter-Register
	
*******************************************************************************/

static void reg_parlistinit()
{
	usedintpar = 0;
	usedfloatpar = 0;
	usedparoverflow = 0;
}





/***************** Funktion: reg_display *************************************
	
	gibt eine Register-Referenz in lesbarer Form auf 'stdout' aus.

******************************************************************************/

static void reg_display (reginfo *ri)
{
	if (ri->typeflags & REG_INMEMORY) {
		printf ("[%d]", (int) (ri->num) );
		}
	else {
		printf ("%s%d", 
	 	  (IS_INT_LNG_REG(ri->typeflags)) ? "$" : "f$", 
	  	  (int) (ri->num) );
		}
}


