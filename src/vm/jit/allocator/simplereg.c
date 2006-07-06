/* src/vm/jit/allocator/simplereg.c - register allocator

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
            Michael Starzinger
            Edwin Steiner

   $Id: simplereg.c 5079 2006-07-06 11:36:01Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include <assert.h>

#include "arch.h"
#include "md-abi.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "mm/memory.h"
#include "vm/method.h"
#include "vm/options.h"
#include "vm/resolve.h"
#include "vm/stringlocal.h"
#include "vm/jit/reg.h"
#include "vm/jit/allocator/simplereg.h"


/* function prototypes for this file ******************************************/

static void interface_regalloc(jitdata *jd);
static void local_regalloc(jitdata *jd);
static void new_allocate_scratch_registers(jitdata *jd);
static void allocate_scratch_registers(jitdata *jd);


/* regalloc ********************************************************************

   Does a simple register allocation.
	
*******************************************************************************/
	
bool new_regalloc(jitdata *jd)
{
	jitdata *newjd;

	/* There is a problem with the use of unused float argument
	   registers in leafmethods for stackslots on c7 (2 * Dual Core
	   AMD Opteron(tm) Processor 270) - runtime for the jvm98 _mtrt
	   benchmark is heaviliy increased. This could be prevented by
	   setting rd->argfltreguse to FLT_ARG_CNT before calling
	   allocate_scratch_registers and setting it back to the original
	   value before calling local_regalloc.  */

	newjd = DNEW(jitdata);
	*newjd = *jd;
	newjd->rd = jd->new_rd;

	interface_regalloc(newjd);
	new_allocate_scratch_registers(newjd);
	local_regalloc(newjd);

	/* everthing's ok */

	return true;
}

bool regalloc(jitdata *jd)
{
	/* There is a problem with the use of unused float argument
	   registers in leafmethods for stackslots on c7 (2 * Dual Core
	   AMD Opteron(tm) Processor 270) - runtime for the jvm98 _mtrt
	   benchmark is heaviliy increased. This could be prevented by
	   setting rd->argfltreguse to FLT_ARG_CNT before calling
	   allocate_scratch_registers and setting it back to the original
	   value before calling local_regalloc.  */

	interface_regalloc(jd);
	allocate_scratch_registers(jd);
	local_regalloc(jd);

	/* everthing's ok */

	return true;
}


/* interface_regalloc **********************************************************

   Allocates registers for all interface variables.
	
*******************************************************************************/
	
static void interface_regalloc(jitdata *jd)
{
	methodinfo   *m;
	codeinfo     *code;
	codegendata  *cd;
	registerdata *rd;

	int     s, t, tt, saved;
	int     intalloc, fltalloc; /* Remember allocated Register/Memory offset */
	              /* in case a more vars are packed into this interface slot */
	varinfo *v;
	int		intregsneeded = 0;
	int		memneeded = 0;
    /* allocate LNG and DBL Types first to ensure 2 memory slots or registers */
	/* on HAS_4BYTE_STACKSLOT architectures */
	int     typeloop[] = { TYPE_LNG, TYPE_DBL, TYPE_INT, TYPE_FLT, TYPE_ADR };

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	rd   = jd->rd;

	/* rd->memuse was already set in stack.c to allocate stack space
	   for passing arguments to called methods. */

#if defined(__I386__)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		/* reserve 0(%esp) for Monitorenter/exit Argument on i386 */
		if (rd->memuse < 1)
			rd->memuse = 1;
	}
#endif

 	if (code->isleafmethod) {
		/* Reserve argument register, which will be used for Locals acting */
		/* as Parameters */
		if (rd->argintreguse < m->parseddesc->argintreguse)
			rd->argintreguse = m->parseddesc->argintreguse;
		if (rd->argfltreguse < m->parseddesc->argfltreguse)
			rd->argfltreguse = m->parseddesc->argfltreguse;
#ifdef HAS_ADDRESS_REGISTER_FILE
		if (rd->argadrreguse < m->parseddesc->argadrreguse)
			rd->argadrreguse = m->parseddesc->argadrreguse;
#endif

 	}

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
						if (!code->isleafmethod 
							&&(rd->argadrreguse < ADR_ARG_CNT)) {
							v->regoff = rd->argadrregs[rd->argadrreguse++];
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
							} else if (rd->argfltreguse < FLT_ARG_CNT) {
								v->regoff = rd->argfltregs[rd->argfltreguse++];
							} else if (rd->tmpfltreguse > 0) {
								v->regoff = rd->tmpfltregs[--rd->tmpfltreguse];
							} else if (rd->savfltreguse > 0) {
								v->regoff = rd->savfltregs[--rd->savfltreguse];
							} else {
								v->flags |= INMEMORY;
#if defined(ALIGN_DOUBLES_IN_MEMORY)
								/* Align doubles in Memory */
								if ( (memneeded) && (rd->memuse & 1))
									rd->memuse++;
#endif
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
#if defined(ALIGN_LONGS_IN_MEMORY)
								/* Align longs in Memory */
								if (rd->memuse & 1)
									rd->memuse++;
#endif
								v->regoff = rd->memuse;
								rd->memuse += memneeded + 1;
							} else
