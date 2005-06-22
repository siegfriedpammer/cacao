/* src/vm/jit/reg.inc - register allocator

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
	    Christian Ullrich

   $Id: reg.inc 2774 2005-06-22 09:47:44Z christian $

*/


#include "arch.h"
#include "mm/memory.h"
#include "vm/method.h" 	 
#include "vm/resolve.h"
#include "vm/jit/reg.h"


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

	rd->argintregs = DMNEW(s4, INT_ARG_CNT);
	rd->tmpintregs = DMNEW(s4, INT_TMP_CNT);
	rd->savintregs = DMNEW(s4, INT_SAV_CNT);
	rd->freeargintregs = DMNEW(s4, INT_ARG_CNT);
	rd->freetmpintregs = DMNEW(s4, INT_TMP_CNT);
	rd->freesavintregs = DMNEW(s4, INT_SAV_CNT);

	rd->argintreguse = 0;
	rd->tmpintreguse = 0;
	rd->savintreguse = 0;

	for (i = 0; i < INT_REG_CNT; i++) {
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
			rd->argintregs[rd->argintreguse++] = i;
			break;
		}
	}
	assert(rd->savintreguse == INT_SAV_CNT);
	assert(rd->tmpintreguse == INT_TMP_CNT);
	assert(rd->argintreguse == INT_ARG_CNT);

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
		
#ifdef HAS_ADDRESS_REGISTER_FILE
	/* setup the address register table */

	rd->argadrregs = DMNEW(s4, ADR_ARG_CNT);
	rd->tmpadrregs = DMNEW(s4, ADR_TMP_CNT);
	rd->savadrregs = DMNEW(s4, ADR_SAV_CNT);
	rd->freeargadrregs = DMNEW(s4, ADR_ARG_CNT);
	rd->freetmpadrregs = DMNEW(s4, ADR_TMP_CNT);
	rd->freesavadrregs = DMNEW(s4, ADR_SAV_CNT);

	rd->adrreg_argnum = 0;
	rd->argadrreguse = 0;
	rd->tmpadrreguse = 0;
	rd->savadrreguse = 0;

	for (i = 0; i < ADR_REG_CNT; i++) {
		switch (nregdescadr[i]) {
		case REG_RET:
			rd->adrreg_ret = i; 
			break;
		case REG_SAV:
			rd->savadrregs[rd->savadrreguse++] = i;
			break;
		case REG_TMP:
  			rd->tmpadrregs[rd->tmpadrreguse++] = i; 
			break;
		case REG_ARG:
			rd->argadrregs[rd->argadrreguse++] = i;
			break;
		}
	}
	assert(rd->savadrreguse == ADR_SAV_CNT);
	assert(rd->tmpadrreguse == ADR_TMP_CNT);
	assert(rd->argadrreguse == ADR_ARG_CNT);
#endif
		
	/* setup the float register table */

	rd->argfltregs = DMNEW(s4, FLT_ARG_CNT);
	rd->tmpfltregs = DMNEW(s4, FLT_TMP_CNT);
	rd->savfltregs = DMNEW(s4, FLT_SAV_CNT);
	rd->freeargfltregs = DMNEW(s4, FLT_ARG_CNT);
	rd->freetmpfltregs = DMNEW(s4, FLT_TMP_CNT);
	rd->freesavfltregs = DMNEW(s4, FLT_SAV_CNT);

	rd->argfltreguse = 0;
	rd->tmpfltreguse = 0;
	rd->savfltreguse = 0;

	for (i = 0; i < FLT_REG_CNT; i++) {
		switch (nregdescfloat[i]) {
		case REG_RET:
			rd->fltreg_ret = i;
			break;
		case REG_SAV:
			rd->savfltregs[rd->savfltreguse++] = i;
			break;
		case REG_TMP:
			rd->tmpfltregs[rd->tmpfltreguse++] = i;
			break;
		case REG_ARG:
			rd->argfltregs[rd->argfltreguse++] = i;
			break;
		}
	}
	assert(rd->savfltreguse == FLT_SAV_CNT);
	assert(rd->tmpfltreguse == FLT_TMP_CNT);
	assert(rd->argfltreguse == FLT_ARG_CNT);


	rd->freemem    = DMNEW(s4, id->cummaxstack);
#if defined(HAS_4BYTE_STACKSLOT)
	rd->freemem_2  = DMNEW(s4, id->cummaxstack);
#endif
	rd->locals     = DMNEW(varinfo5, id->cumlocals);
	rd->interfaces = DMNEW(varinfo5, id->cummaxstack);
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

#if defined(SPECIALMEMUSE)
# if defined(__DARWIN__)
	/* 6*4=24 byte linkage area + 8*4=32 byte minimum parameter Area */
	rd->memuse = LA_WORD_SIZE + INT_ARG_CNT; 
# else
	rd->memuse = LA_WORD_SIZE;
# endif
#else
	rd->memuse = 0; /* init to zero -> analyse_stack will set it to a higher  */
	                /* value, if appropriate */
#endif

	/* Set rd->argxxxreguse to XXX_ARG_CNBT to not use unused argument        */
	/* registers as temp registers  */
