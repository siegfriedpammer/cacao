/* jit/reg.c *******************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	The register-manager.

	Authors:  Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1998/11/09

*******************************************************************************/

static varinfo5 *locals;
static varinfo5 *interfaces;

static int intregsnum;              /* absolute number of integer registers   */
static int floatregsnum;            /* absolute number of float registers     */ 

static int intreg_ret;              /* register to return integer values      */
static int intreg_argnum;           /* number of integer argument registers   */

static int floatreg_ret;            /* register for return float values       */
static int fltreg_argnum;           /* number of float argument registers     */


static int *argintregs;             /* scratch integer registers              */
static int *tmpintregs = NULL;      /* scratch integer registers              */
static int *savintregs;             /* saved integer registers                */
static int *argfltregs;             /* scratch float registers                */
static int *tmpfltregs;             /* scratch float registers                */
static int *savfltregs;             /* saved float registers                  */
static int *freetmpintregs;         /* free scratch integer registers         */
static int *freesavintregs;         /* free saved integer registers           */
static int *freetmpfltregs;         /* free scratch float registers           */
static int *freesavfltregs;         /* free saved float registers             */

static int *freemem;                /* free scratch memory                    */
static int memuse;                  /* used memory count                      */
static int ifmemuse;                /* interface used memory count            */
static int maxmemuse;               /* maximal used memory count (spills)     */
static int freememtop;              /* free memory count                      */

static int tmpintregcnt;            /* scratch integer register count         */
static int savintregcnt;            /* saved integer register count           */
static int tmpfltregcnt;            /* scratch float register count           */
static int savfltregcnt;            /* saved float register count             */

static int iftmpintregcnt;          /* iface scratch integer register count   */
static int ifsavintregcnt;          /* iface saved integer register count     */
static int iftmpfltregcnt;          /* iface scratch float register count     */
static int ifsavfltregcnt;          /* iface saved float register count       */

static int tmpintreguse;            /* used scratch integer register count    */
static int savintreguse;            /* used saved integer register count      */
static int tmpfltreguse;            /* used scratch float register count      */
static int savfltreguse;            /* used saved float register count        */

static int maxtmpintreguse;         /* max used scratch int register count    */
static int maxsavintreguse;         /* max used saved int register count      */
static int maxtmpfltreguse;         /* max used scratch float register count  */
static int maxsavfltreguse;         /* max used saved float register count    */

static int freetmpinttop;           /* free scratch integer register count    */
static int freesavinttop;           /* free saved integer register count      */
static int freetmpflttop;           /* free scratch float register count      */
static int freesavflttop;           /* free saved float register count        */

static int savedregs_num;      	/* total number of registers to be saved      */
static int arguments_num;       /* size of parameter field in the stackframe  */



/* function reg_init ***********************************************************

	initialises the register-allocator
	
*******************************************************************************/

static void reg_init()
{
	int n;
	
	if (!tmpintregs) {

		if (TYPE_INT != 0 || TYPE_ADR != 4) 
			panic ("JAVA-Basictypes have been changed");

		intreg_argnum = 0;
		tmpintregcnt = 0;
		savintregcnt = 0;

		for (intregsnum = 0; nregdescint[intregsnum] != REG_END; intregsnum++) {
			switch (nregdescint[intregsnum]) {
				case REG_SAV: savintregcnt++;
				              break;
				case REG_TMP: tmpintregcnt++;
				              break;
				case REG_ARG: intreg_argnum++;
				}
			}

		argintregs = MNEW (int, intreg_argnum);
		tmpintregs = MNEW (int, tmpintregcnt);
		savintregs = MNEW (int, savintregcnt);
		freetmpintregs = MNEW (int, tmpintregcnt);
		freesavintregs = MNEW (int, savintregcnt);

		intreg_argnum = 0;
		tmpintreguse = 0;
		savintreguse = 0;

   		for (n = 0; n < intregsnum; n++) {
			switch (nregdescint[n]) {
				case REG_RET: intreg_ret = n; 
				              break;
				case REG_SAV: savintregs[savintreguse++] = n;
				              break;
				case REG_TMP: tmpintregs[tmpintreguse++] = n;
				              break;
				case REG_ARG: argintregs[intreg_argnum++] = n;
				              break;
				}
			}
					
		
		fltreg_argnum = 0;
		tmpfltregcnt = 0;
		savfltregcnt = 0;

		for (floatregsnum = 0; nregdescfloat[floatregsnum] != REG_END; floatregsnum++) {
			switch (nregdescfloat[floatregsnum]) {
				case REG_SAV: savfltregcnt++;
				              break;
				case REG_TMP: tmpfltregcnt++;
				              break;
				case REG_ARG: fltreg_argnum++;
				              break;
				}
			}

		argfltregs = MNEW (int, fltreg_argnum);
		tmpfltregs = MNEW (int, tmpfltregcnt);
		savfltregs = MNEW (int, savfltregcnt);
		freetmpfltregs = MNEW (int, tmpfltregcnt);
		freesavfltregs = MNEW (int, savfltregcnt);

		fltreg_argnum = 0;
		tmpfltreguse = 0;
		savfltreguse = 0;

   		for (n = 0; n < floatregsnum; n++) {
			switch (nregdescfloat[n]) {
				case REG_RET: floatreg_ret = n; 
				              break;
				case REG_SAV: savfltregs[savfltreguse++] = n;
				              break;
				case REG_TMP: tmpfltregs[tmpfltreguse++] = n;
				              break;
				case REG_ARG: argfltregs[fltreg_argnum++] = n;
				              break;
				}
			}
					
		}

}


