/* src/vm/jit/lsra.h - linear scan register allocator header

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

   Authors: Christian Ullrich

   $Id: lsra.h 3732 2005-11-22 14:36:16Z christian $

*/


#ifndef _LSRA_H
#define _LSRA_H

/* #define LSRA_DEBUG */  /* lsra debug messages */
/* #define LSRA_SAVEDVAR */
/* #define LSRA_MEMORY */
/* #define LSRA_PRINTLIFETIMES */
/* #define LSRA_USES_REG_RES */ /* is now in i386/codegen.h */
/*  #define LSRA_TESTLT */
/* #define LSRA_LEAF */
#if defined(__I386__) || defined(__X86_64__)
#define JOIN_DEST_STACK           /* The destination stackslot gets the same  */
        /* register as one of the src stackslots. Important for i386 & X86_64 */
        /* since they do not have "3 operand" arithmetic instructions to      */
        /* prevent usage of a reserved register (REG_ITMPX)                   */
#endif
#define JOIN_DUP_STACK         /* join "identical" stackslots created by dup* */

#define USAGE_COUNT        /* influence LSRA with usagecount */
#define USAGE_PER_INSTR    /* divide usagecount by lifetimelength */

#ifdef LSRA_DEBUG
#undef LSRA_LEAF
#endif

#ifdef LSRA_TESTLT
#define VS 999999
#endif


#ifdef LSRA_DEBUG
#define LSRA_PRINTLIFETIMES
#endif

#define LSRA_BB_IN 3
#define LSRA_BB_OUT 2
#define LSRA_STORE 1
#define LSRA_LOAD 0
#define LSRA_POP -1

/* join types and flags*/
#define JOIN     0 /* joins that are not in any way dangerous                 */
#define JOIN_BB  1 /* join Stackslots over Basic Block Boundaries             */
#define JOIN_DUP 2 /* join of two possibly concurring lifeteimes through DUP* */
#define JOIN_OP  4 /* join of src operand with dst operand on i386 and x86_64 */
                   /* architecture                                            */
                   /* JOIN_DUP and JOIN_OP is mutually exclusive as JOIN_OP   */
                   /* and JOIN_BB                                             */
#define JOINING  8 /* set while joining for DUP or OP to prevent assignement  */
                   /* to a REG_RES before all involved lifetimes have been    */
                   /* seen completely */

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))

struct _list {
	int value;
	struct _list *next;
};

struct _backedge {
	int start;
	int end;
	int nesting;
	struct _backedge *next;
};

struct lifetime {
	int i_start;                /* instruction number of first use */
	int i_end;                  /* instruction number of last use */
	int v_index;           /* local variable index or negative for stackslots */
	int type;                   /* TYPE_XXX or -1 for unused lifetime */
	long usagecount;            /* number of references*/
	int reg;                    /* regoffset allocated by lsra*/
	int savedvar;
	int flags;
	struct stackslot *local_ss; /* Stackslots for this Lifetime or NULL ( ==  */
                                /* "pure" Local Var) */
	int bb_last_use;
	int i_last_use;
	int bb_first_def;
	int i_first_def;
};

struct l_loop {
	int b_first;
	int b_last;
	int nesting;
};

struct b_loop {
	int loop;
	int instr;
};


struct stackslot {
	stackptr s;
	int bb;
	struct stackslot *next;
};

struct lsra_register {
	int *sav_reg;
	int *tmp_reg;
	int *nregdesc;
	int sav_top;
	int tmp_top;
};

struct lsra_reg {
	int reg_index;
	int use;
};

struct _sbr {
	int header;          /* BB Index of subroutine start (SBR_HEADER) */
	struct _list *ret;   /* List of possible return BB indizes */
	struct _sbr *next;
};

struct lsradata {
#if defined(LSRA_USES_REG_RES)
	int reg_res_free[REG_RES_CNT];
#endif
	struct _list **succ; /* CFG successors*/
	struct _list **pred; /* CFG predecessors */
	int *num_pred;       /* CFG number of predecessors */
	int *sorted;         /* BB sorted in reverse post order */
	int *sorted_rev;     /* BB reverse lookup of sorted */

