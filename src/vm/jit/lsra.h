/* jit/lsra.inc - linear scan register allocator header

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger, C. Ullrich

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

   $Id: lsra.h 2014 2005-03-08 06:27:57Z christian $

*/


#ifndef _LSRA_H
#define _LSRA_H

/*   #define LSRA_DEBUG */
/* #define LSRA_SAVEDVAR */
/* #define LSRA_MEMORY */
/* #define LSRA_PRINTLIFETIMES */
/* #define LSRA_EDX */


#ifdef LSRA_DEBUG
#define LSRA_PRINTLIFETIMES
#endif

#define LSRA_STORE 1
#define LSRA_LOAD 0
#define LSRA_POP -1

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)<(b)?(b):(a))

#ifdef __I386__
struct tmp_reg {
	int eax;
	int ecx;
	int edx;
};

extern struct tmp_reg icmd_uses_tmp[256];
#endif

struct _list {
	int value;
	struct _list *next;
};

struct _backedge {
	int start;
	int end;
	struct _backedge *next;
};

struct lifetime {
	int i_start; /* instruction number of first use */
	int i_end; /* instruction number of last use */
	int v_index; /* local variable index or negative for stackslots */
	int type;
	int usagecount; /* number of references*/
	int reg; /* regoffset durch lsra zugewiesen */
	int savedvar;
	struct stackslot *passthrough; /* List of Stackslots in Lifetime, which are passed through a Basic Block */
	struct stackslot *local_ss; /* Stackslots for this Lifetime or NULL (=="pure" Local Var) */

	int bb_last_use;
	int i_last_use;
	int bb_first_def;
	int i_first_def;
	struct lifetime *next;
};

struct active_lt {
	struct lifetime *lt;
	struct active_lt *next;
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

struct lsra_reg {
	int reg_index;
	int use;
};

struct lsradata {
#ifdef __I386__
	int edx_free;
#endif
	struct _list **succ;
	struct _list **pred;
	int *num_pred;
	int *sorted;
	int *sorted_rev;
	struct _backedge *_backedges;
	struct _backedge **backedge;
	int backedge_count;
	struct lifetime *ss_lifetimes;
	struct lifetime **locals_lifetimes;
	struct lifetime *lifetimes;
	struct stackslot *stackslots;
	struct active_lt *active_tmp, *active_sav;
	int active_sav_count, active_tmp_count;
	struct lsra_exceptiontable *ex;
	int icount_max;
	int end_bb;
	int v_index;
};

struct freemem {
	int off;
	int end;
	struct freemem *next;
};

struct lsra_exceptiontable {
	int handler_min;
	int handler_max;
	int guarded_min;
	int guarded_max;
};

typedef struct lsradata lsradata;

/* function prototypes */
bool lsra(methodinfo *, codegendata *, registerdata *,t_inlining_globals *);
bool lsra_test(methodinfo *, codegendata *);
void lsra_init(methodinfo *, codegendata *, t_inlining_globals *, lsradata *);
bool lsra_setup(methodinfo *, codegendata *, registerdata *, lsradata *);
void lsra_main(methodinfo *, lsradata *, registerdata *, codegendata *);
void lsra_clean_Graph( methodinfo *, codegendata *, lsradata *);

#ifdef LSRA_DEBUG 
void lsra_dump_stack(stackptr );
#endif
#ifdef LSRA_PRINTLIFETIMES
void print_lifetimes(registerdata *, lsradata *, struct lifetime *);
#endif


void lsra_scan_registers_canditates(methodinfo *,  lsradata *, int);

void lsra_join_lifetimes( methodinfo *, lsradata *, int);
void lsra_calc_lifetime_length(methodinfo *, lsradata *, codegendata *);

void lsra_merge_local_ss(struct lifetime *, struct lifetime *);

void _lsra_new_stack( lsradata *, stackptr , int , int, int);
void _lsra_from_stack(lsradata *, stackptr , int , int, int);
void lsra_add_ss(struct lifetime *, stackptr );
void lsra_usage_local(lsradata *, s4 , int , int , int , int );
void lsra_new_local(lsradata *, s4 , int );

void lsra_sort_lt(struct lifetime **);
void _lsra_main( methodinfo *, lsradata *, struct lifetime *, struct lsra_reg *, int , int , int *, int *);
void lsra_expire_old_intervalls(lsradata *, struct lifetime *, struct lsra_reg *);
void _lsra_expire_old_intervalls(struct lifetime *, struct lsra_reg *, struct active_lt **, int *);
void spill_at_intervall(lsradata *, struct lifetime *);
void _spill_at_intervall(struct lifetime *, struct active_lt **, int *);
void lsra_add_active(struct lifetime *, struct active_lt **, int *);
void lsra_alloc(methodinfo *, registerdata *, struct lifetime *, int *);
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