/* function reg_close **********************************************************

	releases all allocated space for registers

*******************************************************************************/

static void reg_close ()
{
	if (argintregs) MFREE (argintregs, int, intreg_argnum);
	if (argfltregs) MFREE (argfltregs, int, fltreg_argnum);
	if (tmpintregs) MFREE (tmpintregs, int, tmpintregcnt);
	if (savintregs) MFREE (savintregs, int, savintregcnt);
	if (tmpfltregs) MFREE (tmpfltregs, int, tmpfltregcnt);
	if (savfltregs) MFREE (savfltregs, int, savfltregcnt);

	if (freetmpintregs) MFREE (freetmpintregs, int, tmpintregcnt);
	if (freesavintregs) MFREE (freesavintregs, int, savintregcnt);
	if (freetmpfltregs) MFREE (freetmpfltregs, int, tmpfltregcnt);
	if (freesavfltregs) MFREE (freesavfltregs, int, savfltregcnt);
}


/* function local_init *********************************************************

	initialises the local variable and interfaces table
	
*******************************************************************************/

static void local_init()
{
	int i;
	varinfo5 *v;

	freemem    = DMNEW(int, maxstack);
	locals     = DMNEW(varinfo5, maxlocals);
	interfaces = DMNEW(varinfo5, maxstack);

	for (v = locals, i = maxlocals; i > 0; v++, i--) {
		v[0][TYPE_INT].type = -1;
		v[0][TYPE_LNG].type = -1;
		v[0][TYPE_FLT].type = -1;
		v[0][TYPE_DBL].type = -1;
		v[0][TYPE_ADR].type = -1;
		}

	for (v = interfaces, i = maxstack; i > 0; v++, i--) {
		v[0][TYPE_INT].type = -1;
		v[0][TYPE_INT].flags = 0;
		v[0][TYPE_LNG].type = -1;
		v[0][TYPE_LNG].flags = 0;
		v[0][TYPE_FLT].type = -1;
		v[0][TYPE_FLT].flags = 0;
		v[0][TYPE_DBL].type = -1;
		v[0][TYPE_DBL].flags = 0;
		v[0][TYPE_ADR].type = -1;
		v[0][TYPE_ADR].flags = 0;
		}
}


/* function interface_regalloc *************************************************

	allocates registers for all interface variables
	
*******************************************************************************/
	
