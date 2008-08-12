/* src/vm/jit/jit.h - code generation header

   Copyright (C) 1996-2005, 2006, 2007, 2008
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


#ifndef _JIT_H
#define _JIT_H

/* forward typedefs ***********************************************************/

typedef struct jitdata jitdata;
typedef struct basicblock basicblock;
typedef struct instruction instruction;
typedef struct insinfo_inline insinfo_inline;
typedef struct exception_entry exception_entry;


#include "config.h"
#include "vm/types.h"

#include "toolbox/chain.h"

#include "vm/global.h"
#include "vm/method.h"
#include "vm/references.h"
#include "vm/resolve.h"

#if defined(ENABLE_STATISTICS)
# include "vm/statistics.h"
#endif

#include "vm/jit/codegen-common.h"
#include "vm/jit/reg.h"
#include "vm/jit/replace.h"
#include "vm/jit/stack.h"
#include "vm/jit/stacktrace.hpp"

#if defined(ENABLE_INLINING)
# include "vm/jit/inline/inline.h"
#endif

#include "vm/jit/ir/bytecode.h"
#include "vm/jit/ir/icmd.hpp"

#if defined(ENABLE_LOOP)
# include "vm/jit/loop/loop.h"
#endif
#if defined(ENABLE_SSA) 
# include "vm/jit/optimizing/lsra.h"
#endif
#if defined(ENABLE_LSRA)
# include "vm/jit/allocator/lsra.h"
#endif

#include "vm/jit/verify/typeinfo.h"


/* common jit/codegen macros **************************************************/

#if defined(ENABLE_STATISTICS)
# define COUNT(x)        (x)++
# define COUNT_SPILLS             /* use COUNT_(READ|WRITE)_SPILLS instead */
# define COUNT_READ_SPILLS(var) \
	switch(var->type) { \
	case TYPE_FLT: count_spills_read_flt++; break; \
	case TYPE_DBL: count_spills_read_dbl++; break; \
	default: count_spills_read_ila++; break; \
	}

# define COUNT_WRITE_SPILLS(var) \
	switch(var->type) { \
	case TYPE_FLT: count_spills_write_flt++; break; \
	case TYPE_DBL: count_spills_write_dbl++; break; \
	default: count_spills_write_ila++; break; \
	}

#else
# define COUNT(x)                /* nothing */
# define COUNT_SPILLS            /* nothing */
# define COUNT_READ_SPILLS(x)    /* nothing */
# define COUNT_WRITE_SPILLS(x)   /* nothing */
#endif

typedef struct interface_info interface_info;

struct interface_info {
	s4 flags;
	s4 regoff;
};


/* jitdata ********************************************************************/

struct jitdata {
	methodinfo      *m;               /* methodinfo of the method compiled    */
	codeinfo        *code;
	codegendata     *cd;
	registerdata    *rd;
#if defined(ENABLE_LOOP)
	loopdata        *ld;
#endif
#if defined(ENABLE_SSA) || defined(ENABLE_LSRA)
	lsradata        *ls;
#endif

	u4               flags;           /* contains JIT compiler flags          */

	instruction     *instructions;    /* ICMDs, valid between parse and stack */
	basicblock      *basicblocks;     /* start of basic block list            */
	stackelement_t  *stack;           /* XXX should become stack.c internal   */
	s4               instructioncount;/* XXX remove this?                     */
	s4               basicblockcount; /* number of basic blocks               */
	s4               stackcount;      /* number of stackelements to allocate  */
                                      /* (passed from parse to stack)         */

	varinfo         *var;             /* array of variables                   */
	s4               vartop;          /* next free index in var array         */

	s4               varcount;        /* number of variables in var array     */
	s4               localcount;      /* number of locals at start of var ar. */
    s4              *local_map;       /* map for renaming (de-coallescing)    */
					 /* locals and keeping the coalescing info for simplereg. */
	                 /* local_map[javaindex * 5 + type] =                     */
	                 /*     >= 0......index into jd->var, or                  */
					 /*     UNUSED....this (javaindex,type) pair is not used  */

	s4              *reverselocalmap; /* map from CACAO varindex to javaindex */
	                                  /* (varindex must be < localcount)      */

