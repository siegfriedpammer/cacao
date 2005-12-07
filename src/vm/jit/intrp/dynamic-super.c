/* src/vm/jit/intrp/dynamic-super.c  dynamic superinstruction support

   Copyright (C) 1995,1996,1997,1998,2000,2003,2004 Free Software Foundation, Inc.
   Taken from Gforth.

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

   Authors: Christian Thalinger
            Anton Ertl

   Changes:

   $Id: dynamic-super.c 3895 2005-12-07 16:03:37Z anton $
*/


#define NO_IP 0 /* proper native code, without interpreter's ip */

#include "config.h"

#include <alloca.h>
#include <stdlib.h>
#include <assert.h>

#include "mm/memory.h"
#include "vm/options.h"
#include "vm/types.h"
#include "vm/jit/intrp/intrp.h"


s4 no_super=0;   /* option: just use replication, but no dynamic superinsts */

static char MAYBE_UNUSED superend[]={
#include "java-superend.i"
};

const char * const prim_names[]={
#include "java-names.i"
};

enum {
#define INST_ADDR(_inst) N_##_inst
#include "java-labels.i"
#undef INST_ADDR
};

#define MAX_IMMARGS 1

typedef struct {
  Label start; /* NULL if not relocatable */
  s4 length; /* only includes the jump iff superend is true*/
  s4 restlength; /* length of the rest (i.e., the jump or (on superend) 0) */
  char superend; /* true if primitive ends superinstruction, i.e.,
                     unconditional branch, execute, etc. */
  s4 nimmargs;
  struct immarg {
    s4 offset; /* offset of immarg within prim */
    char rel;    /* true if immarg is relative */
  } immargs[MAX_IMMARGS];
} PrimInfo;

PrimInfo *priminfos;
s4 npriminfos;
Label before_goto;
u4 goto_len;

typedef struct superstart {
  struct superstartlist *next;
  s4 dynsuperm;
  s4 dynsupern;
} superstart;

# define debugp(x...) if (opt_verbose) fprintf(x)

java_objectheader *engine2(Inst *ip0, Cell * sp, Cell * fp);

static int compare_labels(const void *pa, const void *pb)
{
  char *a = *(char **)pa;
  char *b = *(char **)pb;
  return a-b;
}

static Label bsearch_next(Label key, Label *a, u4 n)
     /* a is sorted; return the label >=key that is the closest in a;
        return NULL if there is no label in a >=key */
{
  int mid = (n-1)/2;
  if (n<1)
    return NULL;
  if (n == 1) {
    if (a[0] < key)
      return NULL;
    else
      return a[0];
  }
  if (a[mid] < key)
    return bsearch_next(key, a+mid+1, n-mid-1);
  else
    return bsearch_next(key, a, mid+1);
}

#if 0
  each VM instruction <x> looks like this:
  H_<x>:
    <skip>
  I_<x>: /* entry point */
    ...
  J_<x>: /* start of dispatch */
    NEXT_P1_5; /* dispatch except GOTO */
  K_<x>: /* just before goto */
    DO_GOTO; /* goto part of dispatch */

/* 
 * We need I_<x> for threaded code: this is the code address stored in
 * direct threaded code.

 * We need the <skip> to detect non-relocatability (by comparing
 * engine and engine2 with different skips).  H_<x> is there to ensure
 * that gcc does not think that <skip> is dead code.

 * We need J_<x> to implement dynamic superinstructions: copy
 * everything between I_<x> and J_<x> to get a VM instruction without
 * dispatch.

 * At the end of a dynamic superinstruction we need a dispatch.  We
 * need the DO_GOTO to work around gcc bug 15242: we copy everything
 * up to K_<x> and then copy the indirect jump from &&before_goto.
 */

#endif