#endif /* defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE...GISTERS) */
								if (intalloc >= 0) {
		       /* Reuse memory slot(s)/register(s) for shared interface slots */
									v->flags |= 
										rd->interfaces[s][intalloc].flags 
										& INMEMORY;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
									if (!(v->flags & INMEMORY) 
										&& IS_2_WORD_TYPE(intalloc))
										v->regoff = GET_LOW_REG(
											rd->interfaces[s][intalloc].regoff);
									else
#endif
										v->regoff = 
										    rd->interfaces[s][intalloc].regoff;
								} else 
									if (rd->argintreguse + intregsneeded 
										< INT_ARG_CNT) {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (intregsneeded) 
											v->regoff=PACK_REGS( 
										  rd->argintregs[rd->argintreguse],
										  rd->argintregs[rd->argintreguse + 1]);
										else
#endif
											v->regoff = 
											   rd->argintregs[rd->argintreguse];
										rd->argintreguse += intregsneeded + 1;
									}
									else if (rd->tmpintreguse > intregsneeded) {
										rd->tmpintreguse -= intregsneeded + 1;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (intregsneeded) 
											v->regoff=PACK_REGS( 
										  rd->tmpintregs[rd->tmpintreguse],
										  rd->tmpintregs[rd->tmpintreguse + 1]);
										else
#endif
											v->regoff = 
											   rd->tmpintregs[rd->tmpintreguse];
									}
									else if (rd->savintreguse > intregsneeded) {
										rd->savintreguse -= intregsneeded + 1;
										v->regoff = 
											rd->savintregs[rd->savintreguse];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (intregsneeded) 
											v->regoff=PACK_REGS( 
										  rd->savintregs[rd->savintreguse],
										  rd->savintregs[rd->savintreguse + 1]);
										else
#endif
											v->regoff = 
											   rd->savintregs[rd->savintreguse];
									}
									else {
										v->flags |= INMEMORY;
#if defined(ALIGN_LONGS_IN_MEMORY)
										/* Align longs in Memory */
										if ( (memneeded) && (rd->memuse & 1))
											rd->memuse++;
#endif
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
#if defined(ALIGN_DOUBLES_IN_MEMORY)
									/* Align doubles in Memory */
									if ( (memneeded) && (rd->memuse & 1))
										rd->memuse++;
#endif
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
#if defined(ALIGN_LONGS_IN_MEMORY)
								/* Align longs in Memory */
								if (rd->memuse & 1)
									rd->memuse++;
#endif
								v->regoff = rd->memuse;
								rd->memuse += memneeded + 1;
							} else
