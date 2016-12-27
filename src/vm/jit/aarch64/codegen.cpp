/* src/vm/jit/aarch64/codegen.cpp - machine code generator for Aarch64

   Copyright (C) 1996-2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#include "config.h"

#include <cassert>
#include <cstdio>
#include <cmath>

#include "vm/types.hpp"

#include "md.hpp"
#include "md-abi.hpp"

#include "vm/jit/aarch64/arch.hpp"
#include "vm/jit/aarch64/codegen.hpp"

#include "mm/memory.hpp"

#include "native/localref.hpp"
#include "native/native.hpp"

#include "threads/lock.hpp"

#include "vm/descriptor.hpp"
#include "vm/exceptions.hpp"
#include "vm/field.hpp"
#include "vm/global.hpp"
#include "vm/loader.hpp"
#include "vm/options.hpp"
#include "vm/vm.hpp"

#include "vm/jit/abi.hpp"
#include "vm/jit/asmpart.hpp"
#include "vm/jit/builtin.hpp"
#include "vm/jit/codegen-common.hpp"
#include "vm/jit/dseg.hpp"
#include "vm/jit/emit-common.hpp"
#include "vm/jit/jit.hpp"
#include "vm/jit/linenumbertable.hpp"
#include "vm/jit/parse.hpp"
#include "vm/jit/patcher-common.hpp"
#include "vm/jit/reg.hpp"
#include "vm/jit/stacktrace.hpp"
#include "vm/jit/trap.hpp"


/**
 * Generates machine code for the method prolog.
 */
void codegen_emit_prolog(jitdata* jd)
{
	varinfo*    var;
	methoddesc* md;
	int32_t     s1;
	int32_t     p, t, l;
	int32_t     varindex;
	int         i;

	// Get required compiler data.
	methodinfo*   m    = jd->m;
	codeinfo*     code = jd->code;
	codegendata*  cd   = jd->cd;
	registerdata* rd   = jd->rd;
	AsmEmitter asme(cd);

	/* create stack frame (if necessary) */
	/* NOTE: SP on aarch64 has to be quad word aligned */
	if (cd->stackframesize) {
		int offset = cd->stackframesize * 8;
		offset += (offset % 16);
		asme.lda(REG_SP, REG_SP, -offset);
	}

	/* save return address and used callee saved registers */

	p = cd->stackframesize;
	if (!code_is_leafmethod(code)) {
		p--; asme.lst(REG_RA, REG_SP, p * 8);
	}
	for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
		p--; asme.lst(rd->savintregs[i], REG_SP, p * 8);
	}
	for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
		p--; asme.dst(rd->savfltregs[i], REG_SP, p * 8);
	}

	/* take arguments out of register or stack frame */

	md = m->parseddesc;

 	for (p = 0, l = 0; p < md->paramcount; p++) {
 		t = md->paramtypes[p].type;

  		varindex = jd->local_map[l * 5 + t];

 		l++;
 		if (IS_2_WORD_TYPE(t))    /* increment local counter for 2 word types */
 			l++;

		if (varindex == jitdata::UNUSED)
			continue;

 		var = VAR(varindex);

		s1 = md->params[p].regoff;

		if (IS_INT_LNG_TYPE(t)) {                    /* integer args          */
 			if (!md->params[p].inmemory) {           /* register arguments    */
 				if (!IS_INMEMORY(var->flags))
					asme.mov(var->vv.regoff, s1);
				else
 					asme.lst(s1, REG_SP, var->vv.regoff);
			}
			else {                                   /* stack arguments       */
 				if (!IS_INMEMORY(var->flags))
 					asme.lld(var->vv.regoff, REG_SP, cd->stackframesize * 8 + s1);
				else
					var->vv.regoff = cd->stackframesize * 8 + s1;
			}
		}
		else {                                       /* floating args         */
 			if (!md->params[p].inmemory) {           /* register arguments    */
 				if (!IS_INMEMORY(var->flags))
					if (IS_2_WORD_TYPE(t))
						asme.dmov(var->vv.regoff, s1);
					else
						asme.fmov(var->vv.regoff, s1);
 				else
					if (IS_2_WORD_TYPE(t))
						asme.dst(s1, REG_SP, var->vv.regoff);
					else
						asme.fst(s1, REG_SP, var->vv.regoff);
			}
			else {                                   /* stack arguments       */
 				if (!(var->flags & INMEMORY))
					if (IS_2_WORD_TYPE(t))
						asme.dld(var->vv.regoff, REG_SP, cd->stackframesize * 8 + s1);
					else
						asme.fld(var->vv.regoff, REG_SP, cd->stackframesize * 8 + s1);
				else
					var->vv.regoff = cd->stackframesize * 8 + s1;
			}
		}
	}
}


/**
 * Generates machine code for the method epilog.
 */
void codegen_emit_epilog(jitdata* jd)
{
	int32_t p;
	int i;

	// Get required compiler data.
	codeinfo*     code = jd->code;
	codegendata*  cd   = jd->cd;
	registerdata* rd   = jd->rd;
	AsmEmitter asme(cd);

	p = cd->stackframesize;

	/* restore return address */

	if (!code_is_leafmethod(code)) {
		p--; asme.lld(REG_RA, REG_SP, p * 8);
	}

	/* restore saved registers */

	for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
		p--; asme.lld(rd->savintregs[i], REG_SP, p * 8);
	}
	for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
		p--; asme.dld(rd->savfltregs[i], REG_SP, p * 8);
	}

	/* deallocate stack */

	if (cd->stackframesize) {
		int offset = cd->stackframesize * 8;
		offset += (offset % 16);
		asme.lda(REG_SP, REG_SP, offset);
	}

	asme.ret();
}


/**
 * Generates machine code for one ICMD.
 */
