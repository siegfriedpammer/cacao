/* vm/builtin.h - prototypes of builtin functions

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

   Authors: Reinhard Grafl

   Changes: Edwin Steiner

   $Id: builtin.h 1893 2005-01-31 17:06:41Z twisti $

*/


#ifndef _BUILTIN_H
#define _BUILTIN_H

#include "arch.h"
#include "config.h"
#include "toolbox/logging.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
# endif
#endif


/* define infinity for floating point numbers */

#define FLT_NAN     0x7fc00000
#define FLT_POSINF  0x7f800000
#define FLT_NEGINF  0xff800000

/* define infinity for double floating point numbers */

#define DBL_NAN     0x7ff8000000000000LL
#define DBL_POSINF  0x7ff0000000000000LL
#define DBL_NEGINF  0xfff0000000000000LL


/* float versions are not defined in gnu classpath's fdlibm */

#define copysignf    copysign
#define finitef      finite
#define fmodf        fmod
#define isnanf       isnan


/**********************************************************************/
/* BUILTIN FUNCTIONS TABLE                                            */
/**********************************************************************/

/* IMPORTANT:
 * For each builtin function which is used in a BUILTIN* opcode there
 * must be an entry in the builtin_desc table in jit/jit.c.
 */
 
typedef struct builtin_descriptor builtin_descriptor;

/* There is a builtin_descriptor in builtin_desc for every builtin
 * function used in BUILTIN* opcodes.
 */
struct builtin_descriptor {
	int         opcode;   /* opcode which is replaced by this builtin */
	                      /* (255 means no automatic replacement,     */
	                      /*    0 means end of list.)                 */
	functionptr builtin;  /* the builtin function (specify BUILTIN_...*/
	                      /* macro)                                   */
	int         icmd;     /* the BUILTIN* opcode to use (# of args)   */
	u1          type_s1;  /* type of 1st argument                     */
	u1          type_s2;  /* type of 2nd argument, or TYPE_VOID       */
	u1          type_s3;  /* type of 3rd argument, or TYPE_VOID       */
	u1          type_d;   /* type of result (may be TYPE_VOID)        */
	bool        supported;/* is <opcode> supported without builtin?   */
	bool        isfloat;  /* is this a floating point operation?      */
	char        *name;    /* display name of the builtin function     */
};

extern builtin_descriptor builtin_desc[];

/**********************************************************************/
/* GLOBAL VARIABLES                                                   */
/**********************************************************************/

#define THREADSPECIFIC
#define exceptionptr (&_exceptionptr)
#define threadrootmethod (&_threadrootmethod)

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
#ifdef HAVE___THREAD

#undef THREADSPECIFIC
#define THREADSPECIFIC __thread

#endif

#undef exceptionptr
#undef threadrootmethod
#define exceptionptr builtin_get_exceptionptrptr()
#define threadrootmethod  builtin_get_threadrootmethod()
#endif

#if !defined(USE_THREADS) || !defined(NATIVE_THREADS)
extern java_objectheader *_exceptionptr;
extern methodinfo* _threadrootmethod;
#endif


/**********************************************************************/
/* BUILTIN FUNCTIONS                                                  */
/**********************************************************************/

/* NOTE: Builtin functions which are used in the BUILTIN* opcodes must
 * have a BUILTIN_... macro defined as seen below. In code dealing
 * with the BUILTIN* opcodes the functions may only be addressed by
 * these macros, never by their actual name! (This helps to make this
 * code more portable.)
 *
 * C and assembler code which does not deal with the BUILTIN* opcodes,
 * can use the builtin functions normally (like all other functions).
 *
 * IMPORTANT:
 * For each builtin function which is used in a BUILTIN* opcode there
 * must be an entry in the builtin_desc table in jit/jit.c.
 *
 * Below each prototype is either the BUILTIN_ macro definition or a
 * comment specifiying that this function is not used in BUILTIN*
 * opcodes.
 *
 * (The BUILTIN* opcodes are ICMD_BUILTIN1, ICMD_BUILTIN2 and
 * ICMD_BUILTIN3.)
 */