	s4               maxlocals;       /* max. number of javalocals            */

	interface_info  *interface_map;   /* interface variables (for simplereg)  */
	s4               maxinterfaces;   /* max. number of interface variables   */

	s4               exceptiontablelength; /* exceptiontable length           */
	exception_entry *exceptiontable;       /* the exceptiontable              */

	basicblock      *returnblock;          /* block containing the *RETURN    */
	                                       /* (only use if returncount==1)    */
	s4               returncount;          /* number of return instructions   */
	bool             branchtoentry;        /* true if first block is a target */
	bool             branchtoend;          /* true if end dummy is a target   */
};

#define FOR_EACH_BASICBLOCK(jd, it) \
	for ((it) = (jd)->basicblocks; (it) != NULL; (it) = (it)->next)

#define UNUSED                     -1

#define JITDATA_FLAG_PARSE               0x00000001
#define JITDATA_FLAG_VERIFY              0x00000002

#define JITDATA_FLAG_INSTRUMENT          0x00000004

#define JITDATA_FLAG_IFCONV              0x00000008
#define JITDATA_FLAG_REORDER             0x00000010
#define JITDATA_FLAG_INLINE              0x00000020

#define JITDATA_FLAG_COUNTDOWN           0x00000100

#define JITDATA_FLAG_SHOWINTERMEDIATE    0x20000000
#define JITDATA_FLAG_SHOWDISASSEMBLE     0x40000000
#define JITDATA_FLAG_VERBOSECALL         0x80000000


#define JITDATA_HAS_FLAG_PARSE(jd) \
    ((jd)->flags & JITDATA_FLAG_PARSE)

#define JITDATA_HAS_FLAG_VERIFY(jd) \
    ((jd)->flags & JITDATA_FLAG_VERIFY)

#define JITDATA_HAS_FLAG_INSTRUMENT(jd) \
    ((jd)->flags & JITDATA_FLAG_INSTRUMENT)

#define JITDATA_HAS_FLAG_IFCONV(jd) \
    ((jd)->flags & JITDATA_FLAG_IFCONV)

#define JITDATA_HAS_FLAG_REORDER(jd) \
    ((jd)->flags & JITDATA_FLAG_REORDER)

#define JITDATA_HAS_FLAG_INLINE(jd) \
    ((jd)->flags & JITDATA_FLAG_INLINE)

#define JITDATA_HAS_FLAG_COUNTDOWN(jd) \
    ((jd)->flags & JITDATA_FLAG_COUNTDOWN)

#define JITDATA_HAS_FLAG_SHOWINTERMEDIATE(jd) \
    ((jd)->flags & JITDATA_FLAG_SHOWINTERMEDIATE)

#define JITDATA_HAS_FLAG_SHOWDISASSEMBLE(jd) \
    ((jd)->flags & JITDATA_FLAG_SHOWDISASSEMBLE)

#define JITDATA_HAS_FLAG_VERBOSECALL(jd) \
    ((jd)->flags & JITDATA_FLAG_VERBOSECALL)


/* exception_entry ************************************************************/

struct exception_entry {
	basicblock           *start;
	basicblock           *end;
	basicblock           *handler;
	classref_or_classinfo catchtype; /* catchtype of exc. (NULL == catchall)  */
	exception_entry      *next;      /* next in list of exceptions when       */
									 /* loops are copied                      */
	exception_entry      *down;      /* next exception_entry                  */
};


/* macros for accessing variables *********************************************
 
   Use VAROP for s1, s2, s3 and dst operands (eg. VAROP(iptr->s1)),
   use VAR if you have the variable index (eg. VAR(iptr->sx.s23.s2.args[0])).

******************************************************************************/

#define VAROP(v) (jd->var + (v).varindex)
#define VAR(i)   (jd->var + (i))

static inline bool var_is_local(const jitdata *jd, s4 i) {
	return (i < jd->localcount);
}

static inline bool var_is_prealloc(const jitdata *jd, s4 i) {
	return ((i >= jd->localcount) && (jd->var[i].flags & PREALLOC));
}