#endif
							{
								if (intalloc >= 0) {
									v->flags |= 
								   rd->interfaces[s][intalloc].flags & INMEMORY;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
									if (!(v->flags & INMEMORY)
										&& IS_2_WORD_TYPE(intalloc))
										v->regoff =
											GET_LOW_REG(
											rd->interfaces[s][intalloc].regoff);
									else
#endif
										v->regoff =
											rd->interfaces[s][intalloc].regoff;
								} else {
									if (rd->savintreguse > intregsneeded) {
										rd->savintreguse -= intregsneeded + 1;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (intregsneeded) 
											v->regoff = PACK_REGS( 
										  rd->savintregs[rd->savintreguse],
										  rd->savintregs[rd->savintreguse + 1]);
										else
#endif
											v->regoff =
											   rd->savintregs[rd->savintreguse];
									} else {
										v->flags |= INMEMORY;
#if defined(ALIGN_LONGS_IN_MEMORY)
									/* Align longs in Memory */
									if ( (memneeded) && (rd->memuse & 1))
										rd->memuse++;
#endif
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


/* local_regalloc **************************************************************

   Allocates registers for all local variables.
	
*******************************************************************************/
	
static void local_regalloc(jitdata *jd)
{
	methodinfo   *m;
	codeinfo     *code;
	codegendata  *cd;
	registerdata *rd;

	int     p, s, t, tt;
	int     intalloc, fltalloc;
	varinfo *v;
	int     intregsneeded = 0;
	int     memneeded = 0;
	int     typeloop[] = { TYPE_LNG, TYPE_DBL, TYPE_INT, TYPE_FLT, TYPE_ADR };
	int     fargcnt, iargcnt;
#ifdef HAS_ADDRESS_REGISTER_FILE
	int     aargcnt;
#endif

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	rd   = jd->rd;

	if (code->isleafmethod) {
		methoddesc *md = m->parseddesc;

		iargcnt = rd->argintreguse;
		fargcnt = rd->argfltreguse;
#ifdef HAS_ADDRESS_REGISTER_FILE
		aargcnt = rd->argadrreguse;
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
					else if (rd->tmpadrreguse > 0) {
						v->flags = 0;
						v->regoff = rd->tmpadrregs[--rd->tmpadrreguse];
					}
					/* use unused argument registers as local registers */
					else if ((p >= md->paramcount) &&
							 (aargcnt < ADR_ARG_CNT)) {
						v->flags = 0;
						v->regoff = rd->argadrregs[aargcnt++];
					}
					else if (rd->savadrreguse > 0) {
						v->flags = 0;
						v->regoff = rd->savadrregs[--rd->savadrreguse];
					}
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
#if !defined(SUPPORT_PASS_FLOATARGS_IN_INTREGS)
						/* We can only use float arguments as local variables,
						 * if we do not pass them in integer registers. */
  						else if ((p < md->paramcount) &&
								 !md->params[p].inmemory) {
							v->flags = 0;
							v->regoff = rd->argfltregs[md->params[p].regoff];
						}
#endif
						else if (rd->tmpfltreguse > 0) {
							v->flags = 0;
							v->regoff = rd->tmpfltregs[--rd->tmpfltreguse];
						}
						/* use unused argument registers as local registers */
						else if ((p >= md->paramcount) &&
								 (fargcnt < FLT_ARG_CNT)) {
							v->flags = 0;
							v->regoff = rd->argfltregs[fargcnt];
							fargcnt++;
						}
						else if (rd->savfltreguse > 0) {
							v->flags = 0;
							v->regoff = rd->savfltregs[--rd->savfltreguse];
						}
						else {
							v->flags = INMEMORY;
#if defined(ALIGN_DOUBLES_IN_MEMORY)
							/* Align doubles in Memory */
							if ( (memneeded) && (rd->memuse & 1))
								rd->memuse++;
#endif
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
#if defined(ALIGN_LONGS_IN_MEMORY)
							/* Align longs in Memory */
							if (rd->memuse & 1)
								rd->memuse++;
#endif
							v->regoff = rd->memuse;
							rd->memuse += memneeded + 1;
						} else 
#endif
						{
							if (intalloc >= 0) {
								v->flags = rd->locals[s][intalloc].flags;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (!(v->flags & INMEMORY)
									&& IS_2_WORD_TYPE(intalloc))
									v->regoff = GET_LOW_REG(
										    rd->locals[s][intalloc].regoff);
								else
#endif
									v->regoff = rd->locals[s][intalloc].regoff;
							}
							else if ((p < md->paramcount) && 
									 !md->params[p].inmemory) {
								v->flags = 0;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (IS_2_WORD_TYPE(t))
									v->regoff = PACK_REGS(
							rd->argintregs[GET_LOW_REG(md->params[p].regoff)],
							rd->argintregs[GET_HIGH_REG(md->params[p].regoff)]);
									else
#endif
										v->regoff =
									       rd->argintregs[md->params[p].regoff];
							}
							else if (rd->tmpintreguse > intregsneeded) {
								rd->tmpintreguse -= intregsneeded + 1;
								v->flags = 0;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (intregsneeded) 
									v->regoff = PACK_REGS(
									    rd->tmpintregs[rd->tmpintreguse],
										rd->tmpintregs[rd->tmpintreguse + 1]);
								else
#endif
									v->regoff = 
										rd->tmpintregs[rd->tmpintreguse];
							}
							/*
							 * use unused argument registers as local registers
							 */
							else if ((p >= m->parseddesc->paramcount) &&
									 (iargcnt + intregsneeded < INT_ARG_CNT)) {
								v->flags = 0;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (intregsneeded) 
									v->regoff=PACK_REGS( 
												   rd->argintregs[iargcnt],
												   rd->argintregs[iargcnt + 1]);
								else
#endif
									v->regoff = rd->argintregs[iargcnt];
  								iargcnt += intregsneeded + 1;
							}
							else if (rd->savintreguse > intregsneeded) {
								rd->savintreguse -= intregsneeded + 1;
								v->flags = 0;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (intregsneeded) 
									v->regoff = PACK_REGS(
									    rd->savintregs[rd->savintreguse],
										rd->savintregs[rd->savintreguse + 1]);
								else
#endif
									v->regoff =rd->savintregs[rd->savintreguse];
							}
							else {
								v->flags = INMEMORY;
#if defined(ALIGN_LONGS_IN_MEMORY)
								/* Align longs in Memory */
								if ( (memneeded) && (rd->memuse & 1))
									rd->memuse++;
#endif
								v->regoff = rd->memuse;
								rd->memuse += memneeded + 1;
							}
						}
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
#if defined(ALIGN_DOUBLES_IN_MEMORY)
						/* Align doubles in Memory */
						if ( (memneeded) && (rd->memuse & 1))
							rd->memuse++;
#endif
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
#if defined(ALIGN_LONGS_IN_MEMORY)
						/* Align longs in Memory */
						if (rd->memuse & 1)
							rd->memuse++;
#endif
						v->regoff = rd->memuse;
						rd->memuse += memneeded + 1;
					} else {
#endif
						if (intalloc >= 0) {
							v->flags = rd->locals[s][intalloc].flags;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (!(v->flags & INMEMORY)
								&& IS_2_WORD_TYPE(intalloc))
								v->regoff = GET_LOW_REG(
									    rd->locals[s][intalloc].regoff);
							else
#endif
								v->regoff = rd->locals[s][intalloc].regoff;
						}
						else if (rd->savintreguse > intregsneeded) {
							rd->savintreguse -= intregsneeded+1;
							v->flags = 0;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (intregsneeded) 
									v->regoff = PACK_REGS(
										rd->savintregs[rd->savintreguse],
									    rd->savintregs[rd->savintreguse + 1]);
								else
#endif
									v->regoff =rd->savintregs[rd->savintreguse];
						}
						else {
							v->flags = INMEMORY;
#if defined(ALIGN_LONGS_IN_MEMORY)
							/* Align longs in Memory */
							if ( (memneeded) && (rd->memuse & 1))
								rd->memuse++;
#endif
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
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded)
								s->regoff = PACK_REGS(
							        rd->freesavintregs[rd->freesavinttop],
									rd->freesavintregs[rd->freesavinttop + 1]);
						else
#endif
								s->regoff =
									rd->freesavintregs[rd->freesavinttop];
							return;
						} else if (rd->savintreguse > intregsneeded) {
							rd->savintreguse -= intregsneeded + 1;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded)
								s->regoff = PACK_REGS(
									rd->savintregs[rd->savintreguse],
							        rd->savintregs[rd->savintreguse + 1]);
							else
#endif
								s->regoff = rd->savintregs[rd->savintreguse];
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
							s->flags |= TMPARG;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								s->regoff = PACK_REGS(
									rd->freeargintregs[rd->freearginttop],
							        rd->freeargintregs[rd->freearginttop + 1]);
							else
#endif
								s->regoff =
									rd->freeargintregs[rd->freearginttop];
							return;
						} else if (rd->argintreguse 
								   < INT_ARG_CNT - intregsneeded) {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								s->regoff = PACK_REGS(
									rd->argintregs[rd->argintreguse],
								    rd->argintregs[rd->argintreguse + 1]);
							else
#endif
								s->regoff = rd->argintregs[rd->argintreguse];
							s->flags |= TMPARG;
							rd->argintreguse += intregsneeded + 1;
							return;
						} else if (rd->freetmpinttop > intregsneeded) {
							rd->freetmpinttop -= intregsneeded + 1;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								s->regoff = PACK_REGS(
									rd->freetmpintregs[rd->freetmpinttop],
								    rd->freetmpintregs[rd->freetmpinttop + 1]);
							else
#endif
								s->regoff = rd->freetmpintregs[rd->freetmpinttop];
							return;
						} else if (rd->tmpintreguse > intregsneeded) {
							rd->tmpintreguse -= intregsneeded + 1;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								s->regoff = PACK_REGS(
									rd->tmpintregs[rd->tmpintreguse],
								    rd->tmpintregs[rd->tmpintreguse + 1]);
							else
#endif
								s->regoff = rd->tmpintregs[rd->tmpintreguse];
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
#if defined(ALIGN_LONGS_IN_MEMORY) || defined(ALIGN_DOUBLES_IN_MEMORY)
			/* align 2 Word Types */
			if ((memneeded) && ((rd->memuse & 1) == 1)) { 
				/* Put patched memory slot on freemem */
				rd->freemem[rd->freememtop++] = rd->memuse;
				rd->memuse++;
			}
#endif /* defined(ALIGN_LONGS_IN_MEMORY) || defined(ALIGN_DOUBLES_IN_MEMORY) */
			s->regoff = rd->memuse;
			rd->memuse += memneeded + 1;
		}
	s->flags |= INMEMORY;
}


#define reg_free_temp(rd,s) if (((s)->varkind == TEMPVAR) && (!((s)->flags & STCOPY))) reg_free_temp_func(rd, (s))

/* Do not free regs/memory locations used by Stackslots flagged STCOPY! There is still another Stackslot */
/* alive using this reg/memory location */

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
/* 			s->flags &= ~SAVEDTMP; */
			rd->freesavadrregs[rd->freesavadrtop++] = s->regoff;
		} else
			rd->freetmpadrregs[rd->freetmpadrtop++] = s->regoff;
#endif
	} else if (IS_FLT_DBL_TYPE(s->type)) {
		if (s->flags & (SAVEDVAR | SAVEDTMP)) {
/* 			s->flags &= ~SAVEDTMP; */
			rd->freesavfltregs[rd->freesavflttop++] = s->regoff;
		} else if (s->flags & TMPARG) {
/* 			s->flags &= ~TMPARG; */
			rd->freeargfltregs[rd->freeargflttop++] = s->regoff;
		} else
			rd->freetmpfltregs[rd->freetmpflttop++] = s->regoff;
	} else { /* IS_INT_LNG_TYPE */
		if (s->flags & (SAVEDVAR | SAVEDTMP)) {
/* 			s->flags &= ~SAVEDTMP; */
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
			if (intregsneeded) {
				rd->freesavintregs[rd->freesavinttop] =
					GET_LOW_REG(s->regoff);
				rd->freesavintregs[rd->freesavinttop + 1] =
					GET_HIGH_REG(s->regoff);
			} else
#endif
				rd->freesavintregs[rd->freesavinttop] = s->regoff;
			rd->freesavinttop += intregsneeded + 1;

		} else if (s->flags & TMPARG) {
/* 			s->flags &= ~TMPARG; */
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
			if (intregsneeded) {
				rd->freeargintregs[rd->freearginttop] =
					GET_LOW_REG(s->regoff);
				rd->freeargintregs[rd->freearginttop + 1] =
					GET_HIGH_REG(s->regoff);
			} else 
#endif
				rd->freeargintregs[rd->freearginttop] = s->regoff;
			rd->freearginttop += intregsneeded + 1;
		} else {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
			if (intregsneeded) {
				rd->freetmpintregs[rd->freetmpinttop] =
					GET_LOW_REG(s->regoff);
				rd->freetmpintregs[rd->freetmpinttop + 1] =
					GET_HIGH_REG(s->regoff);
			} else
#endif
				rd->freetmpintregs[rd->freetmpinttop] = s->regoff;
			rd->freetmpinttop += intregsneeded + 1;
		}
	}
}