#if defined(HAS_ADDRESS_REGISTER_FILE)
	rd->argadrreguse = 0;
#endif /* defined(HAS_ADDRESS_REGISTER_FILE) */
	rd->argintreguse = 0;
	rd->argfltreguse = 0;
}


/* function reg_free ***********************************************************

   releases all allocated space for registers

*******************************************************************************/

void reg_free(methodinfo *m, registerdata *rd)
{
	/* void */
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
#ifdef INVOKE_NEW_DEBUG
		if (compileverbose) {
			printf("memuse after reg.inc: %3i\n",rd->memuse);
		}
#endif
}


/* function interface_regalloc *************************************************

	allocates registers for all interface variables
	
*******************************************************************************/
	
static void interface_regalloc(methodinfo *m, codegendata *cd, registerdata *rd)
{
	int     s, t, tt, saved;
	int     intalloc, fltalloc; /* Remember allocated Register/Memory offset */
	              /* in case a more vars are packed into this interface slot */
	varinfo *v;
	int		intregsneeded = 0;
	int		memneeded = 0;
    /* allocate LNG and DBL Types first to ensure 2 memory slots or registers */
	/* on HAS_4BYTE_STACKSLOT architectures */
	int     typeloop[] = { TYPE_LNG, TYPE_DBL, TYPE_INT, TYPE_FLT, TYPE_ADR };

	/* rd->memuse was already set in stack.c to allocate stack space for */
	/* passing arguments to called methods                               */
#if defined(__I386__)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		/* reserve 0(%esp) for Monitorenter/exit Argument on i386 */
		if (rd->memuse < 1)
			rd->memuse = 1;
	}
#endif
#ifdef INVOKE_NEW_DEBUG
	if (compileverbose) {
		if (checksync && (m->flags & ACC_SYNCHRONIZED))
			printf("ACC_SYNCHRONIZED\n");
		printf("analyse: argintru %3i argfltru %3i memuse %3i\n",
			   rd->argintreguse, rd->argfltreguse, rd->memuse);
	}
