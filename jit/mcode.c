/***************************** comp/mcode.c ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	This file is an include file for "compiler.c" . It contains (mostly)
	architecture independent functions for writing instructions into the
	code area and constants into the data area.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	Changes: Micheal Gschwind    EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1998/08/10


	All functions assume the following code area / data area layout:

	+-----------+
	|           |
	| code area | code area grows to higher addresses
	|           |
	+-----------+ <-- start of procedure
	|           |
	| data area | data area grows to lower addresses
	|           |
	+-----------+

	The functions first write into a temporary code/data area allocated by
	"mcode_init". "mcode_finish" copies the code and data area into permanent
	memory. All functions writing values into the data area return the offset
	relative the begin of the code area (start of procedure).	

*******************************************************************************/

#define MCODEINITSIZE (1<<15)       /* 32 Kbyte code area initialization size */
#define DSEGINITSIZE  (1<<12)       /*  4 Kbyte data area initialization size */

static u1* mcodebase = NULL;        /* base pointer of code area              */
static s4* mcodeend  = NULL;        /* pointer to end of code area            */
static int mcodesize;               /* complete size of code area (bytes)     */

static u1* dsegtop = NULL;          /* pointer to top (end) of data area      */
static int dsegsize;                /* complete size of data area (bytes)     */
static int dseglen;                 /* used size of data area (bytes)         */
                                    /* data area grows from top to bottom     */

static jumpref *jumpreferences;     /* list of jumptable target addresses     */
static dataref *datareferences;     /* list of data segment references        */
static branchref *xboundrefs;       /* list of bound check branches           */
static branchref *xcheckarefs;      /* list of array size check branches      */
static branchref *xnullrefs;        /* list of null check branches            */
static branchref *xcastrefs;        /* list of cast check branches            */
static branchref *xdivrefs;         /* list of divide by zero branches        */

static void mcode_init();           /* allocates code and data area           */
static void mcode_close();          /* releases temporary storage             */
static void mcode_finish();         /* makes code and data area permanent and */
                                    /* updates branch references to code/data */

static s4 dseg_adds4(s4 value);         /* adds an int to data area           */
static s4 dseg_adds8(s8 value);         /* adds an long to data area          */
static s4 dseg_addfloat (float value);  /* adds an float to data area         */
static s4 dseg_adddouble(double value); /* adds an double to data area        */

#if POINTERSIZE==8
#define dseg_addaddress(value)      dseg_adds8((s8)(value))
#else
#define dseg_addaddress(value)      dseg_adds4((s4)(value))
#endif

static void dseg_addtarget(basicblock *target);
static void dseg_adddata(u1 *ptr);
static void mcode_addreference(basicblock *target, void *branchptr);
static void mcode_addxboundrefs(void *branchptr);
static void mcode_addxnullrefs(void *branchptr);
static void mcode_addxcastrefs(void *branchptr);
static void mcode_addxdivrefs(void *branchptr);

static void dseg_display(s4 *s4ptr);

/* mcode_init allocates and initialises code area, data area and references   */

static void mcode_init()
{
	if (!mcodebase) {
		mcodebase = MNEW (u1, MCODEINITSIZE);
		mcodesize = MCODEINITSIZE;
		}

	if (!dsegtop) {
		dsegtop = MNEW (u1, DSEGINITSIZE);
		dsegsize = DSEGINITSIZE;
		dsegtop += dsegsize;
		}

	dseglen = 0;

	jumpreferences = NULL;
	datareferences = NULL;
	xboundrefs = NULL;
	xnullrefs = NULL;
	xcastrefs = NULL;
	xdivrefs = NULL;
}


/* mcode_close releases temporary code and data area                          */

static void mcode_close()
{
	if (mcodebase) {
		MFREE (mcodebase, u1, mcodesize);
		mcodebase = NULL;
		}
	if (dsegtop) {
		MFREE (dsegtop - dsegsize, u1, dsegsize);
		dsegtop = NULL;
		}
}


/* mcode_increase doubles code area                                           */

static s4 *mcode_increase(u1 *codeptr)
{
	long len;

	len = codeptr - mcodebase;
	mcodebase = MREALLOC(mcodebase, u1, mcodesize, mcodesize * 2);
	mcodesize *= 2;
	mcodeend = (s4*) (mcodebase + mcodesize);
	return (s4*) (mcodebase + len);
}


/* desg_increase doubles data area                                            */

static void dseg_increase() {
	u1 *newstorage = MNEW (u1, dsegsize * 2);
	memcpy ( newstorage + dsegsize, dsegtop - dsegsize, dsegsize);
	MFREE (dsegtop - dsegsize, u1, dsegsize);
	dsegtop = newstorage;
	dsegsize *= 2;
	dsegtop += dsegsize;
}


static s4 dseg_adds4_increase(s4 value)
{
	dseg_increase();
	*((s4 *) (dsegtop - dseglen)) = value;
	return -dseglen;
}


static s4 dseg_adds4(s4 value)
{
	s4 *dataptr;

	dseglen += 4;
	dataptr = (s4 *) (dsegtop - dseglen);
	if (dseglen > dsegsize)
		return dseg_adds4_increase(value);
	*dataptr = value;
	return -dseglen;
}