s4 builtin_instanceof(java_objectheader *obj, classinfo *class);
#define BUILTIN_instanceof (functionptr) builtin_instanceof
s4 builtin_isanysubclass (classinfo *sub, classinfo *super);
/* NOT AN OP */
s4 builtin_isanysubclass_vftbl (vftbl_t *sub, vftbl_t *super);
/* NOT AN OP */
s4 builtin_checkcast(java_objectheader *obj, classinfo *class);
/* NOT AN OP */
s4 builtin_arrayinstanceof(java_objectheader *obj, vftbl_t *target);
#define BUILTIN_arrayinstanceof (functionptr) builtin_arrayinstanceof

#if defined(__I386__)
s4 asm_builtin_arrayinstanceof(java_objectheader *obj, classinfo *class); /* XXX ? */
#undef  BUILTIN_arrayinstanceof
#define BUILTIN_arrayinstanceof (functionptr) asm_builtin_arrayinstanceof
#endif

s4 builtin_checkarraycast(java_objectheader *obj, vftbl_t *target);
/* NOT AN OP */
s4 asm_builtin_checkarraycast(java_objectheader *obj, vftbl_t *target);
#define BUILTIN_checkarraycast (functionptr) asm_builtin_checkarraycast

java_objectheader *builtin_throw_exception(java_objectheader *exception);
/* NOT AN OP */
java_objectheader *builtin_trace_exception(java_objectheader *xptr,
										   methodinfo *m,
										   void *pos,
										   s4 line,
										   s4 noindent);
/* NOT AN OP */

java_objectheader *builtin_new(classinfo *c);
#ifdef __I386__
java_objectheader *asm_builtin_new(classinfo *c);
#define BUILTIN_new (functionptr) asm_builtin_new
#else
#define BUILTIN_new (functionptr) builtin_new
#endif

java_arrayheader *builtin_newarray(s4 size, vftbl_t *arrayvftbl);
#define BUILTIN_newarray (functionptr) builtin_newarray
java_objectarray *builtin_anewarray(s4 size, classinfo *component);
/* NOT AN OP */

#if defined(__I386__)
void asm_builtin_newarray(s4 size, vftbl_t *arrayvftbl);
#undef  BUILTIN_newarray
#define BUILTIN_newarray (functionptr) asm_builtin_newarray
#endif

java_booleanarray *builtin_newarray_boolean(s4 size);
#define BUILTIN_newarray_boolean (functionptr) builtin_newarray_boolean
java_chararray *builtin_newarray_char(s4 size);
#define BUILTIN_newarray_char (functionptr) builtin_newarray_char
java_floatarray *builtin_newarray_float(s4 size);
#define BUILTIN_newarray_float (functionptr) builtin_newarray_float
java_doublearray *builtin_newarray_double(s4 size);
#define BUILTIN_newarray_double (functionptr) builtin_newarray_double
java_bytearray *builtin_newarray_byte(s4 size);
#define BUILTIN_newarray_byte (functionptr) builtin_newarray_byte
java_shortarray *builtin_newarray_short(s4 size);
#define BUILTIN_newarray_short (functionptr) builtin_newarray_short
java_intarray *builtin_newarray_int(s4 size);
#define BUILTIN_newarray_int (functionptr) builtin_newarray_int
java_longarray *builtin_newarray_long(s4 size);
#define BUILTIN_newarray_long (functionptr) builtin_newarray_long
java_arrayheader *builtin_nmultianewarray(int n, vftbl_t *arrayvftbl, long *dims);
/*  java_arrayheader *builtin_nmultianewarray(int n, classinfo *arrayclass, long *dims); */
/* NOT AN OP */

s4 builtin_canstore(java_objectarray *a, java_objectheader *o);
/* NOT AN OP */
void asm_builtin_aastore(java_objectarray *a, s4 index, java_objectheader *o);
#define BUILTIN_aastore (functionptr) asm_builtin_aastore

#if defined(TRACE_ARGS_NUM)
void builtin_trace_args(s8 a0, s8 a1, s8 a2, s8 a3,
#if TRACE_ARGS_NUM >= 6
						s8 a4, s8 a5,
#endif /* TRACE_ARGS_NUM >= 6 */
#if TRACE_ARGS_NUM == 8
						s8 a6, s8 a7,
#endif /* TRACE_ARGS_NUM == 8 */
						methodinfo *m);