#endif


	for (s = 0; s < cd->maxstack; s++) {
		intalloc = -1; fltalloc = -1;
		saved = (rd->interfaces[s][TYPE_INT].flags |
				 rd->interfaces[s][TYPE_LNG].flags |
		         rd->interfaces[s][TYPE_FLT].flags |
				 rd->interfaces[s][TYPE_DBL].flags |
		         rd->interfaces[s][TYPE_ADR].flags) & SAVEDVAR;
 
		for (tt = 0; tt <= 4; tt++) {
			t = typeloop[tt];
			v = &rd->interfaces[s][t];
			if (v->type >= 0) {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
				intregsneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
#if defined(HAS_4BYTE_STACKSLOT)
				memneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
				if (!saved) {
#if defined(HAS_ADDRESS_REGISTER_FILE)
					if (IS_ADR_TYPE(t)) {
						if (!m->isleafmethod 
							&&(rd->argadrreguse < ADR_ARG_CNT)) {
							v->regoff = rd->argadrregs[rd->argadrreguset++];
						} else if (rd->tmpadrreguse > 0) {
								v->regoff = rd->tmpadrregs[--rd->tmpadrreguse];
						} else if (rd->savadrreguse > 0) {
								v->regoff = rd->savadrregs[--rd->savadrreguse];
						} else {
							v->flags |= INMEMORY;
							v->regoff = rd->memuse++;
						}						
					} else /* !IS_ADR_TYPE */
#endif /* defined(HAS_ADDRESS_REGISTER_FILE) */
					{
						if (IS_FLT_DBL_TYPE(t)) {
							if (fltalloc >= 0) {
		       /* Reuse memory slot(s)/register(s) for shared interface slots */
								v->flags |= rd->interfaces[s][fltalloc].flags
									& INMEMORY;
								v->regoff = rd->interfaces[s][fltalloc].regoff;
							} else if (!m->isleafmethod 
									   && (rd->argfltreguse < FLT_ARG_CNT)) {
								v->regoff = rd->argfltregs[rd->argfltreguse++];
							} else if (rd->tmpfltreguse > 0) {
								v->regoff = rd->tmpfltregs[--rd->tmpfltreguse];
							} else if (rd->savfltreguse > 0) {
								v->regoff = rd->savfltregs[--rd->savfltreguse];
							} else {
								v->flags |= INMEMORY;
								v->regoff = rd->memuse;
								rd->memuse += memneeded + 1;
							}
							fltalloc = t;
						} else { /* !IS_FLT_DBL_TYPE(t) */
#if defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							/*
							 * for i386 put all longs in memory
							 */
							if (IS_2_WORD_TYPE(t)) {
								v->flags |= INMEMORY;
								v->regoff = rd->memuse;
								rd->memuse += memneeded + 1;
							} else
#endif /* defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE...GISTERS) */
/* #if !defined(HAS_4BYTE_STACKSLOT) */
								if (intalloc >= 0) {
		       /* Reuse memory slot(s)/register(s) for shared interface slots */
									v->flags |= 
										rd->interfaces[s][intalloc].flags 
										& INMEMORY;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
									if (!(v->flags & INMEMORY))
										v->regoff = GET_FIRST_REG(
											rd->interfaces[s][intalloc].regoff);
									else
#endif
										v->regoff = 
										    rd->interfaces[s][intalloc].regoff;
								} else 
/* #endif *//* !defined(HAS_4BYTE_STACKSLOT) */
									if (!m->isleafmethod && 
			                 (rd->argintreguse + intregsneeded < INT_ARG_CNT)) {
										v->regoff = 
											rd->argintregs[rd->argintreguse];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (intregsneeded) 
											SET_SECOND_REG( v->regoff, 
										  rd->argintregs[rd->argintreguse + 1]);
#endif
										rd->argintreguse += intregsneeded + 1;
									}
									else if (rd->tmpintreguse > intregsneeded) {
										rd->tmpintreguse -= intregsneeded + 1;
										v->regoff = 
											rd->tmpintregs[rd->tmpintreguse];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (intregsneeded) 
											SET_SECOND_REG( v->regoff, 
										  rd->tmpintregs[rd->tmpintreguse + 1]);
#endif
									}
									else if (rd->savintreguse > intregsneeded) {
										rd->savintreguse -= intregsneeded + 1;
										v->regoff = 
											rd->savintregs[rd->savintreguse];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (intregsneeded) 
											SET_SECOND_REG( v->regoff, 
									      rd->savintregs[rd->savintreguse + 1]);
#endif
									}
									else {
										v->flags |= INMEMORY;
										v->regoff = rd->memuse;
										rd->memuse += memneeded + 1;
									}

							intalloc = t;
						} /* if (IS_FLT_DBL_TYPE(t)) */
					} 
				} else { /* (saved) */
/* now the same like above, but without a chance to take a temporary register */
#ifdef HAS_ADDRESS_REGISTER_FILE
					if (IS_ADR_TYPE(t)) {
						if (rd->savadrreguse > 0) {
							v->regoff = rd->savadrregs[--rd->savadrreguse];
						}
						else {
							v->flags |= INMEMORY;
							v->regoff = rd->memuse++;
						}						
					} else
#endif
					{
						if (IS_FLT_DBL_TYPE(t)) {
							if (fltalloc >= 0) {
								v->flags |= rd->interfaces[s][fltalloc].flags
									& INMEMORY;
								v->regoff = rd->interfaces[s][fltalloc].regoff;
							} else
								if (rd->savfltreguse > 0) {
									v->regoff = rd->savfltregs[--rd->savfltreguse];
								}
								else {
									v->flags |= INMEMORY;
									v->regoff = rd->memuse;
									rd->memuse += memneeded + 1;
								}
							fltalloc = t;
						}
						else { /* IS_INT_LNG */
#if defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							/*
							 * for i386 put all longs in memory
							 */
							if (IS_2_WORD_TYPE(t)) {
								v->flags |= INMEMORY;
								v->regoff = rd->memuse;
								rd->memuse += memneeded + 1;
							} else
#endif
							{
/* #if !defined(HAS_4BYTE_STACKSLOT) */
								if (intalloc >= 0) {
									v->flags |= rd->interfaces[s][intalloc].flags
										& INMEMORY;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
									if (!(v->flags & INMEMORY))
										v->regoff =
									   GET_FIRST_REG(rd->interfaces[s][intalloc].regoff);
									else
#endif
										v->regoff =
											rd->interfaces[s][intalloc].regoff;
								} else
/* #endif */
								{
									if (rd->savintreguse > intregsneeded) {
										rd->savintreguse -= intregsneeded + 1;
										v->regoff =
											rd->savintregs[rd->savintreguse];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (intregsneeded) 
											SET_SECOND_REG( v->regoff, 
											    rd->savintregs[rd->savintreguse + 1]);
#endif
									} else {
										v->flags |= INMEMORY;
										v->regoff = rd->memuse;
										rd->memuse += memneeded + 1;
									}
								}
								intalloc = t;
							}
						} /* if (IS_FLT_DBL_TYPE(t) else */
					} /* if (IS_ADR_TYPE(t)) else */
				} /* if (saved) else */
			} /* if (type >= 0) */
		} /* for t */
	} /* for s */
}



/* function local_regalloc *****************************************************

	allocates registers for all local variables
	
*******************************************************************************/
	