static s4 dseg_adds8_increase(s8 value)
{
	dseg_increase();
	*((s8 *) (dsegtop - dseglen)) = value;
	return -dseglen;
}


static s4 dseg_adds8(s8 value)
{
	s8 *dataptr;

	dseglen = ALIGN (dseglen + 8, 8);
	dataptr = (s8 *) (dsegtop - dseglen);
	if (dseglen > dsegsize)
		return dseg_adds8_increase(value);
	*dataptr = value;
	return -dseglen;
}


static s4 dseg_addfloat_increase(float value)
{
	dseg_increase();
	*((float *) (dsegtop - dseglen)) = value;
	return -dseglen;
}


static s4 dseg_addfloat(float value)
{
	float *dataptr;

	dseglen += 4;
	dataptr = (float *) (dsegtop - dseglen);
	if (dseglen > dsegsize)
		return dseg_addfloat_increase(value);
	*dataptr = value;
	return -dseglen;
}


static s4 dseg_adddouble_increase(double value)
{
	dseg_increase();
	*((double *) (dsegtop - dseglen)) = value;
	return -dseglen;
}


static s4 dseg_adddouble(double value)
{
	double *dataptr;

	dseglen = ALIGN (dseglen + 8, 8);
	dataptr = (double *) (dsegtop - dseglen);
	if (dseglen > dsegsize)
		return dseg_adddouble_increase(value);
	*dataptr = value;
	return -dseglen;
}


static void dseg_addtarget(basicblock *target)
{
	jumpref *jr = DNEW(jumpref);

	jr->tablepos = dseg_addaddress(NULL);
	jr->target = target;
	jr->next = jumpreferences;
	jumpreferences = jr;
}


static void dseg_adddata(u1 *ptr)
{
	dataref *dr = DNEW(dataref);

	dr->pos = (u1 *) (ptr - mcodebase);
	dr->next = datareferences;
	datareferences = dr;
}


static void mcode_addreference(basicblock *target, void *branchptr)
{
	s4 branchpos = (u1*) branchptr - mcodebase;

	if (target->mpc >= 0) {
		gen_resolvebranch((u1*) mcodebase + branchpos, branchpos, target->mpc);
		}
	else {
		branchref *br = DNEW(branchref);

		br->branchpos = branchpos;
		br->next = target->branchrefs;
		target->branchrefs= br;
		}
}


static void mcode_addxboundrefs(void *branchptr)
{
	s4 branchpos = (u1*) branchptr - mcodebase;

	branchref *br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next = xboundrefs;
	xboundrefs = br;
}


static void mcode_addxcheckarefs(void *branchptr)
{
	s4 branchpos = (u1*) branchptr - mcodebase;

	branchref *br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next = xcheckarefs;
	xcheckarefs = br;
}


static void mcode_addxnullrefs(void *branchptr)
{
	s4 branchpos = (u1*) branchptr - mcodebase;

	branchref *br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next = xnullrefs;
	xnullrefs = br;
}


static void mcode_addxcastrefs(void *branchptr)
{
	s4 branchpos = (u1*) branchptr - mcodebase;

	branchref *br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next = xcastrefs;
	xcastrefs = br;
}


static void mcode_addxdivrefs(void *branchptr)
{
	s4 branchpos = (u1*) branchptr - mcodebase;

	branchref *br = DNEW(branchref);

	br->branchpos = branchpos;
	br->next = xdivrefs;
	xdivrefs = br;
}


static void mcode_finish(int mcodelen)
{
	jumpref *jr;
	u1 *epoint;

	count_code_len += mcodelen;
	count_data_len += dseglen;

	dseglen = ALIGN(dseglen, MAX_ALIGN);

	method -> mcodelength = mcodelen + dseglen;
	method -> mcode = CNEW(u1, mcodelen + dseglen);

	memcpy ( method->mcode, dsegtop - dseglen, dseglen);
	memcpy ( method->mcode + dseglen, mcodebase, mcodelen);

	method -> entrypoint = epoint = (u1*) (method->mcode + dseglen);

	/* jump table resolving */
	jr = jumpreferences;
	while (jr != NULL) {
	    *((void**) (epoint + jr->tablepos)) = epoint + jr->target->mpc;
	    jr = jr->next;
	    }

#if defined(__I386__) || defined(__X86_64__)
        {
            dataref *dr;
            /* add method into datastructure to find the entrypoint */
            (void) addmethod(method->entrypoint, method->entrypoint + mcodelen);
        
            /* data segment references resolving */
            dr = datareferences;
            while (dr != NULL) {
                *((void**) ((long) epoint + (long) dr->pos - POINTERSIZE)) = epoint;
                dr = dr->next;
            }
        }
#endif
}


static void dseg_display(s4 *s4ptr)
{
	int i;
	
	printf("  --- dump of datasegment\n");
	for (i = dseglen; i > 0 ; i -= 4) {
		printf("-%6x: %8x\n", i, (int)(*s4ptr++));
		}
	printf("  --- begin of data segment: %p\n", s4ptr);
}