static void interface_regalloc ()
{
	int     s, t, saved;
	int     intalloc, fltalloc;
	varinfo *v;
	
	/* allocate stack space for passing arguments to called methods */

	if (arguments_num > intreg_argnum)
		ifmemuse = arguments_num - intreg_argnum;
	else
		ifmemuse = 0;

	iftmpintregcnt = tmpintregcnt;
	ifsavintregcnt = savintregcnt;
	iftmpfltregcnt = tmpfltregcnt;
	ifsavfltregcnt = savfltregcnt;

	for (s = 0; s < maxstack; s++) {
		intalloc = -1; fltalloc = -1;
		saved = (interfaces[s][TYPE_INT].flags | interfaces[s][TYPE_LNG].flags |
		         interfaces[s][TYPE_FLT].flags | interfaces[s][TYPE_DBL].flags |
		         interfaces[s][TYPE_ADR].flags) & SAVEDVAR;
 
		for (t = TYPE_INT; t <= TYPE_ADR; t++) {
			v = &interfaces[s][t];
			if (v->type >= 0) {
				if (!saved) {
					if (IS_FLT_DBL_TYPE(t)) {
						if (fltalloc >= 0) {
							v->flags |= interfaces[s][fltalloc].flags & INMEMORY;
							v->regoff = interfaces[s][fltalloc].regoff;
							}
						else if (iftmpfltregcnt > 0) {
							iftmpfltregcnt--;
							v->regoff = tmpfltregs[iftmpfltregcnt];
							}
						else if (ifsavfltregcnt > 0) {
							ifsavfltregcnt--;
							v->regoff = savfltregs[ifsavfltregcnt];
							}
						else {
							v->flags |= INMEMORY;
							v->regoff = ifmemuse++;
							}
						fltalloc = t;
						}
					else {
						if (intalloc >= 0) {
							v->flags |= interfaces[s][intalloc].flags & INMEMORY;
							v->regoff = interfaces[s][intalloc].regoff;
							}
						else if (iftmpintregcnt > 0) {
							iftmpintregcnt--;
							v->regoff = tmpintregs[iftmpintregcnt];
							}
						else if (ifsavintregcnt > 0) {
							ifsavintregcnt--;
							v->regoff = savintregs[ifsavintregcnt];
							}
						else {
							v->flags |= INMEMORY;
							v->regoff = ifmemuse++;
							}
						intalloc = t;
						}
					}
				else {
					if (IS_FLT_DBL_TYPE(t)) {
						if (fltalloc >= 0) {
							v->flags |= interfaces[s][fltalloc].flags & INMEMORY;
							v->regoff = interfaces[s][fltalloc].regoff;
							}
						else if (ifsavfltregcnt > 0) {
							ifsavfltregcnt--;
							v->regoff = savfltregs[ifsavfltregcnt];
							}
						else {
							v->flags |= INMEMORY;
							v->regoff = ifmemuse++;
							}
						fltalloc = t;
						}
					else {
						if (intalloc >= 0) {
							v->flags |= interfaces[s][intalloc].flags & INMEMORY;
							v->regoff = interfaces[s][intalloc].regoff;
							}
						else if (ifsavintregcnt > 0) {
							ifsavintregcnt--;
							v->regoff = savintregs[ifsavintregcnt];
							}
						else {
							v->flags |= INMEMORY;
							v->regoff = ifmemuse++;
							}
						intalloc = t;
						}
					}
				} /* if (type >= 0) */
			}     /* for t */
		}         /* for s */

	maxmemuse = ifmemuse;
	maxtmpintreguse = iftmpintregcnt;
	maxsavintreguse = ifsavintregcnt;
	maxtmpfltreguse = iftmpfltregcnt;
	maxsavfltreguse = ifsavfltregcnt;

}


/* function local_regalloc *****************************************************

	allocates registers for all local variables
	
*******************************************************************************/
	
