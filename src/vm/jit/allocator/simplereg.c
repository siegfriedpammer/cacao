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

   $Id: simplereg.c 5463 2006-09-11 14:37:06Z edwin $

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
	/* There is a problem with the use of unused float argument
	   registers in leafmethods for stackslots on c7 (2 * Dual Core
	   AMD Opteron(tm) Processor 270) - runtime for the jvm98 _mtrt
	   benchmark is heaviliy increased. This could be prevented by
	   setting rd->argfltreguse to FLT_ARG_CNT before calling
	   allocate_scratch_registers and setting it back to the original
	   value before calling local_regalloc.  */

	interface_regalloc(jd);
	new_allocate_scratch_registers(jd);
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
	codegendata  *cd;
	registerdata *rd;

	int     s, t, tt, saved;
	int     intalloc, fltalloc; /* Remember allocated Register/Memory offset */
	                /* in case more vars are packed into this interface slot */
	int		intregsneeded = 0;
	int		memneeded = 0;
    /* allocate LNG and DBL Types first to ensure 2 memory slots or registers */
	/* on HAS_4BYTE_STACKSLOT architectures */
	int     typeloop[] = { TYPE_LNG, TYPE_DBL, TYPE_INT, TYPE_FLT, TYPE_ADR };
	int flags, regoff;

	/* get required compiler data */

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	/* rd->memuse was already set in stack.c to allocate stack space
	   for passing arguments to called methods. */

#if defined(__I386__)
	if (checksync && (m->flags & ACC_SYNCHRONIZED)) {
		/* reserve 0(%esp) for Monitorenter/exit Argument on i386 */
		if (rd->memuse < 1)
			rd->memuse = 1;
	}