	struct _backedge **backedge; /* backedge data structure */
	int backedge_count;          /* number of backedges */

	struct _sbr sbr;     /* list of subroutines, sorted by header */

	long *nesting;    /* Nesting level of BB*/

	int maxlifetimes; /* copy from methodinfo to prevent passing methodinfo   */
                      /* as parameter */

	struct lifetime *lifetime; /* array of lifetimes */
	int lifetimecount;         /* number of lifetimes */
	int *lt_used;              /* index to lifetimearray for used lifetimes   */
	int *lt_int;               /* index to lifetimearray for int lifetimes    */
	int lt_int_count;          /* number of int/[lng]/[adr] lifetimes */
	int *lt_flt;               /* index to lifetimearray for float lifetimes  */
	int lt_flt_count;          /* number of float/double lifetimes */
	int *lt_mem;               /* index to lifetimearray for all lifetimes    */
                               /* not to be allocated in registers */
	int lt_mem_count;          /* number of this other lifetimes */

	struct lifetime **active_tmp, **active_sav;
	int active_tmp_top, active_sav_top;

	struct lsra_exceptiontable *ex;
	int v_index;               /* next free index for stack slot lifetimes    */
	                           /* decrements from -1 */
};

struct freemem {
	int off;
	int end;
	struct freemem *next;
};

typedef struct lsradata lsradata;

/* function prototypes */
void lsra(methodinfo *, codegendata *, registerdata *,t_inlining_globals *);
bool lsra_test(methodinfo *, codegendata *);
void lsra_init(methodinfo *, codegendata *, t_inlining_globals *, lsradata *);
void lsra_setup(methodinfo *, codegendata *, registerdata *, lsradata *);
void lsra_main(methodinfo *, lsradata *, registerdata *, codegendata *);
void lsra_clean_Graph( methodinfo *, codegendata *, lsradata *);

#ifdef LSRA_DEBUG 
void lsra_dump_stack(stackptr );
#endif
#ifdef LSRA_PRINTLIFETIMES
void print_lifetimes(registerdata *, lsradata *, int *, int);
#endif
#ifdef LSRA_TESTLT
void test_lifetimes( methodinfo *, lsradata *, registerdata *);
#endif

void lsra_reg_setup(methodinfo *m ,registerdata *,struct lsra_register *,struct lsra_register * );


void lsra_scan_registers_canditates(methodinfo *,  lsradata *, int);

void lsra_join_lifetimes( methodinfo *, lsradata *, int);
void lsra_calc_lifetime_length(methodinfo *, lsradata *, codegendata *);

void _lsra_new_stack( lsradata *, stackptr , int , int, int);
void _lsra_from_stack(lsradata *, stackptr , int , int, int);
void lsra_add_ss(struct lifetime *, stackptr );
void lsra_usage_local(lsradata *, s4 , int , int , int , int );

void _lsra_main( methodinfo *, lsradata *, int *, int, struct lsra_register *, int *);
void lsra_expire_old_intervalls(methodinfo *, lsradata *, struct lifetime *, struct lsra_register *);
void spill_at_intervall(methodinfo *, lsradata *, struct lifetime *);
void lsra_add_active(struct lifetime *, struct lifetime **, int *);
void _lsra_expire_old_intervalls(methodinfo *, struct lifetime *, struct lsra_register *, struct lifetime **, int *);
void _spill_at_intervall(struct lifetime *, struct lifetime **, int *);

void lsra_alloc(methodinfo *, registerdata *, struct lsradata *, int *, int, int *);
int lsra_getmem(struct lifetime *, struct freemem *, int *);
struct freemem *lsra_getnewmem(int *);
void lsra_align_stackslots(struct lsradata *, stackptr, stackptr);
void lsra_setflags(int *, int);

#endif /* _LSRA_H */


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