static void local_regalloc ()
{
	int      s, t;
	int      intalloc, fltalloc;
	varinfo *v;
	
	if (isleafmethod) {
		int arg, doublewordarg;
		arg = 0;
		doublewordarg = 0;
		for (s = 0; s < maxlocals; s++) {
			intalloc = -1; fltalloc = -1;
			for (t = TYPE_INT; t <= TYPE_ADR; t++) {
				v = &locals[s][t];
				if (v->type >= 0) {
					if (IS_FLT_DBL_TYPE(t)) {
						if (fltalloc >= 0) {
							v->flags = locals[s][fltalloc].flags;
							v->regoff = locals[s][fltalloc].regoff;
							}
						else if (!doublewordarg && (arg < mparamcount)
						                        && (arg < fltreg_argnum)) {
							v->flags = 0;
							v->regoff = argfltregs[arg];
							}
						else if (maxtmpfltreguse > 0) {
							maxtmpfltreguse--;
							v->flags = 0;
							v->regoff = tmpfltregs[maxtmpfltreguse];
							}
						else if (maxsavfltreguse > 0) {
							maxsavfltreguse--;
							v->flags = 0;
							v->regoff = savfltregs[maxsavfltreguse];
							}
						else {
							v->flags = INMEMORY;
							v->regoff = maxmemuse++;
							}
						fltalloc = t;
						}
					else {
						if (intalloc >= 0) {
							v->flags = locals[s][intalloc].flags;
							v->regoff = locals[s][intalloc].regoff;
							}
						else if (!doublewordarg && (arg < mparamcount)
						                        && (arg < intreg_argnum)) {
							v->flags = 0;
							v->regoff = argintregs[arg];
							}
						else if (maxtmpintreguse > 0) {
							maxtmpintreguse--;
							v->flags = 0;
							v->regoff = tmpintregs[maxtmpintreguse];
							}
						else if (maxsavintreguse > 0) {
							maxsavintreguse--;
							v->flags = 0;
							v->regoff = savintregs[maxsavintreguse];
							}
						else {
							v->flags = INMEMORY;
							v->regoff = maxmemuse++;
							}
						intalloc = t;
						}
					}
				}
			if (arg < mparamcount) {
				if (doublewordarg) {
					doublewordarg = 0;
					arg++;
					}
				else if (IS_2_WORD_TYPE(mparamtypes[arg]))
					doublewordarg = 1;
				else
					arg++;
				}
			}
		return;
		}
	for (s = 0; s < maxlocals; s++) {
		intalloc = -1; fltalloc = -1;
		for (t=TYPE_INT; t<=TYPE_ADR; t++) {
			v = &locals[s][t];
			if (v->type >= 0) {
				if (IS_FLT_DBL_TYPE(t)) {
					if (fltalloc >= 0) {
						v->flags = locals[s][fltalloc].flags;
						v->regoff = locals[s][fltalloc].regoff;
						}
					else if (maxsavfltreguse > 0) {
						maxsavfltreguse--;
						v->flags = 0;
						v->regoff = savfltregs[maxsavfltreguse];
						}
					else {
						v->flags = INMEMORY;
						v->regoff = maxmemuse++;
						}
					fltalloc = t;
					}
				else {
					if (intalloc >= 0) {
						v->flags = locals[s][intalloc].flags;
						v->regoff = locals[s][intalloc].regoff;
						}
					else if (maxsavintreguse > 0) {
						maxsavintreguse--;
						v->flags = 0;
						v->regoff = savintregs[maxsavintreguse];
						}
					else {
						v->flags = INMEMORY;
						v->regoff = maxmemuse++;
						}
					intalloc = t;
					}
				}
			}
		}
}


static void reg_init_temp()
{
	freememtop = 0;
	memuse = ifmemuse;

	freetmpinttop = 0;
	freesavinttop = 0;
	freetmpflttop = 0;
	freesavflttop = 0;
	tmpintreguse = iftmpintregcnt;
	savintreguse = ifsavintregcnt;
	tmpfltreguse = iftmpfltregcnt;
	savfltreguse = ifsavfltregcnt;
}


#define reg_new_temp(s) if(s->varkind==TEMPVAR)reg_new_temp_func(s)

static void reg_new_temp_func(stackptr s)
{
if (s->flags & SAVEDVAR) {
	if (IS_FLT_DBL_TYPE(s->type)) {
		if (freesavflttop > 0) {
			freesavflttop--;
			s->regoff = freesavfltregs[freesavflttop];
			return;
			}
		else if (savfltreguse > 0) {
			savfltreguse--;
			if (savfltreguse < maxsavfltreguse)
				maxsavfltreguse = savfltreguse;
			s->regoff = savfltregs[savfltreguse];
			return;
			}
		}
	else {
		if (freesavinttop > 0) {
			freesavinttop--;
			s->regoff = freesavintregs[freesavinttop];
			return;
			}
		else if (savintreguse > 0) {
			savintreguse--;
			if (savintreguse < maxsavintreguse)
				maxsavintreguse = savintreguse;
			s->regoff = savintregs[savintreguse];
			return;
			}
		}
	}
else {
	if (IS_FLT_DBL_TYPE(s->type)) {
		if (freetmpflttop > 0) {
			freetmpflttop--;
			s->regoff = freetmpfltregs[freetmpflttop];
			return;
			}
		else if (tmpfltreguse > 0) {
			tmpfltreguse--;
			if (tmpfltreguse < maxtmpfltreguse)
				maxtmpfltreguse = tmpfltreguse;
			s->regoff = tmpfltregs[tmpfltreguse];
			return;
			}
		}
	else {
		if (freetmpinttop > 0) {
			freetmpinttop--;
			s->regoff = freetmpintregs[freetmpinttop];
			return;
			}
		else if (tmpintreguse > 0) {
			tmpintreguse--;
			if (tmpintreguse < maxtmpintreguse)
				maxtmpintreguse = tmpintreguse;
			s->regoff = tmpintregs[tmpintreguse];
			return;
			}
		}
	}
if (freememtop > 0) {
	freememtop--;
	s->regoff = freemem[freememtop];
	}
else {
	s->regoff = memuse++;
	if (memuse > maxmemuse)
		maxmemuse = memuse;
	}
s->flags |= INMEMORY;
}