static inline bool var_is_inout(const jitdata *jd, s4 i) {
	const varinfo *v = jd->var + i;
	return (
		(i >= jd->localcount) && (
			(!(v->flags & PREALLOC) && (v->flags & INOUT)) ||
			/* special case of TYPE_RET, used with JSR */
			((v->flags & PREALLOC) && (v->flags & INOUT) && (v->type == TYPE_RET))
		)
	);
}

static inline bool var_is_temp(const jitdata *jd, s4 i) {
	const varinfo *v = jd->var + i;
	return ((i >= jd->localcount) && !(v->flags & PREALLOC) && !(v->flags & INOUT));
}

static inline bool var_is_saved(const jitdata *jd, s4 i) {
	return (jd->var[i].flags & SAVEDVAR);
}


/**************************** instruction structure ***************************/

/* branch_target_t: used in TABLESWITCH tables */

typedef union {
    s4                         insindex; /* used in parse                     */
    basicblock                *block;    /* valid after parse                 */
} branch_target_t;

/* lookup_target_t: used in LOOKUPSWITCH tables */

typedef struct {
    s4                         value;    /* case value                        */
    branch_target_t            target;   /* branch target, see above          */
} lookup_target_t;

/*** s1 operand ***/

typedef union {
	s4                         varindex;
    s4                         argcount;
} s1_operand_t;

/*** s2 operand ***/

typedef union {
	s4                         varindex;
	s4                        *args;
    classref_or_classinfo      c;
    unresolved_class          *uc;
    ptrint                     constval;         /* for PUT*CONST             */
    s4                         tablelow;         /* for TABLESWITCH           */
    u4                         lookupcount;      /* for LOOKUPSWITCH          */
	s4                         retaddrnr;        /* for ASTORE                */
	instruction              **iargs;            /* for PHI                   */
} s2_operand_t;

/*** s3 operand ***/

typedef union {
	s4                         varindex;
    ptrint                     constval;
    classref_or_classinfo      c;
    constant_FMIref           *fmiref;
    unresolved_method         *um;
    unresolved_field          *uf;
    insinfo_inline            *inlineinfo;       /* for INLINE_START/END      */
    s4                         tablehigh;        /* for TABLESWITCH           */
    branch_target_t            lookupdefault;    /* for LOOKUPSWITCH          */
    branch_target_t            jsrtarget;        /* for JSR                   */
	s4                         javaindex;        /* for *STORE                */
    struct builtintable_entry *bte;
} s3_operand_t;

/*** val operand ***/

typedef union {
    s4                        i;
    s8                        l;
    float                     f;
    double                    d;
    void                     *anyptr;
    java_handle_t            *stringconst;       /* for ACONST with string    */
    classref_or_classinfo     c;                 /* for ACONST with class     */
} val_operand_t;

/*** dst operand ***/

typedef union {
	s4                         varindex;
    basicblock                *block;       /* valid after parse              */
    branch_target_t           *table;       /* for TABLESWITCH                */
    lookup_target_t           *lookup;      /* for LOOKUPSWITCH               */
    s4                         insindex;    /* used in parse                  */
} dst_operand_t;

/*** flags (32 bits) ***/

#define INS_FLAG_BASICBLOCK    0x01    /* marks a basic block start           */
#define INS_FLAG_UNRESOLVED    0x02    /* contains unresolved field/meth/class*/
#define INS_FLAG_CLASS         0x04    /* for ACONST, PUT*CONST with class    */
#define INS_FLAG_ARRAY         0x08    /* for CHECKCAST/INSTANCEOF with array */
#define INS_FLAG_CHECK         0x10    /* for *ALOAD|*ASTORE: check index     */
                                       /* for BUILTIN: check exception        */
#define INS_FLAG_KILL_PREV     0x04    /* for *STORE, invalidate prev local   */
#define INS_FLAG_KILL_NEXT     0x08    /* for *STORE, invalidate next local   */
#define INS_FLAG_RETADDR       0x10    /* for ASTORE: op is a returnAddress   */

#define INS_FLAG_ID_SHIFT      5
#define INS_FLAG_ID_MASK       (~0 << INS_FLAG_ID_SHIFT)

typedef union {
    u4                  bits;
} flags_operand_t;

/*** instruction ***/

/* The instruction format for the intermediate representation: */