static void local_regalloc(methodinfo *m, codegendata *cd, registerdata *rd)
{
	int     p, s, t, tt;
	int     intalloc, fltalloc;
	varinfo *v;
	int     intregsneeded = 0;
	int     memneeded = 0;
	int     typeloop[] = { TYPE_LNG, TYPE_DBL, TYPE_INT, TYPE_FLT, TYPE_ADR };
/* 	int     fargcnt, iargcnt; */
#ifdef HAS_ADDRESS_REGISTER_FILE
	int     aargcnt;
#endif

	if (m->isleafmethod) {
		methoddesc *md = m->parseddesc;

		iargcnt = md->argintreguse;
		fargcnt = md->argfltreguse;
#ifdef HAS_ADDRESS_REGISTER_FILE
		aargcnt = md->argadrreguse;
#endif
		for (p = 0, s = 0; s < cd->maxlocals; s++, p++) {
			intalloc = -1; fltalloc = -1;
			for (tt = 0; tt <= 4; tt++) {
				t = typeloop[tt];
				v = &rd->locals[s][t];

				if (v->type < 0)
					continue;

#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
				intregsneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
#if defined(HAS_4BYTE_STACKSLOT)
				memneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif

				/*
				 *  The order of
				 *
				 *  #ifdef HAS_ADDRESS_REGISTER_FILE
				 *  if (IS_ADR_TYPE) { 
				 *  ...
				 *  } else 
				 *  #endif
				 *  if (IS_FLT_DBL) {
				 *  ...
				 *  } else { / int & lng
				 *  ...
				 *  }
				 *
				 *  must not to be changed!
				 */

#ifdef HAS_ADDRESS_REGISTER_FILE
				if (IS_ADR_TYPE(t)) {
					if ((p < md->paramcount) && !md->params[p].inmemory) {
						v->flags = 0;
						v->regoff = rd->argadrregs[md->params[p].regoff];
					}
					if (rd->tmpadrreguse > 0) {
						v->flags = 0;
						v->regoff = rd->tmpadrregs[--rd->tmpadrreguse];
					}
					else if (rd->savadrreguse > 0) {
						v->flags = 0;
						v->regoff = rd->savadrregs[--rd->savadrreguse];
					}
					/* use unused argument registers as local registers */
/* 					else if ((p >= md->paramcount) && */
/* 							 (aargcnt < ADR_ARG_CNT)) { */
/* 						v->flags = 0; */
/* 						v->regoff = rd->argadrregs[aargcnt++]; */
/* 					} */
					else {
						v->flags |= INMEMORY;
						v->regoff = rd->memuse++;
					}						
				} else {
#endif
					if (IS_FLT_DBL_TYPE(t)) {
						if (fltalloc >= 0) {
							v->flags = rd->locals[s][fltalloc].flags;
							v->regoff = rd->locals[s][fltalloc].regoff;
						}
  						else if ((p < md->paramcount) &&
								 !md->params[p].inmemory) {
							v->flags = 0;
							v->regoff = rd->argfltregs[md->params[p].regoff];
						}
						else if (rd->tmpfltreguse > 0) {
							v->flags = 0;
							v->regoff = rd->tmpfltregs[--rd->tmpfltreguse];
						}
						else if (rd->savfltreguse > 0) {
							v->flags = 0;
							v->regoff = rd->savfltregs[--rd->savfltreguse];
						}
						/* use unused argument registers as local registers */
/* 						else if ((p >= m->paramcount) && */
/* 								 (fargcnt < rd->fltreg_argnum)) { */
/* 							v->flags = 0; */
/* 							v->regoff = rd->argfltregs[fargcnt]; */
/* 							fargcnt++; */
/* 						} */
						else {
							v->flags = INMEMORY;
							v->regoff = rd->memuse;
							rd->memuse += memneeded + 1;
						}
						fltalloc = t;

					} else {
#if defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
						/*
						 * for i386 put all longs in memory
						 */
						if (IS_2_WORD_TYPE(t)) {
							v->flags = INMEMORY;
							v->regoff = rd->memuse;
							rd->memuse += memneeded + 1;
						} else {
#endif
							if (intalloc >= 0) {
								v->flags = rd->locals[s][intalloc].flags;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (!(v->flags & INMEMORY))
									v->regoff =
									    GET_FIRST_REG(
										    rd->locals[s][intalloc].regoff);
								else
#endif
									v->regoff = rd->locals[s][intalloc].regoff;
							}
							else if ((p < md->paramcount) && 
									 !md->params[p].inmemory) {
								v->flags = 0;
								v->regoff =
									rd->argintregs[md->params[p].regoff];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (IS_2_WORD_TYPE(t))
/* For ARM: - if md->params[p].regoff == R4, prevent here that SECOND_REG */
/* tries to set R5                                                        */
									SET_SECOND_REG( v->regoff, rd->argintregs[
												   md->params[p].regoff + 1]);
#endif
							}
							else if (rd->tmpintreguse > intregsneeded) {
								rd->tmpintreguse -= intregsneeded + 1;
								v->flags = 0;
								v->regoff = rd->tmpintregs[rd->tmpintreguse];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (intregsneeded) 
									SET_SECOND_REG( v->regoff, 
									   rd->tmpintregs[rd->tmpintreguse + 1]);
#endif
							}
							else if (rd->savintreguse > intregsneeded) {
								rd->savintreguse -= intregsneeded + 1;
								v->flags = 0;
								v->regoff = rd->savintregs[rd->savintreguse];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (intregsneeded) 
									SET_SECOND_REG( v->regoff, 
									   rd->savintregs[rd->savintreguse + 1]);
#endif
							}
							/*
							 * use unused argument registers as local registers
							 */
/* 							else if ((p >= m->paramcount) && */
/* 									 (iargcnt < rd->intreg_argnum)) { */
/* 								v->flags = 0; */
/* 								v->regoff = rd->argintregs[iargcnt]; */
/*   								iargcnt++; */
/* 							} */
							else {
								v->flags = INMEMORY;
								v->regoff = rd->memuse;
								rd->memuse += memneeded + 1;
							}
#if defined(__I386__)
						}
#endif
						intalloc = t;
					}
#ifdef HAS_ADDRESS_REGISTER_FILE
				}
#endif
			} /* for (tt=0;...) */

			/* If the current parameter is a 2-word type, the next local slot */
			/* is skipped.                                                    */

			if (p < md->paramcount)
				if (IS_2_WORD_TYPE(md->paramtypes[p].type))
					s++;
		}
		return;
	}

	for (s = 0; s < cd->maxlocals; s++) {
		intalloc = -1; fltalloc = -1;
		for (tt=0; tt<=4; tt++) {
			t = typeloop[tt];
			v = &rd->locals[s][t];

			if (v->type >= 0) {
#ifdef SUPPORT_COMBINE_INTEGER_REGISTERS
				intregsneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
#if defined(HAS_4BYTE_STACKSLOT)
				memneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
#ifdef HAS_ADDRESS_REGISTER_FILE
				if ( IS_ADR_TYPE(t) ) {
					if (rd->savadrreguse > 0) {
						v->flags = 0;
						v->regoff = rd->savadrregs[--rd->savadrreguse];
					}
					else {
						v->flags = INMEMORY;
						v->regoff = rd->memuse++;
					}
				} else {
#endif
				if (IS_FLT_DBL_TYPE(t)) {
					if (fltalloc >= 0) {
						v->flags = rd->locals[s][fltalloc].flags;
						v->regoff = rd->locals[s][fltalloc].regoff;
					}
					else if (rd->savfltreguse > 0) {
						v->flags = 0;
						v->regoff = rd->savfltregs[--rd->savfltreguse];
					}
					else {
						v->flags = INMEMORY;
						/* Align doubles in Memory */
						if ( (memneeded) && (rd->memuse & 1))
							rd->memuse++;
						v->regoff = rd->memuse;
						rd->memuse += memneeded + 1;
					}
					fltalloc = t;
				}
				else {
#if defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
					/*
					 * for i386 put all longs in memory
					 */
					if (IS_2_WORD_TYPE(t)) {
						v->flags = INMEMORY;
						v->regoff = rd->memuse;
						rd->memuse += memneeded + 1;
					} else {
#endif
						if (intalloc >= 0) {
							v->flags = rd->locals[s][intalloc].flags;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (!(v->flags & INMEMORY))
								v->regoff =
								    GET_FIRST_REG(
									    rd->locals[s][intalloc].regoff);
							else
#endif
								v->regoff = rd->locals[s][intalloc].regoff;
						}
						else if (rd->savintreguse > intregsneeded) {
							rd->savintreguse -= intregsneeded+1;
							v->flags = 0;
							v->regoff = rd->savintregs[rd->savintreguse];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (intregsneeded) 
									SET_SECOND_REG( v->regoff, 
									   rd->savintregs[rd->savintreguse + 1]);
#endif
						}
						else {
							v->flags = INMEMORY;
							v->regoff = rd->memuse;
							rd->memuse += memneeded + 1;
						}
#if defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
					}
#endif
					intalloc = t;
				}
#ifdef HAS_ADDRESS_REGISTER_FILE
				}
#endif

			}
		}
	}
}