#endif

 	if (jd->isleafmethod) {
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

		saved = 0;

		for (tt = 0; tt <=4; tt++) {
			if ((t = jd->interface_map[s * 5 + tt].flags) != UNUSED) {
				saved |= t & SAVEDVAR;
			}
		}

		for (tt = 0; tt <= 4; tt++) {
			t = typeloop[tt];
			if (jd->interface_map[s * 5 + t].flags == UNUSED)
				continue;

			flags = saved;
			regoff = -1; /* XXX for debugging */

#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
				intregsneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
#if defined(HAS_4BYTE_STACKSLOT)
				memneeded = (IS_2_WORD_TYPE(t)) ? 1 : 0;
#endif
				if (!saved) {
#if defined(HAS_ADDRESS_REGISTER_FILE)
					if (IS_ADR_TYPE(t)) {
						if (!jd->isleafmethod 
							&&(rd->argadrreguse < ADR_ARG_CNT)) {
							regoff = rd->argadrregs[rd->argadrreguse++];
						} else if (rd->tmpadrreguse > 0) {
								regoff = rd->tmpadrregs[--rd->tmpadrreguse];
						} else if (rd->savadrreguse > 0) {
								regoff = rd->savadrregs[--rd->savadrreguse];
						} else {
							flags |= INMEMORY;
							regoff = rd->memuse++;
						}						
					} else /* !IS_ADR_TYPE */
#endif /* defined(HAS_ADDRESS_REGISTER_FILE) */
					{
						if (IS_FLT_DBL_TYPE(t)) {
							if (fltalloc >= 0) {
		       /* Reuse memory slot(s)/register(s) for shared interface slots */
								flags |= jd->interface_map[fltalloc].flags & INMEMORY;
								regoff = jd->interface_map[fltalloc].regoff;
							} else if (rd->argfltreguse < FLT_ARG_CNT) {
								regoff = rd->argfltregs[rd->argfltreguse++];
							} else if (rd->tmpfltreguse > 0) {
								regoff = rd->tmpfltregs[--rd->tmpfltreguse];
							} else if (rd->savfltreguse > 0) {
								regoff = rd->savfltregs[--rd->savfltreguse];
							} else {
								flags |= INMEMORY;
#if defined(ALIGN_DOUBLES_IN_MEMORY)
								/* Align doubles in Memory */
								if ( (memneeded) && (rd->memuse & 1))
									rd->memuse++;
#endif
								regoff = rd->memuse;
								rd->memuse += memneeded + 1;
							}
							fltalloc = s * 5 + t;
						} else { /* !IS_FLT_DBL_TYPE(t) */
#if defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							/*
							 * for i386 put all longs in memory
							 */
							if (IS_2_WORD_TYPE(t)) {
								flags |= INMEMORY;
#if defined(ALIGN_LONGS_IN_MEMORY)
								/* Align longs in Memory */
								if (rd->memuse & 1)
									rd->memuse++;
#endif
								regoff = rd->memuse;
								rd->memuse += memneeded + 1;
							} else
#endif /* defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE...GISTERS) */
								if (intalloc >= 0) {
		       /* Reuse memory slot(s)/register(s) for shared interface slots */
									flags |= jd->interface_map[intalloc].flags & INMEMORY;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
									if (!(flags & INMEMORY) 
									  && IS_2_WORD_TYPE(intalloc % 5))
										regoff = GET_LOW_REG(
											jd->interface_map[intalloc].regoff);
									else
#endif
										regoff = 
										    jd->interface_map[intalloc].regoff;
								} else 
									if (rd->argintreguse + intregsneeded 
										< INT_ARG_CNT) {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (intregsneeded) 
											regoff=PACK_REGS( 
										  rd->argintregs[rd->argintreguse],
										  rd->argintregs[rd->argintreguse + 1]);
										else
#endif
											regoff = 
											   rd->argintregs[rd->argintreguse];
										rd->argintreguse += intregsneeded + 1;
									}
									else if (rd->tmpintreguse > intregsneeded) {
										rd->tmpintreguse -= intregsneeded + 1;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (intregsneeded) 
											regoff=PACK_REGS( 
										  rd->tmpintregs[rd->tmpintreguse],
										  rd->tmpintregs[rd->tmpintreguse + 1]);
										else
#endif
											regoff = 
											   rd->tmpintregs[rd->tmpintreguse];
									}
									else if (rd->savintreguse > intregsneeded) {
										rd->savintreguse -= intregsneeded + 1;
										regoff = 
											rd->savintregs[rd->savintreguse];
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (intregsneeded) 
											regoff=PACK_REGS( 
										  rd->savintregs[rd->savintreguse],
										  rd->savintregs[rd->savintreguse + 1]);
										else
#endif
											regoff = 
											   rd->savintregs[rd->savintreguse];
									}
									else {
										flags |= INMEMORY;
#if defined(ALIGN_LONGS_IN_MEMORY)
										/* Align longs in Memory */
										if ( (memneeded) && (rd->memuse & 1))
											rd->memuse++;
#endif
										regoff = rd->memuse;
										rd->memuse += memneeded + 1;
									}

							intalloc = s * 5 + t;
						} /* if (IS_FLT_DBL_TYPE(t)) */
					} 
				} else { /* (saved) */
/* now the same like above, but without a chance to take a temporary register */
#ifdef HAS_ADDRESS_REGISTER_FILE
					if (IS_ADR_TYPE(t)) {
						if (rd->savadrreguse > 0) {
							regoff = rd->savadrregs[--rd->savadrreguse];
						}
						else {
							flags |= INMEMORY;
							regoff = rd->memuse++;
						}						
					} else
#endif
					{
						if (IS_FLT_DBL_TYPE(t)) {
							if (fltalloc >= 0) {
								flags |= jd->interface_map[fltalloc].flags & INMEMORY;
								regoff = jd->interface_map[fltalloc].regoff;
							} else
								if (rd->savfltreguse > 0) {
									regoff = 
										rd->savfltregs[--rd->savfltreguse];
								}
								else {
									flags |= INMEMORY;
#if defined(ALIGN_DOUBLES_IN_MEMORY)
									/* Align doubles in Memory */
									if ( (memneeded) && (rd->memuse & 1))
										rd->memuse++;
#endif
									regoff = rd->memuse;
									rd->memuse += memneeded + 1;
								}
							fltalloc = s * 5 + t;
						}
						else { /* IS_INT_LNG */
#if defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							/*
							 * for i386 put all longs in memory
							 */
							if (IS_2_WORD_TYPE(t)) {
								flags |= INMEMORY;
#if defined(ALIGN_LONGS_IN_MEMORY)
								/* Align longs in Memory */
								if (rd->memuse & 1)
									rd->memuse++;
#endif
								regoff = rd->memuse;
								rd->memuse += memneeded + 1;
							} else
#endif
							{
								if (intalloc >= 0) {
									flags |= jd->interface_map[intalloc].flags & INMEMORY;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
									if (!(flags & INMEMORY)
									  && IS_2_WORD_TYPE(intalloc % 5))
										regoff =
											GET_LOW_REG(
											jd->interface_map[intalloc].regoff);
									else
#endif
										regoff =
										    jd->interface_map[intalloc].regoff;
								} else {
									if (rd->savintreguse > intregsneeded) {
										rd->savintreguse -= intregsneeded + 1;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
										if (intregsneeded) 
											regoff = PACK_REGS( 
										  rd->savintregs[rd->savintreguse],
										  rd->savintregs[rd->savintreguse + 1]);
										else
#endif
											regoff =
											   rd->savintregs[rd->savintreguse];
									} else {
										flags |= INMEMORY;
#if defined(ALIGN_LONGS_IN_MEMORY)
									/* Align longs in Memory */
									if ( (memneeded) && (rd->memuse & 1))
										rd->memuse++;
#endif
										regoff = rd->memuse;
										rd->memuse += memneeded + 1;
									}
								}
								intalloc = s*5 + t;
							}
						} /* if (IS_FLT_DBL_TYPE(t) else */
					} /* if (IS_ADR_TYPE(t)) else */
				} /* if (saved) else */
			/* if (type >= 0) */

			assert(regoff >= 0);
			jd->interface_map[5*s + t].flags = flags | OUTVAR;
			jd->interface_map[5*s + t].regoff = regoff;
		} /* for t */
	} /* for s */
}