struct instruction {
    u2                      opc;    /* opcode       */
    u2                      line;   /* line number  */
#if SIZEOF_VOID_P == 8
    flags_operand_t         flags;  /* 4 bytes      */
#endif
    s1_operand_t            s1;     /* pointer-size */
    union {
        struct {
            s2_operand_t    s2;     /* pointer-size */
            s3_operand_t    s3;     /* pointer-size */
        } s23;                      /*     XOR      */
        val_operand_t       val;    /*  long-size   */
    } sx;
    dst_operand_t           dst;    /* pointer-size */
#if SIZEOF_VOID_P == 4
    flags_operand_t         flags;  /* 4 bytes      */
#endif
#if defined(ENABLE_ESCAPE_REASON)
	void *escape_reasons;
#endif
};


#define INSTRUCTION_STARTS_BASICBLOCK(iptr) \
	((iptr)->flags.bits & INS_FLAG_BASICBLOCK)

#define INSTRUCTION_IS_RESOLVED(iptr) \
	(!((iptr)->flags.bits & INS_FLAG_UNRESOLVED))

#define INSTRUCTION_IS_UNRESOLVED(iptr) \
	((iptr)->flags.bits & INS_FLAG_UNRESOLVED)

#define INSTRUCTION_MUST_CHECK(iptr) \
	((iptr)->flags.bits & INS_FLAG_CHECK)

#define INSTRUCTION_GET_FIELDREF(iptr,fref) \
	do { \
		if (iptr->flags.bits & INS_FLAG_UNRESOLVED) \
			fref = iptr->sx.s23.s3.uf->fieldref; \
		else \
			fref = iptr->sx.s23.s3.fmiref; \
	} while (0)

#define INSTRUCTION_GET_METHODREF(iptr,mref) \
	do { \
		if (iptr->flags.bits & INS_FLAG_UNRESOLVED) \
			mref = iptr->sx.s23.s3.um->methodref; \
		else \
			mref = iptr->sx.s23.s3.fmiref; \
	} while (0)

#define INSTRUCTION_GET_METHODDESC(iptr, md) \
	do { \
		if (iptr->flags.bits & INS_FLAG_UNRESOLVED) \
			md = iptr->sx.s23.s3.um->methodref->parseddesc.md; \
		else \
			md = iptr->sx.s23.s3.fmiref->parseddesc.md; \
	} while (0)

/* additional info structs for special instructions ***************************/

/* for ICMD_INLINE_START and ICMD_INLINE_END */

struct insinfo_inline {
	/* fields copied from the inlining tree ----------------------------------*/
	insinfo_inline *parent;     /* insinfo of the surrounding inlining, if any*/
	methodinfo     *method;     /* the inlined method starting/ending here    */
	methodinfo     *outer;      /* the outer method suspended/resumed here    */
	s4              synclocal;      /* local index used for synchronization   */
	bool            synchronize;    /* true if synchronization is needed      */
	s4              throughcount;   /* total # of pass-through variables      */
	s4              paramcount;     /* number of parameters of original call  */
	s4              stackvarscount; /* source stackdepth at INLINE_START      */
	s4             *stackvars;      /* stack vars at INLINE_START             */

	/* fields set by inlining ------------------------------------------------*/
	s4         *javalocals_start; /* javalocals at start of inlined body      */
	s4         *javalocals_end;   /* javalocals after inlined body            */

	/* fields set by replacement point creation ------------------------------*/
#if defined(ENABLE_REPLACEMENT)
	rplpoint   *rp;             /* replacement point at INLINE_START          */
#endif

	/* fields set by the codegen ---------------------------------------------*/
	s4          startmpc;       /* machine code offset of start of inlining   */
};


/* basicblock *****************************************************************/

/* flags */

#define BBDELETED            -2
#define BBUNDEF              -1
#define BBREACHED            0
#define BBFINISHED           1

#define BBTYPECHECK_UNDEF    2
#define BBTYPECHECK_REACHED  3

#define BBTYPE_STD           0  /* standard basic block type                  */
#define BBTYPE_EXH           1  /* exception handler basic block type         */
#define BBTYPE_SBR           2  /* subroutine basic block type                */