static void reg_init_temp(methodinfo *m, registerdata *rd)
{
	rd->freememtop = 0;
#if defined(HAS_4BYTE_STACKSLOT)
	rd->freememtop_2 = 0;
#endif

	rd->freetmpinttop = 0;
	rd->freesavinttop = 0;
	rd->freetmpflttop = 0;
	rd->freesavflttop = 0;
#ifdef HAS_ADDRESS_REGISTER_FILE
	rd->freetmpadrtop = 0;
	rd->freesavadrtop = 0;
#endif

	rd->freearginttop = 0;
	rd->freeargflttop = 0;
#ifdef HAS_ADDRESS_REGISTER_FILE
	rd->freeargadrtop = 0;
#endif

#if 0
 	if (m->isleafmethod) {
		/* Don't use not used Argument Registers in Leafmethods -> they could */
		/* already be in use for Locals passed as parameter to this Method    */
#endif
		rd->argintreguse = INT_ARG_CNT;
		rd->argfltreguse = FLT_ARG_CNT;
#ifdef HAS_ADDRESS_REGISTER_FILE
		rd->argadrreguse = ADR_ARG_CNT;
#endif
#if 0
 	}
#endif
}


#define reg_new_temp(rd,s) if (s->varkind == TEMPVAR) reg_new_temp_func(rd, s)