/* local_regalloc **************************************************************

   Allocates registers for all local variables.
	
*******************************************************************************/
	
static void local_regalloc(jitdata *jd)
{
	methodinfo   *m;
	codegendata  *cd;
	registerdata *rd;

	int     p, s, t, tt,lm;
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

	m  = jd->m;
	cd = jd->cd;
	rd = jd->rd;

	if (jd->isleafmethod) {
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
				lm = jd->local_map[s * 5 + t];
				if (lm == UNUSED)
					continue;

				v = &(jd->var[lm]);

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
						v->vv.regoff = rd->argadrregs[md->params[p].regoff];
					}
					else if (rd->tmpadrreguse > 0) {
						v->flags = 0;
						v->vv.regoff = rd->tmpadrregs[--rd->tmpadrreguse];
					}
					/* use unused argument registers as local registers */
					else if ((p >= md->paramcount) &&
							 (aargcnt < ADR_ARG_CNT)) {
						v->flags = 0;
						v->vv.regoff = rd->argadrregs[aargcnt++];
					}
					else if (rd->savadrreguse > 0) {
						v->flags = 0;
						v->vv.regoff = rd->savadrregs[--rd->savadrreguse];
					}
					else {
						v->flags |= INMEMORY;
						v->vv.regoff = rd->memuse++;
					}						
				} else {
#endif
					if (IS_FLT_DBL_TYPE(t)) {
						if (fltalloc >= 0) {
							v->flags = jd->var[fltalloc].flags;
							v->vv.regoff = jd->var[fltalloc].vv.regoff;
						}
#if !defined(SUPPORT_PASS_FLOATARGS_IN_INTREGS)
						/* We can only use float arguments as local variables,
						 * if we do not pass them in integer registers. */
  						else if ((p < md->paramcount) &&
								 !md->params[p].inmemory) {
							v->flags = 0;
							v->vv.regoff = rd->argfltregs[md->params[p].regoff];
						}
#endif
						else if (rd->tmpfltreguse > 0) {
							v->flags = 0;
							v->vv.regoff = rd->tmpfltregs[--rd->tmpfltreguse];
						}
						/* use unused argument registers as local registers */
						else if ((p >= md->paramcount) &&
								 (fargcnt < FLT_ARG_CNT)) {
							v->flags = 0;
							v->vv.regoff = rd->argfltregs[fargcnt];
							fargcnt++;
						}
						else if (rd->savfltreguse > 0) {
							v->flags = 0;
							v->vv.regoff = rd->savfltregs[--rd->savfltreguse];
						}
						else {
							v->flags = INMEMORY;
#if defined(ALIGN_DOUBLES_IN_MEMORY)
							/* Align doubles in Memory */
							if ( (memneeded) && (rd->memuse & 1))
								rd->memuse++;
#endif
							v->vv.regoff = rd->memuse;
							rd->memuse += memneeded + 1;
						}
						fltalloc = jd->local_map[s * 5 + t];

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
							v->vv.regoff = rd->memuse;
							rd->memuse += memneeded + 1;
						} else 
#endif
						{
							if (intalloc >= 0) {
								v->flags = jd->var[intalloc].flags;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (!(v->flags & INMEMORY)
									&& IS_2_WORD_TYPE(jd->var[intalloc].type))
									v->vv.regoff = GET_LOW_REG(
													jd->var[intalloc].vv.regoff);
								else
#endif
									v->vv.regoff = jd->var[intalloc].vv.regoff;
							}
							else if ((p < md->paramcount) && 
									 !md->params[p].inmemory) {
								v->flags = 0;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (IS_2_WORD_TYPE(t))
									v->vv.regoff = PACK_REGS(
							rd->argintregs[GET_LOW_REG(md->params[p].regoff)],
							rd->argintregs[GET_HIGH_REG(md->params[p].regoff)]);
									else
#endif
										v->vv.regoff =
									       rd->argintregs[md->params[p].regoff];
							}
							else if (rd->tmpintreguse > intregsneeded) {
								rd->tmpintreguse -= intregsneeded + 1;
								v->flags = 0;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (intregsneeded) 
									v->vv.regoff = PACK_REGS(
									    rd->tmpintregs[rd->tmpintreguse],
										rd->tmpintregs[rd->tmpintreguse + 1]);
								else
#endif
									v->vv.regoff = 
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
									v->vv.regoff=PACK_REGS( 
												   rd->argintregs[iargcnt],
												   rd->argintregs[iargcnt + 1]);
								else
#endif
								v->vv.regoff = rd->argintregs[iargcnt];
  								iargcnt += intregsneeded + 1;
							}
							else if (rd->savintreguse > intregsneeded) {
								rd->savintreguse -= intregsneeded + 1;
								v->flags = 0;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (intregsneeded) 
									v->vv.regoff = PACK_REGS(
									    rd->savintregs[rd->savintreguse],
										rd->savintregs[rd->savintreguse + 1]);
								else
#endif
									v->vv.regoff =rd->savintregs[rd->savintreguse];
							}
							else {
								v->flags = INMEMORY;
#if defined(ALIGN_LONGS_IN_MEMORY)
								/* Align longs in Memory */
								if ( (memneeded) && (rd->memuse & 1))
									rd->memuse++;
#endif
								v->vv.regoff = rd->memuse;
								rd->memuse += memneeded + 1;
							}
						}
						intalloc = jd->local_map[s * 5 + t];
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

			lm = jd->local_map[s * 5 + t];
			if (lm == UNUSED)
				continue;

			v = &(jd->var[lm]);

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
						v->vv.regoff = rd->savadrregs[--rd->savadrreguse];
					}
					else {
						v->flags = INMEMORY;
						v->vv.regoff = rd->memuse++;
					}
				} else {
#endif
				if (IS_FLT_DBL_TYPE(t)) {
					if (fltalloc >= 0) {
						v->flags = jd->var[fltalloc].flags;
						v->vv.regoff = jd->var[fltalloc].vv.regoff;
					}
					else if (rd->savfltreguse > 0) {
						v->flags = 0;
						v->vv.regoff = rd->savfltregs[--rd->savfltreguse];
					}
					else {
						v->flags = INMEMORY;
#if defined(ALIGN_DOUBLES_IN_MEMORY)
						/* Align doubles in Memory */
						if ( (memneeded) && (rd->memuse & 1))
							rd->memuse++;
#endif
						v->vv.regoff = rd->memuse;
						rd->memuse += memneeded + 1;
					}
					fltalloc = jd->local_map[s * 5 + t];
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
						v->vv.regoff = rd->memuse;
						rd->memuse += memneeded + 1;
					} else {
#endif
						if (intalloc >= 0) {
							v->flags = jd->var[intalloc].flags;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (!(v->flags & INMEMORY)
								&& IS_2_WORD_TYPE(jd->var[intalloc].type))
								v->vv.regoff = GET_LOW_REG(
											    jd->var[intalloc].vv.regoff);
							else
#endif
								v->vv.regoff = jd->var[intalloc].vv.regoff;
						}
						else if (rd->savintreguse > intregsneeded) {
							rd->savintreguse -= intregsneeded+1;
							v->flags = 0;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
								if (intregsneeded) 
									v->vv.regoff = PACK_REGS(
										rd->savintregs[rd->savintreguse],
									    rd->savintregs[rd->savintreguse + 1]);
								else
#endif
									v->vv.regoff =rd->savintregs[rd->savintreguse];
						}
						else {
							v->flags = INMEMORY;
#if defined(ALIGN_LONGS_IN_MEMORY)
							/* Align longs in Memory */
							if ( (memneeded) && (rd->memuse & 1))
								rd->memuse++;
#endif
							v->vv.regoff = rd->memuse;
							rd->memuse += memneeded + 1;
						}
#if defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
					}
#endif
					intalloc = jd->local_map[s * 5 + t];
				}