void check_prims(Label symbols1[])
{
  int i;
  Label *symbols2, *symbols3, *js1, *ks1, *ks1sorted;
  Label after_goto, before_goto2;

  if (opt_verbose)
#ifdef __VERSION__
    fprintf(stderr, "Compiled with gcc-" __VERSION__ "\n");
#endif
  for (i=0; symbols1[i]!=0; i++)
    ;
  npriminfos = i;
  
#ifndef NO_DYNAMIC
  if (opt_no_dynamic)
    return;
  symbols2=(Inst *)engine2(0,0,0);
#if NO_IP
  symbols3=(Inst *)engine3(0,0,0);
#else
  symbols3=symbols1;
#endif
  js1 = symbols1+i+1;
  ks1 = js1+i;
  before_goto = ks1[i+1]; /* after ks and after after_last */
  after_goto = ks1[i+2];
  before_goto2 = (ks1+(symbols2-symbols1))[i+1];
  /* some gccs reorder the code; below we look for the next K label
     with binary search, and whether it belongs to the current
     instruction; prepare for this by sorting the K labels */
  ks1sorted = (Label *)alloca((i+1)*sizeof(Label));
  MCOPY(ks1sorted,ks1,Label,i+1);
  qsort(ks1sorted, i+1, sizeof(Label), compare_labels);

  /* check whether the "goto *" is relocatable */
  goto_len = after_goto-before_goto;
  debugp(stderr, "goto * %p %p len=%ld\n",
	 before_goto,before_goto2,goto_len);
  if (memcmp(before_goto, before_goto2, goto_len)!=0) { /* unequal */
    opt_no_dynamic = true;
    debugp(stderr,"  not relocatable, disabling dynamic code generation\n");
    return;
  }
  
  priminfos = calloc(i,sizeof(PrimInfo));
  for (i=0; symbols1[i]!=0; i++) {
    /* check whether the VM instruction i has the same code in
       symbols1 and symbols2 (then it is relocatable).  Also, for real
       native code (not activated), check where the versions in
       symbols1 and symbols 3 differ; these are places for
       patching. */
    int prim_len = js1[i]-symbols1[i];
    PrimInfo *pi=&priminfos[i];
    int j=0;
    char *s1 = (char *)symbols1[i];
    char *s2 = (char *)symbols2[i];
    char *s3 = (char *)symbols3[i];
    Label endlabel = bsearch_next(s1+1,ks1sorted,npriminfos+1);

    pi->start = s1;
    pi->superend = superend[i]|no_super;
    pi->length = prim_len;
    pi->restlength = endlabel - symbols1[i] - pi->length;
    pi->nimmargs = 0;

    debugp(stderr, "%-15s %3d %p %p %p len=%3ld restlen=%2ld s-end=%1d",
	      prim_names[i], i, s1, s2, s3, (long)(pi->length), (long)(pi->restlength), pi->superend);
    if (endlabel == NULL) {
      pi->start = NULL; /* not relocatable */
      if (pi->length<0) pi->length=1000;
      debugp(stderr,"\n   non_reloc: no K label > start found\n");
      continue;
    }
    if (js1[i] > endlabel && !pi->superend) {
      pi->start = NULL; /* not relocatable */
      pi->length = endlabel-symbols1[i];
      debugp(stderr,"\n   non_reloc: there is a K label before the J label (restlength<0)\n");
      continue;
    }
    if (js1[i] < pi->start && !pi->superend) {
      pi->start = NULL; /* not relocatable */
      pi->length = endlabel-symbols1[i];
      debugp(stderr,"\n   non_reloc: J label before I label (length<0)\n");
      continue;
    }
    assert(pi->length>=0);
    assert(pi->restlength >=0);
    while (j<(pi->length+pi->restlength)) {
      if (s1[j]==s3[j]) {
	if (s1[j] != s2[j]) {
	  pi->start = NULL; /* not relocatable */
	  debugp(stderr,"\n   non_reloc: engine1!=engine2 offset %3d",j);
	  /* assert(j<prim_len); */
	  break;
	}
	j++;
      } else {
        /* only used for NO_IP native code generation */
        /* here we find differences in inline arguments */
        assert(false);
      }
    }
    debugp(stderr,"\n");
  }
#endif
}

static bool is_relocatable(u4 p)
{
  return !opt_no_dynamic && priminfos[p].start != NULL;
}

static
void append_prim(codegendata *cd, u4 p)
{
  PrimInfo *pi = &priminfos[p];
  debugp(stderr,"append_prim %p %s\n",cd->mcodeptr, prim_names[p]);
  if (cd->ncodeptr + pi->length + pi->restlength + goto_len >
      cd->ncodebase + cd->ncodesize) {
    cd->ncodeptr = codegen_ncode_increase(cd, cd->ncodeptr);
  }
  memcpy(cd->ncodeptr, pi->start, pi->length);
  cd->ncodeptr += pi->length;
}

static void init_dynamic_super(codegendata *cd)
{
  cd->lastpatcheroffset = -1;
  cd->dynsuperm = -1;  /* the next VM instr may be non-relocatable */
  cd->dynsupern = cd->ncodeptr - cd->ncodebase;
}

void dynamic_super_rewrite(codegendata *cd)
     /* rewrite the threaded code in the superstarts list to point to
        the dynamic superinstructions */
{
  superstart *ss = cd->superstarts;
  for (; ss != NULL; ss = ss->next) {
    *(Inst *)(cd->mcodebase + ss->dynsuperm) =
      (Inst)(cd->ncodebase + ss->dynsupern);
    debugp(stderr, "rewrote %p into %p\n", 
            cd->mcodebase + ss->dynsuperm, cd->ncodebase + ss->dynsupern); 
  }
}

static void new_dynamic_super(codegendata *cd)
     /* add latest dynamic superinstruction to the appropriate table
        for rewriting the threaded code either on relocation (if there
        is no patcher), or when the last patcher is executed (if there
        is a patcher). */
{
  if (cd->lastpatcheroffset==-1) {
    superstart *ss = DNEW(superstart);
    ss->dynsuperm = cd->dynsuperm;
    ss->dynsupern = cd->dynsupern;
    ss->next = cd->superstarts;
    cd->superstarts = ss;
  } else {
  }
}