static void reg_new_temp_func(registerdata *rd, stackptr s)
{
	s4 intregsneeded;
	s4 memneeded;
	s4 tryagain;

	/* Try to allocate a saved register if there is no temporary one          */
	/* available. This is what happens during the second run.                 */
	tryagain = (s->flags & SAVEDVAR) ? 1 : 2;

#ifdef SUPPORT_COMBINE_INTEGER_REGISTERS
	intregsneeded = (IS_2_WORD_TYPE(s->type)) ? 1 : 0;
#else
	intregsneeded = 0;
#endif
#if defined(HAS_4BYTE_STACKSLOT)
	memneeded = (IS_2_WORD_TYPE(s->type)) ? 1 : 0;
#else
	memneeded = 0;
#endif

	for(; tryagain; --tryagain) {
		if (tryagain == 1) {
			if (!(s->flags & SAVEDVAR))
				s->flags |= SAVEDTMP;
#ifdef HAS_ADDRESS_REGISTER_FILE
			if (IS_ADR_TYPE(s->type)) {
				if (rd->freesavadrtop > 0) {
					s->regoff = rd->freesavadrregs[--rd->freesavadrtop];
					return;
				} else if (rd->savadrreguse > 0) {
					s->regoff = rd->savadrregs[--rd->savadrreguse];
					return;
				}
			} else
#endif
			{
				if (IS_FLT_DBL_TYPE(s->type)) {
					if (rd->freesavflttop > 0) {
						s->regoff = rd->freesavfltregs[--rd->freesavflttop];
						return;
					} else if (rd->savfltreguse > 0) {
						s->regoff = rd->savfltregs[--rd->savfltreguse];
						return;
					}
				} else {
#if defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
					/*
					 * for i386 put all longs in memory
					 */
					if (!IS_2_WORD_TYPE(s->type))
#endif
					{
						if (rd->freesavinttop > intregsneeded) {
							rd->freesavinttop -= intregsneeded + 1;
							s->regoff = rd->freesavintregs[rd->freesavinttop];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								SET_SECOND_REG( s->regoff, 
									rd->freesavintregs[rd->freesavinttop + 1]);
#endif
							return;
						} else if (rd->savintreguse > intregsneeded) {
							rd->savintreguse -= intregsneeded + 1;
							s->regoff = rd->savintregs[rd->savintreguse];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								SET_SECOND_REG( s->regoff, 
							        rd->savintregs[rd->savintreguse + 1]);
#endif
							return;
						}
					}
				}
			}
		} else { /* tryagain == 2 */
#ifdef HAS_ADDRESS_REGISTER_FILE
			if (IS_ADR_TYPE(s->type)) {
				if (rd->freetmpadrtop > 0) {
					s->regoff = rd->freetmpadrregs[--rd->freetmpadrtop];
					return;
				} else if (rd->tmpadrreguse > 0) {
					s->regoff = rd->tmpadrregs[--rd->tmpadrreguse];
					return;
				}
			} else
#endif
			{
				if (IS_FLT_DBL_TYPE(s->type)) {
					if (rd->freeargflttop > 0) {
						s->regoff = rd->freeargfltregs[--rd->freeargflttop];
						s->flags |= TMPARG;
						return;
					} else if (rd->argfltreguse < FLT_ARG_CNT) {
						s->regoff = rd->argfltregs[rd->argfltreguse++];
						s->flags |= TMPARG;
						return;
					} else if (rd->freetmpflttop > 0) {
						s->regoff = rd->freetmpfltregs[--rd->freetmpflttop];
						return;
					} else if (rd->tmpfltreguse > 0) {
						s->regoff = rd->tmpfltregs[--rd->tmpfltreguse];
						return;
					}

				} else {
#if defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
					/*
					 * for i386 put all longs in memory
					 */
					if (!IS_2_WORD_TYPE(s->type))
#endif
					{
						if (rd->freearginttop > intregsneeded) {
							rd->freearginttop -= intregsneeded + 1;
							s->regoff = rd->freeargintregs[rd->freearginttop];
							s->flags |= TMPARG;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								SET_SECOND_REG( s->regoff, 
							        rd->freeargintregs[rd->freearginttop + 1]);
#endif
							return;
						} else if (rd->argintreguse 
								   < INT_ARG_CNT - intregsneeded) {
							s->regoff = rd->argintregs[rd->argintreguse];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								SET_SECOND_REG( s->regoff, 
								    rd->argintregs[rd->argintreguse + 1]);
#endif
							s->flags |= TMPARG;
							rd->argintreguse += intregsneeded + 1;
							return;
						} else if (rd->freetmpinttop > intregsneeded) {
							rd->freetmpinttop -= intregsneeded + 1;
							s->regoff = rd->freetmpintregs[rd->freetmpinttop];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								SET_SECOND_REG( s->regoff, 
								    rd->freetmpintregs[rd->freetmpinttop + 1]);
#endif
							return;
						} else if (rd->tmpintreguse > intregsneeded) {
							rd->tmpintreguse -= intregsneeded + 1;
							s->regoff = rd->tmpintregs[rd->tmpintreguse];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								SET_SECOND_REG( s->regoff, 
								    rd->tmpintregs[rd->tmpintreguse + 1]);
#endif
							return;
						}
					} /* if (!IS_2_WORD_TYPE(s->type)) */
				} /* if (IS_FLT_DBL_TYPE(s->type)) */
			} /* if (IS_ADR_TYPE(s->type)) */
		} /* if (tryagain == 1) else */
	} /* for(; tryagain; --tryagain) */

#if defined(HAS_4BYTE_STACKSLOT)
	if ((memneeded == 1) && (rd->freememtop_2 > 0)) {
		rd->freememtop_2--;
		s->regoff = rd->freemem_2[rd->freememtop_2];
	} else
#endif /*defined(HAS_4BYTE_STACKSLOT) */
		if ((memneeded == 0) && (rd->freememtop > 0)) {
			rd->freememtop--;;
			s->regoff = rd->freemem[rd->freememtop];
		} else {
#if defined(HAS_4BYTE_STACKSLOT)
			/* align 2 Word Types */
			if ((memneeded) && ((rd->memuse & 1) == 1)) { 
				/* Put patched memory slot on freemem */
				rd->freemem[rd->freememtop++] = rd->memuse;
				rd->memuse++;
			}
#endif /*defined(HAS_4BYTE_STACKSLOT) */
			s->regoff = rd->memuse;
			rd->memuse += memneeded + 1;
		}
	s->flags |= INMEMORY;
}