static bool reg_alloc_dup(stackptr src, stackptr dst) {
	/* only copy TEMPVARS, do not mess with STACKVAR,      */
	/* LOCALVAR, or ARGVAR        */
	if ((src->varkind == TEMPVAR) && (dst->varkind == TEMPVAR)) {
		/* can not allocate a REG_TMP to a REG_SAV Slot */
		if (src->flags & INMEMORY) {
			dst->regoff  = src->regoff;
			dst->flags |= INMEMORY;
			return true;
		} else if ((src->flags & SAVEDVAR) == (dst->flags & SAVEDVAR)) {
			dst->regoff  = src->regoff;
			dst->flags |= src->flags & SAVEDTMP;
			dst->flags |= src->flags & TMPARG;
			return true;
#if 0
		} else if ((dst->flags & SAVEDVAR) == 0) {
			/* can only use a REG_SAV as REG_TMP! */
			dst->regoff = src->regoff;
			dst->flags |= src->flags & TMPARG;
			dst->flags |= SAVEDTMP;
			return true;
#endif
		} 
	}
	/* no copy possible - allocate a new reg/memory location*/
	return false;
}

/* Mark the copies (STCOPY) at the dst stack right for DUPx and SWAP */
static void new_reg_mark_copy(registerdata *rd, stackptr *dupslots, 
							  int nin, int nout, int nthrough)
{
	s4 src_regoff[4];
	s4 src_flags[4];
	stackptr dst_stackslots[6];
	int s_bottom, d_bottom, i, j, slots;
	bool found;
	stackptr sp;
	stackptr *argp;

	assert(nin <= 4 && (nout + nthrough) <= 6);

	/* remember all different Registers/Memory Location of used TEMPVAR       */
	/* instacks in src_varnum[] and src_flags[] _uniquely_. Take the STCOPY   */
	/* flag of the last (deepest) occurence */
	slots = nin;
	argp = dupslots + slots;
	for (s_bottom = 4; slots--; ) {
		sp = *--argp;
		if (sp->varkind == TEMPVAR) {
			found = false;
			for (i = 3; i >= s_bottom; i--) {
				if ((src_regoff[i] == sp->regoff) && 
					((src_flags[i] & INMEMORY) == (sp->flags & INMEMORY)) ) 
				{
					src_flags[i] &= (~STCOPY | (sp->flags & STCOPY));
					found = true;
				}
			}
			if (!found) {
				s_bottom--;
				src_regoff[s_bottom] = sp->regoff;
				src_flags[s_bottom] = sp->flags;
			}
		}
	}

	/* Remember used TEMPVAR dst Stackslots in dst_stackslots[], since they   */
	/* have to be from the "lowest" upwards, and the stackelements list is    */
	/* linked from only top downwards */
	
	slots = nthrough + nout;
	argp = dupslots + nin + nout;
	for (d_bottom = 6; slots--; ) {
		sp = *--argp;
		if (sp->varkind == TEMPVAR) {
			d_bottom--;
			dst_stackslots[d_bottom] = sp;
		}
	}

	/* Mark all reused reg/mem in dst stacklots with STCOPY, if the           */
	/* corresponding src stackslot was marked STCOPY*/
	/* if the correspondig STCOPY from the src stackslot was not set, do not  */
	/* mark the lowest occurence at dst stackslots */
	/* mark in src_flag reg/mem with STKEEP, if they where reused in the dst  */
	/* stacklots, so they are not freed afterwards */
	for (i = d_bottom; i < 6; i++) {
		for (j = s_bottom; j < 4; j++) {
			if ( (src_regoff[j] == dst_stackslots[i]->regoff) &&
				 ((src_flags[j] & INMEMORY) == (dst_stackslots[i]->flags & INMEMORY)) ) 
			{
				if (src_flags[j] & STCOPY) {
					dst_stackslots[i]->flags |= STCOPY;
				}
				else {
					src_flags[j] |= STCOPY;
					dst_stackslots[i]->flags &= ~STCOPY;
				}
				/* do not free reg/mem of src Stackslot */
				src_flags[j] |= STKEEP;
			}
		}
	}

	/* free all reg/mem of src stack, which where not marked with STKEEP */
	for (j=s_bottom; j < 4; j++) {
		if ((src_flags[j] & STKEEP)==0) {
			/* free, if STCOPY of src stackslot is not set */
			/* STCOPY is already checked in reg_free_temp macro! */
			slots = nin;
			argp = dupslots + slots;
			while (--slots) {
				sp = *--argp;
				if ((src_regoff[j] == sp->regoff) && 
					((src_flags[j] & INMEMORY) == (sp->flags & INMEMORY)) ) 
				{
					reg_free_temp(rd, sp);
				}
			}
		}
	}
}

