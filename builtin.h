/****************************** builtin.h **************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the prototypes for the functions of file builtin.c which has
	a more detailed description.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/11/14

*******************************************************************************/


/***************************** Prototypes *************************************/       

typedef struct builtin_descriptor {
	functionptr bptr;
	char        *name;
	} builtin_descriptor;

extern builtin_descriptor builtin_desc[];
extern java_objectheader* exceptionptr;

s4 builtin_instanceof(java_objectheader *obj, classinfo *class);
s4 builtin_isanysubclass (classinfo *sub, classinfo *super);
s4 builtin_checkcast(java_objectheader *obj, classinfo *class);
s4 asm_builtin_checkcast(java_objectheader *obj, classinfo *class);
s4 builtin_arrayinstanceof
	(java_objectheader *obj, constant_arraydescriptor *desc);
s4 builtin_checkarraycast
	(java_objectheader *obj, constant_arraydescriptor *desc);
s4 asm_builtin_checkarraycast
	(java_objectheader *obj, constant_arraydescriptor *desc);

java_objectheader *builtin_throw_exception (java_objectheader *exception);
java_objectheader *builtin_trace_exception (java_objectheader *exceptionptr,
                   methodinfo *method, int *pos, int noindent);

java_objectheader *builtin_new (classinfo *c);


java_objectarray *builtin_anewarray (s4 size, classinfo *elementtype);
#ifdef __I386__
void asm_builtin_anewarray (s4 size, classinfo *elementtype);
#endif
java_arrayarray *builtin_newarray_array 
        (s4 size, constant_arraydescriptor *elementdesc);
java_booleanarray *builtin_newarray_boolean (s4 size);
java_chararray *builtin_newarray_char (s4 size);
java_floatarray *builtin_newarray_float (s4 size);
java_doublearray *builtin_newarray_double (s4 size);
java_bytearray *builtin_newarray_byte (s4 size);
java_shortarray *builtin_newarray_short (s4 size);
java_intarray *builtin_newarray_int (s4 size);
java_longarray *builtin_newarray_long (s4 size);
java_arrayheader *builtin_multianewarray (java_intarray *dims,
                       constant_arraydescriptor *desc);
java_arrayheader *builtin_nmultianewarray (int size,
                      constant_arraydescriptor *desc, long *dims);

s4 builtin_canstore (java_objectarray *a, java_objectheader *o);
s4 builtin_aastore (java_objectarray *a, s4 index, java_objectheader *o);
void asm_builtin_aastore (java_objectarray *a, s4 index, java_objectheader *o);

#ifdef TRACE_ARGS_NUM
#if TRACE_ARGS_NUM == 6
void builtin_trace_args(s8 a0, s8 a1, s8 a2, s8 a3, s8 a4, s8 a5, methodinfo *method);
#else
void builtin_trace_args(s8 a0, s8 a1, s8 a2, s8 a3, s8 a4, s8 a5, s8 a6, s8 a7, methodinfo *method);
#endif
#endif
void builtin_displaymethodstart(methodinfo *method);
void builtin_displaymethodstop(methodinfo *method, s8 l, double d, float f);
/* void builtin_displaymethodstop(methodinfo *method); */
void builtin_displaymethodexception(methodinfo *method);

void builtin_monitorenter (java_objectheader *o);
void asm_builtin_monitorenter (java_objectheader *o);
void builtin_monitorexit (java_objectheader *o);
void asm_builtin_monitorexit (java_objectheader *o);

s4 builtin_idiv (s4 a, s4 b); 
s4 asm_builtin_idiv (s4 a, s4 b); 
s4 builtin_irem (s4 a, s4 b);
s4 asm_builtin_irem (s4 a, s4 b);

s8 builtin_ladd (s8 a, s8 b);
s8 builtin_lsub (s8 a, s8 b);
s8 builtin_lmul (s8 a, s8 b);
s8 builtin_ldiv (s8 a, s8 b);
s8 asm_builtin_ldiv (s8 a, s8 b);
s8 builtin_lrem (s8 a, s8 b);
s8 asm_builtin_lrem (s8 a, s8 b);
s8 builtin_lshl (s8 a, s4 b);
s8 builtin_lshr (s8 a, s4 b);
s8 builtin_lushr (s8 a, s4 b);
s8 builtin_land (s8 a, s8 b);
s8 builtin_lor (s8 a, s8 b);
s8 builtin_lxor (s8 a, s8 b);
s8 builtin_lneg (s8 a);
s4 builtin_lcmp (s8 a, s8 b);

float builtin_fadd (float a, float b);
float builtin_fsub (float a, float b);
float builtin_fmul (float a, float b);
float builtin_fdiv (float a, float b);
float builtin_frem (float a, float b);
float builtin_fneg (float a);
s4 builtin_fcmpl (float a, float b);
s4 builtin_fcmpg (float a, float b);

double builtin_dadd (double a, double b);
double builtin_dsub (double a, double b);
double builtin_dmul (double a, double b);
double builtin_ddiv (double a, double b);
double builtin_drem (double a, double b);
double builtin_dneg (double a);
s4 builtin_dcmpl (double a, double b);
s4 builtin_dcmpg (double a, double b);

s8       builtin_i2l (s4 i);
float    builtin_i2f (s4 i);
double   builtin_i2d (s4 i);
s4       builtin_l2i (s8 l);
float    builtin_l2f (s8 l);
double   builtin_l2d (s8 l);
s4       builtin_f2i (float a);
s8       builtin_f2l (float a);
double   builtin_f2d (float a);
s4       builtin_d2i (double a);
s8       builtin_d2l (double a);
float    builtin_d2f (double a);