#define BBFLAG_REPLACEMENT   0x01  /* put a replacement point at the start    */

/* XXX basicblock wastes quite a lot of memory by having four flag fields     */
/* (flags, bitflags, type and lflags). Probably the last three could be       */
/* combined without loss of efficiency. The first one could be combined with  */
/* the others by using bitfields.                                             */

/* XXX "flags" should probably be called "state", as it is an integer state   */

struct basicblock {
	s4            nr;           /* basic block number                         */
	s4            flags;        /* used during stack analysis, init with -1   */
	s4            bitflags;     /* OR of BBFLAG_... constants, init with 0    */
	s4            type;         /* basic block type (std, xhandler, subroutine*/
	s4            lflags;       /* used during loop copying, init with 0	  */

	s4            icount;       /* number of intermediate code instructions   */
	instruction  *iinstr;       /* pointer to intermediate code instructions  */

	varinfo      *inlocals;     /* copy of locals on block entry              */
	s4           *javalocals;   /* map from java locals to cacao variables[+] */
	s4           *invars;       /* array of in-variables at begin of block    */
	s4           *outvars;      /* array of out-variables at end of block     */
	s4            indepth;      /* stack depth at begin of basic block        */
	s4            outdepth;     /* stack depth end of basic block             */
	s4            varstart;     /* index of first non-invar block variable    */
	s4            varcount;     /* number of non-invar block variables        */

	s4            predecessorcount;
	s4            successorcount;
	basicblock  **predecessors; /* array of predecessor basic blocks          */
	basicblock  **successors;   /* array of successor basic blocks            */

	branchref    *branchrefs;   /* list of branches to be patched             */

	basicblock   *next;         /* used to build a BB list (instead of array) */
	basicblock   *copied_to;    /* points to the copy of this basic block	  */
                                /* when loop nodes are copied                 */
	basicblock   *original;     /* block of which this block is a clone       */
	                            /* NULL for the original block itself         */
	methodinfo   *method;       /* method this block belongs to               */
	insinfo_inline *inlineinfo; /* inlineinfo for the start of this block     */

	s4            mpc;          /* machine code pc at start of block          */

	/* TODO: those fields are probably usefull for other passes as well. */

#if defined(ENABLE_SSA)         
	basicblock   *idom;         /* Immediate dominator, parent in dominator tree */
	basicblock  **domsuccessors;/* Children in dominator tree                 */
	s4            domsuccessorcount;

	basicblock  **domfrontier;  /* Dominance frontier                         */
	s4            domfrontiercount;

	basicblock  **exhandlers;   /* Exception handlers for this block */
	s4            exhandlercount;
	basicblock  **expredecessors; /* Blocks this block is exception handler for */
	s4            expredecessorcount;
	s4            exouts;       /* Number of exceptional exits */

	instruction  *phis;         /* Phi functions */
	s4            phicount;     /* Number of phi functions */

	void         *vp;           /* Freely used by different passes            */
#endif
};

#define FOR_EACH_SUCCESSOR(bptr, it) \
	for ((it) = (bptr)->successors; (it) != (bptr)->successors + (bptr)->successorcount; ++(it))

#define FOR_EACH_PREDECESSOR(bptr, it) \
	for ( \
		(it) = (bptr)->predecessors; \
		(it) != (bptr)->predecessors + ((bptr)->predecessorcount < 0 ? 0 : (bptr)->predecessorcount); \
		++(it) \
	)

#define FOR_EACH_INSTRUCTION(bptr, it) \
	for ((it) = (bptr)->iinstr; (it) != (bptr)->iinstr + (bptr)->icount; ++(it))

#define FOR_EACH_INSTRUCTION_REV(bptr, it) \
	for ((it) = (bptr)->iinstr + (bptr)->icount - 1; (it) != (bptr)->iinstr - 1; --(it))

#if defined(ENABLE_SSA)

#define FOR_EACH_EXHANDLER(bptr, it) \
	for ((it) = (bptr)->exhandlers; (it) != (bptr)->exhandlers + (bptr)->exhandlercount; ++(it))

#define FOR_EACH_EXPREDECESSOR(bptr, it) \
	for ((it) = (bptr)->expredecessors; (it) != (bptr)->expredecessors + (bptr)->expredecessorcount; ++(it))