#ifdef HAS_ADDRESS_REGISTER_FILE
				}
#endif

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


#define reg_new_temp(jd,index) \
	if ( (index >= jd->localcount) \
		 && (!(jd->var[index].flags & OUTVAR))	 \
		 && (!(jd->var[index].flags & PREALLOC)) )	\
		reg_new_temp_func(jd, index)

static void reg_new_temp_func(jitdata *jd, s4 index)
{
	s4 intregsneeded;
	s4 memneeded;
	s4 tryagain;
	registerdata *rd;
	varinfo      *v;

	rd = jd->rd;
	v = &(jd->var[index]);

	/* Try to allocate a saved register if there is no temporary one          */
	/* available. This is what happens during the second run.                 */
	tryagain = (v->flags & SAVEDVAR) ? 1 : 2;

#ifdef SUPPORT_COMBINE_INTEGER_REGISTERS
	intregsneeded = (IS_2_WORD_TYPE(v->type)) ? 1 : 0;
#else
	intregsneeded = 0;
#endif
#if defined(HAS_4BYTE_STACKSLOT)
	memneeded = (IS_2_WORD_TYPE(v->type)) ? 1 : 0;
#else
	memneeded = 0;
#endif

	for(; tryagain; --tryagain) {
		if (tryagain == 1) {
			if (!(v->flags & SAVEDVAR))
				v->flags |= SAVEDTMP;
#ifdef HAS_ADDRESS_REGISTER_FILE
			if (IS_ADR_TYPE(v->type)) {
				if (rd->freesavadrtop > 0) {
					v->vv.regoff = rd->freesavadrregs[--rd->freesavadrtop];
					return;
				} else if (rd->savadrreguse > 0) {
					v->vv.regoff = rd->savadrregs[--rd->savadrreguse];
					return;
				}
			} else
#endif
			{
				if (IS_FLT_DBL_TYPE(v->type)) {
					if (rd->freesavflttop > 0) {
						v->vv.regoff = rd->freesavfltregs[--rd->freesavflttop];
						return;
					} else if (rd->savfltreguse > 0) {
						v->vv.regoff = rd->savfltregs[--rd->savfltreguse];
						return;
					}
				} else {
#if defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
					/*
					 * for i386 put all longs in memory
					 */
					if (!IS_2_WORD_TYPE(v->type))
#endif
					{
						if (rd->freesavinttop > intregsneeded) {
							rd->freesavinttop -= intregsneeded + 1;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded)
								v->vv.regoff = PACK_REGS(
							        rd->freesavintregs[rd->freesavinttop],
									rd->freesavintregs[rd->freesavinttop + 1]);
						else
#endif
								v->vv.regoff =
									rd->freesavintregs[rd->freesavinttop];
							return;
						} else if (rd->savintreguse > intregsneeded) {
							rd->savintreguse -= intregsneeded + 1;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded)
								v->vv.regoff = PACK_REGS(
									rd->savintregs[rd->savintreguse],
							        rd->savintregs[rd->savintreguse + 1]);
							else
#endif
								v->vv.regoff = rd->savintregs[rd->savintreguse];
							return;
						}
					}
				}
			}
		} else { /* tryagain == 2 */
#ifdef HAS_ADDRESS_REGISTER_FILE
			if (IS_ADR_TYPE(v->type)) {
				if (rd->freetmpadrtop > 0) {
					v->vv.regoff = rd->freetmpadrregs[--rd->freetmpadrtop];
					return;
				} else if (rd->tmpadrreguse > 0) {
					v->vv.regoff = rd->tmpadrregs[--rd->tmpadrreguse];
					return;
				}
			} else
#endif
			{
				if (IS_FLT_DBL_TYPE(v->type)) {
					if (rd->freeargflttop > 0) {
						v->vv.regoff = rd->freeargfltregs[--rd->freeargflttop];
						v->flags |= TMPARG;
						return;
					} else if (rd->argfltreguse < FLT_ARG_CNT) {
						v->vv.regoff = rd->argfltregs[rd->argfltreguse++];
						v->flags |= TMPARG;
						return;
					} else if (rd->freetmpflttop > 0) {
						v->vv.regoff = rd->freetmpfltregs[--rd->freetmpflttop];
						return;
					} else if (rd->tmpfltreguse > 0) {
						v->vv.regoff = rd->tmpfltregs[--rd->tmpfltreguse];
						return;
					}

				} else {
#if defined(HAS_4BYTE_STACKSLOT) && !defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
					/*
					 * for i386 put all longs in memory
					 */
					if (!IS_2_WORD_TYPE(v->type))
#endif
					{
						if (rd->freearginttop > intregsneeded) {
							rd->freearginttop -= intregsneeded + 1;
							v->flags |= TMPARG;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								v->vv.regoff = PACK_REGS(
									rd->freeargintregs[rd->freearginttop],
							        rd->freeargintregs[rd->freearginttop + 1]);
							else
#endif
								v->vv.regoff =
									rd->freeargintregs[rd->freearginttop];
							return;
						} else if (rd->argintreguse 
								   < INT_ARG_CNT - intregsneeded) {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								v->vv.regoff = PACK_REGS(
									rd->argintregs[rd->argintreguse],
								    rd->argintregs[rd->argintreguse + 1]);
							else
#endif
								v->vv.regoff = rd->argintregs[rd->argintreguse];
							v->flags |= TMPARG;
							rd->argintreguse += intregsneeded + 1;
							return;
						} else if (rd->freetmpinttop > intregsneeded) {
							rd->freetmpinttop -= intregsneeded + 1;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								v->vv.regoff = PACK_REGS(
									rd->freetmpintregs[rd->freetmpinttop],
								    rd->freetmpintregs[rd->freetmpinttop + 1]);
							else
#endif
								v->vv.regoff = rd->freetmpintregs[rd->freetmpinttop];
							return;
						} else if (rd->tmpintreguse > intregsneeded) {
							rd->tmpintreguse -= intregsneeded + 1;
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
							if (intregsneeded) 
								v->vv.regoff = PACK_REGS(
									rd->tmpintregs[rd->tmpintreguse],
								    rd->tmpintregs[rd->tmpintreguse + 1]);
							else
#endif
								v->vv.regoff = rd->tmpintregs[rd->tmpintreguse];
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
		v->vv.regoff = rd->freemem_2[rd->freememtop_2];
	} else
#endif /*defined(HAS_4BYTE_STACKSLOT) */
		if ((memneeded == 0) && (rd->freememtop > 0)) {
			rd->freememtop--;;
			v->vv.regoff = rd->freemem[rd->freememtop];
		} else {
#if defined(ALIGN_LONGS_IN_MEMORY) || defined(ALIGN_DOUBLES_IN_MEMORY)
			/* align 2 Word Types */
			if ((memneeded) && ((rd->memuse & 1) == 1)) { 
				/* Put patched memory slot on freemem */
				rd->freemem[rd->freememtop++] = rd->memuse;
				rd->memuse++;
			}
#endif /* defined(ALIGN_LONGS_IN_MEMORY) || defined(ALIGN_DOUBLES_IN_MEMORY) */
			v->vv.regoff = rd->memuse;
			rd->memuse += memneeded + 1;
		}
	v->flags |= INMEMORY;
}


#define reg_free_temp(jd,index) \
	if ((index > jd->localcount) \
		&& (!(jd->var[index].flags & OUTVAR))	\
		&& (!(jd->var[index].flags & PREALLOC)) ) \
		reg_free_temp_func(jd, index)

/* Do not free regs/memory locations used by Stackslots flagged STCOPY! There is still another Stackslot */
/* alive using this reg/memory location */

static void reg_free_temp_func(jitdata *jd, s4 index)
{
	s4 intregsneeded;
	s4 memneeded;
	registerdata *rd;
	varinfo *v;

	rd = jd->rd;
	v = &(jd->var[index]);


#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
	intregsneeded = (IS_2_WORD_TYPE(v->type)) ? 1 : 0;
#else
	intregsneeded = 0;
#endif

#if defined(HAS_4BYTE_STACKSLOT)
	memneeded = (IS_2_WORD_TYPE(v->type)) ? 1 : 0;
#else
	memneeded = 0;
#endif

	if (v->flags & INMEMORY) {
#if defined(HAS_4BYTE_STACKSLOT)
		if (memneeded > 0) {
			rd->freemem_2[rd->freememtop_2] = v->vv.regoff;
			rd->freememtop_2++;
		} else 
#endif
		{
			rd->freemem[rd->freememtop] = v->vv.regoff;
			rd->freememtop++;
		}

#ifdef HAS_ADDRESS_REGISTER_FILE
	} else if (IS_ADR_TYPE(v->type)) {
		if (v->flags & (SAVEDVAR | SAVEDTMP)) {
/* 			v->flags &= ~SAVEDTMP; */
			rd->freesavadrregs[rd->freesavadrtop++] = v->vv.regoff;
		} else
			rd->freetmpadrregs[rd->freetmpadrtop++] = v->vv.regoff;
#endif
	} else if (IS_FLT_DBL_TYPE(v->type)) {
		if (v->flags & (SAVEDVAR | SAVEDTMP)) {
/* 			v->flags &= ~SAVEDTMP; */
			rd->freesavfltregs[rd->freesavflttop++] = v->vv.regoff;
		} else if (v->flags & TMPARG) {
/* 			v->flags &= ~TMPARG; */
			rd->freeargfltregs[rd->freeargflttop++] = v->vv.regoff;
		} else
			rd->freetmpfltregs[rd->freetmpflttop++] = v->vv.regoff;
	} else { /* IS_INT_LNG_TYPE */
		if (v->flags & (SAVEDVAR | SAVEDTMP)) {
/* 			v->flags &= ~SAVEDTMP; */
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
			if (intregsneeded) {
				rd->freesavintregs[rd->freesavinttop] =
					GET_LOW_REG(v->vv.regoff);
				rd->freesavintregs[rd->freesavinttop + 1] =
					GET_HIGH_REG(v->vv.regoff);
			} else
#endif
			rd->freesavintregs[rd->freesavinttop] = v->vv.regoff;
			rd->freesavinttop += intregsneeded + 1;

		} else if (v->flags & TMPARG) {
/* 			s->flags &= ~TMPARG; */
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
			if (intregsneeded) {
				rd->freeargintregs[rd->freearginttop] =
					GET_LOW_REG(v->vv.regoff);
				rd->freeargintregs[rd->freearginttop + 1] =
					GET_HIGH_REG(v->vv.regoff);
			} else 
#endif
			rd->freeargintregs[rd->freearginttop] = v->vv.regoff;
			rd->freearginttop += intregsneeded + 1;
		} else {
#if defined(SUPPORT_COMBINE_INTEGER_REGISTERS)
			if (intregsneeded) {
				rd->freetmpintregs[rd->freetmpinttop] =
					GET_LOW_REG(s->vv.regoff);
				rd->freetmpintregs[rd->freetmpinttop + 1] =
					GET_HIGH_REG(s->vv.regoff);
			} else
#endif
		    rd->freetmpintregs[rd->freetmpinttop] = v->vv.regoff;
			rd->freetmpinttop += intregsneeded + 1;
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
	instruction        *iptr;
	basicblock         *bptr;
	builtintable_entry *bte;
	methoddesc         *md;
	s4                 *argp;

	/* get required compiler data */

	m  = jd->m;
	rd = jd->rd;

	/* initialize temp registers */

	reg_init_temp(m, rd);

	bptr = jd->new_basicblocks;

	while (bptr != NULL) {
		if (bptr->flags >= BBREACHED) {

			/* set allocation of invars */

			for (i=0; i<bptr->indepth; ++i) 
			{
				varinfo *v = jd->var + bptr->invars[i];

				v->vv.regoff = jd->interface_map[5*i + v->type].regoff;
				v->flags  = jd->interface_map[5*i + v->type].flags;
			}

			/* set allocation of outvars */

			for (i=0; i<bptr->outdepth; ++i) 
			{
				varinfo *v = jd->var + bptr->outvars[i];

				v->vv.regoff = jd->interface_map[5*i + v->type].regoff;
				v->flags  = jd->interface_map[5*i + v->type].flags;
			}

			/* iterate over ICMDS to allocate temporary variables */

			iptr = bptr->iinstr;
			len = bptr->icount;

			while (--len >= 0)  {
				switch (iptr->opc) {

					/* pop 0 push 0 */

				case ICMD_NOP:
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
					reg_new_temp(jd, iptr->dst.varindex);
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
					reg_free_temp(jd, iptr->sx.s23.s2.varindex);
					reg_free_temp(jd, iptr->s1.varindex);
					reg_new_temp(jd, iptr->dst.varindex);
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
					reg_free_temp(jd, iptr->sx.s23.s3.varindex);
					reg_free_temp(jd, iptr->sx.s23.s2.varindex);
					reg_free_temp(jd, iptr->s1.varindex);
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
					reg_free_temp(jd, iptr->s1.varindex);
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
					reg_free_temp(jd, iptr->sx.s23.s2.varindex);
					reg_free_temp(jd, iptr->s1.varindex);
					break;

					/* pop 0 push 1 copy */
					
				case ICMD_COPY:
					/* src === dst->prev (identical Stackslot Element)     */
					/* src --> dst       (copied value, take same reg/mem) */

/* 					if (!reg_alloc_dup(iptr->s1.varindex, iptr->dst.varindex)) { */
						reg_new_temp(jd, iptr->dst.varindex);
/* 					} else { */
/* 						iptr->dst.varindex->flags |= STCOPY; */
/* 					} */
					break;

					/* pop 1 push 1 move */

				case ICMD_MOVE:
						reg_new_temp(jd, iptr->dst.varindex);
						reg_free_temp(jd, iptr->s1.varindex);
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
					reg_free_temp(jd, iptr->sx.s23.s2.varindex);
					reg_free_temp(jd, iptr->s1.varindex);
					reg_new_temp(jd, iptr->dst.varindex);
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
					reg_free_temp(jd, iptr->s1.varindex);
					reg_new_temp(jd, iptr->dst.varindex);
					break;

					/* pop 0 push 1 */
					
				case ICMD_GETSTATIC:

				case ICMD_NEW:
					reg_new_temp(jd, iptr->dst.varindex);
					break;

					/* pop many push any */
					
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKEINTERFACE:
					INSTRUCTION_GET_METHODDESC(iptr,md);
					i = md->paramcount;
					argp = iptr->sx.s23.s2.args;
					while (--i >= 0) {
						reg_free_temp(jd, *argp);
						argp++;
					}
					if (md->returntype.type != TYPE_VOID)
						reg_new_temp(jd, iptr->dst.varindex);
					break;

				case ICMD_BUILTIN:
					bte = iptr->sx.s23.s3.bte;
					md = bte->md;
					i = md->paramcount;
					argp = iptr->sx.s23.s2.args;
					while (--i >= 0) {
						reg_free_temp(jd, *argp);
						argp++;
					}
					if (md->returntype.type != TYPE_VOID)
						reg_new_temp(jd, iptr->dst.varindex);
					break;

				case ICMD_MULTIANEWARRAY:
					i = iptr->s1.argcount;
					argp = iptr->sx.s23.s2.args;
					while (--i >= 0) {
						reg_free_temp(jd, *argp);
						argp++;
					}
					reg_new_temp(jd, iptr->dst.varindex);
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

		bptr = jd->new_basicblocks;

		while (bptr != NULL) {
			if (bptr->flags >= BBREACHED) {

#if defined(ENABLE_LSRA) || defined(ENABLE_SSA)
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
							   (dst->vv.regoff != rd->interfaces[len][dst->type].regoff) ))
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
							   (dst->vv.regoff != rd->interfaces[len][dst->type].regoff) ))
						{
							/* one in memory or both inmemory at different offsets */
							count_mem_move_bb++;
							in_register=false;
						}
					}

					dst = dst->prev;
				}
#if defined(ENABLE_LSRA) || defined(ENABLE_SSA)
			}
#endif	


				dst = bptr->instack;
				iptr = bptr->iinstr;
				len = bptr->icount;
				src_old = NULL;

				while (--len >= 0)  {
					src = dst;
					dst = iptr->dst.var;

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
		if (in_register) {
			printf("INREGISTER: %s%s%s\n",m->class->name->text, m->name->text, m->descriptor->text);
		}
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