/* Mark the copies (STCOPY) at the dst stack right for DUPx and SWAP */
static void reg_mark_copy(registerdata *rd, stackptr src_top, stackptr src_bottom, stackptr dst_top, stackptr dst_bottom) {
	s4 src_regoff[4];
	s4 src_flags[4];
	stackptr dst_stackslots[6];
	int s_bottom, d_bottom, i, j;
	bool found;

	stackptr sp;

	/* remember all different Registers/Memory Location of used TEMPVAR       */
	/* instacks in src_varnum[] and src_flags[] _uniquely_. Take the STCOPY   */
	/* flag of the last (deepest) occurence */
	for(s_bottom = 4, sp = src_top; sp >= src_bottom; sp = sp->prev) {
		if (sp->varkind == TEMPVAR) {
			found = false;
			for( i = 3; i >= s_bottom; i--) {
				if ((src_regoff[i] == sp->regoff) && 
					((src_flags[i] & INMEMORY) == (sp->flags & INMEMORY)) ) {
					src_flags[i] &= (~STCOPY | (sp->flags & STCOPY));
					found = true;
				}
			}
			if (!found) {
				s_bottom--;
				src_regoff[s_bottom] = sp->regoff;
				src_flags[s_bottom] = sp->flags;
			}
		}
	}

	/* Remember used TEMPVAR dst Stackslots in dst_stackslots[], since they   */
	/* have to be from the "lowest" upwards, and the stackelements list is    */
	/* linked from only top downwards */
	
	for(d_bottom = 6, sp =dst_top; sp >= dst_bottom; sp = sp->prev) {
		if (sp->varkind == TEMPVAR) {
			d_bottom--;
			dst_stackslots[d_bottom] = sp;
		}
	}

	/* Mark all reused reg/mem in dst stacklots with STCOPY, if the           */
	/* corresponding src stackslot was marked STCOPY*/
	/* if the correspondig STCOPY from the src stackslot was not set, do not  */
	/* mark the lowest occurence at dst stackslots */
	/* mark in src_flag reg/mem with STKEEP, if they where reused in the dst  */
	/* stacklots, so they are not freed afterwards */
	for(i = d_bottom; i < 6; i++) {
		for(j = s_bottom; j < 4; j++) {
			if ( (src_regoff[j] == dst_stackslots[i]->regoff) &&
				 ((src_flags[j] & INMEMORY) == (dst_stackslots[i]->flags & INMEMORY)) ) {
				if (src_flags[j] & STCOPY)
					dst_stackslots[i]->flags |= STCOPY;
				else {
					src_flags[j] |= STCOPY;
					dst_stackslots[i]->flags &= ~STCOPY;
				}
				/* do not free reg/mem of src Stackslot */
				src_flags[j] |= STKEEP;
			}
		}
	}

	/* free all reg/mem of src stack, which where not marked with STKEEP */
	for(j=s_bottom; j < 4; j++) {
		if ((src_flags[j] & STKEEP)==0) {
			/* free, if STCOPY of src stackslot is not set */
			/* STCOPY is already checked in reg_free_temp macro! */
			for(sp = src_top; sp >= src_bottom; sp = sp->prev)
				if ((src_regoff[j] == sp->regoff) && 
					((src_flags[j] & INMEMORY) == (sp->flags & INMEMORY)) ) {
					reg_free_temp(rd, sp);
				}
		}
	}
}


/* allocate_scratch_registers **************************************************

   Allocate temporary (non-interface, non-local) registers.

*******************************************************************************/

