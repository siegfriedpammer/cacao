/***************************** comp/mcode.c ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	This file is an include file for "compiler.c" . It contains (mostly)
	architecture independent functions for writing instructions into the
	code area and constants into the data area.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	Changes: Micheal Gschwind    EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/04/13


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

#include <sys/mman.h>
#include <errno.h>

#define MCODEINITSIZE (1<<15)       /* 32 Kbyte code area initialization size */
#define DSEGINITSIZE  (1<<12)       /*  4 Kbyte data area initialization size */

static u1* mcodebase = NULL;        /* base pointer of code area              */
static int mcodesize;               /* complete size of code area (bytes)     */
static int mcodelen;                /* used size of code area (bytes)         */

static u1* dsegtop = NULL;          /* pointer to top (end) of data area      */
static int dsegsize;                /* complete size of data area (bytes)     */
static int dseglen;                 /* used size of data area (bytes)         */
                                    /* data area grows from top to bottom     */

static list mcodereferences;        /* list of branch instruction adresses    */
                                    /* and of jumptable target addresses      */

static void mcode_init();           /* allocates code and data area           */
static void mcode_close();          /* releases temporary storage             */
static void mcode_finish();         /* makes code and data area permanent and */
                                    /* updates branch references to code/data */
static void mcode_adds4(s4 code);   /* adds an instruction to code area       */

static s4 dseg_adds4(s4 value);         /* adds an int to data area           */
static s4 dseg_adds8(s8 value);         /* adds an long to data area          */
static s4 dseg_addfloat (float value);  /* adds an float to data area         */
static s4 dseg_adddouble(double value); /* adds an double to data area        */

/*     s4 dseg_addaddress(void* value); */
static s4 dseg_addtarget(basicblock *target);
static void mcode_addreference(basicblock *target);
static void dseg_display();

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

	mcodelen = 0;
	dseglen = 0;

	list_init (&mcodereferences, OFFSET(mcodereference, linkage) );
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


/* mcode_adds4_increase doubles code area and adds instruction                */

static void mcode_adds4_increase(s4 code)
{
	mcodebase = MREALLOC(mcodebase, u1, mcodesize, mcodesize * 2);
	mcodesize *= 2;
	*((s4 *) (mcodebase + mcodelen - 4)) = code;
}


/* mcode_adds4 checks code area size and adds instruction                     */

static void mcode_adds4(s4 code)
{
	s4 *codeptr;

	codeptr = (s4 *) (mcodebase + mcodelen);
	mcodelen += 4;
	if (mcodelen <= mcodesize)
		*codeptr = code;
	else
		mcode_adds4_increase(code);
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


#if POINTERSIZE==8
#define dseg_addaddress(value)      dseg_adds8((s8)(value))
#else
#define dseg_addaddress(value)      dseg_adds4((s4)(value))
#endif


static s4 dseg_addtarget(basicblock *target)
{
	mcodereference *cr = DNEW(mcodereference);

	dseglen = ALIGN (dseglen + sizeof(void*), sizeof(void*));
	if (dseglen > dsegsize)
		dseg_increase();

	cr -> incode = false;
	cr -> msourcepos = -dseglen;
	cr -> target = target;
	
	list_addlast (&mcodereferences, cr);

	return -dseglen;
}


static void mcode_addreference(basicblock *target)
{
	mcodereference *cr = DNEW(mcodereference);
	
	cr -> incode = true;
	cr -> msourcepos = mcodelen;
	cr -> target = target;
	
	list_addlast (&mcodereferences, cr);
}



static void mcode_blockstart (basicblock *b)
{
	b -> mpc = mcodelen;
}



static void gen_resolvebranch(void* mcodepiece, s4 sourcepos, s4 targetpos);

static void mcode_finish()
{
	mcodereference *cr;

	count_code_len += mcodelen;
	count_data_len += dseglen;

	dseglen = ALIGN(dseglen, MAX_ALIGN);

	method->mcodelength = mcodelen + dseglen;
	method->mcode = CNEW(u1, mcodelen + dseglen);

	memcpy ( method->mcode, dsegtop - dseglen, dseglen);
	memcpy ( method->mcode + dseglen, mcodebase, mcodelen);

	method -> entrypoint = (u1*) (method->mcode + dseglen);

		cr = list_first (&mcodereferences);
	while (cr != NULL) {

		if (cr->incode) {  /* branch resolving */
			gen_resolvebranch ( ((u1*)(method->entrypoint)) + cr->msourcepos, 
		    	            cr->msourcepos,
		        	        cr->target->mpc);
			}
		else {             /* jump table resolving */
			void **p;
			p = (void**) ( ((u1*)(method->entrypoint)) + cr->msourcepos);

			*p = ((u1*)(method->entrypoint)) + cr->target->mpc;
			}

		cr = list_next (&mcodereferences, cr);
		}

#ifdef CACHE_FLUSH_BLOCK
	synchronize_caches(method->mcode, (mcodelen>>2));
#endif
	
}


static void dseg_display()
{
	int i;
	printf ("  --- dump of datasegment\n");
	for (i = dseglen - 4; i >= 0 ; i -= 4) {
		printf ("%6x: %8x\n", i, (int)(*((s4*)(dsegtop - i))));
		}
}