void codegen_emit_instruction(jitdata* jd, instruction* iptr)
{
	varinfo*            var;
	builtintable_entry* bte;
	methodinfo*         lm;             // Local methodinfo for ICMD_INVOKE*.
	unresolved_method*  um;
	fieldinfo*          fi;
	unresolved_field*   uf;
	int32_t             fieldtype;
	int32_t             s1, s2, s3, d = 0;
	int32_t             disp;

	// Get required compiler data.
	codegendata* cd = jd->cd;

	AsmEmitter asme(cd);

	switch (iptr->opc) {

		/* constant operations ************************************************/

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				constant_classref *cr = iptr->sx.val.c.ref;

				disp = dseg_add_unique_address(cd, cr);

				/* XXX Only add the patcher, if this position needs to
				   be patched.  If there was a previous position which
				   resolved the same class, the returned displacement
				   of dseg_add_address is ok to use. */

				patcher_add_patch_ref(jd, PATCHER_resolve_classref_to_classinfo,
									  cr, disp);

				asme.ald(d, REG_PV, disp);
			}
			else {
				if (iptr->sx.val.anyptr == NULL) {
					asme.lconst(d, 0);
				}
				else {
					disp = dseg_add_address(cd, iptr->sx.val.anyptr);
					asme.ald(d, REG_PV, disp);
				}
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */

			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			disp = dseg_add_float(cd, iptr->sx.val.f);
			asme.fld(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */

			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			disp = dseg_add_double(cd, iptr->sx.val.d);
			asme.dld(d, REG_PV, disp);
			emit_store_dst(jd, iptr, d);
			break;

		/* integer operations *************************************************/

		case ICMD_INEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1); 
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.isub(d, REG_ZERO, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.lsub(d, REG_ZERO, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			asme.sxtw(d, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.ubfx(d, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.sxtb(d, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.uxth(d, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.sxth(d, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.iadd(d, s1, s2);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.ladd(d, s1, s2);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IINC:
		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			
			if ((iptr->sx.val.i >= 0) && (iptr->sx.val.i <= 0xffffff)) {
				asme.iadd_imm(d, s1, iptr->sx.val.i);
			} else if ((-iptr->sx.val.i >= 0) && (-iptr->sx.val.i <= 0xffffff)) {
				asme.isub_imm(d, s1, -iptr->sx.val.i);
			} else {
				asme.iconst(REG_ITMP2, iptr->sx.val.i);
				asme.iadd(d, s1, REG_ITMP2);
			}

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* sx.val.l = constant                             */

			// assert(iptr->sx.val.l >= 0); // TODO: check why this was here

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if ((iptr->sx.val.l >= 0) && (iptr->sx.val.l <= 0xffffff)) {
				asme.ladd_imm(d, s1, iptr->sx.val.l);
			} else if ((-iptr->sx.val.l >= 0) && (-iptr->sx.val.l <= 0xffffff)) {
				asme.lsub_imm(d, s1, -iptr->sx.val.l);
			} else {
				asme.lconst(REG_ITMP2, iptr->sx.val.l);
				asme.ladd(d, s1, REG_ITMP2);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.isub(d, s1, s2);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.lsub(d, s1, s2);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			
			if ((iptr->sx.val.i >= 0) && (iptr->sx.val.i <= 0xffffff)) {
				asme.isub_imm(d, s1, iptr->sx.val.i);
			} else if ((-iptr->sx.val.i >= 0) && (-iptr->sx.val.i <= 0xffffff)) {
				asme.iadd_imm(d, s1, -iptr->sx.val.i);
			} else {
				asme.iconst(REG_ITMP2, iptr->sx.val.i);
				asme.isub(d, s1, REG_ITMP2);
			}

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			
			if ((iptr->sx.val.l >= 0) && (iptr->sx.val.l <= 0xffffff)) {
				asme.lsub_imm(d, s1, iptr->sx.val.l);
			} else if ((-iptr->sx.val.l >= 0) && (-iptr->sx.val.l <= 0xffffff)) {
				asme.ladd_imm(d, s1, -iptr->sx.val.l);
			} else {
				asme.lconst(REG_ITMP2, iptr->sx.val.l);
				asme.lsub(d, s1, REG_ITMP2);
			}

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.imul(d, s1, s2);
			asme.ubfx(d, d); // cut back to int
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.lmul(d, s1, s2);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IMULPOW2:   /* ..., value  ==> ..., value * (2 ^ constant)  */
		                      /* sx.val.i = constant                          */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.ilsl_imm(d, s1, iptr->sx.val.i);
			asme.ubfx(d, d); // cut back to int
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LMULPOW2:   /* ..., value  ==> ..., value * (2 ^ constant)  */
		                      /* sx.val.i = constant                          */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			asme.llsl_imm(d, s1, iptr->sx.val.i);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			emit_arithmetic_check(cd, iptr, s2);

			asme.idiv(REG_ITMP3, s1, s2);
			asme.imsub(d, REG_ITMP3, s2, s1);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			emit_arithmetic_check(cd, iptr, s2);

			asme.ldiv(REG_ITMP3, s1, s2);
			asme.lmsub(d, REG_ITMP3, s2, s1);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			emit_arithmetic_check(cd, iptr, s2);

			asme.idiv(d, s1, s2);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			emit_arithmetic_check(cd, iptr, s2);

			asme.ldiv(d, s1, s2);

			emit_store_dst(jd, iptr, d);
			break;

			// TODO: implement this using shift operators
		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value / (2 ^ constant)  */
		                      /* sx.val.i = constant                          */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.iconst(REG_ITMP3, pow(2, iptr->sx.val.i));
			asme.idiv(d, s1, REG_ITMP3);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant */
		                      /* sx.val.i = constant [ (2 ^ x) - 1 ]   */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			// Use absolute value
			asme.icmp_imm(s1, 0);
			asme.icsneg(d, s1, s1, COND_PL);

			asme.iconst(REG_ITMP3, iptr->sx.val.i);
			asme.iand(d, d, REG_ITMP3);
			
			// Negate the result again if the value was negative
			asme.icsneg(d, d, d, COND_PL);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.iconst(REG_ITMP3, 0x1f);
			asme.iand(REG_ITMP3, s2, REG_ITMP3);
			asme.ilsl(d, s1, REG_ITMP3);
			asme.ubfx(d, d); // cut back to int

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.lconst(REG_ITMP3, 0x3f);
			asme.land(REG_ITMP3, s2, REG_ITMP3);
			asme.llsl(d, s1, REG_ITMP3);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			
			asme.ilsl_imm(d, s1, iptr->sx.val.i & 0x1f); // shift amout is between 0 and 31 incl
			asme.ubfx(d, d); // cut back to int

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.llsl_imm(d, s1, iptr->sx.val.i & 0x3f);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.iconst(REG_ITMP3, 0x1f);
			asme.iand(REG_ITMP3, s2, REG_ITMP3);
			asme.iasr(d, s1, REG_ITMP3);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.lconst(REG_ITMP3, 0x3f);
			asme.land(REG_ITMP3, s2, REG_ITMP3);
			asme.lasr(d, s1, REG_ITMP3);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISHRCONST: /* ..., value  ==> ..., value >> constant      */
						     /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.iasr_imm(d, s1, iptr->sx.val.i & 0x1f);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.lasr_imm(d, s1, iptr->sx.val.i & 0x3f); // TODO: is the constant really in sx.val.i?

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IUSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.iconst(REG_ITMP3, 0x1f);
			asme.iand(REG_ITMP3, s2, REG_ITMP3);
			asme.ilsr(d, s1, REG_ITMP3);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LUSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.lconst(REG_ITMP3, 0x3f);
			asme.land(REG_ITMP3, s2, REG_ITMP3);
			asme.llsr(d, s1, REG_ITMP3);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >> constant      */
							  /* sx.val.i = constant */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.ilsr_imm(d, s1, iptr->sx.val.i & 0x1f);

			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >> constant      */
							  /* sx.val.i = constant */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.llsr_imm(d, s1, iptr->sx.val.i & 0x3f);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.iand(d, s1, s2);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.land(d, s1, s2);

			emit_store_dst(jd, iptr, d);
			break;

		// TODO: implement this using the immediate variant
		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.iconst(REG_ITMP3, iptr->sx.val.i);
			asme.iand(d, s1, REG_ITMP3);

			emit_store_dst(jd, iptr, d);
			break;

		// TODO: implement this using the immediate variant
		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* sx.val.l = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.lconst(REG_ITMP3, iptr->sx.val.l);
			asme.land(d, s1, REG_ITMP3);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.ior(d, s1, s2);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.lor(d, s1, s2);

			emit_store_dst(jd, iptr, d);
			break;

		// TODO: implement this using the immediate variant
		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* sx.val.i = constant                          */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.iconst(REG_ITMP2, iptr->sx.val.i);
			asme.ior(d, s1, REG_ITMP2);

			emit_store_dst(jd, iptr, d);
			break;

		// TODO: implement this using the immediate variant
		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* sx.val.l = constant                          */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.lconst(REG_ITMP2, iptr->sx.val.l);
			asme.lor(d, s1, REG_ITMP2);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.ixor(d, s1, s2);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.lxor(d, s1, s2);

			emit_store_dst(jd, iptr, d);
			break;

		// TODO: implement this using the immediate variant
		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* sx.val.i = constant                          */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.iconst(REG_ITMP2, iptr->sx.val.i);
			asme.ixor(d, s1, REG_ITMP2);

			emit_store_dst(jd, iptr, d);
			break;

		// TODO: implement this using the immediate variant
		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* sx.val.l = constant                          */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			asme.lconst(REG_ITMP2, iptr->sx.val.l);
			asme.lxor(d, s1, REG_ITMP2);

			emit_store_dst(jd, iptr, d);
			break;

		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			asme.fneg(d, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			asme.dneg(d, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			asme.fadd(d, s1, s2);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			asme.dadd(d, s1, s2);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			asme.fsub(d, s1, s2);
			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			asme.dsub(d, s1, s2);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			if (d == s1 || d == s2) {
				asme.fmul(REG_FTMP3, s1, s2);
				asme.fmov(d, REG_FTMP3);
			} else {
				asme.fmul(d, s1, s2);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 *** val2      */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			if (d == s1 || d == s2) {
				asme.dmul(REG_FTMP3, s1, s2);
				asme.dmov(d, REG_FTMP3);
			} else {
				asme.dmul(d, s1, s2);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			asme.fdiv(d, s1, s2);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			asme.ddiv(d, s1, s2);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_I2F:		/* ..., value  ==> ..., (float) value            */
		case ICMD_L2F:		
		case ICMD_I2D:      /* ..., value  ==> ..., (double) value           */
		case ICMD_L2D:      
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);

			switch (iptr->opc) {
				case ICMD_I2F: asme.i2f(d, s1); break;
				case ICMD_L2F: asme.l2f(d, s1); break;
				case ICMD_I2D: asme.i2d(d, s1); break;
				case ICMD_L2D: asme.l2d(d, s1); break;
			}

			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
		case ICMD_D2I:
		case ICMD_F2L:       /* ..., value  ==> ..., (long) value             */
		case ICMD_D2L:
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);

			// If the fp value is NaN (unordered) set the result to 0
			asme.iconst(d, 0);

			// Use the correct comparison instruction
			if (iptr->opc == ICMD_F2I || iptr->opc == ICMD_F2L)
				asme.fcmp(s1);
			else
				asme.dcmp(s1, s1);
		
			// Jump over the conversion if unordered 
			asme.b_vs(2);

			// Rounding towards zero (see Java spec)
			switch (iptr->opc) {
				case ICMD_F2I: asme.f2i(d, s1); break;
				case ICMD_D2I: asme.d2i(d, s1); break;
				case ICMD_F2L: asme.f2l(d, s1); break;
				case ICMD_D2L: asme.d2l(d, s1); break;
			}

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			asme.f2d(d, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_D2F:       /* ..., value  ==> ..., (float) value           */

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP3);
			asme.d2f(d, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
		case ICMD_DCMPL:
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);

			if (iptr->opc == ICMD_FCMPL)
				asme.fcmp(s1, s2);
			else
				asme.dcmp(s1, s2);

			asme.iconst(d, 0);
			asme.iconst(REG_ITMP1, 1);
			asme.iconst(REG_ITMP2, -1);

			/* set to -1 if less than or unordered (NaN) */
			/* set to 1 if greater than */
			asme.icsel(REG_ITMP1, REG_ITMP1, REG_ITMP2, COND_GT);

			/* set to 0 if equal or result of previous csel */
			asme.icsel(d, d, REG_ITMP1, COND_EQ);

			emit_store_dst(jd, iptr, d);
			break;
			
		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
		case ICMD_DCMPG:
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);

			if (iptr->opc == ICMD_FCMPG)
				asme.fcmp(s1, s2);
			else
				asme.dcmp(s1, s2);

			asme.iconst(d, 0);
			asme.iconst(REG_ITMP1, 1);
			asme.iconst(REG_ITMP2, -1);

			/* set to 1 if greater than or unordered (NaN) */
			/* set to -1 if less than */
			asme.icsel(REG_ITMP1, REG_ITMP1, REG_ITMP2, COND_HI);

			/* set to 0 if equal or result of previous csel */
			asme.icsel(d, d, REG_ITMP1, COND_EQ);

			emit_store_dst(jd, iptr, d);
			break;

		/* memory operations **************************************************/

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			asme.ladd(REG_ITMP1, s1, s2);
			asme.ldrsb32(d, REG_ITMP1, OFFSET (java_bytearray_t, data[0]));

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 1); /* REG_ITMP1 = s1 + (2 * s2) */
			asme.ldrh(d, REG_ITMP1, OFFSET(java_chararray_t, data[0]));

			emit_store_dst(jd, iptr, d);
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 1); /* REG_ITMP1 = s1 + (2 * s2) */
			asme.ldrsh32(d, REG_ITMP1, OFFSET(java_shortarray_t, data[0]));

			emit_store_dst(jd, iptr, d);
			break;			

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 2);
			asme.ild(d, REG_ITMP1, OFFSET(java_intarray_t, data[0]));

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 3); // REG_ITMP1 = s1 + lsl(s2, 3)
			asme.lld(d, REG_ITMP1, OFFSET(java_longarray_t, data[0]));

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 2);
			asme.fld(d, REG_ITMP1, OFFSET(java_floatarray_t, data[0]));

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 3); // REG_ITMP1 = s1 + lsl(s2, 3)
			asme.dld(d, REG_ITMP1, OFFSET(java_doublearray_t, data[0]));

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 3); // REG_ITMP1 = s1 + lsl(s2, 3)
			asme.ald(d, REG_ITMP1, OFFSET(java_objectarray_t, data[0]));

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);

			asme.ladd(REG_ITMP1, s1, s2);
			asme.strb(s3, REG_ITMP1, OFFSET(java_bytearray_t, data[0]));

			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 1); // REG_ITMP1 = s1 + (2 * s2)
			asme.strh(s3, REG_ITMP1, OFFSET(java_chararray_t, data[0]));

			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 1); // REG_ITMP1 = s1 + (2 * s2)
			asme.strh(s3, REG_ITMP1, OFFSET(java_shortarray_t, data[0]));

			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 2);
			asme.ist(s3, REG_ITMP1, OFFSET(java_intarray_t, data[0]));

			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 3); // REG_ITMP1 = s1 + lsl(s2, 3)
			asme.lst(s3, REG_ITMP1, OFFSET(java_longarray_t, data[0]));

			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_FTMP3);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 2);
			asme.fst(s3, REG_ITMP1, OFFSET(java_floatarray_t, data[0]));

			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_FTMP3);

			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 3); // REG_ITMP1 = s1 + lsl(s2, 3)
			asme.dst(s3, REG_ITMP1, OFFSET(java_doublearray_t, data[0]));

			break;

		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_A0);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_A1);

			asme.mov(REG_A0, s1);
			asme.mov(REG_A1, s3);

			disp = dseg_add_functionptr(cd, BUILTIN_FAST_canstore);
			asme.ald(REG_PV, REG_PV, disp);
			asme.blr(REG_PV);

			disp = (s4) (cd->mcodeptr - cd->mcodebase);
			asme.lda(REG_PV, REG_RA, -disp);
			emit_arraystore_check(cd, iptr);

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);
			asme.ladd_shift(REG_ITMP1, s1, s2, CODE_LSL, 3); // REG_ITMP1 = s1 + lsl(s2, 3)
			asme.ast(s3, REG_ITMP1, OFFSET(java_objectarray_t, data[0]));
			break;

		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				uf        = iptr->sx.s23.s3.uf;
				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp      = 0;

				patcher_add_patch_ref(jd, PATCHER_get_putfield, uf, 0);
			}
			else {
				fi        = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				disp      = fi->offset;
			}

			if (IS_INT_LNG_TYPE(fieldtype))
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			else
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);

			/* implicit null-pointer check */
			switch (fieldtype) {
			case TYPE_INT:
				asme.ild(d, s1, disp);
				break;
			case TYPE_LNG:
				asme.lld(d, s1, disp);
				break;
			case TYPE_ADR:
				asme.ald(d, s1, disp);
				break;
			case TYPE_FLT:
				asme.fld(d, s1, disp);
				break;
			case TYPE_DBL:				
				asme.dld(d, s1, disp);
				break;
			}
			asme.nop();
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				uf        = iptr->sx.s23.s3.uf;
				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp      = 0;
			}
			else {
				uf        = NULL;
				fi        = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				disp      = fi->offset;
			}

			if (IS_INT_LNG_TYPE(fieldtype))
				s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			else
				s2 = emit_load_s2(jd, iptr, REG_FTMP2);

			if (INSTRUCTION_IS_UNRESOLVED(iptr))
				patcher_add_patch_ref(jd, PATCHER_get_putfield, uf, 0);

			/* implicit null-pointer check */
			switch (fieldtype) {
			case TYPE_INT:
				asme.ist(s2, s1, disp);
				break;
			case TYPE_LNG:
				asme.lst(s2, s1, disp);
				break;
			case TYPE_ADR:
				asme.ast(s2, s1, disp);
				break;
			case TYPE_FLT:
				asme.fst(s2, s1, disp);
				break;
			case TYPE_DBL:
				asme.dst(s2, s1, disp);
				break;
			}
			asme.nop();
			break;

		/* branch operations **************************************************/

		case ICMD_IF_LEQ:       /* ..., value ==> ...                       */
		case ICMD_IF_LNE:
		case ICMD_IF_LLT:
		case ICMD_IF_LGE:
		case ICMD_IF_LGT:
		case ICMD_IF_LLE:

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);

			if (iptr->sx.val.l >= 0 && iptr->sx.val.l <= 0xfff) {
				asme.lcmp_imm(s1, iptr->sx.val.l);
			} else if ((-iptr->sx.val.l) >= 0 && (-iptr->sx.val.l) <= 0xfff) {
				asme.lcmn_imm(s1, -iptr->sx.val.l);
			} else {
				asme.lconst(REG_ITMP2, iptr->sx.val.l);
				asme.lcmp(s1, REG_ITMP2);
			}

			emit_bcc(cd, iptr->dst.block, iptr->opc - ICMD_IF_LEQ, BRANCH_OPT_NONE);

			break;

		case ICMD_IF_LCMPEQ:  /* ..., value, value ==> ...                */
		case ICMD_IF_LCMPNE:  /* op1 = target JavaVM pc                   */
		case ICMD_IF_LCMPLT:
		case ICMD_IF_LCMPGT:
		case ICMD_IF_LCMPLE:
		case ICMD_IF_LCMPGE:

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);

			asme.lcmp(s1, s2);
			emit_bcc(cd, iptr->dst.block, iptr->opc - ICMD_IF_LCMPEQ, BRANCH_OPT_NONE);

			break;

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			emit_trap(cd, REG_METHODPTR, TRAP_THROW);
			break;

		case ICMD_BUILTIN:      /* ..., arg1, arg2, arg3 ==> ...              */
			bte = iptr->sx.s23.s3.bte;
			if (bte->stub == NULL)
				disp = dseg_add_functionptr(cd, bte->fp);
			else
				disp = dseg_add_functionptr(cd, bte->stub);

			asme.ald(REG_PV, REG_PV, disp);  /* Pointer to built-in-function */

			/* generate the actual call */
			asme.blr(REG_PV);
			break;

		case ICMD_INVOKESPECIAL:
			emit_nullpointer_check(cd, iptr, REG_A0);
			/* fall-through */

		case ICMD_INVOKESTATIC: /* ..., [arg1, [arg2 ...]] ==> ...            */
			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				um = iptr->sx.s23.s3.um;
				disp = dseg_add_unique_address(cd, um);

				patcher_add_patch_ref(jd, PATCHER_invokestatic_special,
									  um, disp);
			}
			else {
				lm = iptr->sx.s23.s3.fmiref->p.method;
				disp = dseg_add_address(cd, lm->stubroutine);
			}

			asme.ald(REG_PV, REG_PV, disp);         /* method pointer in r27 */

			/* generate the actual call */
			asme.blr(REG_PV);
			break;

		case ICMD_INVOKEVIRTUAL:/* op1 = arg count, val.a = method pointer    */
			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				um = iptr->sx.s23.s3.um;
				patcher_add_patch_ref(jd, PATCHER_invokevirtual, um, 0);

				s1 = 0;
			}
			else {
				lm = iptr->sx.s23.s3.fmiref->p.method;
				s1 = OFFSET(vftbl_t, table[0]) + sizeof(methodptr) * lm->vftblindex;
			}

			/* implicit null-pointer check */
			asme.ald(REG_METHODPTR, REG_A0, OFFSET(java_object_t, vftbl));
			asme.ald(REG_PV, REG_METHODPTR, s1);

			/* generate the actual call */
			asme.blr(REG_PV);

			break;

		case ICMD_INVOKEINTERFACE:
			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				um = iptr->sx.s23.s3.um;
				patcher_add_patch_ref(jd, PATCHER_invokeinterface, um, 0);

				s1 = 0;
				s2 = 0;
			}
			else {
				lm = iptr->sx.s23.s3.fmiref->p.method;
				s1 = OFFSET(vftbl_t, interfacetable[0]) -
					sizeof(methodptr*) * lm->clazz->index;

				s2 = sizeof(methodptr) * (lm - lm->clazz->methods);
			}

			/* implicit null-pointer check */
			asme.ald(REG_METHODPTR, REG_A0, OFFSET(java_object_t, vftbl));

			/* on aarch64 we only have negative offsets in the range of -255 to 255 so we need a mov */
			assert(abs(s1) <= 0xffff);
			assert(abs(s2) <= 0xffff);
			asme.lconst(REG_ITMP1, s1);
			asme.lconst(REG_ITMP3, s2);

			emit_ldr_reg(cd, REG_METHODPTR, REG_METHODPTR, REG_ITMP1); // TODO: move to emitter
			emit_ldr_reg(cd, REG_PV, REG_METHODPTR, REG_ITMP3); // TODO: move to emitter

			/* generate the actual call */
			asme.blr(REG_PV);
			break;

		case ICMD_TABLESWITCH:  /* ..., index ==> ...                         */

			s4 i, l;
			branch_target_t *table;

			table = iptr->dst.table;

			l = iptr->sx.s23.s2.tablelow;
			i = iptr->sx.s23.s3.tablehigh;

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			if (l == 0) {
				asme.imov(REG_ITMP1, s1); // TODO: check that this works
			} else if (abs(l) <= 32768) {
				if (l < 0) {
					asme.iadd_imm(REG_ITMP1, s1, -l);
				} else {
					asme.isub_imm(REG_ITMP1, s1, l);
				}
			} else {
				asme.iconst(REG_ITMP2, l);
				asme.isub(REG_ITMP1, s1, REG_ITMP2);
			}

			/* number of targets */
			i = i - l + 1;

			/* range check */
			emit_icmp_imm(cd, REG_ITMP1, i-1);
			emit_bcc(cd, table[0].block, BRANCH_UGT, BRANCH_OPT_NONE);

			/* build jump table top down and use address of lowest entry */

			table += i;

			while (--i >= 0) {
				dseg_add_target(cd, table->block); 
				--table;
			}

			/* length of dataseg after last dseg_add_target is used by load */
			asme.sxtw(REG_ITMP1, REG_ITMP1);
			asme.ladd_shift(REG_ITMP2, REG_PV, REG_ITMP1, CODE_LSL, 3);
			asme.ald(REG_ITMP2, REG_ITMP2, -(cd->dseglen));
			asme.br(REG_ITMP2);
			ALIGNCODENOP;
			break;

		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */

			if (!(iptr->flags.bits & INS_FLAG_ARRAY)) {
				// object type cast-check 

				classinfo *super;
				s4         superindex;

				if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
					super      = NULL;
					superindex = 0;
				}
				else {
					super      = iptr->sx.s23.s3.c.cls;
					superindex = super->index;
				}

				s1 = emit_load_s1(jd, iptr, REG_ITMP1);

				// if class is not resolved, check which code to call 

				if (super == NULL) {
					asme.lcmp_imm(s1, 0);
					emit_label_beq(cd, BRANCH_LABEL_1);

					disp = dseg_add_unique_s4(cd, 0); /* super->flags */

					patcher_add_patch_ref(jd, PATCHER_resolve_classref_to_flags,
										  iptr->sx.s23.s3.c.ref, disp);

					asme.ild(REG_ITMP2, REG_PV, disp);

					disp = dseg_add_s4(cd, ACC_INTERFACE);
					asme.ild(REG_ITMP3, REG_PV, disp);

					asme.ltst(REG_ITMP2, REG_ITMP3);
					emit_label_beq(cd, BRANCH_LABEL_2);
				}

				// interface checkcast code 

				if ((super == NULL) || (super->flags & ACC_INTERFACE)) {
					if (super != NULL) {
						asme.lcmp_imm(s1, 0);
						emit_label_beq(cd, BRANCH_LABEL_3);
					}

					asme.ald(REG_ITMP2, s1, OFFSET(java_object_t, vftbl));

					if (super == NULL) {
						patcher_add_patch_ref(jd, PATCHER_checkcast_interface,
											  iptr->sx.s23.s3.c.ref, 0);
					} 

					asme.ild(REG_ITMP3, REG_ITMP2,
						  OFFSET(vftbl_t, interfacetablelength));

					assert(abs(superindex) <= 0xfff);
					asme.icmp_imm(REG_ITMP3, superindex);
					emit_classcast_check(cd, iptr, BRANCH_LE, REG_ITMP3, s1);

					s4 offset = (s4) (OFFSET(vftbl_t, interfacetable[0]) -
							superindex * sizeof(methodptr*));

					assert(abs(offset) <= 0xffff);
					asme.lconst(REG_ITMP3, offset);
					emit_ldr_reg(cd, REG_ITMP3, REG_ITMP2, REG_ITMP3); // TODO: mov this to emitter
					asme.lcmp_imm(REG_ITMP3, 0);
					emit_classcast_check(cd, iptr, BRANCH_EQ, REG_ITMP3, s1);

					if (super == NULL)
						emit_label_br(cd, BRANCH_LABEL_4);
					else
						emit_label(cd, BRANCH_LABEL_3);
				}

				// class checkcast code 

				if ((super == NULL) || !(super->flags & ACC_INTERFACE)) {
					if (super == NULL) {
						emit_label(cd, BRANCH_LABEL_2);

						disp = dseg_add_unique_address(cd, NULL);

						patcher_add_patch_ref(jd, PATCHER_resolve_classref_to_vftbl,
											  iptr->sx.s23.s3.c.ref, disp);
					}
					else {
						disp = dseg_add_address(cd, super->vftbl);

						asme.lcmp_imm(s1, 0);
						emit_label_beq(cd, BRANCH_LABEL_5);
					}

					asme.ald(REG_ITMP2, s1, OFFSET(java_object_t, vftbl));
					asme.ald(REG_ITMP3, REG_PV, disp);

					if (super == NULL || super->vftbl->subtype_depth >= DISPLAY_SIZE) {
						asme.ild(REG_ITMP1, REG_ITMP3, OFFSET(vftbl_t, subtype_offset));
						asme.ladd(REG_ITMP1, REG_ITMP2, REG_ITMP1);
						asme.ald(REG_ITMP1, REG_ITMP1, 0);

						asme.lcmp(REG_ITMP1, REG_ITMP3);
						emit_label_beq(cd, BRANCH_LABEL_6);  // good 

						if (super == NULL) {
							asme.ild(REG_ITMP1, REG_ITMP3, OFFSET(vftbl_t, subtype_offset));
							asme.icmp_imm(REG_ITMP1, OFFSET(vftbl_t, subtype_display[DISPLAY_SIZE]));
							emit_label_bne(cd, BRANCH_LABEL_10); // throw 
						}

						asme.ild(REG_ITMP1, REG_ITMP3, OFFSET(vftbl_t, subtype_depth));
						asme.ild(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, subtype_depth));
						asme.icmp(REG_ITMP1, REG_ITMP3);
						emit_label_bgt(cd, BRANCH_LABEL_9); // throw 

						// reload 
						asme.ald(REG_ITMP3, REG_PV, disp);
						asme.ald(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, subtype_overflow));
						asme.ladd_shift(REG_ITMP2, REG_ITMP2, REG_ITMP1, CODE_LSL, 3);
						asme.ald(REG_ITMP1, REG_ITMP2, -DISPLAY_SIZE*8);

						asme.lcmp(REG_ITMP1, REG_ITMP3);
						emit_label_beq(cd, BRANCH_LABEL_7); // good 

						emit_label(cd, BRANCH_LABEL_9);
						if (super == NULL)
							emit_label(cd, BRANCH_LABEL_10);

						// reload s1, might have been destroyed 
						emit_load_s1(jd, iptr, REG_ITMP1);
						emit_trap(cd, s1, TRAP_ClassCastException);

						emit_label(cd, BRANCH_LABEL_7);
						emit_label(cd, BRANCH_LABEL_6);
						// reload s1, might have been destroyed 
						emit_load_s1(jd, iptr, REG_ITMP1);
					}
					else {
						asme.ald(REG_ITMP2, REG_ITMP2, super->vftbl->subtype_offset);

						asme.lcmp(REG_ITMP2, REG_ITMP3);
						emit_classcast_check(cd, iptr, BRANCH_NE, REG_ITMP3, s1);
					}

					if (super != NULL)
						emit_label(cd, BRANCH_LABEL_5);
				}

				if (super == NULL) {
					emit_label(cd, BRANCH_LABEL_1);
					emit_label(cd, BRANCH_LABEL_4);
				}

				d = codegen_reg_of_dst(jd, iptr, s1);
			}
			else {
				/* array type cast-check */

				s1 = emit_load_s1(jd, iptr, REG_A0);
				asme.imov(REG_A0, s1);

				if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
					disp = dseg_add_unique_address(cd, NULL);

					patcher_add_patch_ref(jd,
										  PATCHER_resolve_classref_to_classinfo,
										  iptr->sx.s23.s3.c.ref,
										  disp);
				}
				else
					disp = dseg_add_address(cd, iptr->sx.s23.s3.c.cls);

				asme.ald(REG_A1, REG_PV, disp);
				disp = dseg_add_functionptr(cd, BUILTIN_arraycheckcast);
				asme.ald(REG_PV, REG_PV, disp);
				asme.blr(REG_PV);
				disp = (s4) (cd->mcodeptr - cd->mcodebase);
				asme.lda(REG_PV, REG_RA, -disp);

				s1 = emit_load_s1(jd, iptr, REG_ITMP1);
				asme.ltst(REG_RESULT, REG_RESULT);
				emit_classcast_check(cd, iptr, BRANCH_EQ, REG_RESULT, s1);

				d = codegen_reg_of_dst(jd, iptr, s1); 
			}

			asme.mov(d, s1);
			emit_store_dst(jd, iptr, d); 
			break;

		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */

			{
			classinfo *super;
			vftbl_t   *supervftbl;
			s4         superindex;

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				super = NULL;
				superindex = 0;
				supervftbl = NULL;

			} else {
				super = iptr->sx.s23.s3.c.cls;
				superindex = super->index;
				supervftbl = super->vftbl;
			}

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			if (s1 == d) {
				asme.mov(REG_ITMP1, s1);
				s1 = REG_ITMP1;
			}

			/* if class is not resolved, check which code to call */

			if (super == NULL) {
				asme.clr(d);
				asme.lcmp_imm(s1, 0);
				emit_label_beq(cd, BRANCH_LABEL_1);

				disp = dseg_add_unique_s4(cd, 0); /* super->flags */

				patcher_add_patch_ref(jd, PATCHER_resolve_classref_to_flags,
									  iptr->sx.s23.s3.c.ref, disp);

				asme.ild(REG_ITMP3, REG_PV, disp);
				asme.iconst(REG_ITMP2, ACC_INTERFACE);

				asme.itst(REG_ITMP3, REG_ITMP2);
				emit_label_beq(cd, BRANCH_LABEL_2);
			}

			/* interface instanceof code */

			if ((super == NULL) || (super->flags & ACC_INTERFACE)) {
				if (super == NULL) {
					/* If d == REG_ITMP2, then it's destroyed in check
					   code above. */
					if (d == REG_ITMP2)
					 	asme.clr(d);

					patcher_add_patch_ref(jd,
										  PATCHER_instanceof_interface,
										  iptr->sx.s23.s3.c.ref, 0);
				} else {
					asme.clr(d);
					asme.lcmp_imm(s1, 0);
					emit_label_beq(cd, BRANCH_LABEL_3);
				}
				
				asme.ald(REG_ITMP1, s1, OFFSET(java_object_t, vftbl));
				asme.ild(REG_ITMP3, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
				assert(abs(superindex) <= 0xfff);
				asme.icmp_imm(REG_ITMP3, superindex);
				asme.b_le(5);
				
				s4 offset = (s4) (OFFSET(vftbl_t, interfacetable[0]) -
							superindex * sizeof(methodptr*));
				assert(abs(offset) <= 0xffff);
				asme.lconst(REG_ITMP3, offset);
				emit_ldr_reg(cd, REG_ITMP1, REG_ITMP1, REG_ITMP3);

				asme.lcmp_imm(REG_ITMP1, 0);
				asme.cset(d, COND_NE); /* if REG_ITMP != 0 then d = 1 */

				if (super == NULL)
					emit_label_br(cd, BRANCH_LABEL_4);
				else
					emit_label(cd, BRANCH_LABEL_3);
			}

			/* class instanceof code */

			if ((super == NULL) || !(super->flags & ACC_INTERFACE)) {
				if (super == NULL) {
					emit_label(cd, BRANCH_LABEL_2);

					disp = dseg_add_unique_address(cd, NULL);

					patcher_add_patch_ref(jd, PATCHER_resolve_classref_to_vftbl,
										  iptr->sx.s23.s3.c.ref,
										  disp);
				}
				else {
					disp = dseg_add_address(cd, supervftbl);

					asme.clr(d);
					asme.lcmp_imm(s1, 0);
					emit_label_beq(cd, BRANCH_LABEL_5);
				}

				asme.ald(REG_ITMP2, s1, OFFSET(java_object_t, vftbl));
				asme.ald(REG_ITMP3, REG_PV, disp);

				if (super == NULL || super->vftbl->subtype_depth >= DISPLAY_SIZE) {
					asme.ild(REG_ITMP1, REG_ITMP3, OFFSET(vftbl_t, subtype_offset));
					asme.ladd(REG_ITMP1, REG_ITMP2, REG_ITMP1);
					asme.ald(REG_ITMP1, REG_ITMP1, 0);
					asme.lcmp(REG_ITMP1, REG_ITMP3);
					emit_label_bne(cd, BRANCH_LABEL_8); 

					asme.iconst(d, 1);
					emit_label_br(cd, BRANCH_LABEL_6); /* true */
					emit_label(cd, BRANCH_LABEL_8);

					if (super == NULL) {
						asme.ild(REG_ITMP1, REG_ITMP3, OFFSET(vftbl_t, subtype_offset));
						asme.icmp_imm(REG_ITMP1, OFFSET(vftbl_t, subtype_display[DISPLAY_SIZE]));
						emit_label_bne(cd, BRANCH_LABEL_10); /* false */
					}

					asme.ild(REG_ITMP1, REG_ITMP3, OFFSET(vftbl_t, subtype_depth));

					asme.ild(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, subtype_depth));
					asme.icmp(REG_ITMP1, REG_ITMP3);
					emit_label_bgt(cd, BRANCH_LABEL_9); /* false */

					/* reload */
					asme.ald(REG_ITMP3, REG_PV, disp);
					asme.ald(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, subtype_overflow));
					asme.ladd_shift(REG_ITMP2, REG_ITMP2, REG_ITMP1, CODE_LSL, 3);
					asme.ald(REG_ITMP1, REG_ITMP2, -DISPLAY_SIZE*8);

					asme.lcmp(REG_ITMP1, REG_ITMP3);
					asme.cset(d, COND_EQ);

					if (d == REG_ITMP2)
						emit_label_br(cd, BRANCH_LABEL_7);
					emit_label(cd, BRANCH_LABEL_9);

					if (super == NULL)
						emit_label(cd, BRANCH_LABEL_10);

					if (d == REG_ITMP2) {
						asme.clr(d);
						emit_label(cd, BRANCH_LABEL_7);
					}
					emit_label(cd, BRANCH_LABEL_6);

				}
				else {
					asme.ald(REG_ITMP2, REG_ITMP2, super->vftbl->subtype_offset);
					asme.lcmp(REG_ITMP2, REG_ITMP3);
					asme.cset(d, COND_EQ);
				}

				if (super != NULL)
					emit_label(cd, BRANCH_LABEL_5);
			}

			if (super == NULL) {
				emit_label(cd, BRANCH_LABEL_1);
				emit_label(cd, BRANCH_LABEL_4);
			}

			emit_store_dst(jd, iptr, d);
			}
			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */

			/* check for negative sizes and copy sizes to stack if necessary  */

			MCODECHECK((iptr->s1.argcount << 1) + 64);

			for (s1 = iptr->s1.argcount; --s1 >= 0; ) {

				var = VAR(iptr->sx.s23.s2.args[s1]);
	
				/* copy SAVEDVAR sizes to stack */

				/* Already Preallocated? */

				if (!(var->flags & PREALLOC)) {
					s2 = emit_load(jd, iptr, var, REG_ITMP1);
					asme.lst(s2, REG_SP, s1 * 8);
				}
			}

			/* a0 = dimension count */

			asme.iconst(REG_A0, iptr->s1.argcount);

			/* is patcher function set? */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				disp = dseg_add_unique_address(cd, 0);

				patcher_add_patch_ref(jd, PATCHER_resolve_classref_to_classinfo,
									  iptr->sx.s23.s3.c.ref,
									  disp);
			}
			else
				disp = dseg_add_address(cd, iptr->sx.s23.s3.c.cls);

			/* a1 = arraydescriptor */

			asme.ald(REG_A1, REG_PV, disp);

			/* a2 = pointer to dimensions = stack pointer */

			asme.mov(REG_A2, REG_SP);

			disp = dseg_add_functionptr(cd, BUILTIN_multianewarray);
			asme.ald(REG_PV, REG_PV, disp);
			asme.blr(REG_PV);
			disp = (s4) (cd->mcodeptr - cd->mcodebase);
			asme.lda(REG_PV, REG_RA, -disp);

			/* check for exception before result assignment */

			emit_exception_check(cd, iptr);

			d = codegen_reg_of_dst(jd, iptr, REG_RESULT);
			asme.imov(d, REG_RESULT);
			emit_store_dst(jd, iptr, d);
			break;

		default:
			os::abort("ICMD (%s, %d) not implemented yet on Aarch64!", 
					icmd_table[iptr->opc].name, iptr->opc);
	}

	return;
}


