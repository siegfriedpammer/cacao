/* jit/reg.inc - register allocator

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Andreas Krall

   Changes: Stefan Ring
            Christian Thalinger

   $Id: reg.inc 1459 2004-11-05 16:23:02Z twisti $

*/


#include "jit/reg.h"
#include "toolbox/memory.h"


/* function prototypes for this file */

static void interface_regalloc(methodinfo *m, codegendata *cd, registerdata *rd);
static void local_regalloc(methodinfo *m, codegendata *cd, registerdata *rd);
static void allocate_scratch_registers(methodinfo *m, registerdata *rd);


/* reg_init ********************************************************************

   TODO
	
*******************************************************************************/

void reg_init()
{
	/* void */
}


/* reg_setup *******************************************************************

   TODO

*******************************************************************************/

void reg_setup(methodinfo *m, registerdata *rd, t_inlining_globals *id)
{
	s4 i;
	varinfo5 *v;

	/* setup the integer register table */
	rd->intreg_argnum = 0;
	rd->tmpintregcnt = 0;
	rd->savintregcnt = 0;

	for (rd->intregsnum = 0; nregdescint[rd->intregsnum] != REG_END; rd->intregsnum++) {
		switch (nregdescint[rd->intregsnum]) {
		case REG_SAV:
			rd->savintregcnt++;
			break;
		case REG_TMP:
			rd->tmpintregcnt++;
			break;
		case REG_ARG:
			rd->intreg_argnum++;
			break;
		}
	}

	rd->argintregs = MNEW(s4, rd->intreg_argnum);
	rd->tmpintregs = MNEW(s4, rd->tmpintregcnt);
	rd->savintregs = MNEW(s4, rd->savintregcnt);
	rd->freeargintregs = MNEW(s4, rd->intreg_argnum);
	rd->freetmpintregs = MNEW(s4, rd->tmpintregcnt);
	rd->freesavintregs = MNEW(s4, rd->savintregcnt);
#ifdef USETWOREGS
	rd->secondregs = MNEW(s4, rd->intregsnum);
#endif

	rd->intreg_argnum = 0;
	rd->argintreguse = 0;
	rd->tmpintreguse = 0;
	rd->savintreguse = 0;

	for (i = 0; i < rd->intregsnum; i++) {
		switch (nregdescint[i]) {
		case REG_RET:
			rd->intreg_ret = i; 
			break;
		case REG_SAV:
			rd->savintregs[rd->savintreguse++] = i;
			break;
		case REG_TMP:
			rd->tmpintregs[rd->tmpintreguse++] = i;
			break;
		case REG_ARG:
			rd->argintregs[rd->intreg_argnum++] = i;
			rd->argintreguse++;
			break;
		}
	}

#if defined(__X86_64__)
	/* 
	 * on x86_64 the argument registers are not in ascending order 
	 * a00 (%rdi) <-> a03 (%rcx) and a01 (%rsi) <-> a02 (%rdx)
	 */
	i = rd->argintregs[3];
	rd->argintregs[3] = rd->argintregs[0];
	rd->argintregs[0] = i;

	i = rd->argintregs[2];
	rd->argintregs[2] = rd->argintregs[1];
	rd->argintregs[1] = i;
#endif
		
#ifdef USETWOREGS
	for (i = 1; i < rd->intreg_argnum; i++)
		rd->secondregs[rd->argintregs[i - 1]] = rd->argintregs[i];
	for (i = 1; i < rd->tmpintregcnt; i++)
		rd->secondregs[rd->tmpintregs[i - 1]] = rd->tmpintregs[i];
	for (i = 1; i < rd->savintregcnt; i++)
		rd->secondregs[rd->savintregs[i - 1]] = rd->savintregs[i];

	rd->secondregs[REG_ITMP1] = REG_ITMP2;
	rd->secondregs[REG_ITMP3] = REG_ITMP2;
	rd->secondregs[REG_RESULT] = REG_RESULT + 1;
	rd->secondregs[rd->argintregs[rd->intreg_argnum - 1]] = REG_ITMP3;
#endif

	/* setup the float register table */
	rd->fltreg_argnum = 0;
	rd->tmpfltregcnt = 0;
	rd->savfltregcnt = 0;

	for (rd->floatregsnum = 0; nregdescfloat[rd->floatregsnum] != REG_END; rd->floatregsnum++) {
		switch (nregdescfloat[rd->floatregsnum]) {
		case REG_SAV:
			rd->savfltregcnt++;
			break;
		case REG_TMP:
			rd->tmpfltregcnt++;
			break;
		case REG_ARG:
			rd->fltreg_argnum++;
			break;
		}
	}

	rd->argfltregs = MNEW(s4, rd->fltreg_argnum);
	rd->tmpfltregs = MNEW(s4, rd->tmpfltregcnt);
	rd->savfltregs = MNEW(s4, rd->savfltregcnt);
	rd->freeargfltregs = MNEW(s4, rd->fltreg_argnum);
	rd->freetmpfltregs = MNEW(s4, rd->tmpfltregcnt);
	rd->freesavfltregs = MNEW(s4, rd->savfltregcnt);

	rd->fltreg_argnum = 0;
	rd->argfltreguse = 0;
	rd->tmpfltreguse = 0;
	rd->savfltreguse = 0;

	for (i = 0; i < rd->floatregsnum; i++) {
		switch (nregdescfloat[i]) {
		case REG_RET:
			rd->floatreg_ret = i; 
			break;
		case REG_SAV:
			rd->savfltregs[rd->savfltreguse++] = i;
			break;
		case REG_TMP:
			rd->tmpfltregs[rd->tmpfltreguse++] = i;
			break;
		case REG_ARG:
			rd->argfltregs[rd->fltreg_argnum++] = i;
			rd->argfltreguse++;
			break;
		}
	}


	rd->freemem    = MNEW(s4, id->cummaxstack);
	rd->locals     = MNEW(varinfo5, id->cumlocals);
	rd->interfaces = MNEW(varinfo5, id->cummaxstack);

	for (v = rd->locals, i = id->cumlocals; i > 0; v++, i--) {
		v[0][TYPE_INT].type = -1;
		v[0][TYPE_LNG].type = -1;
		v[0][TYPE_FLT].type = -1;
		v[0][TYPE_DBL].type = -1;
		v[0][TYPE_ADR].type = -1;
	}

	for (v = rd->interfaces, i = id->cummaxstack; i > 0; v++, i--) {
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


/* function reg_free ***********************************************************

   releases all allocated space for registers

*******************************************************************************/

void reg_free(methodinfo *m, registerdata *rd)
{
	if (rd->argintregs) MFREE(rd->argintregs, s4, rd->intreg_argnum);
	if (rd->argfltregs) MFREE(rd->argfltregs, s4, rd->fltreg_argnum);
	if (rd->tmpintregs) MFREE(rd->tmpintregs, s4, rd->tmpintregcnt);
	if (rd->savintregs) MFREE(rd->savintregs, s4, rd->savintregcnt);
	if (rd->tmpfltregs) MFREE(rd->tmpfltregs, s4, rd->tmpfltregcnt);
	if (rd->savfltregs) MFREE(rd->savfltregs, s4, rd->savfltregcnt);

	if (rd->freeargintregs) MFREE(rd->freeargintregs, s4, rd->intreg_argnum);
	if (rd->freeargfltregs) MFREE(rd->freeargfltregs, s4, rd->fltreg_argnum);
	if (rd->freetmpintregs) MFREE(rd->freetmpintregs, s4, rd->tmpintregcnt);
	if (rd->freesavintregs) MFREE(rd->freesavintregs, s4, rd->savintregcnt);
	if (rd->freetmpfltregs) MFREE(rd->freetmpfltregs, s4, rd->tmpfltregcnt);
	if (rd->freesavfltregs) MFREE(rd->freesavfltregs, s4, rd->savfltregcnt);

#ifdef USETWOREGS
	if (rd->secondregs) MFREE(rd->secondregs, s4, rd->intregsnum);
#endif

	if (rd->freemem) MFREE(rd->freemem, s4, m->maxstack);
	if (rd->locals)  MFREE(rd->locals, varinfo5, m->maxlocals);
	if (rd->interfaces) MFREE(rd->interfaces, varinfo5, m->maxstack);
}


/* reg_close *******************************************************************

   TODO

*******************************************************************************/

void reg_close()
{
	/* void */
}


/* function interface_regalloc *************************************************

	allocates registers for all interface variables
	
*******************************************************************************/
	
void regalloc(methodinfo *m, codegendata *cd, registerdata *rd)
{
	interface_regalloc(m, cd, rd);
	allocate_scratch_registers(m, rd);
	local_regalloc(m, cd, rd);
}


/* function interface_regalloc *************************************************

	allocates registers for all interface variables
	
*******************************************************************************/
	
static void interface_regalloc(methodinfo *m, codegendata *cd, registerdata *rd)
{
	int     s, t, saved;
	int     intalloc, fltalloc;
	varinfo *v;
	int		regsneeded = 0;

	/* allocate stack space for passing arguments to called methods */

#ifndef SPECIALMEMUSE
#if defined(__X86_64__)
	/*
	 * XXX: we have a problem here, but allocating a little more stack space
	 *      is better than having a bug
	 */
	/*  	if (arguments_num > (intreg_argnum + fltreg_argnum)) */
	/*  		ifmemuse = arguments_num - (intreg_argnum + fltreg_argnum); */
	if (rd->arguments_num > rd->fltreg_argnum)
		rd->ifmemuse = rd->arguments_num - rd->fltreg_argnum;
#else
	if (rd->arguments_num > rd->intreg_argnum)
		rd->ifmemuse = rd->arguments_num - rd->intreg_argnum;
#endif
	else
		rd->ifmemuse = 0;
#endif

	rd->iftmpintregcnt = rd->tmpintregcnt;
	rd->ifsavintregcnt = rd->savintregcnt;
	rd->iftmpfltregcnt = rd->tmpfltregcnt;
	rd->ifsavfltregcnt = rd->savfltregcnt;

	for (s = 0; s < cd->maxstack; s++) {
		intalloc = -1; fltalloc = -1;
		saved = (rd->interfaces[s][TYPE_INT].flags |
				 rd->interfaces[s][TYPE_LNG].flags |
		         rd->interfaces[s][TYPE_FLT].flags |
				 rd->interfaces[s][TYPE_DBL].flags |
		         rd->interfaces[s][TYPE_ADR].flags) & SAVEDVAR;
 
		for (t = TYPE_INT; t <= TYPE_ADR; t++) {
			v = &rd->interfaces[s][t];
			if (v->type >= 0) {
#ifdef USETWOREGS
				regsneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
				if (!saved) {
					if (IS_FLT_DBL_TYPE(t)) {
						if (fltalloc >= 0) {
							v->flags |= rd->interfaces[s][fltalloc].flags & INMEMORY;
							v->regoff = rd->interfaces[s][fltalloc].regoff;
						}
						else if (rd->iftmpfltregcnt > 0) {
							rd->iftmpfltregcnt--;
							v->regoff = rd->tmpfltregs[rd->iftmpfltregcnt];
						}
						else if (rd->ifsavfltregcnt > 0) {
							rd->ifsavfltregcnt--;
							v->regoff = rd->savfltregs[rd->ifsavfltregcnt];
						}
						else {
							v->flags |= INMEMORY;
							v->regoff = rd->ifmemuse;
							rd->ifmemuse += regsneeded + 1;
						}
						fltalloc = t;
					}
					else {
#if defined(__I386__)
						/*
						 * for i386 put all longs in memory
						 */
						if (IS_2_WORD_TYPE(t)) {
							v->flags |= INMEMORY;
							v->regoff = rd->ifmemuse++;
						} else {
#endif
							if (intalloc >= 0) {
								v->flags |= rd->interfaces[s][intalloc].flags & INMEMORY;
								v->regoff = rd->interfaces[s][intalloc].regoff;
							}
							else if (rd->iftmpintregcnt > regsneeded) {
								rd->iftmpintregcnt -= regsneeded + 1;
								v->regoff = rd->tmpintregs[rd->iftmpintregcnt];
							}
							else if (rd->ifsavintregcnt > regsneeded) {
								rd->ifsavintregcnt -= regsneeded + 1;
								v->regoff = rd->savintregs[rd->ifsavintregcnt];
							}
							else {
								v->flags |= INMEMORY;
								v->regoff = rd->ifmemuse;
								rd->ifmemuse += regsneeded + 1;
							}
#if defined(__I386__)
						}
#endif
						intalloc = t;
					}
				}
				else {
					if (IS_FLT_DBL_TYPE(t)) {
						if (fltalloc >= 0) {
							v->flags |= rd->interfaces[s][fltalloc].flags & INMEMORY;
							v->regoff = rd->interfaces[s][fltalloc].regoff;
						}
						else if (rd->ifsavfltregcnt > 0) {
							rd->ifsavfltregcnt--;
							v->regoff = rd->savfltregs[rd->ifsavfltregcnt];
						}
						else {
							v->flags |= INMEMORY;
							v->regoff = rd->ifmemuse;
							rd->ifmemuse += regsneeded + 1;
						}
						fltalloc = t;
					}
					else {
#if defined(__I386__)
						/*
						 * for i386 put all longs in memory
						 */
						if (IS_2_WORD_TYPE(t)) {
							v->flags |= INMEMORY;
							v->regoff = rd->ifmemuse++;
						} else {
#endif
							if (intalloc >= 0) {
								v->flags |= rd->interfaces[s][intalloc].flags & INMEMORY;
								v->regoff = rd->interfaces[s][intalloc].regoff;
							}
							else if (rd->ifsavintregcnt > regsneeded) {
								rd->ifsavintregcnt -= regsneeded + 1;
								v->regoff = rd->savintregs[rd->ifsavintregcnt];
							}
							else {
								v->flags |= INMEMORY;
								v->regoff = rd->ifmemuse;
								rd->ifmemuse += regsneeded + 1;
							}
#if defined(__I386__)
						}
#endif
						intalloc = t;
					}
				}
			} /* if (type >= 0) */
		} /* for t */
	} /* for s */

	rd->maxmemuse = rd->ifmemuse;
	rd->maxargintreguse = -1;
	rd->maxtmpintreguse = rd->iftmpintregcnt;
	rd->maxsavintreguse = rd->ifsavintregcnt;
	rd->maxargfltreguse = -1;
	rd->maxtmpfltreguse = rd->iftmpfltregcnt;
	rd->maxsavfltreguse = rd->ifsavfltregcnt;
}



/* function local_regalloc *****************************************************

	allocates registers for all local variables
	
*******************************************************************************/
	
static void local_regalloc(methodinfo *m, codegendata *cd, registerdata *rd)
{
	int      s, t, tt;
	int      intalloc, fltalloc;
	varinfo *v;
	int      regsneeded = 0;
	int typeloop[] = { TYPE_LNG, TYPE_DBL, TYPE_INT, TYPE_FLT, TYPE_ADR };

	if (m->isleafmethod) {
		int arg, doublewordarg, iargcnt, fargcnt;

		arg = 0, iargcnt = 0, fargcnt = 0;
		doublewordarg = 0;
		for (s = 0; s < cd->maxlocals; s++) {
			intalloc = -1; fltalloc = -1;
			for (tt = 0; tt <= 4; tt++) {
				t = typeloop[tt];
				v = &rd->locals[s][t];

				if (v->type >= 0) {
#ifdef USETWOREGS
					regsneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
					if (IS_FLT_DBL_TYPE(t)) {
#if !defined(CONSECUTIVE_FLOATARGS)
						fargcnt = arg;
#endif
						if (fltalloc >= 0) {
							v->flags = rd->locals[s][fltalloc].flags;
							v->regoff = rd->locals[s][fltalloc].regoff;
						}
  						else if (!doublewordarg && (arg < m->paramcount) &&
								 (fargcnt < rd->fltreg_argnum)) {
							v->flags = 0;
							v->regoff = rd->argfltregs[fargcnt];
						}
						else if (rd->maxtmpfltreguse > 0) {
							rd->maxtmpfltreguse--;
							v->flags = 0;
							v->regoff = rd->tmpfltregs[rd->maxtmpfltreguse];
						}
						else if (rd->maxsavfltreguse > 0) {
							rd->maxsavfltreguse--;
							v->flags = 0;
							v->regoff = rd->savfltregs[rd->maxsavfltreguse];
						}
						else {
							v->flags = INMEMORY;
							v->regoff = rd->maxmemuse;
							rd->maxmemuse += regsneeded + 1;
						}
						fltalloc = t;

					} else {
						int regtouse;
#if defined(__I386__)
						/*
						 * for i386 put all longs in memory
						 */
						if (IS_2_WORD_TYPE(t)) {
							v->flags = INMEMORY;
							v->regoff = rd->maxmemuse++;
						} else {
#endif
#if !defined(CONSECUTIVE_INTARGS)
							iargcnt = arg;
#endif
							if (intalloc >= 0) {
								v->flags = rd->locals[s][intalloc].flags;
								v->regoff = rd->locals[s][intalloc].regoff;
							}
							else if (!doublewordarg && (arg < m->paramcount)
#ifndef USETWOREGS
									 && ((regtouse = iargcnt) < rd->intreg_argnum)
#else
									 && ((regtouse = s) < rd->intreg_argnum - regsneeded)
#endif
									 ) {
								v->flags = 0;
								v->regoff = rd->argintregs[regtouse];
							}
							else if (rd->maxtmpintreguse > regsneeded) {
								rd->maxtmpintreguse -= regsneeded + 1;
								v->flags = 0;
								v->regoff = rd->tmpintregs[rd->maxtmpintreguse];
							}
							else if (rd->maxsavintreguse > regsneeded) {
								rd->maxsavintreguse -= regsneeded + 1;
								v->flags = 0;
								v->regoff = rd->savintregs[rd->maxsavintreguse];
							}
							/*
							 * use unused argument registers as local registers
							 */
							else if (!doublewordarg && (arg >= m->paramcount) &&
									 (iargcnt < rd->intreg_argnum)) {
								v->flags = 0;
								v->regoff = rd->argintregs[iargcnt];
  								iargcnt++;
								arg++;
							}
							else {
								v->flags = INMEMORY;
								v->regoff = rd->maxmemuse;
								rd->maxmemuse += regsneeded + 1;
							}
#if defined(__I386__)
						}
#endif
						intalloc = t;
					}
				}
			}
			if (arg < m->paramcount) {
				if (doublewordarg) {
					doublewordarg = 0;
					/* what type was the double arg? */
					if (IS_FLT_DBL_TYPE(m->paramtypes[arg])) {
						fargcnt++;

					} else {
						iargcnt++;
					}
					arg++;

				} else if (IS_2_WORD_TYPE(m->paramtypes[arg])) {
					doublewordarg = 1;

				} else {
					if (IS_FLT_DBL_TYPE(m->paramtypes[arg])) {
						fargcnt++;

					} else {
						iargcnt++;
					}
					arg++;
				}
			}
		}
		return;
	}

	for (s = 0; s < cd->maxlocals; s++) {
		intalloc = -1; fltalloc = -1;
		for (tt=0; tt<=4; tt++) {
			t = typeloop[tt];
			v = &rd->locals[s][t];
			if (v->type >= 0) {
#ifdef USETWOREGS
				regsneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
				if (IS_FLT_DBL_TYPE(t)) {
					if (fltalloc >= 0) {
						v->flags = rd->locals[s][fltalloc].flags;
						v->regoff = rd->locals[s][fltalloc].regoff;
					}
					else if (rd->maxsavfltreguse > 0) {
						rd->maxsavfltreguse--;
						v->flags = 0;
						v->regoff = rd->savfltregs[rd->maxsavfltreguse];
					}
					else {
						v->flags = INMEMORY;
						v->regoff = rd->maxmemuse;
						rd->maxmemuse += regsneeded + 1;
					}
					fltalloc = t;
				}
				else {
#if defined(__I386__)
					/*
					 * for i386 put all longs in memory
					 */
					if (IS_2_WORD_TYPE(t)) {
						v->flags = INMEMORY;
						v->regoff = rd->maxmemuse++;
					} else {
#endif
						if (intalloc >= 0) {
							v->flags = rd->locals[s][intalloc].flags;
							v->regoff = rd->locals[s][intalloc].regoff;
						}
						else if (rd->maxsavintreguse > regsneeded) {
							rd->maxsavintreguse -= regsneeded+1;
							v->flags = 0;
							v->regoff = rd->savintregs[rd->maxsavintreguse];
						}
						else {
							v->flags = INMEMORY;
							v->regoff = rd->maxmemuse;
							rd->maxmemuse += regsneeded + 1;
						}
#if defined(__I386__)
					}
#endif
					intalloc = t;
				}
			}
		}
	}
}



static void reg_init_temp(registerdata *rd)
{
	rd->freememtop = 0;
	rd->memuse = rd->ifmemuse;

	rd->freetmpinttop = 0;
	rd->freesavinttop = 0;
	rd->freetmpflttop = 0;
	rd->freesavflttop = 0;

	rd->tmpintreguse = rd->iftmpintregcnt;
	rd->savintreguse = rd->ifsavintregcnt;
	rd->tmpfltreguse = rd->iftmpfltregcnt;
	rd->savfltreguse = rd->ifsavfltregcnt;

	/* all argument registers are available */
	rd->argintreguse = rd->intreg_argnum;
	rd->argfltreguse = rd->fltreg_argnum;
}


#define reg_new_temp(rd,s) if (s->varkind == TEMPVAR) reg_new_temp_func(rd, s)


static void reg_new_temp_func(registerdata *rd, stackptr s)
{
	s4 regsneeded;
	s4 tryagain;

	/* Try to allocate a saved register if there is no temporary one          */
	/* available. This is what happens during the second run.                 */
	tryagain = (s->flags & SAVEDVAR) ? 1 : 2;

#ifdef USETWOREGS
	regsneeded = (IS_2_WORD_TYPE(s->type)) ? 1 : 0;
#else
	regsneeded = 0;
#endif

	for(; tryagain; --tryagain) {
		if (tryagain == 1) {
			if (!(s->flags & SAVEDVAR))
				s->flags |= SAVEDTMP;
			if (IS_FLT_DBL_TYPE(s->type)) {
				if (rd->freesavflttop > 0) {
					rd->freesavflttop--;
					s->regoff = rd->freesavfltregs[rd->freesavflttop];
					return;

				} else if (rd->savfltreguse > 0) {
					rd->savfltreguse--;
					if (rd->savfltreguse < rd->maxsavfltreguse)
						rd->maxsavfltreguse = rd->savfltreguse;
					s->regoff = rd->savfltregs[rd->savfltreguse];
					return;
				}

			} else {
#if defined(__I386__)
				/*
				 * for i386 put all longs in memory
				 */
				if (!IS_2_WORD_TYPE(s->type)) {
#endif
					if (rd->freesavinttop > regsneeded) {
						rd->freesavinttop -= regsneeded + 1;
						s->regoff = rd->freesavintregs[rd->freesavinttop];
						return;

					} else if (rd->savintreguse > regsneeded) {
						rd->savintreguse -= regsneeded + 1;
						if (rd->savintreguse < rd->maxsavintreguse)
							rd->maxsavintreguse = rd->savintreguse;
						s->regoff = rd->savintregs[rd->savintreguse];
						return;
					}
#if defined(__I386__)
				}
#endif
			}

		} else {
			if (IS_FLT_DBL_TYPE(s->type)) {
				if (rd->freetmpflttop > 0) {
					rd->freetmpflttop--;
					s->regoff = rd->freetmpfltregs[rd->freetmpflttop];
					return;

				} else if (rd->tmpfltreguse > 0) {
					rd->tmpfltreguse--;
					if (rd->tmpfltreguse < rd->maxtmpfltreguse)
						rd->maxtmpfltreguse = rd->tmpfltreguse;
					s->regoff = rd->tmpfltregs[rd->tmpfltreguse];
					return;
				}

			} else {
#if defined(__I386__)
				/*
				 * for i386 put all longs in memory
				 */
				if (!IS_2_WORD_TYPE(s->type)) {
#endif
					if (rd->freetmpinttop > regsneeded) {
						rd->freetmpinttop -= regsneeded + 1;
						s->regoff = rd->freetmpintregs[rd->freetmpinttop];
						return;

					} else if (rd->tmpintreguse > regsneeded) {
						rd->tmpintreguse -= regsneeded + 1;
						if (rd->tmpintreguse < rd->maxtmpintreguse)
							rd->maxtmpintreguse = rd->tmpintreguse;
						s->regoff = rd->tmpintregs[rd->tmpintreguse];
						return;
					}
#if defined(__I386__)
				}
#endif
			}
		}
	}

	if (rd->freememtop > regsneeded) {
		rd->freememtop -= regsneeded + 1;
		s->regoff = rd->freemem[rd->freememtop];

	} else {
		s->regoff = rd->memuse;
		rd->memuse += regsneeded + 1;
		if (rd->memuse > rd->maxmemuse)
			rd->maxmemuse = rd->memuse;
	}
	s->flags |= INMEMORY;
}


#define reg_free_temp(rd,s) if (s->varkind == TEMPVAR) reg_free_temp_func(rd, s)


static void reg_free_temp_func(registerdata *rd, stackptr s)
{
	s4 regsneeded;

#ifdef USETWOREGS
	regsneeded = (IS_2_WORD_TYPE(s->type)) ? 1 : 0;
#else
	regsneeded = 0;
#endif

	if (s->flags & INMEMORY) {
		rd->freemem[rd->freememtop] = s->regoff;
		if (regsneeded)
			rd->freemem[rd->freememtop + 1] = s->regoff + 1;
		rd->freememtop += regsneeded + 1;

	} else if (IS_FLT_DBL_TYPE(s->type)) {
		if (s->flags & (SAVEDVAR | SAVEDTMP)) {
			s->flags &= ~SAVEDTMP;
			rd->freesavfltregs[rd->freesavflttop++] = s->regoff;

		} else
			rd->freetmpfltregs[rd->freetmpflttop++] = s->regoff;

	} else {
		if (s->flags & (SAVEDVAR | SAVEDTMP)) {
			s->flags &= ~SAVEDTMP;
			rd->freesavintregs[rd->freesavinttop] = s->regoff;
#ifdef USETWOREGS
			if (regsneeded)
				rd->freesavintregs[rd->freesavinttop + 1] = rd->secondregs[s->regoff];
#endif
			rd->freesavinttop += regsneeded + 1;

		} else {
			rd->freetmpintregs[rd->freetmpinttop] = s->regoff;
#ifdef USETWOREGS
			if (regsneeded)
				rd->freetmpintregs[rd->freetmpinttop + 1] = rd->secondregs[s->regoff];
#endif
			rd->freetmpinttop += regsneeded + 1;
		}
	}
}



static void allocate_scratch_registers(methodinfo *m, registerdata *rd)
{
	s4 opcode;
	s4 i;
	s4 len;
	stackptr    src;
	stackptr    dst;
	instruction *iptr;
	basicblock  *bptr;

	bptr = m->basicblocks;

	while (bptr != NULL) {
		if (bptr->flags >= BBREACHED) {
			dst = bptr->instack;

			/* initialize temp registers */
			reg_init_temp(rd);

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
				case ICMD_CHECKEXCEPTION:
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
					reg_new_temp(rd, dst);
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
					reg_free_temp(rd, src);
					reg_free_temp(rd, src->prev);
					reg_new_temp(rd, dst);
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
					reg_free_temp(rd, src);
					reg_free_temp(rd, src->prev);
					reg_free_temp(rd, src->prev->prev);
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
					reg_free_temp(rd, src);
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

				case ICMD_IASTORECONST:
				case ICMD_LASTORECONST:
				case ICMD_AASTORECONST:
				case ICMD_BASTORECONST:
				case ICMD_CASTORECONST:
				case ICMD_SASTORECONST:
					reg_free_temp(rd, src);
					reg_free_temp(rd, src->prev);
					break;

					/* pop 0 push 1 dup */
					
				case ICMD_DUP:
					reg_new_temp(rd, dst);
					break;

					/* pop 0 push 2 dup */
					
				case ICMD_DUP2:
					reg_new_temp(rd, dst->prev);
					reg_new_temp(rd, dst);
					break;

					/* pop 2 push 3 dup */
					
				case ICMD_DUP_X1:
					reg_free_temp(rd, src);
					reg_new_temp(rd, dst);
					reg_free_temp(rd, src->prev);
					reg_new_temp(rd, dst->prev);
					reg_new_temp(rd, dst->prev->prev);
					break;

					/* pop 3 push 4 dup */
					
				case ICMD_DUP_X2:
					reg_free_temp(rd, src);
					reg_new_temp(rd, dst);
					reg_free_temp(rd, src->prev);
					reg_new_temp(rd, dst->prev);
					reg_free_temp(rd, src->prev->prev);
					reg_new_temp(rd, dst->prev->prev);
					reg_new_temp(rd, dst->prev->prev->prev);
					break;

					/* pop 3 push 5 dup */
					
				case ICMD_DUP2_X1:
					reg_free_temp(rd, src);
					reg_new_temp(rd, dst);
					reg_free_temp(rd, src->prev);
					reg_new_temp(rd, dst->prev);
					reg_free_temp(rd, src->prev->prev);
					reg_new_temp(rd, dst->prev->prev);
					reg_new_temp(rd, dst->prev->prev->prev);
					reg_new_temp(rd, dst->prev->prev->prev->prev);
					break;

					/* pop 4 push 6 dup */
					
				case ICMD_DUP2_X2:
					reg_free_temp(rd, src);
					reg_new_temp(rd, dst);
					reg_free_temp(rd, src->prev);
					reg_new_temp(rd, dst->prev);
					reg_free_temp(rd, src->prev->prev);
					reg_new_temp(rd, dst->prev->prev);
					reg_free_temp(rd, src->prev->prev->prev);
					reg_new_temp(rd, dst->prev->prev->prev);
					reg_new_temp(rd, dst->prev->prev->prev->prev);
					reg_new_temp(rd, dst->prev->prev->prev->prev->prev);
					break;

					/* pop 2 push 2 swap */
					
				case ICMD_SWAP:
					reg_free_temp(rd, src);
					reg_new_temp(rd, dst->prev);
					reg_free_temp(rd, src->prev);
					reg_new_temp(rd, dst);
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
					reg_free_temp(rd, src);
					reg_free_temp(rd, src->prev);
					reg_new_temp(rd, dst);
					break;

					/* pop 1 push 1 */
					
				case ICMD_IADDCONST:
				case ICMD_ISUBCONST:
				case ICMD_IMULCONST:
				case ICMD_IDIVPOW2:
				case ICMD_IREMPOW2:
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
					reg_free_temp(rd, src);
					reg_new_temp(rd, dst);
					break;

					/* pop 0 push 1 */
					
				case ICMD_GETSTATIC:

				case ICMD_NEW:
					reg_new_temp(rd, dst);
					break;

					/* pop many push any */
					
				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKEINTERFACE:
					i = iptr->op1;
					while (--i >= 0) {
						reg_free_temp(rd, src);
						src = src->prev;
					}
					if (((methodinfo *) iptr->val.a)->returntype != TYPE_VOID)
						reg_new_temp(rd, dst);
					break;

				case ICMD_BUILTIN3:
					reg_free_temp(rd, src);
					src = src->prev;
				case ICMD_BUILTIN2:
					reg_free_temp(rd, src);
					src = src->prev;
				case ICMD_BUILTIN1:
					reg_free_temp(rd, src);
					src = src->prev;
					if (iptr->op1 != TYPE_VOID)
						reg_new_temp(rd, dst);
					break;

				case ICMD_MULTIANEWARRAY:
					i = iptr->op1;
					while (--i >= 0) {
						reg_free_temp(rd, src);
						src = src->prev;
					}
					reg_new_temp(rd, dst);
					break;

				default:
					printf("ICMD %d at %d\n", iptr->opc, (s4) (iptr - m->instructions));
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