#define reg_free_temp(rd,s) if (s->varkind == TEMPVAR) reg_free_temp_func(rd, s)

static void reg_free_temp_func(registerdata *rd, stackptr s)
{
	s4 intregsneeded;
	s4 memneeded;

#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
	intregsneeded = (IS_2_WORD_TYPE(s->type)) ? 1 : 0;
#else
	intregsneeded = 0;
#endif

#if defined(HAS_4BYTE_STACKSLOT)
	memneeded = (IS_2_WORD_TYPE(s->type)) ? 1 : 0;
#else
	memneeded = 0;
#endif

	if (s->flags & INMEMORY) {
#if defined(HAS_4BYTE_STACKSLOT)
		if (memneeded > 0) {
			rd->freemem_2[rd->freememtop_2] = s->regoff;
			rd->freememtop_2++;
		} else 
#endif
		{
			rd->freemem[rd->freememtop] = s->regoff;
			rd->freememtop++;
		}

#ifdef HAS_ADDRESS_REGISTER_FILE
	} else if (IS_ADR_TYPE(s->type)) {
		if (s->flags & (SAVEDVAR | SAVEDTMP)) {
			s->flags &= ~SAVEDTMP;
			rd->freesavadrregs[rd->freesavadrtop++] = s->regoff;
		} else
			rd->freetmpadrregs[rd->freetmpadrtop++] = s->regoff;
#endif
	} else if (IS_FLT_DBL_TYPE(s->type)) {
		if (s->flags & (SAVEDVAR | SAVEDTMP)) {
			s->flags &= ~SAVEDTMP;
			rd->freesavfltregs[rd->freesavflttop++] = s->regoff;
		} else if (s->flags & TMPARG) {
			s->flags &= ~TMPARG;
			rd->freeargfltregs[rd->freeargflttop++] = s->regoff;
		} else
			rd->freetmpfltregs[rd->freetmpflttop++] = s->regoff;
	} else { /* IS_INT_LNG_TYPE */
		if (s->flags & (SAVEDVAR | SAVEDTMP)) {
			s->flags &= ~SAVEDTMP;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
			if (intregsneeded) {
				rd->freesavintregs[rd->freesavinttop] =
					GET_FIRST_REG(s->regoff);
				rd->freesavintregs[rd->freesavinttop + 1] =
					GET_SECOND_REG(s->regoff);
			} else
#endif
				rd->freesavintregs[rd->freesavinttop] = s->regoff;
			rd->freesavinttop += intregsneeded + 1;

		} else if (s->flags & TMPARG) {
			s->flags &= ~TMPARG;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
			if (intregsneeded) {
				rd->freeargintregs[rd->freearginttop] =
					GET_FIRST_REG(s->regoff);
				rd->freeargintregs[rd->freearginttop + 1] =
					GET_SECOND_REG(s->regoff);
			} else 
#endif
				rd->freesavintregs[rd->freearginttop] = s->regoff;
			rd->freearginttop += intregsneeded + 1;
		} else {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
			if (intregsneeded) {
				rd->freetmpintregs[rd->freetmpinttop] =
					GET_FIRST_REG(s->regoff);
				rd->freetmpintregs[rd->freetmpinttop + 1] =
					GET_SECOND_REG(s->regoff);
			} else
#endif
				rd->freetmpintregs[rd->freetmpinttop] = s->regoff;
			rd->freetmpinttop += intregsneeded + 1;
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

	/* initialize temp registers */
	reg_init_temp(m, rd);

	bptr = m->basicblocks;

	while (bptr != NULL) {
		if (bptr->flags >= BBREACHED) {
			dst = bptr->instack;


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
				case ICMD_CHECKNULL:
				case ICMD_CHECKASIZE:
				case ICMD_CHECKEXCEPTION:
				case ICMD_IINC:
				case ICMD_JSR:
				case ICMD_RET:
				case ICMD_RETURN:
				case ICMD_GOTO:
				case ICMD_PUTSTATICCONST:
				case ICMD_INLINE_START:
				case ICMD_INLINE_END:
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
				case ICMD_PUTFIELDCONST:

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
				case ICMD_IMULPOW2:
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
				case ICMD_LMULPOW2:
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
					
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKEINTERFACE:
					i = iptr->op1;
					while (--i >= 0) {
						reg_free_temp(rd, src);
						src = src->prev;
					}
					if (((unresolved_method *) iptr->target)->methodref->parseddesc.md->returntype.type != TYPE_VOID)
						reg_new_temp(rd, dst);
					break;

				case ICMD_BUILTIN:
					i = iptr->op1;
					while (--i >= 0) {
						reg_free_temp(rd, src);
						src = src->prev;
					}
					if (((builtintable_entry *) iptr->val.a)->md->returntype.type != TYPE_VOID)
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
					throw_cacao_exception_exit(string_java_lang_InternalError,
											   "Unknown ICMD %d during register allocation",
											   iptr->opc);
				} /* switch */
				iptr++;
			} /* while instructions */
		} /* if */
		bptr = bptr->next;
	} /* while blocks */
}