/* codegen_emit_stub_native ****************************************************

   Emits a stub routine which calls a native method.

*******************************************************************************/

void codegen_emit_stub_native(jitdata *jd, methoddesc *nmd, functionptr f, int skipparams)
{
	methodinfo  *m;
	codeinfo    *code;
	codegendata *cd;
	methoddesc  *md;
	int          i, j;
	int          t;
	int          s1, s2;
	int          disp;

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	AsmEmitter asme(cd);

	/* initialize variables */

	md = m->parseddesc;

	/* calculate stack frame size */

	cd->stackframesize =
		1 +                             /* return address                     */
		sizeof(stackframeinfo_t) / SIZEOF_VOID_P +
		sizeof(localref_table) / SIZEOF_VOID_P +
		1 +                             /* methodinfo for call trace          */
		md->paramcount +
		nmd->memuse;

	/* create method header */
	u4 stackoffset = (cd->stackframesize * 8);
	stackoffset += stackoffset % 16;

	(void) dseg_add_unique_address(cd, code);              /* CodeinfoPointer */
	(void) dseg_add_unique_s4(cd, stackoffset);			   /* FrameSize       */
	(void) dseg_add_unique_s4(cd, 0);                      /* IsLeaf          */
	(void) dseg_add_unique_s4(cd, 0);                      /* IntSave         */
	(void) dseg_add_unique_s4(cd, 0);                      /* FltSave         */

	/* generate stub code */

	asme.lda(REG_SP, REG_SP, -stackoffset);
	asme.lst(REG_RA, REG_SP, stackoffset - SIZEOF_VOID_P);

#if defined(ENABLE_GC_CACAO)
	/* Save callee saved integer registers in stackframeinfo (GC may
	   need to recover them during a collection). */

	disp = cd->stackframesize * 8 - SIZEOF_VOID_P - sizeof(stackframeinfo_t) +
		OFFSET(stackframeinfo_t, intregs);

	for (i = 0; i < INT_SAV_CNT; i++)
		asme.lst(abi_registers_integer_saved[i], REG_SP, disp + i * 8);
#endif

	/* save integer and float argument registers */

	for (i = 0; i < md->paramcount; i++) {
		if (!md->params[i].inmemory) {
			s1 = md->params[i].regoff;

			switch (md->paramtypes[i].type) {
			case TYPE_INT:
			case TYPE_LNG:
			case TYPE_ADR:
				asme.lst(s1, REG_SP, i * 8);
				break;
			case TYPE_FLT:
				asme.fst(s1, REG_SP, i * 8);
				break;
			case TYPE_DBL:
				asme.dst(s1, REG_SP, i * 8);
				break;
			default:
				assert(false);
				break;
			}
		}
	}

	/* prepare data structures for native function call */

	asme.mov(REG_A0, REG_SP);
	asme.mov(REG_A1, REG_PV);
	disp = dseg_add_functionptr(cd, codegen_start_native_call);
	asme.lld(REG_PV, REG_PV, disp);
	asme.blr(REG_PV);
	disp = (s4) (cd->mcodeptr - cd->mcodebase);
	asme.lda(REG_PV, REG_RA, -disp);

	/* remember class argument */

	if (m->flags & ACC_STATIC)
		asme.mov(REG_ITMP3, REG_RESULT);

	/* restore integer and float argument registers */

	for (i = 0; i < md->paramcount; i++) {
		if (!md->params[i].inmemory) {
			s1 = md->params[i].regoff;

			switch (md->paramtypes[i].type) {
			case TYPE_INT:
			case TYPE_LNG:
			case TYPE_ADR:
				asme.lld(s1, REG_SP, i * 8);
				break;
			case TYPE_FLT:
				asme.fld(s1, REG_SP, i * 8);
				break;
			case TYPE_DBL:
				asme.dld(s1, REG_SP, i * 8);
				break;
			default:
				assert(false);
				break;
			}
		}
	}

	/* copy or spill arguments to new locations */

	for (i = md->paramcount - 1, j = i + skipparams; i >= 0; i--, j--) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[i].inmemory) {
				s1 = md->params[i].regoff;
				s2 = nmd->params[j].regoff;

				if (!nmd->params[j].inmemory)
					asme.mov(s2, s1);
				else
					asme.lst(s1, REG_SP, s2);
			}
			else {
				s1 = md->params[i].regoff + stackoffset;
				s2 = nmd->params[j].regoff;
				asme.lld(REG_ITMP1, REG_SP, s1);
				asme.lst(REG_ITMP1, REG_SP, s2);
			}
		}
		else {
			if (!md->params[i].inmemory) {
				s1 = md->params[i].regoff;
				s2 = nmd->params[j].regoff;

				if (!nmd->params[j].inmemory)
					if (IS_2_WORD_TYPE(t))
						asme.dmov(s2, s1);
					else
						asme.fmov(s2, s1);
				else {
					if (IS_2_WORD_TYPE(t))
						asme.dst(s1, REG_SP, s2);
					else
						asme.fst(s1, REG_SP, s2);
				}
			}
			else {
				s1 = md->params[i].regoff + stackoffset;
				s2 = nmd->params[j].regoff;
				if (IS_2_WORD_TYPE(t)) {
					asme.dld(REG_FTMP1, REG_SP, s1);
					asme.dst(REG_FTMP1, REG_SP, s2);
				}
				else {
					asme.fld(REG_FTMP1, REG_SP, s1);
					asme.fst(REG_FTMP1, REG_SP, s2);
				}
			}
		}
	}

	/* Handle native Java methods. */

	if (m->flags & ACC_NATIVE) {
		/* put class into second argument register */

		if (m->flags & ACC_STATIC)
			asme.mov(REG_A1, REG_ITMP3);

		/* put env into first argument register */

		disp = dseg_add_address(cd, VM::get_current()->get_jnienv());
		asme.lld(REG_A0, REG_PV, disp);
	}

	/* Call the native function. */

	disp = dseg_add_functionptr(cd, f);
	asme.lld(REG_PV, REG_PV, disp);
	asme.blr(REG_PV);                    /* call native method                 */
	disp = (s4) (cd->mcodeptr - cd->mcodebase);
	asme.lda(REG_PV, REG_RA, -disp);       /* recompute pv from ra               */

	/* save return value */

	switch (md->returntype.type) {
	case TYPE_INT:
	case TYPE_LNG:
	case TYPE_ADR:
		asme.lst(REG_RESULT, REG_SP, 0 * 8);
		break;
	case TYPE_FLT:
		asme.fst(REG_FRESULT, REG_SP, 0 * 8);
		break;
	case TYPE_DBL:
		asme.dst(REG_FRESULT, REG_SP, 0 * 8);
		break;
	case TYPE_VOID:
		break;
	default:
		assert(false);
		break;
	}

	/* remove native stackframe info */

	asme.mov(REG_A0, REG_SP);
	asme.mov(REG_A1, REG_PV);
	disp = dseg_add_functionptr(cd, codegen_finish_native_call);
	asme.lld(REG_PV, REG_PV, disp);
	asme.blr(REG_PV);
	disp = (s4) (cd->mcodeptr - cd->mcodebase);
	asme.lda(REG_PV, REG_RA, -disp);
	asme.mov(REG_ITMP1_XPTR, REG_RESULT);

	/* restore return value */

	switch (md->returntype.type) {
	case TYPE_INT:
	case TYPE_LNG:
	case TYPE_ADR:
		asme.lld(REG_RESULT, REG_SP, 0 * 8);
		break;
	case TYPE_FLT:
		asme.fld(REG_FRESULT, REG_SP, 0 * 8);
		break;
	case TYPE_DBL:
		asme.dld(REG_FRESULT, REG_SP, 0 * 8);
		break;
	case TYPE_VOID:
		break;
	default:
		assert(false);
		break;
	}

#if defined(ENABLE_GC_CACAO)
	/* Restore callee saved integer registers from stackframeinfo (GC
	   might have modified them during a collection). */
	os::abort("NOT IMPLEMENTED YET!");
  	 
	disp = cd->stackframesize * 8 - SIZEOF_VOID_P - sizeof(stackframeinfo_t) +
		OFFSET(stackframeinfo_t, intregs);

	for (i = 0; i < INT_SAV_CNT; i++)
		asme.lld(abi_registers_integer_saved[i], REG_SP, disp + i * 8);
#endif

	asme.lld(REG_RA, REG_SP, stackoffset - 8); /* get RA            */
	asme.lda(REG_SP, REG_SP, stackoffset);

	/* check for exception */


	asme.cbnz(REG_ITMP1_XPTR, 2);       /* if no exception then return        */
	asme.ret();                         /* return to caller                   */

	/* handle exception */

	asme.lsub_imm(REG_ITMP2_XPC, REG_RA, 4); /* get exception address            */

	emit_trap(cd, REG_ITMP2, TRAP_NAT_EXCEPTION);
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