static void new_allocate_scratch_registers(jitdata *jd)
{
	methodinfo         *m;
	registerdata       *rd;
	s4                  i;
	s4                  len;
	new_instruction    *iptr;
	basicblock         *bptr;
	builtintable_entry *bte;
	methoddesc         *md;
	stackptr           *argp;

	/* get required compiler data */

	m  = jd->m;
	rd = jd->rd;

	/* initialize temp registers */
	reg_init_temp(m, rd);

	bptr = jd->new_basicblocks;

	while (bptr != NULL) {
		if (bptr->flags >= BBREACHED) {
			iptr = /* XXX */ (new_instruction *) bptr->iinstr;
			len = bptr->icount;

			while (--len >= 0)  {
				switch (iptr->opc) {

					/* pop 0 push 0 */

				case ICMD_NOP:
				case ICMD_ELSE_ICONST:
				case ICMD_CHECKNULL:
				case ICMD_IINC:
				case ICMD_JSR:
				case ICMD_RET:
				case ICMD_RETURN:
				case ICMD_GOTO:
				case ICMD_PUTSTATICCONST:
				case ICMD_INLINE_START:
				case ICMD_INLINE_END:
				case ICMD_INLINE_GOTO:
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
					reg_new_temp(rd, iptr->dst.var);
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
					reg_free_temp(rd, iptr->sx.s23.s2.var);
					reg_free_temp(rd, iptr->s1.var);
					reg_new_temp(rd, iptr->dst.var);
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
					reg_free_temp(rd, iptr->sx.s23.s3.var);
					reg_free_temp(rd, iptr->sx.s23.s2.var);
					reg_free_temp(rd, iptr->s1.var);
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
					reg_free_temp(rd, iptr->s1.var);
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

				case ICMD_IF_FCMPEQ:
				case ICMD_IF_FCMPNE:

				case ICMD_IF_FCMPL_LT:
				case ICMD_IF_FCMPL_GE:
				case ICMD_IF_FCMPL_GT:
				case ICMD_IF_FCMPL_LE:

				case ICMD_IF_FCMPG_LT:
				case ICMD_IF_FCMPG_GE:
				case ICMD_IF_FCMPG_GT:
				case ICMD_IF_FCMPG_LE:

				case ICMD_IF_DCMPEQ:
				case ICMD_IF_DCMPNE:

				case ICMD_IF_DCMPL_LT:
				case ICMD_IF_DCMPL_GE:
				case ICMD_IF_DCMPL_GT:
				case ICMD_IF_DCMPL_LE:

				case ICMD_IF_DCMPG_LT:
				case ICMD_IF_DCMPG_GE:
				case ICMD_IF_DCMPG_GT:
				case ICMD_IF_DCMPG_LE:

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
					reg_free_temp(rd, iptr->sx.s23.s2.var);
					reg_free_temp(rd, iptr->s1.var);
					break;

					/* pop 0 push 1 dup */
					
				case ICMD_DUP:
					/* src === dst->prev (identical Stackslot Element)     */
					/* src --> dst       (copied value, take same reg/mem) */

					if (!reg_alloc_dup(iptr->s1.var, iptr->dst.var)) {
						reg_new_temp(rd, iptr->dst.var);
					} else {
						iptr->dst.var->flags |= STCOPY;
					}
					break;

					/* pop 0 push 2 dup */
					
				case ICMD_DUP2:
					/* src->prev === dst->prev->prev->prev (identical Stackslot Element)     */
					/* src       === dst->prev->prev       (identical Stackslot Element)     */
					/* src->prev --> dst->prev             (copied value, take same reg/mem) */
					/* src       --> dst                   (copied value, take same reg/mem) */
												
					if (!reg_alloc_dup(iptr->dst.dupslots[0], iptr->dst.dupslots[2+0]))
						reg_new_temp(rd, iptr->dst.dupslots[2+0]);
					if (!reg_alloc_dup(iptr->dst.dupslots[1], iptr->dst.dupslots[2+1]))
						reg_new_temp(rd, iptr->dst.dupslots[2+1]);
					new_reg_mark_copy(rd, iptr->dst.dupslots, 2, 2, 2);
					break;

					/* pop 2 push 3 dup */
					
				case ICMD_DUP_X1:
					/* src->prev --> dst->prev       (copied value, take same reg/mem) */
					/* src       --> dst             (copied value, take same reg/mem) */
					/* src       --> dst->prev->prev (copied value, take same reg/mem) */
												
					if (!reg_alloc_dup(iptr->dst.dupslots[2], iptr->dst.dupslots[2+0]))
						reg_new_temp(rd, iptr->dst.dupslots[2+0]);
					if (!reg_alloc_dup(iptr->dst.dupslots[1], iptr->dst.dupslots[2+2]))
						reg_new_temp(rd, iptr->dst.dupslots[2+2]);
					if (!reg_alloc_dup(iptr->dst.dupslots[0], iptr->dst.dupslots[2+1]))
						reg_new_temp(rd, iptr->dst.dupslots[2+1]);
					new_reg_mark_copy(rd, iptr->dst.dupslots, 2, 3, 0);
					break;

					/* pop 3 push 4 dup */
					
				case ICMD_DUP_X2:
					/* src->prev->prev --> dst->prev->prev        */
					/* src->prev       --> dst->prev              */
					/* src             --> dst                    */
					/* src             --> dst->prev->prev->prev  */
					
					if (!reg_alloc_dup(iptr->dst.dupslots[2], iptr->dst.dupslots[3+0]))
						reg_new_temp(rd, iptr->dst.dupslots[3+0]);
					if (!reg_alloc_dup(iptr->dst.dupslots[2], iptr->dst.dupslots[3+3]))
						reg_new_temp(rd, iptr->dst.dupslots[3+3]);
					if (!reg_alloc_dup(iptr->dst.dupslots[1], iptr->dst.dupslots[3+2]))
						reg_new_temp(rd, iptr->dst.dupslots[3+2]);
					if (!reg_alloc_dup(iptr->dst.dupslots[0], iptr->dst.dupslots[3+1]))
						reg_new_temp(rd, iptr->dst.dupslots[3+1]);
					new_reg_mark_copy(rd, iptr->dst.dupslots, 3, 4, 0);
					break;

					/* pop 3 push 5 dup */
					
				case ICMD_DUP2_X1:
					/* src->prev->prev --> dst->prev->prev             */
					/* src->prev       --> dst->prev                   */
					/* src             --> dst                         */
					/* src->prev       --> dst->prev->prev->prev->prev */
					/* src             --> dst->prev->prev->prev       */
												
					if (!reg_alloc_dup(iptr->dst.dupslots[2], iptr->dst.dupslots[3+1]))
						reg_new_temp(rd, iptr->dst.dupslots[3+1]);
					if (!reg_alloc_dup(iptr->dst.dupslots[2], iptr->dst.dupslots[3+4]))
						reg_new_temp(rd, iptr->dst.dupslots[3+4]);
					if (!reg_alloc_dup(iptr->dst.dupslots[1], iptr->dst.dupslots[3+0]))
						reg_new_temp(rd, iptr->dst.dupslots[3+0]);
					if (!reg_alloc_dup(iptr->dst.dupslots[1], iptr->dst.dupslots[3+3]))
						reg_new_temp(rd, iptr->dst.dupslots[3+3]);
					if (!reg_alloc_dup(iptr->dst.dupslots[0], iptr->dst.dupslots[3+2]))
						reg_new_temp(rd, iptr->dst.dupslots[3+2]);
					new_reg_mark_copy(rd, iptr->dst.dupslots, 3, 5, 0);
					break;

					/* pop 4 push 6 dup */
					
				case ICMD_DUP2_X2:
					/* src->prev->prev->prev --> dst->prev->prev->prev             */
					/* src->prev->prev       --> dst->prev->prev                   */
					/* src->prev             --> dst->prev                         */
					/* src                   --> dst                               */
					/* src->prev             --> dst->prev->prev->prev->prev->prev */
					/* src                   --> dst->prev->prev->prev->prev       */
												
					if (!reg_alloc_dup(iptr->dst.dupslots[3], iptr->dst.dupslots[4+1]))
						reg_new_temp(rd, iptr->dst.dupslots[4+1]);
					if (!reg_alloc_dup(iptr->dst.dupslots[3], iptr->dst.dupslots[4+5]))
						reg_new_temp(rd, iptr->dst.dupslots[4+5]);
					if (!reg_alloc_dup(iptr->dst.dupslots[2], iptr->dst.dupslots[4+0]))
						reg_new_temp(rd, iptr->dst.dupslots[4+0]);
					if (!reg_alloc_dup(iptr->dst.dupslots[2], iptr->dst.dupslots[4+4]))
						reg_new_temp(rd, iptr->dst.dupslots[4+4]);
					if (!reg_alloc_dup(iptr->dst.dupslots[1], iptr->dst.dupslots[4+3]))
						reg_new_temp(rd, iptr->dst.dupslots[4+3]);
					if (!reg_alloc_dup(iptr->dst.dupslots[0], iptr->dst.dupslots[4+2]))
						reg_new_temp(rd, iptr->dst.dupslots[4+2]);
					new_reg_mark_copy(rd, iptr->dst.dupslots, 4, 6, 0);
					break;

					/* pop 2 push 2 swap */
					
				case ICMD_SWAP:
					/* src       --> dst->prev   (copy) */
					/* src->prev --> dst         (copy) */
												
					if (!reg_alloc_dup(iptr->dst.dupslots[1], iptr->dst.dupslots[+0]))
						reg_new_temp(rd, iptr->dst.dupslots[2+0]);
					if (!reg_alloc_dup(iptr->dst.dupslots[0], iptr->dst.dupslots[+1]))
						reg_new_temp(rd, iptr->dst.dupslots[2+1]);
					new_reg_mark_copy(rd, iptr->dst.dupslots, 2, 2, 0);
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
					reg_free_temp(rd, iptr->sx.s23.s2.var);
					reg_free_temp(rd, iptr->s1.var);
					reg_new_temp(rd, iptr->dst.var);
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
					reg_free_temp(rd, iptr->s1.var);
					reg_new_temp(rd, iptr->dst.var);
					break;

					/* pop 0 push 1 */
					
				case ICMD_GETSTATIC:

				case ICMD_NEW:
					reg_new_temp(rd, iptr->dst.var);
					break;

					/* pop many push any */
					
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKEINTERFACE:
					NEW_INSTRUCTION_GET_METHODDESC(iptr,md);
					i = md->paramcount;
					argp = iptr->sx.s23.s2.args;
					while (--i >= 0) {
						reg_free_temp(rd, *argp);
						argp++;
					}
					if (md->returntype.type != TYPE_VOID)
						reg_new_temp(rd, iptr->dst.var);
					break;

				case ICMD_BUILTIN:
					bte = iptr->sx.s23.s3.bte;
					md = bte->md;
					i = md->paramcount;
					argp = iptr->sx.s23.s2.args;
					while (--i >= 0) {
						reg_free_temp(rd, *argp);
						argp++;
					}
					if (md->returntype.type != TYPE_VOID)
						reg_new_temp(rd, iptr->dst.var);
					break;

				case ICMD_MULTIANEWARRAY:
					i = iptr->s1.argcount;
					argp = iptr->sx.s23.s2.args;
					while (--i >= 0) {
						reg_free_temp(rd, *argp);
						argp++;
					}
					reg_new_temp(rd, iptr->dst.var);
					break;

				default:
					*exceptionptr =
						new_internalerror("Unknown ICMD %d during register allocation",
										  iptr->opc);
					return;
				} /* switch */
				iptr++;
			} /* while instructions */
		} /* if */
		bptr = bptr->next;
	} /* while blocks */
}

