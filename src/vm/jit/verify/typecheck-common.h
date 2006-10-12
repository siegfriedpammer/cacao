#ifndef _TYPECHECK_COMMON_H
#define _TYPECHECK_COMMON_H

#include "config.h"
#include "vm/types.h"
#include "vm/global.h"

#include <assert.h>

#include "vm/jit/jit.h"


/****************************************************************************/
/* DEBUG HELPERS                                                            */
/****************************************************************************/

#ifdef TYPECHECK_DEBUG
#define TYPECHECK_ASSERT(cond)  assert(cond)
#else
#define TYPECHECK_ASSERT(cond)
#endif

#ifdef TYPECHECK_VERBOSE_OPT
extern bool opt_typecheckverbose;
#define DOLOG(action)  do { if (opt_typecheckverbose) {action;} } while(0)
#else
#define DOLOG(action)
#endif

#ifdef TYPECHECK_VERBOSE
#define TYPECHECK_VERBOSE_IMPORTANT
#define LOGNL              DOLOG(puts(""))
#define LOG(str)           DOLOG(puts(str);)
#define LOG1(str,a)        DOLOG(printf(str,a); LOGNL)
#define LOG2(str,a,b)      DOLOG(printf(str,a,b); LOGNL)
#define LOG3(str,a,b,c)    DOLOG(printf(str,a,b,c); LOGNL)
#define LOGIF(cond,str)    DOLOG(do {if (cond) { puts(str); }} while(0))
#ifdef  TYPEINFO_DEBUG
#define LOGINFO(info)      DOLOG(do {typeinfo_print_short(stdout,(info)); LOGNL;} while(0))
#else
#define LOGINFO(info)
#define typevector_print(x,y,z)
#endif
#define LOGFLUSH           DOLOG(fflush(stdout))
#define LOGSTR(str)        DOLOG(printf("%s", str))
#define LOGSTR1(str,a)     DOLOG(printf(str,a))
#define LOGSTR2(str,a,b)   DOLOG(printf(str,a,b))
#define LOGSTR3(str,a,b,c) DOLOG(printf(str,a,b,c))
#define LOGNAME(c)         DOLOG(class_classref_or_classinfo_print(c))
#define LOGMETHOD(str,m)   DOLOG(printf("%s", str); method_println(m);)
#else
#define LOG(str)
#define LOG1(str,a)
#define LOG2(str,a,b)
#define LOG3(str,a,b,c)
#define LOGIF(cond,str)
#define LOGINFO(info)
#define LOGFLUSH
#define LOGNL
#define LOGSTR(str)
#define LOGSTR1(str,a)
#define LOGSTR2(str,a,b)
#define LOGSTR3(str,a,b,c)
#define LOGNAME(c)
#define LOGMETHOD(str,m)
#endif

#ifdef TYPECHECK_VERBOSE_IMPORTANT
#define LOGimp(str)     DOLOG(puts(str);LOGNL)
#define LOGimpSTR(str)  DOLOG(puts(str))
#else
#define LOGimp(str)
#define LOGimpSTR(str)
#endif

#if defined(TYPECHECK_VERBOSE) || defined(TYPECHECK_VERBOSE_IMPORTANT)
#include <stdio.h>
void typecheck_print_var(FILE *file, jitdata *jd, s4 index);
void typecheck_print_vararray(FILE *file, jitdata *jd, s4 *vars, int len);
#endif


/****************************************************************************/
/* STATISTICS                                                               */
/****************************************************************************/

#ifdef TYPECHECK_DEBUG
/*#define TYPECHECK_STATISTICS*/
#endif

#ifdef TYPECHECK_STATISTICS
#define STAT_ITERATIONS  10
#define STAT_BLOCKS      10
#define STAT_LOCALS      16

extern int stat_typechecked;
extern int stat_methods_with_handlers;
extern int stat_methods_maythrow;
extern int stat_iterations[STAT_ITERATIONS+1];
extern int stat_reached;
extern int stat_copied;
extern int stat_merged;
extern int stat_merging_changed;
extern int stat_blocks[STAT_BLOCKS+1];
extern int stat_locals[STAT_LOCALS+1];
extern int stat_ins;
extern int stat_ins_maythrow;
extern int stat_ins_stack;
extern int stat_ins_field;
extern int stat_ins_field_unresolved;
extern int stat_ins_field_uninitialized;
extern int stat_ins_invoke;
extern int stat_ins_invoke_unresolved;
extern int stat_ins_primload;
extern int stat_ins_aload;
extern int stat_ins_builtin;
extern int stat_ins_builtin_gen;
extern int stat_ins_branch;
extern int stat_ins_switch;
extern int stat_ins_primitive_return;
extern int stat_ins_areturn;
extern int stat_ins_areturn_unresolved;
extern int stat_ins_athrow;
extern int stat_ins_athrow_unresolved;
extern int stat_ins_unchecked;
extern int stat_handlers_reached;
extern int stat_savedstack;

#define TYPECHECK_MARK(var)   ((var) = true)
#define TYPECHECK_COUNT(cnt)  (cnt)++
#define TYPECHECK_COUNTIF(cond,cnt)  do{if(cond) (cnt)++;} while(0)
#define TYPECHECK_COUNT_FREQ(array,val,limit) \
	do {									  \
		if ((val) < (limit)) (array)[val]++;  \
		else (array)[limit]++;				  \
	} while (0)

#else
						   
#define TYPECHECK_COUNT(cnt)
#define TYPECHECK_MARK(var)
#define TYPECHECK_COUNTIF(cond,cnt)
#define TYPECHECK_COUNT_FREQ(array,val,limit)
#endif


/****************************************************************************/
/* MACROS FOR THROWING EXCEPTIONS                                           */
/****************************************************************************/

#define TYPECHECK_VERIFYERROR_ret(m,msg,retval)                      \
    do {                                                             \
        exceptions_throw_verifyerror((m), (msg));                    \
        return (retval);                                             \
    } while (0)

#define TYPECHECK_VERIFYERROR_main(msg)  TYPECHECK_VERIFYERROR_ret(state.m,(msg),NULL)
#define TYPECHECK_VERIFYERROR_bool(msg)  TYPECHECK_VERIFYERROR_ret(state->m,(msg),false)


/****************************************************************************/
/* VERIFIER STATE STRUCT                                                    */
/****************************************************************************/

/* verifier_state - This structure keeps the current state of the      */
/* bytecode verifier for passing it between verifier functions.        */

typedef struct verifier_state {
    instruction *iptr;               /* pointer to current instruction */
    basicblock *bptr;                /* pointer to current basic block */

	methodinfo *m;                               /* the current method */
	jitdata *jd;                         /* jitdata for current method */
	codegendata *cd;                 /* codegendata for current method */

	basicblock *basicblocks;
	s4 basicblockcount;
	
	s4 numlocals;                         /* number of local variables */
	s4 validlocals;                /* number of Java-accessible locals */
	s4 *reverselocalmap;
	
	typedescriptor returntype;    /* return type of the current method */

	s4 *savedindices;
	s4 *savedinvars;                            /* saved invar pointer */

	s4 exinvars;
	
    exceptiontable **handlers;            /* active exception handlers */
	
    bool repeat;            /* if true, blocks are iterated over again */
    bool initmethod;             /* true if this is an "<init>" method */

#ifdef TYPECHECK_STATISTICS
	bool stat_maythrow;          /* at least one instruction may throw */
#endif
} verifier_state;

void typecheck_init_flags(verifier_state *state, s4 minflags);
void typecheck_reset_flags(verifier_state *state);

#endif /* _TYPECHECK_COMMON_H */

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