#ifdef STATISTICS
void reg_make_statistics( methodinfo *m, codegendata *cd, registerdata *rd) {
	int i,type;
	s4 len;
	stackptr    src, src_old;
	stackptr    dst;
	instruction *iptr;
	basicblock  *bptr;
	int size_interface; /* == maximum size of in/out stack at basic block boundaries */
	bool in_register;

	in_register = true;

	size_interface = 0;

		/* count how many local variables are held in memory or register */
		for(i=0; i < cd->maxlocals; i++)
			for (type=0; type <=4; type++)
				if (rd->locals[i][type].type != -1) { /* valid local */
					if (rd->locals[i][type].flags & INMEMORY) {
						count_locals_spilled++;
						in_register=false;
					}
					else
						count_locals_register++;
				}
		/* count how many stack slots are held in memory or register */

		bptr = m->basicblocks;
		while (bptr != NULL) {
			if (bptr->flags >= BBREACHED) {

#ifdef LSRA
			if (!opt_lsra) {
#endif	
				/* check for memory moves from interface to BB instack */
				dst = bptr->instack;
				len = bptr->indepth;
				
				if (len > size_interface) size_interface = len;

				while (dst != NULL) {
					len--;
					if (dst->varkind != STACKVAR) {
						if ( (dst->flags & INMEMORY) ||
							 (rd->interfaces[len][dst->type].flags & INMEMORY) || 
							 ( (dst->flags & INMEMORY) && 
							   (rd->interfaces[len][dst->type].flags & INMEMORY) && 
							   (dst->regoff != rd->interfaces[len][dst->type].regoff) ))
						{
							/* one in memory or both inmemory at different offsets */
							count_mem_move_bb++;
							in_register=false;
						}
					}

					dst = dst->prev;
				}

				/* check for memory moves from BB outstack to interface */
				dst = bptr->outstack;
				len = bptr->outdepth;
				if (len > size_interface) size_interface = len;

				while (dst) {
					len--;
					if (dst->varkind != STACKVAR) {
						if ( (dst->flags & INMEMORY) || \
							 (rd->interfaces[len][dst->type].flags & INMEMORY) || \
							 ( (dst->flags & INMEMORY) && \
							   (rd->interfaces[len][dst->type].flags & INMEMORY) && \
							   (dst->regoff != rd->interfaces[len][dst->type].regoff) ))
						{
							/* one in memory or both inmemory at different offsets */
							count_mem_move_bb++;
							in_register=false;
						}
					}

					dst = dst->prev;
				}
#ifdef LSRA
			}
#endif	


				dst = bptr->instack;
				iptr = bptr->iinstr;
				len = bptr->icount;
				src_old = NULL;

				while (--len >= 0)  {
					src = dst;
					dst = iptr->dst;

					if ((src!= NULL) && (src != src_old)) { /* new stackslot */
						switch (src->varkind) {
						case TEMPVAR:
						case STACKVAR:
							if (!(src->flags & INMEMORY)) 
								count_ss_register++;
							else {
								count_ss_spilled++;
								in_register=false;
							}				
							break;
							/* 					case LOCALVAR: */
							/* 						if (!(rd->locals[src->varnum][src->type].flags & INMEMORY)) */
							/* 							count_ss_register++; */
							/* 						else */
							/* 							count_ss_spilled++; */
							/* 						break; */
						case ARGVAR:
							if (!(src->flags & INMEMORY)) 
								count_argument_mem_ss++;
							else
								count_argument_reg_ss++;
							break;


							/* 						if (IS_FLT_DBL_TYPE(src->type)) { */
							/* 							if (src->varnum < FLT_ARG_CNT) { */
							/* 								count_ss_register++; */
							/* 								break; */
							/* 							} */
							/* 						} else { */
							/* #if defined(__POWERPC__) */
							/* 							if (src->varnum < INT_ARG_CNT - (IS_2_WORD_TYPE(src->type) != 0)) { */
							/* #else */
							/* 							if (src->varnum < INT_ARG_CNT) { */
							/* #endif */
							/* 								count_ss_register++; */
							/* 								break; */
							/* 							} */
							/* 						} */
							/* 						count_ss_spilled++; */
							/* 						break; */
						}
					}
					src_old = src;
					
					iptr++;
				} /* while instructions */
			} /* if */
			bptr = bptr->next;
		} /* while blocks */
		count_interface_size += size_interface; /* accummulate the size of the interface (between bb boundaries) */
		if (in_register) count_method_in_register++;
}
#endif
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