static void allocate_scratch_registers(jitdata *jd)
{
	methodinfo         *m;
	registerdata       *rd;
	s4                  i;
	s4                  len;
	stackptr            src;
	stackptr            dst;
	instruction        *iptr;
	basicblock         *bptr;
	builtintable_entry *bte;
	methoddesc         *md;

	/* get required compiler data */

	m  = jd->m;
	rd = jd->rd;

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

				switch (iptr->opc) {

					/* pop 0 push 0 */

				case ICMD_NOP:
				case ICMD_ELSE_ICONST:
				case ICMD_CHECKNULL:
				case ICMD_IINC:
				case ICMD_JSR:
				case ICMD_RET:
				case ICMD_RETURN:
				case ICMD_GOTO:
				case ICMD_PUTSTATICCONST:
				case ICMD_INLINE_START:
				case ICMD_INLINE_END:
				case ICMD_INLINE_GOTO:
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

				case ICMD_IF_FCMPEQ:
				case ICMD_IF_FCMPNE:

				case ICMD_IF_FCMPL_LT:
				case ICMD_IF_FCMPL_GE:
				case ICMD_IF_FCMPL_GT:
				case ICMD_IF_FCMPL_LE:

				case ICMD_IF_FCMPG_LT:
				case ICMD_IF_FCMPG_GE:
				case ICMD_IF_FCMPG_GT:
				case ICMD_IF_FCMPG_LE:

				case ICMD_IF_DCMPEQ:
				case ICMD_IF_DCMPNE:

				case ICMD_IF_DCMPL_LT:
				case ICMD_IF_DCMPL_GE:
				case ICMD_IF_DCMPL_GT:
				case ICMD_IF_DCMPL_LE:

				case ICMD_IF_DCMPG_LT:
				case ICMD_IF_DCMPG_GE:
				case ICMD_IF_DCMPG_GT:
				case ICMD_IF_DCMPG_LE:

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
					/* src === dst->prev (identical Stackslot Element)     */
					/* src --> dst       (copied value, take same reg/mem) */

					if (!reg_alloc_dup(src, dst)) {
						reg_new_temp(rd, dst);
					} else {
						dst->flags |= STCOPY;
					}
					break;

					/* pop 0 push 2 dup */
					
				case ICMD_DUP2:
					/* src->prev === dst->prev->prev->prev (identical Stackslot Element)     */
					/* src       === dst->prev->prev       (identical Stackslot Element)     */
					/* src->prev --> dst->prev             (copied value, take same reg/mem) */
					/* src       --> dst                   (copied value, take same reg/mem) */
												
					if (!reg_alloc_dup(src->prev, dst->prev))
						reg_new_temp(rd, dst->prev);
					if (!reg_alloc_dup(src, dst))
						reg_new_temp(rd, dst);
					reg_mark_copy(rd, src, src->prev, dst, dst->prev->prev->prev);
					break;

					/* pop 2 push 3 dup */
					
				case ICMD_DUP_X1:
					/* src->prev --> dst->prev       (copied value, take same reg/mem) */
					/* src       --> dst             (copied value, take same reg/mem) */
					/* src       --> dst->prev->prev (copied value, take same reg/mem) */
												
					if (!reg_alloc_dup(src, dst->prev->prev))
						reg_new_temp(rd, dst->prev->prev);
					if (!reg_alloc_dup(src, dst))
						reg_new_temp(rd, dst);
					if (!reg_alloc_dup(src->prev, dst->prev))
						reg_new_temp(rd, dst->prev);
					reg_mark_copy(rd, src, src->prev, dst, dst->prev->prev);
					break;

					/* pop 3 push 4 dup */
					
				case ICMD_DUP_X2:
					/* src->prev->prev --> dst->prev->prev        */
					/* src->prev       --> dst->prev              */
					/* src             --> dst                    */
					/* src             --> dst->prev->prev->prev  */
					
					if (!reg_alloc_dup(src, dst->prev->prev->prev))
						reg_new_temp(rd, dst->prev->prev->prev);
					if (!reg_alloc_dup(src, dst))
						reg_new_temp(rd, dst);
					if (!reg_alloc_dup(src->prev, dst->prev))
						reg_new_temp(rd, dst->prev);
					if (!reg_alloc_dup(src->prev->prev, dst->prev->prev))
						reg_new_temp(rd, dst->prev->prev);
					reg_mark_copy(rd, src, src->prev->prev, dst, dst->prev->prev->prev);
					break;

					/* pop 3 push 5 dup */
					
				case ICMD_DUP2_X1:
					/* src->prev->prev --> dst->prev->prev             */
					/* src->prev       --> dst->prev                   */
					/* src             --> dst                         */
					/* src->prev       --> dst->prev->prev->prev->prev */
					/* src             --> dst->prev->prev->prev       */
												
					if (!reg_alloc_dup(src, dst->prev->prev->prev))
						reg_new_temp(rd, dst->prev->prev->prev);
					if (!reg_alloc_dup(src, dst))
						reg_new_temp(rd, dst);
					if (!reg_alloc_dup(src->prev, dst->prev->prev->prev->prev))
						reg_new_temp(rd, dst->prev->prev->prev->prev);
					if (!reg_alloc_dup(src->prev, dst->prev))
						reg_new_temp(rd, dst->prev);
					if (!reg_alloc_dup(src->prev->prev, dst->prev->prev))
						reg_new_temp(rd, dst->prev->prev);
					reg_mark_copy(rd, src, src->prev->prev, dst, dst->prev->prev->prev->prev);
					break;

					/* pop 4 push 6 dup */
					
				case ICMD_DUP2_X2:
					/* src->prev->prev->prev --> dst->prev->prev->prev             */
					/* src->prev->prev       --> dst->prev->prev                   */
					/* src->prev             --> dst->prev                         */
					/* src                   --> dst                               */
					/* src->prev             --> dst->prev->prev->prev->prev->prev */
					/* src                   --> dst->prev->prev->prev->prev       */
												
					if (!reg_alloc_dup(src, dst->prev->prev->prev->prev))
						reg_new_temp(rd, dst->prev->prev->prev->prev);
					if (!reg_alloc_dup(src, dst))
						reg_new_temp(rd, dst);
					if (!reg_alloc_dup(src->prev, dst->prev->prev->prev->prev->prev))
						reg_new_temp(rd, dst->prev->prev->prev->prev->prev);
					if (!reg_alloc_dup(src->prev, dst->prev))
						reg_new_temp(rd, dst->prev);
					if (!reg_alloc_dup(src->prev->prev, dst->prev->prev))
						reg_new_temp(rd, dst->prev->prev);
					if (!reg_alloc_dup(src->prev->prev->prev, dst->prev->prev->prev))
						reg_new_temp(rd, dst->prev->prev->prev);
					reg_mark_copy(rd, src, src->prev->prev->prev, dst, dst->prev->prev->prev->prev->prev);
					break;

					/* pop 2 push 2 swap */
					
				case ICMD_SWAP:
					/* src       --> dst->prev   (copy) */
					/* src->prev --> dst         (copy) */
												
					if (!reg_alloc_dup(src, dst->prev))
						reg_new_temp(rd, dst->prev);
					if (!reg_alloc_dup(src->prev, dst))
						reg_new_temp(rd, dst);
					reg_mark_copy(rd, src, src->prev, dst, dst->prev);
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
					INSTRUCTION_GET_METHODDESC(iptr,md);
					i = md->paramcount;
					while (--i >= 0) {
						reg_free_temp(rd, src);
						src = src->prev;
					}
					if (md->returntype.type != TYPE_VOID)
						reg_new_temp(rd, dst);
					break;

				case ICMD_BUILTIN:
					bte = iptr->val.a;
					md = bte->md;
					i = md->paramcount;
					while (--i >= 0) {
						reg_free_temp(rd, src);
						src = src->prev;
					}
					if (md->returntype.type != TYPE_VOID)
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
					*exceptionptr =
						new_internalerror("Unknown ICMD %d during register allocation",
										  iptr->opc);
					return;
				} /* switch */
				iptr++;
			} /* while instructions */
		} /* if */
		bptr = bptr->next;
	} /* while blocks */
}


#if defined(ENABLE_STATISTICS)
void reg_make_statistics(jitdata *jd)
{
	methodinfo   *m;
	codegendata  *cd;
	registerdata *rd;
	int i,type;
	s4 len;
	stackptr    src, src_old;
	stackptr    dst;
	instruction *iptr;
	basicblock  *bptr;
	int size_interface; /* == maximum size of in/out stack at basic block boundaries */
	bool in_register;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

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

#if defined(ENABLE_LSRA)
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
#if defined(ENABLE_LSRA)
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
#endif /* defined(ENABLE_STATISTICS) */


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
 * vim:noexpandtab:sw=4:ts=4:
 */