#define reg_free_temp(s) if(s->varkind==TEMPVAR)reg_free_temp_func(s)

static void reg_free_temp_func(stackptr s)
{
if (s->flags & INMEMORY)
	freemem[freememtop++] = s->regoff;
else if (IS_FLT_DBL_TYPE(s->type)) {
	if (s->flags & SAVEDVAR)
		freesavfltregs[freesavflttop++] = s->regoff;
	else
		freetmpfltregs[freetmpflttop++] = s->regoff;
	}
else
	if (s->flags & SAVEDVAR)
		freesavintregs[freesavinttop++] = s->regoff;
	else
		freetmpintregs[freetmpinttop++] = s->regoff;
}


static void allocate_scratch_registers()
{
	int b_count;
	int opcode, i, len;
	stackptr    src, dst;
	instruction *iptr = instr;
	basicblock  *bptr;
	
	/* b_count = block_count; */

	bptr = block;
	while (bptr != NULL) {

		if (bptr->flags >= BBREACHED) {
			dst = bptr->instack;
			reg_init_temp();
			iptr = bptr->iinstr;
			len = bptr->icount;
  
			while (--len >= 0)  {
				src = dst;
				dst = iptr->dst;
				opcode = iptr->opc;

				switch (opcode) {

					/* pop 0 push 0 */

					case ICMD_NOP:
					case ICMD_ELSE_ICONST:
					case ICMD_CHECKASIZE:
					case ICMD_IINC:
					case ICMD_JSR:
					case ICMD_RET:
					case ICMD_RETURN:
					case ICMD_GOTO:
						break;

					/* pop 0 push 1 const */
					
					case ICMD_ICONST:
					case ICMD_LCONST:
					case ICMD_FCONST:
					case ICMD_DCONST:
					case ICMD_ACONST:

					/* pop 0 push 1 load */
					
					case ICMD_ILOAD:
					case ICMD_LLOAD:
					case ICMD_FLOAD:
					case ICMD_DLOAD:
					case ICMD_ALOAD:
						reg_new_temp(dst);
						break;

					/* pop 2 push 1 */

					case ICMD_IALOAD:
					case ICMD_LALOAD:
					case ICMD_FALOAD:
					case ICMD_DALOAD:
					case ICMD_AALOAD:

					case ICMD_BALOAD:
					case ICMD_CALOAD:
					case ICMD_SALOAD:

						reg_free_temp(src);
						reg_free_temp(src->prev);
						reg_new_temp(dst);
						break;

					/* pop 3 push 0 */

					case ICMD_IASTORE:
					case ICMD_LASTORE:
					case ICMD_FASTORE:
					case ICMD_DASTORE:
					case ICMD_AASTORE:

					case ICMD_BASTORE:
					case ICMD_CASTORE:
					case ICMD_SASTORE:

						reg_free_temp(src);
						reg_free_temp(src->prev);
						reg_free_temp(src->prev->prev);
						break;

					/* pop 1 push 0 store */

					case ICMD_ISTORE:
					case ICMD_LSTORE:
					case ICMD_FSTORE:
					case ICMD_DSTORE:
					case ICMD_ASTORE:

					/* pop 1 push 0 */

					case ICMD_POP:

					case ICMD_IRETURN:
					case ICMD_LRETURN:
					case ICMD_FRETURN:
					case ICMD_DRETURN:
					case ICMD_ARETURN:

					case ICMD_ATHROW:

					case ICMD_PUTSTATIC:

					/* pop 1 push 0 branch */

					case ICMD_IFNULL:
					case ICMD_IFNONNULL:

					case ICMD_IFEQ:
					case ICMD_IFNE:
					case ICMD_IFLT:
					case ICMD_IFGE:
					case ICMD_IFGT:
					case ICMD_IFLE:

					case ICMD_IF_LEQ:
					case ICMD_IF_LNE:
					case ICMD_IF_LLT:
					case ICMD_IF_LGE:
					case ICMD_IF_LGT:
					case ICMD_IF_LLE:

					/* pop 1 push 0 table branch */

					case ICMD_TABLESWITCH:
					case ICMD_LOOKUPSWITCH:

					case ICMD_NULLCHECKPOP:
					case ICMD_MONITORENTER:
					case ICMD_MONITOREXIT:
						reg_free_temp(src);
						break;

					/* pop 2 push 0 branch */

					case ICMD_IF_ICMPEQ:
					case ICMD_IF_ICMPNE:
					case ICMD_IF_ICMPLT:
					case ICMD_IF_ICMPGE:
					case ICMD_IF_ICMPGT:
					case ICMD_IF_ICMPLE:

					case ICMD_IF_LCMPEQ:
					case ICMD_IF_LCMPNE:
					case ICMD_IF_LCMPLT:
					case ICMD_IF_LCMPGE:
					case ICMD_IF_LCMPGT:
					case ICMD_IF_LCMPLE:

					case ICMD_IF_ACMPEQ:
					case ICMD_IF_ACMPNE:

					/* pop 2 push 0 */

					case ICMD_POP2:

					case ICMD_PUTFIELD:
						reg_free_temp(src);
						reg_free_temp(src->prev);
						break;

					/* pop 0 push 1 dup */
					
					case ICMD_DUP:
						reg_new_temp(dst);
						break;

					/* pop 0 push 2 dup */
					
					case ICMD_DUP2:
						reg_new_temp(dst->prev);
						reg_new_temp(dst);
						break;

					/* pop 2 push 3 dup */
					
					case ICMD_DUP_X1:
						reg_new_temp(dst->prev->prev);
						reg_new_temp(dst->prev);
						reg_new_temp(dst);
						reg_free_temp(src);
						reg_free_temp(src->prev);
						break;

					/* pop 3 push 4 dup */
					
					case ICMD_DUP_X2:
						reg_new_temp(dst->prev->prev->prev);
						reg_new_temp(dst->prev->prev);
						reg_new_temp(dst->prev);
						reg_new_temp(dst);
						reg_free_temp(src);
						reg_free_temp(src->prev);
						reg_free_temp(src->prev->prev);
						break;

					/* pop 3 push 5 dup */
					
					case ICMD_DUP2_X1:
						reg_new_temp(dst->prev->prev->prev->prev);
						reg_new_temp(dst->prev->prev->prev);
						reg_new_temp(dst->prev->prev);
						reg_new_temp(dst->prev);
						reg_new_temp(dst);
						reg_free_temp(src);
						reg_free_temp(src->prev);
						reg_free_temp(src->prev->prev);
						break;

					/* pop 4 push 6 dup */
					
					case ICMD_DUP2_X2:
						reg_new_temp(dst->prev->prev->prev->prev->prev);
						reg_new_temp(dst->prev->prev->prev->prev);
						reg_new_temp(dst->prev->prev->prev);
						reg_new_temp(dst->prev->prev);
						reg_new_temp(dst->prev);
						reg_new_temp(dst);
						reg_free_temp(src);
						reg_free_temp(src->prev);
						reg_free_temp(src->prev->prev);
						reg_free_temp(src->prev->prev->prev);
						break;

					/* pop 2 push 2 swap */
					
					case ICMD_SWAP:
						reg_new_temp(dst->prev);
						reg_new_temp(dst);
						reg_free_temp(src);
						reg_free_temp(src->prev);
						break;

					/* pop 2 push 1 */
					
					case ICMD_IADD:
					case ICMD_ISUB:
					case ICMD_IMUL:
					case ICMD_IDIV:
					case ICMD_IREM:

					case ICMD_ISHL:
					case ICMD_ISHR:
					case ICMD_IUSHR:
					case ICMD_IAND:
					case ICMD_IOR:
					case ICMD_IXOR:

					case ICMD_LADD:
					case ICMD_LSUB:
					case ICMD_LMUL:
					case ICMD_LDIV:
					case ICMD_LREM:

					case ICMD_LOR:
					case ICMD_LAND:
					case ICMD_LXOR:

					case ICMD_LSHL:
					case ICMD_LSHR:
					case ICMD_LUSHR:

					case ICMD_FADD:
					case ICMD_FSUB:
					case ICMD_FMUL:
					case ICMD_FDIV:
					case ICMD_FREM:

					case ICMD_DADD:
					case ICMD_DSUB:
					case ICMD_DMUL:
					case ICMD_DDIV:
					case ICMD_DREM:

					case ICMD_LCMP:
					case ICMD_FCMPL:
					case ICMD_FCMPG:
					case ICMD_DCMPL:
					case ICMD_DCMPG:
						reg_free_temp(src);
						reg_free_temp(src->prev);
						reg_new_temp(dst);
						break;

					/* pop 1 push 1 */
					
					case ICMD_IADDCONST:
					case ICMD_ISUBCONST:
					case ICMD_IMULCONST:
					case ICMD_IDIVPOW2:
					case ICMD_IREMPOW2:
					case ICMD_IREM0X10001:
					case ICMD_IANDCONST:
					case ICMD_IORCONST:
					case ICMD_IXORCONST:
					case ICMD_ISHLCONST:
					case ICMD_ISHRCONST:
					case ICMD_IUSHRCONST:

					case ICMD_LADDCONST:
					case ICMD_LSUBCONST:
					case ICMD_LMULCONST:
					case ICMD_LDIVPOW2:
					case ICMD_LREMPOW2:
					case ICMD_LREM0X10001:
					case ICMD_LANDCONST:
					case ICMD_LORCONST:
					case ICMD_LXORCONST:
					case ICMD_LSHLCONST:
					case ICMD_LSHRCONST:
					case ICMD_LUSHRCONST:

					case ICMD_IFEQ_ICONST:
					case ICMD_IFNE_ICONST:
					case ICMD_IFLT_ICONST:
					case ICMD_IFGE_ICONST:
					case ICMD_IFGT_ICONST:
					case ICMD_IFLE_ICONST:

					case ICMD_INEG:
					case ICMD_INT2BYTE:
					case ICMD_INT2CHAR:
					case ICMD_INT2SHORT:
					case ICMD_LNEG:
					case ICMD_FNEG:
					case ICMD_DNEG:

					case ICMD_I2L:
					case ICMD_I2F:
					case ICMD_I2D:
					case ICMD_L2I:
					case ICMD_L2F:
					case ICMD_L2D:
					case ICMD_F2I:
					case ICMD_F2L:
					case ICMD_F2D:
					case ICMD_D2I:
					case ICMD_D2L:
					case ICMD_D2F:

					case ICMD_CHECKCAST:

					case ICMD_ARRAYLENGTH:
					case ICMD_INSTANCEOF:

					case ICMD_NEWARRAY:
					case ICMD_ANEWARRAY:

					case ICMD_GETFIELD:
						reg_free_temp(src);
						reg_new_temp(dst);
						break;

					/* pop 0 push 1 */
					
					case ICMD_GETSTATIC:

					case ICMD_NEW:

						reg_new_temp(dst);
						break;

					/* pop many push any */
					
					case ICMD_INVOKEVIRTUAL:
					case ICMD_INVOKESPECIAL:
					case ICMD_INVOKESTATIC:
					case ICMD_INVOKEINTERFACE:
						{
						i = iptr->op1;
						while (--i >= 0) {
							reg_free_temp(src);
							src = src->prev;
							}
						if (((methodinfo*)iptr->val.a)->returntype != TYPE_VOID)
							reg_new_temp(dst);
						break;
						}

					case ICMD_BUILTIN3:
						reg_free_temp(src);
						src = src->prev;
					case ICMD_BUILTIN2:
						reg_free_temp(src);
						src = src->prev;
					case ICMD_BUILTIN1:
						reg_free_temp(src);
						src = src->prev;
						if (iptr->op1 != TYPE_VOID)
							reg_new_temp(dst);
						break;

					case ICMD_MULTIANEWARRAY:
						i = iptr->op1;
						while (--i >= 0) {
							reg_free_temp(src);
							src = src->prev;
							}
						reg_new_temp(dst);
						break;

					default:
						printf("ICMD %d at %d\n", iptr->opc, (int)(iptr-instr));
						panic("Missing ICMD code during register allocation");
					} /* switch */
				iptr++;
				} /* while instructions */
			} /* if */
		bptr = bptr->next;
	} /* while blocks */
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