void append_dispatch(codegendata *cd)
{
  debugp(stderr,"append_dispatch %p\n",cd->mcodeptr);
  if (cd->lastinstwithoutdispatch != ~0) {
    PrimInfo *pi = &priminfos[cd->lastinstwithoutdispatch];
    
    memcpy(cd->ncodeptr, pi->start + pi->length, pi->restlength);
    cd->ncodeptr += pi->restlength;
    memcpy(cd->ncodeptr, before_goto, goto_len);
    cd->ncodeptr += goto_len;
    cd->lastinstwithoutdispatch = ~0;
    new_dynamic_super(cd);
  }
  init_dynamic_super(cd);
}

static void compile_prim_dyn(codegendata *cd, u4 p)
     /* compile prim #p dynamically (mod flags etc.)  */
{
  if (opt_no_dynamic)
    return;
  assert(p<npriminfos);
  if (!is_relocatable(p)) {
    append_dispatch(cd); /* to previous dynamic superinstruction */
    return;
  }
  if (cd->dynsuperm == -1)
    cd->dynsuperm = cd->mcodeptr - cd->mcodebase;
  append_prim(cd, p);
  cd->lastinstwithoutdispatch = p;
  if (priminfos[p].superend)
    append_dispatch(cd);
  return;
}

static void replace_patcher(codegendata *cd, u4 p)
     /* compile p dynamically, and note that there is a patcher here */
{
  cd->lastpatcheroffset = cd->mcodeptr - cd->mcodebase;
  compile_prim_dyn(cd, p);
}

void gen_inst(codegendata *cd, u4 instr)
{
  /* actually generate the threaded code instruction */

  debugp(stderr, "gen_inst %p, %s\n", cd->mcodeptr, prim_names[instr]);
  *((Inst *) cd->mcodeptr) = vm_prim[instr];
  switch (instr) {
  case N_PATCHER_ACONST:         replace_patcher(cd, N_ACONST1); break;
  case N_PATCHER_ARRAYCHECKCAST: replace_patcher(cd, N_ARRAYCHECKCAST); break;
  case N_PATCHER_GETSTATIC_INT:  replace_patcher(cd, N_GETSTATIC_INT); break;
  case N_PATCHER_GETSTATIC_LONG: replace_patcher(cd, N_GETSTATIC_LONG); break;
  case N_PATCHER_GETSTATIC_CELL: replace_patcher(cd, N_GETSTATIC_CELL); break;
  case N_PATCHER_PUTSTATIC_INT:  replace_patcher(cd, N_PUTSTATIC_INT); break;
  case N_PATCHER_PUTSTATIC_LONG: replace_patcher(cd, N_PUTSTATIC_LONG); break;
  case N_PATCHER_PUTSTATIC_CELL: replace_patcher(cd, N_PUTSTATIC_CELL); break;
  case N_PATCHER_GETFIELD_INT:   replace_patcher(cd, N_GETFIELD_INT); break;
  case N_PATCHER_GETFIELD_LONG:  replace_patcher(cd, N_GETFIELD_LONG); break;
  case N_PATCHER_GETFIELD_CELL:  replace_patcher(cd, N_GETFIELD_CELL); break;
  case N_PATCHER_PUTFIELD_INT:   replace_patcher(cd, N_PUTFIELD_INT); break;
  case N_PATCHER_PUTFIELD_LONG:  replace_patcher(cd, N_PUTFIELD_LONG); break;
  case N_PATCHER_PUTFIELD_CELL:  replace_patcher(cd, N_PUTFIELD_CELL); break;
  case N_PATCHER_MULTIANEWARRAY: replace_patcher(cd, N_MULTIANEWARRAY); break;
  case N_PATCHER_INVOKESTATIC:   replace_patcher(cd, N_INVOKESTATIC); break;
  case N_PATCHER_INVOKESPECIAL:  replace_patcher(cd, N_INVOKESPECIAL); break;
  case N_PATCHER_INVOKEVIRTUAL:  replace_patcher(cd, N_INVOKEVIRTUAL); break;
  case N_PATCHER_INVOKEINTERFACE:replace_patcher(cd, N_INVOKEINTERFACE); break;
  case N_PATCHER_CHECKCAST:      replace_patcher(cd, N_CHECKCAST); break;
  case N_PATCHER_INSTANCEOF:     replace_patcher(cd, N_INSTANCEOF); break;
  case N_PATCHER_NATIVECALL:     
    if (runverbose)
      replace_patcher(cd, N_TRACENATIVECALL);
    else
      replace_patcher(cd, N_NATIVECALL);
    break;
  case N_TRANSLATE: break;

  default:
    compile_prim_dyn(cd, instr);
    break;
  }
  cd->mcodeptr += sizeof(Inst); /* cd->mcodeptr used in compile_prim_dyn() */
}