#endif

/* [+]...the javalocals array: This array is indexed by the javaindex (the    */
/*       local variable index ocurring in the original bytecode). An element  */
/*       javalocals[javaindex] encodes where to find the contents of the      */
/*       original variable at this point in the program.                      */
/*       There are three cases for javalocals[javaindex]:                     */
/*           >= 0.......it's an index into the jd->var array, where the       */
/*                      CACAO variable corresponding to the original local    */
/*                      can be found.                                         */
/*           UNUSED.....the original variable is not live at this point       */
/*           < UNUSED...the original variable contains a returnAddress at     */
/*                      this point. The number of the block to return to can  */
/*                      be calculated using RETADDR_FROM_JAVALOCAL:           */
/*                                                                            */
/*                      javalocals[javaindex] == JAVALOCAL_FROM_RETADDR(nr)   */
/*                      RETADDR_FROM_JAVALOCAL(javalocals[javaindex]) == nr   */

#define JAVALOCAL_FROM_RETADDR(nr)  (UNUSED - (1 + (nr)))
#define RETADDR_FROM_JAVALOCAL(jl)  (UNUSED - (1 + (jl)))


/* Macro for initializing newly allocated basic block's. It does not
   need to zero fields, as we zero out the whole basic block array. */

#define BASICBLOCK_INIT(bptr,m)                        \
	do {                                               \
		bptr->mpc    = -1;                             \
		bptr->flags  = -1;                             \
		bptr->type   = BBTYPE_STD;                     \
		bptr->method = (m);                            \
	} while (0)

static inline bool basicblock_reached(const basicblock *bptr) {
	return (bptr->flags >= BBREACHED);
}


/* Additional instruction accessors */

methoddesc *instruction_call_site(const instruction *iptr);

static inline bool instruction_has_dst(const instruction *iptr) {
	if (
		(icmd_table[iptr->opc].dataflow == DF_INVOKE) ||
		(icmd_table[iptr->opc].dataflow == DF_BUILTIN)
		) {
		return instruction_call_site(iptr)->returntype.type != TYPE_VOID;
	} else {
		return icmd_table[iptr->opc].dataflow >= DF_DST_BASE;
	}
}

/***************************** register types *********************************/

#define REG_RES   0         /* reserved register for OS or code generator     */
#define REG_RET   1         /* return value register                          */
#define REG_EXC   2         /* exception value register                       */
#define REG_SAV   3         /* (callee) saved register                        */
#define REG_TMP   4         /* scratch temporary register (caller saved)      */
#define REG_ARG   5         /* argument register (caller saved)               */

#define REG_END   -1        /* last entry in tables                           */
 
#define PARAMMODE_NUMBERED  0 
#define PARAMMODE_STUFFED   1


/* function prototypes ********************************************************/

/* compiler initialisation */
void jit_init(void);

/* compiler finalisation */
void jit_close(void);

/* create a new jitdata */
jitdata *jit_jitdata_new(methodinfo *m);

/* compile a method with jit compiler */
u1 *jit_compile(methodinfo *m);
u1 *jit_recompile(methodinfo *m);

void jit_invalidate_code(methodinfo *m);
codeinfo *jit_get_current_code(methodinfo *m);
void jit_request_optimization(methodinfo *m);

/* patch the method entrypoint */
#if !defined(JIT_COMPILER_VIA_SIGNAL)
u1 *jit_asm_compile(methodinfo *m, u1 *mptr, u1 *sp, u1 *ra);
#endif
void *jit_compile_handle(methodinfo *m, void *pv, void *ra, void *mptr);

s4 jit_complement_condition(s4 opcode);

void jit_renumber_basicblocks(jitdata *jd);
#if !defined(NDEBUG)
void jit_check_basicblock_numbers(jitdata *jd);
#endif


/* machine dependent functions ************************************************/

#if defined(ENABLE_JIT)
void md_init(void);
#endif

#if defined(ENABLE_INTRP)
void intrp_md_init(void);
#endif

void *md_jit_method_patch_address(void *pv, void *ra, void *mptr);

#endif /* _JIT_H */


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