/* NOT AN OP */
#endif /* defined(TRACE_ARGS_NUM) */
void builtin_displaymethodstart(methodinfo *m);
/* NOT AN OP */
void builtin_displaymethodstop(methodinfo *m, s8 l, double d, float f);
/* NOT AN OP */

#if defined(USE_THREADS)
void builtin_monitorenter(java_objectheader *o);
/* NOT AN OP */
void builtin_staticmonitorenter(classinfo *c);
/* NOT AN OP */
void asm_builtin_monitorenter(java_objectheader *o);
#define BUILTIN_monitorenter (functionptr) asm_builtin_monitorenter
void *builtin_monitorexit(java_objectheader *o);
/* NOT AN OP */
void *asm_builtin_monitorexit(java_objectheader *o);
#define BUILTIN_monitorexit (functionptr) asm_builtin_monitorexit
#endif

s4 builtin_idiv(s4 a, s4 b);
/* NOT AN OP */
s4 asm_builtin_idiv(s4 a, s4 b);
#define BUILTIN_idiv (functionptr) asm_builtin_idiv
s4 builtin_irem(s4 a, s4 b);
/* NOT AN OP */
s4 asm_builtin_irem(s4 a, s4 b);
#define BUILTIN_irem (functionptr) asm_builtin_irem

s8 builtin_ladd(s8 a, s8 b);
#define BUILTIN_ladd (functionptr) builtin_ladd
s8 builtin_lsub(s8 a, s8 b);
#define BUILTIN_lsub (functionptr) builtin_lsub
s8 builtin_lmul(s8 a, s8 b);
#define BUILTIN_lmul (functionptr) builtin_lmul
s8 builtin_ldiv(s8 a, s8 b);
/* NOT AN OP */
s8 asm_builtin_ldiv(s8 a, s8 b);
#define BUILTIN_ldiv (functionptr) asm_builtin_ldiv
s8 builtin_lrem(s8 a, s8 b);
/* NOT AN OP */
s8 asm_builtin_lrem(s8 a, s8 b);
#define BUILTIN_lrem (functionptr) asm_builtin_lrem
s8 builtin_lshl(s8 a, s4 b);
#define BUILTIN_lshl (functionptr) builtin_lshl
s8 builtin_lshr(s8 a, s4 b);
#define BUILTIN_lshr (functionptr) builtin_lshr
s8 builtin_lushr(s8 a, s4 b);
#define BUILTIN_lushr (functionptr) builtin_lushr
s8 builtin_land(s8 a, s8 b);
#define BUILTIN_land (functionptr) builtin_land
s8 builtin_lor(s8 a, s8 b);
#define BUILTIN_lor (functionptr) builtin_lor
s8 builtin_lxor(s8 a, s8 b);
#define BUILTIN_lxor (functionptr) builtin_lxor
s8 builtin_lneg(s8 a);
#define BUILTIN_lneg (functionptr) builtin_lneg
s4 builtin_lcmp(s8 a, s8 b);
#define BUILTIN_lcmp (functionptr) builtin_lcmp

float builtin_fadd(float a, float b);
#define BUILTIN_fadd (functionptr) builtin_fadd
float builtin_fsub(float a, float b);
#define BUILTIN_fsub (functionptr) builtin_fsub
float builtin_fmul(float a, float b);
#define BUILTIN_fmul (functionptr) builtin_fmul
float builtin_fdiv(float a, float b);
#define BUILTIN_fdiv (functionptr) builtin_fdiv
float builtin_fneg(float a);         
#define BUILTIN_fneg (functionptr) builtin_fneg
s4 builtin_fcmpl(float a, float b);  
#define BUILTIN_fcmpl (functionptr) builtin_fcmpl
s4 builtin_fcmpg(float a, float b);  
#define BUILTIN_fcmpg (functionptr) builtin_fcmpg
float builtin_frem(float a, float b);
#define BUILTIN_frem (functionptr) builtin_frem

double builtin_dadd(double a, double b);
#define BUILTIN_dadd (functionptr) builtin_dadd
double builtin_dsub(double a, double b);
#define BUILTIN_dsub (functionptr) builtin_dsub
double builtin_dmul(double a, double b);
#define BUILTIN_dmul (functionptr) builtin_dmul
double builtin_ddiv(double a, double b);
#define BUILTIN_ddiv (functionptr) builtin_ddiv
double builtin_dneg(double a);          
#define BUILTIN_dneg (functionptr) builtin_dneg
s4 builtin_dcmpl(double a, double b);   
#define BUILTIN_dcmpl (functionptr) builtin_dcmpl
s4 builtin_dcmpg(double a, double b);   
#define BUILTIN_dcmpg (functionptr) builtin_dcmpg
double builtin_drem(double a, double b);
#define BUILTIN_drem (functionptr) builtin_drem

s8       builtin_i2l(s4 i);
/* NOT AN OP */
float    builtin_i2f(s4 i);
#define BUILTIN_i2f (functionptr) builtin_i2f
double   builtin_i2d(s4 i);
#define BUILTIN_i2d (functionptr) builtin_i2d
s4       builtin_l2i(s8 l);
/* NOT AN OP */
float    builtin_l2f(s8 l);
#define BUILTIN_l2f (functionptr) builtin_l2f
double   builtin_l2d(s8 l);
#define BUILTIN_l2d (functionptr) builtin_l2d

s4       builtin_f2i(float a);
#define BUILTIN_f2i (functionptr) builtin_f2i
s4       asm_builtin_f2i(float a);
/* NOT AN OP */
s8       builtin_f2l(float a);
#define BUILTIN_f2l (functionptr) builtin_f2l
s8       asm_builtin_f2l(float a);
/* NOT AN OP */

double   builtin_f2d(float a);
#define BUILTIN_f2d (functionptr) builtin_f2d

s4       builtin_d2i(double a);
#define BUILTIN_d2i (functionptr) builtin_d2i
s4       asm_builtin_d2i(double a);
/* NOT AN OP */
s8       builtin_d2l(double a);
#define BUILTIN_d2l (functionptr) builtin_d2l
s8       asm_builtin_d2l(double a);
/* NOT AN OP */

float    builtin_d2f(double a);
#define BUILTIN_d2f (functionptr) builtin_d2f

java_arrayheader *builtin_clone_array(void *env, java_arrayheader *o);
/* NOT AN OP */

/* builtin_dummy just panics if it is executed. */
s4 builtin_dummy(void);
/* NOT AN OP */

/* conversion helper functions */
inline float intBitsToFloat(s4 i);
inline float longBitsToDouble(s8 l);

/* this is a wrapper for calls from asmpart */
java_objectheader **builtin_asm_get_exceptionptrptr(void);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
static inline java_objectheader **builtin_get_exceptionptrptr(void);
/* NOT AN OP */
static inline methodinfo **builtin_get_threadrootmethod(void);
/* NOT AN OP */

inline java_objectheader **builtin_get_exceptionptrptr(void)
{
	return &THREADINFO->_exceptionptr;
}

inline methodinfo **builtin_get_threadrootmethod(void)
{
	return &THREADINFO->_threadrootmethod;
}
#endif


/* returns the root method of a thread. this is used in asmpart.S and delivers the abort condition
   for the stack unwinding for getClassContext and getClassLoader. For the main thread this is the main function.
   Otherwhise it is the thread's run method (at least that's how I see it) (jowenn) */
methodinfo *builtin_asm_get_threadrootmethod(void);

/* returns the current top element of the stack frame info list (needed for unwinding across native functions) */
/* on i386 this is a pointer to a structure 
		------------------------------------------------
		| return adress out of native stub        	|
		| pointer to method info			| either i have to save an arbitrary adress within this native stub or the pointer to the method info, both are equaly costly, I have chosen the method  info (JOWENN)
		| pointer to thread specific top of this list	|<----stack frame begin
points here---->| previous element in list        		|
		------------------------------------------------
*/
void *builtin_asm_get_stackframeinfo(void);
stacktraceelement *builtin_stacktrace_copy(stacktraceelement **,stacktraceelement *begin, stacktraceelement *end);
#endif /* _BUILTIN_H */


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
