/* jit/jit.c - calls the code generation functions

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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
            Reinhard Grafl

   $Id: jit.c 725 2003-12-10 00:24:36Z edwin $

*/


#include <stdlib.h>
#include <string.h>
#include "global.h"    /* we define _GNU_SOURCE there */
#include "tables.h"
#include "loader.h"
#include "jit.h"
#include "parse.h"
#include "stack.h"
#include "reg.h"
#include "inline.h"
#include "builtin.h"
#include "native.h"
#include "asmpart.h"
#include "codegen.h"
#include "types.h"
#include "threads/thread.h"
#include "disass.h"
#include "loop/loop.h"
#include "loop/graph.h"
#include "loop/analyze.h"
#include "toolbox/loging.h"
#include "toolbox/memory.h"


/* global switches ************************************************************/
bool compileverbose =  false;
bool showstack = false;
bool showdisassemble = false; 
bool showddatasegment = false; 
bool showintermediate = false;
int  optimizelevel = 0;

bool useinlining = false;
bool inlinevirtuals = false;
bool inlineexceptions = false;
bool inlineparamopt = false;
bool inlineoutsiders = false;

bool checkbounds = true;
bool checknull = true;
bool opt_noieee = false;
bool checksync = true;
bool opt_loops = false;

bool getcompilingtime = false;
long compilingtime = 0;

int  has_ext_instr_set = 0;

bool statistics = false;         

int count_jit_calls = 0;
int count_methods = 0;
int count_spills = 0;
int count_pcmd_activ = 0;
int count_pcmd_drop = 0;
int count_pcmd_zero = 0;
int count_pcmd_const_store = 0;
int count_pcmd_const_alu = 0;
int count_pcmd_const_bra = 0;
int count_pcmd_load = 0;
int count_pcmd_move = 0;
int count_load_instruction = 0;
int count_pcmd_store = 0;
int count_pcmd_store_comb = 0;
int count_dup_instruction = 0;
int count_pcmd_op = 0;
int count_pcmd_mem = 0;
int count_pcmd_met = 0;
int count_pcmd_bra = 0;
int count_pcmd_table = 0;
int count_pcmd_return = 0;
int count_pcmd_returnx = 0;
int count_check_null = 0;
int count_check_bound = 0;
int count_max_basic_blocks = 0;
int count_basic_blocks = 0;
int count_javainstr = 0;
int count_max_javainstr = 0;
int count_javacodesize = 0;
int count_javaexcsize = 0;
int count_calls = 0;
int count_tryblocks = 0;
int count_code_len = 0;
int count_data_len = 0;
int count_cstub_len = 0;
int count_nstub_len = 0;
int count_max_new_stack = 0;
int count_upper_bound_new_stack = 0;
static int count_block_stack_init[11] = {
	0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 
	0
};
int *count_block_stack = count_block_stack_init;
static int count_analyse_iterations_init[5] = {
	0, 0, 0, 0, 0
};
int *count_analyse_iterations = count_analyse_iterations_init;
static int count_method_bb_distribution_init[9] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0
};
int *count_method_bb_distribution = count_method_bb_distribution_init;
static int count_block_size_distribution_init[18] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0
};
int *count_block_size_distribution = count_block_size_distribution_init;
static int count_store_length_init[21] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0
};
int *count_store_length = count_store_length_init;
static int count_store_depth_init[11] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0
};
int *count_store_depth = count_store_depth_init;



/* global compiler variables **************************************************/

                                /* data about the currently compiled method   */

classinfo  *class;              /* class the compiled method belongs to       */
methodinfo *method;             /* pointer to method info of compiled method  */
static utf *descriptor;         /* type descriptor of compiled method         */
int         mparamcount;        /* number of parameters (incl. this)          */
u1         *mparamtypes;        /* types of all parameters (TYPE_INT, ...)    */
static int mreturntype;         /* return type of method                      */
	
int maxstack;                   /* maximal JavaVM stack size                  */
int maxlocals;                  /* maximal number of local JavaVM variables   */
int jcodelength;                /* length of JavaVM-codes                     */
u1 *jcode;                      /* pointer to start of JavaVM-code            */
int exceptiontablelength;       /* length of exception table                  */
xtable *extable;                /* pointer to start of exception table        */
exceptiontable *raw_extable;

int block_count;                /* number of basic blocks                     */
basicblock *block;              /* points to basic block array                */
int *block_index;               /* a table which contains for every byte of   */
                                /* JavaVM code a basic block index if at this */
                                /* byte there is the start of a basic block   */

int instr_count;                /* number of JavaVM instructions              */
instruction *instr;             /* points to intermediate code instructions   */

int stack_count;                /* number of stack elements                   */
stackelement *stack;            /* points to intermediate code instructions   */

bool isleafmethod;              /* true if a method doesn't call subroutines  */

basicblock *last_block;         /* points to the end of the BB list           */

bool regs_ok;                   /* true if registers have been allocated      */

/* list of all classes used by the compiled method which have to be           */
/* initialised (if not already done) before execution of this method          */
chain *uninitializedclasses;

int stackreq[256];
                                

int jcommandsize[256] = {

#define JAVA_NOP               0
#define ICMD_NOP               0
	1,
#define JAVA_ACONST_NULL       1
#define ICMD_ACONST            1        /* val.a = constant                   */
	1,
#define JAVA_ICONST_M1         2
#define ICMD_NULLCHECKPOP      2
	1,
#define JAVA_ICONST_0          3
#define ICMD_ICONST            3        /* val.i = constant                   */
	1,
#define JAVA_ICONST_1          4
#define ICMD_IREM0X10001       4
	1,
#define JAVA_ICONST_2          5
#define ICMD_IDIVPOW2          5        /* val.i = constant                   */
	1,
#define JAVA_ICONST_3          6
#define ICMD_LDIVPOW2          6        /* val.l = constant                   */
	1,
#define JAVA_ICONST_4          7
	1,
#define JAVA_ICONST_5          8
#define ICMD_LREM0X10001       8
	1,
#define JAVA_LCONST_0          9
#define ICMD_LCONST            9        /* val.l = constant                   */
	1,
#define JAVA_LCONST_1         10
#define ICMD_LCMPCONST        10        /* val.l = constant                   */
	1,
#define JAVA_FCONST_0         11
#define ICMD_FCONST           11        /* val.f = constant                   */
	1,
#define JAVA_FCONST_1         12
	1,
#define JAVA_FCONST_2         13
#define ICMD_ELSE_ICONST      13
	1,
#define JAVA_DCONST_0         14
#define ICMD_DCONST           14        /* val.d = constant                   */
	1,
#define JAVA_DCONST_1         15
#define ICMD_IFEQ_ICONST      15
	1,
#define JAVA_BIPUSH           16
#define ICMD_IFNE_ICONST      16
	2,
#define JAVA_SIPUSH           17
#define ICMD_IFLT_ICONST      17
	3,
#define JAVA_LDC1             18
#define ICMD_IFGE_ICONST      18
	2,
#define JAVA_LDC2             19
#define ICMD_IFGT_ICONST      19
	3,
#define JAVA_LDC2W            20
#define ICMD_IFLE_ICONST      20
	3,
	                                    /* order of LOAD instructions must be */
	                                    /* equal to order of TYPE_XXX defines */
#define JAVA_ILOAD            21
#define ICMD_ILOAD            21        /* op1 = local variable               */
	2,                      
#define JAVA_LLOAD            22
#define ICMD_LLOAD            22        /* op1 = local variable               */
	2,
#define JAVA_FLOAD            23
#define ICMD_FLOAD            23        /* op1 = local variable               */
	2,
#define JAVA_DLOAD            24
#define ICMD_DLOAD            24        /* op1 = local variable               */
	2,
#define JAVA_ALOAD            25
#define ICMD_ALOAD            25        /* op1 = local variable               */
	2,
#define JAVA_ILOAD_0          26
#define ICMD_IADDCONST        26        /* val.i = constant                   */
	1,
#define JAVA_ILOAD_1          27
#define ICMD_ISUBCONST        27        /* val.i = constant                   */
	1,
#define JAVA_ILOAD_2          28
#define ICMD_IMULCONST        28        /* val.i = constant                   */
	1,
#define JAVA_ILOAD_3          29
#define ICMD_IANDCONST        29        /* val.i = constant                   */
	1,
#define JAVA_LLOAD_0          30
#define ICMD_IORCONST         30        /* val.i = constant                   */
	1,
#define JAVA_LLOAD_1          31
#define ICMD_IXORCONST        31        /* val.i = constant                   */
	1,
#define JAVA_LLOAD_2          32
#define ICMD_ISHLCONST        32        /* val.i = constant                   */
	1,
#define JAVA_LLOAD_3          33
#define ICMD_ISHRCONST        33        /* val.i = constant                   */
	1,
#define JAVA_FLOAD_0          34
#define ICMD_IUSHRCONST       34        /* val.i = constant                   */
	1,
#define JAVA_FLOAD_1          35
#define ICMD_IREMPOW2         35        /* val.i = constant                   */
	1,
#define JAVA_FLOAD_2          36
#define ICMD_LADDCONST        36        /* val.l = constant                   */
	1,
#define JAVA_FLOAD_3          37
#define ICMD_LSUBCONST        37        /* val.l = constant                   */
	1,
#define JAVA_DLOAD_0          38
#define ICMD_LMULCONST        38        /* val.l = constant                   */
	1,
#define JAVA_DLOAD_1          39
#define ICMD_LANDCONST        39        /* val.l = constant                   */
	1,
#define JAVA_DLOAD_2          40
#define ICMD_LORCONST         40        /* val.l = constant                   */
	1,
#define JAVA_DLOAD_3          41
#define ICMD_LXORCONST        41        /* val.l = constant                   */
	1,
#define JAVA_ALOAD_0          42
#define ICMD_LSHLCONST        42        /* val.l = constant                   */
	1,
#define JAVA_ALOAD_1          43
#define ICMD_LSHRCONST        43        /* val.l = constant                   */
	1,
#define JAVA_ALOAD_2          44
#define ICMD_LUSHRCONST       44        /* val.l = constant                   */
	1,
#define JAVA_ALOAD_3          45
#define ICMD_LREMPOW2         45        /* val.l = constant                   */
	1,
#define JAVA_IALOAD           46
#define ICMD_IALOAD           46
	1,
#define JAVA_LALOAD           47
#define ICMD_LALOAD           47
	1,
#define JAVA_FALOAD           48
#define ICMD_FALOAD           48
	1,
#define JAVA_DALOAD           49
#define ICMD_DALOAD           49
	1,
#define JAVA_AALOAD           50
#define ICMD_AALOAD           50
	1,
#define JAVA_BALOAD           51
#define ICMD_BALOAD           51
	1,
#define JAVA_CALOAD           52
#define ICMD_CALOAD           52
	1,
#define JAVA_SALOAD           53
#define ICMD_SALOAD           53
	1,
	                                    /* order of STORE instructions must be*/
	                                    /* equal to order of TYPE_XXX defines */
#define JAVA_ISTORE           54
#define ICMD_ISTORE           54        /* op1 = local variable               */
	2,
#define JAVA_LSTORE           55
#define ICMD_LSTORE           55        /* op1 = local variable               */
	2,
#define JAVA_FSTORE           56
#define ICMD_FSTORE           56        /* op1 = local variable               */
	2,
#define JAVA_DSTORE           57
#define ICMD_DSTORE           57        /* op1 = local variable               */
	2,
#define JAVA_ASTORE           58
#define ICMD_ASTORE           58        /* op1 = local variable               */
	2,
#define JAVA_ISTORE_0         59
#define ICMD_IF_LEQ           59        /* op1 = target JavaVM pc, val.l      */
	1,
#define JAVA_ISTORE_1         60
#define ICMD_IF_LNE           60        /* op1 = target JavaVM pc, val.l      */
	1,
#define JAVA_ISTORE_2         61
#define ICMD_IF_LLT           61        /* op1 = target JavaVM pc, val.l      */
	1,
#define JAVA_ISTORE_3         62
#define ICMD_IF_LGE           62        /* op1 = target JavaVM pc, val.l      */
	1,
#define JAVA_LSTORE_0         63
#define ICMD_IF_LGT           63        /* op1 = target JavaVM pc, val.l      */
	1,
#define JAVA_LSTORE_1         64
#define ICMD_IF_LLE           64        /* op1 = target JavaVM pc, val.l      */
	1,
#define JAVA_LSTORE_2         65
#define ICMD_IF_LCMPEQ        65        /* op1 = target JavaVM pc             */
	1,
#define JAVA_LSTORE_3         66
#define ICMD_IF_LCMPNE        66        /* op1 = target JavaVM pc             */
	1,
#define JAVA_FSTORE_0         67
#define ICMD_IF_LCMPLT        67        /* op1 = target JavaVM pc             */
	1,
#define JAVA_FSTORE_1         68
#define ICMD_IF_LCMPGE        68        /* op1 = target JavaVM pc             */
	1,
#define JAVA_FSTORE_2         69
#define ICMD_IF_LCMPGT        69        /* op1 = target JavaVM pc             */
	1,
#define JAVA_FSTORE_3         70
#define ICMD_IF_LCMPLE        70        /* op1 = target JavaVM pc             */
	1,
#define JAVA_DSTORE_0         71
	1,
#define JAVA_DSTORE_1         72
	1,
#define JAVA_DSTORE_2         73
	1,
#define JAVA_DSTORE_3         74
	1,
#define JAVA_ASTORE_0         75
	1,
#define JAVA_ASTORE_1         76
	1,
#define JAVA_ASTORE_2         77
	1,
#define JAVA_ASTORE_3         78
	1,
#define JAVA_IASTORE          79
#define ICMD_IASTORE          79
	1,
#define JAVA_LASTORE          80
#define ICMD_LASTORE          80
	1,
#define JAVA_FASTORE          81
#define ICMD_FASTORE          81
	1,
#define JAVA_DASTORE          82
#define ICMD_DASTORE          82
	1,
#define JAVA_AASTORE          83
#define ICMD_AASTORE          83
	1,
#define JAVA_BASTORE          84
#define ICMD_BASTORE          84
	1,
#define JAVA_CASTORE          85
#define ICMD_CASTORE          85
	1,
#define JAVA_SASTORE          86
#define ICMD_SASTORE          86
	1,
#define JAVA_POP              87
#define ICMD_POP              87
	1,
#define JAVA_POP2             88
#define ICMD_POP2             88
	1,
#define JAVA_DUP              89
#define ICMD_DUP              89
	1,
#define JAVA_DUP_X1           90
#define ICMD_DUP_X1           90
	1,
#define JAVA_DUP_X2           91
#define ICMD_DUP_X2           91
	1,
#define JAVA_DUP2             92
#define ICMD_DUP2             92
	1,
#define JAVA_DUP2_X1          93
#define ICMD_DUP2_X1          93
	1,
#define JAVA_DUP2_X2          94
#define ICMD_DUP2_X2          94
	1,
#define JAVA_SWAP             95
#define ICMD_SWAP             95
	1,
#define JAVA_IADD             96
#define ICMD_IADD             96
	1,
#define JAVA_LADD             97
#define ICMD_LADD             97
	1,
#define JAVA_FADD             98
#define ICMD_FADD             98
	1,
#define JAVA_DADD             99
#define ICMD_DADD             99
	1,
#define JAVA_ISUB             100
#define ICMD_ISUB             100
	1,
#define JAVA_LSUB             101
#define ICMD_LSUB             101
	1,
#define JAVA_FSUB             102
#define ICMD_FSUB             102
	1,
#define JAVA_DSUB             103
#define ICMD_DSUB             103
	1,
#define JAVA_IMUL             104
#define ICMD_IMUL             104
	1,
#define JAVA_LMUL             105
#define ICMD_LMUL             105
	1,
#define JAVA_FMUL             106
#define ICMD_FMUL             106
	1,
#define JAVA_DMUL             107
#define ICMD_DMUL             107
	1,
#define JAVA_IDIV             108
#define ICMD_IDIV             108
	1,
#define JAVA_LDIV             109
#define ICMD_LDIV             109
	1,
#define JAVA_FDIV             110
#define ICMD_FDIV             110
	1,
#define JAVA_DDIV             111
#define ICMD_DDIV             111
	1,
#define JAVA_IREM             112
#define ICMD_IREM             112
	1,
#define JAVA_LREM             113
#define ICMD_LREM             113
	1,
#define JAVA_FREM             114
#define ICMD_FREM             114
	1,
#define JAVA_DREM             115
#define ICMD_DREM             115
	1,
#define JAVA_INEG             116
#define ICMD_INEG             116
	1,
#define JAVA_LNEG             117
#define ICMD_LNEG             117
	1,
#define JAVA_FNEG             118
#define ICMD_FNEG             118
	1,
#define JAVA_DNEG             119
#define ICMD_DNEG             119
	1,
#define JAVA_ISHL             120
#define ICMD_ISHL             120
	1,
#define JAVA_LSHL             121
#define ICMD_LSHL             121
	1,
#define JAVA_ISHR             122
#define ICMD_ISHR             122
	1,
#define JAVA_LSHR             123
#define ICMD_LSHR             123
	1,
#define JAVA_IUSHR            124
#define ICMD_IUSHR            124
	1,
#define JAVA_LUSHR            125
#define ICMD_LUSHR            125
	1,
#define JAVA_IAND             126
#define ICMD_IAND             126
	1,
#define JAVA_LAND             127
#define ICMD_LAND             127
	1,
#define JAVA_IOR              128
#define ICMD_IOR              128
	1,
#define JAVA_LOR              129
#define ICMD_LOR              129
	1,
#define JAVA_IXOR             130
#define ICMD_IXOR             130
	1,
#define JAVA_LXOR             131
#define ICMD_LXOR             131
	1,
#define JAVA_IINC             132
#define ICMD_IINC             132   /* op1 = local variable, val.i = constant */
	3,
#define JAVA_I2L              133
#define ICMD_I2L              133
	1,
#define JAVA_I2F              134
#define ICMD_I2F              134
	1,
#define JAVA_I2D              135
#define ICMD_I2D              135
	1,
#define JAVA_L2I              136
#define ICMD_L2I              136
	1,
#define JAVA_L2F              137
#define ICMD_L2F              137
	1,
#define JAVA_L2D              138
#define ICMD_L2D              138
	1,
#define JAVA_F2I              139
#define ICMD_F2I              139
	1,
#define JAVA_F2L              140
#define ICMD_F2L              140
	1,
#define JAVA_F2D              141
#define ICMD_F2D              141
	1,
#define JAVA_D2I              142
#define ICMD_D2I              142
	1,
#define JAVA_D2L              143
#define ICMD_D2L              143
	1,
#define JAVA_D2F              144
#define ICMD_D2F              144
	1,
#define JAVA_INT2BYTE         145
#define ICMD_INT2BYTE         145
	1,
#define JAVA_INT2CHAR         146
#define ICMD_INT2CHAR         146
	1,
#define JAVA_INT2SHORT        147
#define ICMD_INT2SHORT        147
	1,
#define JAVA_LCMP             148
#define ICMD_LCMP             148
	1,
#define JAVA_FCMPL            149
#define ICMD_FCMPL            149
	1,
#define JAVA_FCMPG            150
#define ICMD_FCMPG            150
	1,
#define JAVA_DCMPL            151
#define ICMD_DCMPL            151
	1,
#define JAVA_DCMPG            152
#define ICMD_DCMPG            152
	1,
#define JAVA_IFEQ             153
#define ICMD_IFEQ             153       /* op1 = target JavaVM pc, val.i      */
	3,
#define JAVA_IFNE             154
#define ICMD_IFNE             154       /* op1 = target JavaVM pc, val.i      */
	3,
#define JAVA_IFLT             155
#define ICMD_IFLT             155       /* op1 = target JavaVM pc, val.i      */
	3,
#define JAVA_IFGE             156
#define ICMD_IFGE             156       /* op1 = target JavaVM pc, val.i      */
	3,
#define JAVA_IFGT             157
#define ICMD_IFGT             157       /* op1 = target JavaVM pc, val.i      */
	3,
#define JAVA_IFLE             158
#define ICMD_IFLE             158       /* op1 = target JavaVM pc, val.i      */
	3,
#define JAVA_IF_ICMPEQ        159
#define ICMD_IF_ICMPEQ        159       /* op1 = target JavaVM pc             */
	3,
#define JAVA_IF_ICMPNE        160
#define ICMD_IF_ICMPNE        160       /* op1 = target JavaVM pc             */
	3,
#define JAVA_IF_ICMPLT        161
#define ICMD_IF_ICMPLT        161       /* op1 = target JavaVM pc             */
	3,
#define JAVA_IF_ICMPGE        162
#define ICMD_IF_ICMPGE        162       /* op1 = target JavaVM pc             */
	3,
#define JAVA_IF_ICMPGT        163
#define ICMD_IF_ICMPGT        163       /* op1 = target JavaVM pc             */
	3,
#define JAVA_IF_ICMPLE        164
#define ICMD_IF_ICMPLE        164       /* op1 = target JavaVM pc             */
	3,
#define JAVA_IF_ACMPEQ        165
#define ICMD_IF_ACMPEQ        165       /* op1 = target JavaVM pc             */
	3,
#define JAVA_IF_ACMPNE        166
#define ICMD_IF_ACMPNE        166       /* op1 = target JavaVM pc             */
	3,
#define JAVA_GOTO             167
#define ICMD_GOTO             167       /* op1 = target JavaVM pc             */
	3,
#define JAVA_JSR              168
#define ICMD_JSR              168       /* op1 = target JavaVM pc             */
	3,
#define JAVA_RET              169
#define ICMD_RET              169       /* op1 = local variable               */
	2,
#define JAVA_TABLESWITCH      170
#define ICMD_TABLESWITCH      170       /* val.a = pointer to s4 table        */
	0,                                  /* length must be computed            */
#define JAVA_LOOKUPSWITCH     171
#define ICMD_LOOKUPSWITCH     171       /* val.a = pointer to s4 table        */
	0,                                  /* length must be computed            */
#define JAVA_IRETURN          172
#define ICMD_IRETURN          172
	1,
#define JAVA_LRETURN          173
#define ICMD_LRETURN          173
	1,
#define JAVA_FRETURN          174
#define ICMD_FRETURN          174
	1,
#define JAVA_DRETURN          175
#define ICMD_DRETURN          175
	1,
#define JAVA_ARETURN          176
#define ICMD_ARETURN          176
	1,
#define JAVA_RETURN           177
#define ICMD_RETURN           177
	1,
#define JAVA_GETSTATIC        178
#define ICMD_GETSTATIC        178       /* op1 = type, val.a = field address  */
	3,
#define JAVA_PUTSTATIC        179
#define ICMD_PUTSTATIC        179       /* op1 = type, val.a = field address  */
	3,
#define JAVA_GETFIELD         180
#define ICMD_GETFIELD         180       /* op1 = type, val.i = field offset   */
	3,
#define JAVA_PUTFIELD         181
#define ICMD_PUTFIELD         181       /* op1 = type, val.i = field offset   */
	3,
#define JAVA_INVOKEVIRTUAL    182
#define ICMD_INVOKEVIRTUAL    182       /* val.a = method info pointer        */
	3,
#define JAVA_INVOKESPECIAL    183
#define ICMD_INVOKESPECIAL    183       /* val.a = method info pointer        */
	3,
#define JAVA_INVOKESTATIC     184
#define ICMD_INVOKESTATIC     184       /* val.a = method info pointer        */
	3,
#define JAVA_INVOKEINTERFACE  185
#define ICMD_INVOKEINTERFACE  185       /* val.a = method info pointer        */
	5,
#define ICMD_CHECKASIZE       186       /*                                    */
	1, /* unused */
#define JAVA_NEW              187
#define ICMD_NEW              187       /* op1 = 1, val.a = class pointer     */
	3,
#define JAVA_NEWARRAY         188
#define ICMD_NEWARRAY         188       /* op1 = basic type                   */
	2,
#define JAVA_ANEWARRAY        189
#define ICMD_ANEWARRAY        189       /* op1 = 0, val.a = array pointer     */
	3,                                  /* op1 = 1, val.a = class pointer     */
#define JAVA_ARRAYLENGTH      190
#define ICMD_ARRAYLENGTH      190
	1,
#define JAVA_ATHROW           191
#define ICMD_ATHROW           191
	1,
#define JAVA_CHECKCAST        192
#define ICMD_CHECKCAST        192       /* op1 = 0, val.a = array pointer     */
	3,                                  /* op1 = 1, val.a = class pointer     */
#define JAVA_INSTANCEOF       193
#define ICMD_INSTANCEOF       193       /* op1 = 0, val.a = array pointer     */
	3,                                  /* op1 = 1, val.a = class pointer     */
#define JAVA_MONITORENTER     194
#define ICMD_MONITORENTER     194
	1,
#define JAVA_MONITOREXIT      195
#define ICMD_MONITOREXIT      195
	1,
#define JAVA_WIDE             196
	0, /* length must be computed */
#define JAVA_MULTIANEWARRAY   197
#define ICMD_MULTIANEWARRAY   197       /* op1 = dimension, val.a = array     */
	4,                                  /* pointer                            */
#define JAVA_IFNULL           198
#define ICMD_IFNULL           198       /* op1 = target JavaVM pc             */
	3,
#define JAVA_IFNONNULL        199
#define ICMD_IFNONNULL        199       /* op1 = target JavaVM pc             */
	3,
#define JAVA_GOTO_W           200
	5,
#define JAVA_JSR_W            201
	5,
#define JAVA_BREAKPOINT       202
	1,

	    1,1,1,1,1,1,1,1,            /* unused */
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1
};


char *icmd_names[256] = {
	"NOP          ", /*               0 */
	"ACONST       ", /*               1 */
	"NULLCHECKPOP ", /* ICONST_M1     2 */
	"ICONST       ", /*               3 */
	"IREM0X10001  ", /* ICONST_1      4 */
	"IDIVPOW2     ", /* ICONST_2      5 */
	"LDIVPOW2     ", /* ICONST_3      6 */
	"UNDEF__7     ", /* ICONST_4      7 */
	"LREM0X10001  ", /* ICONST_5      8 */
	"LCONST       ", /*               9 */
	"LCMPCONST    ", /* LCONST_1     10 */
	"FCONST       ", /*              11 */
	"UNDEF_12     ", /* FCONST_1     12 */
	"ELSE_ICONST  ", /* FCONST_2     13 */
	"DCONST       ", /*              14 */
	"IFEQ_ICONST  ", /* DCONST_1     15 */
	"IFNE_ICONST  ", /* BIPUSH       16 */
	"IFLT_ICONST  ", /* SIPUSH       17 */
	"IFGE_ICONST  ", /* LDC1         18 */
	"IFGT_ICONST  ", /* LDC2         19 */
	"IFLE_ICONST  ", /* LDC2W        20 */
	"ILOAD        ", /*              21 */
	"LLOAD        ", /*              22 */
	"FLOAD        ", /*              23 */
	"DLOAD        ", /*              24 */
	"ALOAD        ", /*              25 */
	"IADDCONST    ", /* ILOAD_0      26 */
	"ISUBCONST    ", /* ILOAD_1      27 */
	"IMULCONST    ", /* ILOAD_2      28 */
	"IANDCONST    ", /* ILOAD_3      29 */
	"IORCONST     ", /* LLOAD_0      30 */
	"IXORCONST    ", /* LLOAD_1      31 */
	"ISHLCONST    ", /* LLOAD_2      32 */
	"ISHRCONST    ", /* LLOAD_3      33 */
	"IUSHRCONST   ", /* FLOAD_0      34 */
	"IREMPOW2     ", /* FLOAD_1      35 */
	"LADDCONST    ", /* FLOAD_2      36 */
	"LSUBCONST    ", /* FLOAD_3      37 */
	"LMULCONST    ", /* DLOAD_0      38 */
	"LANDCONST    ", /* DLOAD_1      39 */
	"LORCONST     ", /* DLOAD_2      40 */
	"LXORCONST    ", /* DLOAD_3      41 */
	"LSHLCONST    ", /* ALOAD_0      42 */
	"LSHRCONST    ", /* ALOAD_1      43 */
	"LUSHRCONST   ", /* ALOAD_2      44 */
	"LREMPOW2     ", /* ALOAD_3      45 */
	"IALOAD       ", /*              46 */
	"LALOAD       ", /*              47 */
	"FALOAD       ", /*              48 */
	"DALOAD       ", /*              49 */
	"AALOAD       ", /*              50 */
	"BALOAD       ", /*              51 */
	"CALOAD       ", /*              52 */
	"SALOAD       ", /*              53 */
	"ISTORE       ", /*              54 */
	"LSTORE       ", /*              55 */
	"FSTORE       ", /*              56 */
	"DSTORE       ", /*              57 */
	"ASTORE       ", /*              58 */
	"IF_LEQ       ", /* ISTORE_0     59 */
	"IF_LNE       ", /* ISTORE_1     60 */
	"IF_LLT       ", /* ISTORE_2     61 */
	"IF_LGE       ", /* ISTORE_3     62 */
	"IF_LGT       ", /* LSTORE_0     63 */
	"IF_LLE       ", /* LSTORE_1     64 */
	"IF_LCMPEQ    ", /* LSTORE_2     65 */
	"IF_LCMPNE    ", /* LSTORE_3     66 */
	"IF_LCMPLT    ", /* FSTORE_0     67 */
	"IF_LCMPGE    ", /* FSTORE_1     68 */
	"IF_LCMPGT    ", /* FSTORE_2     69 */
	"IF_LCMPLE    ", /* FSTORE_3     70 */
	"UNDEF_71     ", /* DSTORE_0     71 */
	"UNDEF_72     ", /* DSTORE_1     72 */
	"UNDEF_73     ", /* DSTORE_2     73 */
	"UNDEF_74     ", /* DSTORE_3     74 */
	"UNDEF_75     ", /* ASTORE_0     75 */
	"UNDEF_76     ", /* ASTORE_1     76 */
	"UNDEF_77     ", /* ASTORE_2     77 */
	"UNDEF_78     ", /* ASTORE_3     78 */
	"IASTORE      ", /*              79 */
	"LASTORE      ", /*              80 */
	"FASTORE      ", /*              81 */
	"DASTORE      ", /*              82 */
	"AASTORE      ", /*              83 */
	"BASTORE      ", /*              84 */
	"CASTORE      ", /*              85 */
	"SASTORE      ", /*              86 */
	"POP          ", /*              87 */
	"POP2         ", /*              88 */
	"DUP          ", /*              89 */
	"DUP_X1       ", /*              90 */
	"DUP_X2       ", /*              91 */
	"DUP2         ", /*              92 */
	"DUP2_X1      ", /*              93 */
	"DUP2_X2      ", /*              94 */
	"SWAP         ", /*              95 */
	"IADD         ", /*              96 */
	"LADD         ", /*              97 */
	"FADD         ", /*              98 */
	"DADD         ", /*              99 */
	"ISUB         ", /*             100 */
	"LSUB         ", /*             101 */
	"FSUB         ", /*             102 */
	"DSUB         ", /*             103 */
	"IMUL         ", /*             104 */
	"LMUL         ", /*             105 */
	"FMUL         ", /*             106 */
	"DMUL         ", /*             107 */
	"IDIV         ", /*             108 */
	"LDIV         ", /*             109 */
	"FDIV         ", /*             110 */
	"DDIV         ", /*             111 */
	"IREM         ", /*             112 */
	"LREM         ", /*             113 */
	"FREM         ", /*             114 */
	"DREM         ", /*             115 */
	"INEG         ", /*             116 */
	"LNEG         ", /*             117 */
	"FNEG         ", /*             118 */
	"DNEG         ", /*             119 */
	"ISHL         ", /*             120 */
	"LSHL         ", /*             121 */
	"ISHR         ", /*             122 */
	"LSHR         ", /*             123 */
	"IUSHR        ", /*             124 */
	"LUSHR        ", /*             125 */
	"IAND         ", /*             126 */
	"LAND         ", /*             127 */
	"IOR          ", /*             128 */
	"LOR          ", /*             129 */
	"IXOR         ", /*             130 */
	"LXOR         ", /*             131 */
	"IINC         ", /*             132 */
	"I2L          ", /*             133 */
	"I2F          ", /*             134 */
	"I2D          ", /*             135 */
	"L2I          ", /*             136 */
	"L2F          ", /*             137 */
	"L2D          ", /*             138 */
	"F2I          ", /*             139 */
	"F2L          ", /*             140 */
	"F2D          ", /*             141 */
	"D2I          ", /*             142 */
	"D2L          ", /*             143 */
	"D2F          ", /*             144 */
	"INT2BYTE     ", /*             145 */
	"INT2CHAR     ", /*             146 */
	"INT2SHORT    ", /*             147 */
	"LCMP         ", /*             148 */
	"FCMPL        ", /*             149 */
	"FCMPG        ", /*             150 */
	"DCMPL        ", /*             151 */
	"DCMPG        ", /*             152 */
	"IFEQ         ", /*             153 */
	"IFNE         ", /*             154 */
	"IFLT         ", /*             155 */
	"IFGE         ", /*             156 */
	"IFGT         ", /*             157 */
	"IFLE         ", /*             158 */
	"IF_ICMPEQ    ", /*             159 */
	"IF_ICMPNE    ", /*             160 */
	"IF_ICMPLT    ", /*             161 */
	"IF_ICMPGE    ", /*             162 */
	"IF_ICMPGT    ", /*             163 */
	"IF_ICMPLE    ", /*             164 */
	"IF_ACMPEQ    ", /*             165 */
	"IF_ACMPNE    ", /*             166 */
	"GOTO         ", /*             167 */
	"JSR          ", /*             168 */
	"RET          ", /*             169 */
	"TABLESWITCH  ", /*             170 */
	"LOOKUPSWITCH ", /*             171 */
	"IRETURN      ", /*             172 */
	"LRETURN      ", /*             173 */
	"FRETURN      ", /*             174 */
	"DRETURN      ", /*             175 */
	"ARETURN      ", /*             176 */
	"RETURN       ", /*             177 */
	"GETSTATIC    ", /*             178 */
	"PUTSTATIC    ", /*             179 */
	"GETFIELD     ", /*             180 */
	"PUTFIELD     ", /*             181 */
	"INVOKEVIRTUAL", /*             182 */
	"INVOKESPECIAL", /*             183 */
	"INVOKESTATIC ", /*             184 */
	"INVOKEINTERFACE",/*            185 */
	"CHECKASIZE   ", /* UNDEF186    186 */
	"NEW          ", /*             187 */
	"NEWARRAY     ", /*             188 */
	"ANEWARRAY    ", /*             189 */
	"ARRAYLENGTH  ", /*             190 */
	"ATHROW       ", /*             191 */
	"CHECKCAST    ", /*             192 */
	"INSTANCEOF   ", /*             193 */
	"MONITORENTER ", /*             194 */
	"MONITOREXIT  ", /*             195 */
	"UNDEF196     ", /* WIDE        196 */
	"MULTIANEWARRAY",/*             197 */
	"IFNULL       ", /*             198 */
	"IFNONNULL    ", /*             199 */
	"UNDEF200     ", /* GOTO_W      200 */
	"UNDEF201     ", /* JSR_W       201 */
	"UNDEF202     ", /* BREAKPOINT  202 */

	                      "UNDEF203","UNDEF204","UNDEF205",
	"UNDEF206","UNDEF207","UNDEF208","UNDEF209","UNDEF210",
	"UNDEF","UNDEF","UNDEF","UNDEF","UNDEF",
	"UNDEF216","UNDEF217","UNDEF218","UNDEF219","UNDEF220",
	"UNDEF","UNDEF","UNDEF","UNDEF","UNDEF",
	"UNDEF226","UNDEF227","UNDEF228","UNDEF229","UNDEF230",
	"UNDEF","UNDEF","UNDEF","UNDEF","UNDEF",
	"UNDEF236","UNDEF237","UNDEF238","UNDEF239","UNDEF240",
	"UNDEF","UNDEF","UNDEF","UNDEF","UNDEF",
	"UNDEF246","UNDEF247","UNDEF248","UNDEF249","UNDEF250",
	"UNDEF251","UNDEF252",

	"BUILTIN3     ", /*             253 */
	"BUILTIN2     ", /*             254 */
	"BUILTIN1     "  /*             255 */
};


char *opcode_names[256] = {
	"NOP          ", /*               0 */
	"ACONST       ", /*               1 */
	"ICONST_M1    ", /* ICONST_M1     2 */
	"ICONST_0     ", /* ICONST_0      3 */
	"ICONST_1     ", /* ICONST_1      4 */
	"ICONST_2     ", /* ICONST_2      5 */
	"ICONST_3     ", /* ICONST_3      6 */
	"ICONST_4     ", /* ICONST_4      7 */
	"ICONST_5     ", /* ICONST_5      8 */
	"LCONST_0     ", /* LCONST_0      9 */
	"LCONST_1     ", /* LCONST_1     10 */
	"FCONST_0     ", /* FCONST_0     11 */
	"FCONST_1     ", /* FCONST_1     12 */
	"FCONST_2     ", /* FCONST_2     13 */
	"DCONST_0     ", /* DCONST_0     14 */
	"DCONST_1     ", /* DCONST_1     15 */
	"BIPUSH       ", /* BIPUSH       16 */
	"SIPUSH       ", /* SIPUSH       17 */
	"LDC          ", /* LDC          18 */
	"LDC_W        ", /* LDC_W        19 */
	"LDC2_W       ", /* LDC2_W       20 */
	"ILOAD        ", /*              21 */
	"LLOAD        ", /*              22 */
	"FLOAD        ", /*              23 */
	"DLOAD        ", /*              24 */
	"ALOAD        ", /*              25 */
	"ILOAD_0      ", /* ILOAD_0      26 */
	"ILOAD_1      ", /* ILOAD_1      27 */
	"ILOAD_2      ", /* ILOAD_2      28 */
	"ILOAD_3      ", /* ILOAD_3      29 */
	"LLOAD_0      ", /* LLOAD_0      30 */
	"LLOAD_1      ", /* LLOAD_1      31 */
	"LLOAD_2      ", /* LLOAD_2      32 */
	"LLOAD_3      ", /* LLOAD_3      33 */
	"FLOAD_0      ", /* FLOAD_0      34 */
	"FLOAD_1      ", /* FLOAD_1      35 */
	"FLOAD_2      ", /* FLOAD_2      36 */
	"FLOAD_3      ", /* FLOAD_3      37 */
	"DLOAD_0      ", /* DLOAD_0      38 */
	"DLOAD_1      ", /* DLOAD_1      39 */
	"DLOAD_2      ", /* DLOAD_2      40 */ 
	"DLOAD_3      ", /* DLOAD_3      41 */
	"ALOAD_0      ", /* ALOAD_0      42 */
	"ALOAD_1      ", /* ALOAD_1      43 */
	"ALOAD_2      ", /* ALOAD_2      44 */
	"ALOAD_3      ", /* ALOAD_3      45 */
	"IALOAD       ", /*              46 */
	"LALOAD       ", /*              47 */
	"FALOAD       ", /*              48 */
	"DALOAD       ", /*              49 */
	"AALOAD       ", /*              50 */
	"BALOAD       ", /*              51 */
	"CALOAD       ", /*              52 */
	"SALOAD       ", /*              53 */
	"ISTORE       ", /*              54 */
	"LSTORE       ", /*              55 */
	"FSTORE       ", /*              56 */
	"DSTORE       ", /*              57 */
	"ASTORE       ", /*              58 */
	"ISTORE_0     ", /* ISTORE_0     59 */
	"ISTORE_1     ", /* ISTORE_1     60 */
	"ISTORE_2     ", /* ISTORE_2     61 */
	"ISTORE_3     ", /* ISTORE_3     62 */
	"LSTORE_0     ", /* LSTORE_0     63 */
	"LSTORE_1     ", /* LSTORE_1     64 */
	"LSTORE_2     ", /* LSTORE_2     65 */
	"LSTORE_3     ", /* LSTORE_3     66 */
	"FSTORE_0     ", /* FSTORE_0     67 */
	"FSTORE_1     ", /* FSTORE_1     68 */
	"FSTORE_2     ", /* FSTORE_2     69 */
	"FSTORE_3     ", /* FSTORE_3     70 */
	"DSTORE_0     ", /* DSTORE_0     71 */
	"DSTORE_1     ", /* DSTORE_1     72 */
	"DSTORE_2     ", /* DSTORE_2     73 */
	"DSTORE_3     ", /* DSTORE_3     74 */
	"ASTORE_0     ", /* ASTORE_0     75 */
	"ASTORE_1     ", /* ASTORE_1     76 */
	"ASTORE_2     ", /* ASTORE_2     77 */
	"ASTORE_3     ", /* ASTORE_3     78 */
	"IASTORE      ", /*              79 */
	"LASTORE      ", /*              80 */
	"FASTORE      ", /*              81 */
	"DASTORE      ", /*              82 */
	"AASTORE      ", /*              83 */
	"BASTORE      ", /*              84 */
	"CASTORE      ", /*              85 */
	"SASTORE      ", /*              86 */
	"POP          ", /*              87 */
	"POP2         ", /*              88 */
	"DUP          ", /*              89 */
	"DUP_X1       ", /*              90 */
	"DUP_X2       ", /*              91 */
	"DUP2         ", /*              92 */
	"DUP2_X1      ", /*              93 */
	"DUP2_X2      ", /*              94 */
	"SWAP         ", /*              95 */
	"IADD         ", /*              96 */
	"LADD         ", /*              97 */
	"FADD         ", /*              98 */
	"DADD         ", /*              99 */
	"ISUB         ", /*             100 */
	"LSUB         ", /*             101 */
	"FSUB         ", /*             102 */
	"DSUB         ", /*             103 */
	"IMUL         ", /*             104 */
	"LMUL         ", /*             105 */
	"FMUL         ", /*             106 */
	"DMUL         ", /*             107 */
	"IDIV         ", /*             108 */
	"LDIV         ", /*             109 */
	"FDIV         ", /*             110 */
	"DDIV         ", /*             111 */
	"IREM         ", /*             112 */
	"LREM         ", /*             113 */
	"FREM         ", /*             114 */
	"DREM         ", /*             115 */
	"INEG         ", /*             116 */
	"LNEG         ", /*             117 */
	"FNEG         ", /*             118 */
	"DNEG         ", /*             119 */
	"ISHL         ", /*             120 */
	"LSHL         ", /*             121 */
	"ISHR         ", /*             122 */
	"LSHR         ", /*             123 */
	"IUSHR        ", /*             124 */
	"LUSHR        ", /*             125 */
	"IAND         ", /*             126 */
	"LAND         ", /*             127 */
	"IOR          ", /*             128 */
	"LOR          ", /*             129 */
	"IXOR         ", /*             130 */
	"LXOR         ", /*             131 */
	"IINC         ", /*             132 */
	"I2L          ", /*             133 */
	"I2F          ", /*             134 */
	"I2D          ", /*             135 */
	"L2I          ", /*             136 */
	"L2F          ", /*             137 */
	"L2D          ", /*             138 */
	"F2I          ", /*             139 */
	"F2L          ", /*             140 */
	"F2D          ", /*             141 */
	"D2I          ", /*             142 */
	"D2L          ", /*             143 */
	"D2F          ", /*             144 */
	"INT2BYTE     ", /*             145 */
	"INT2CHAR     ", /*             146 */
	"INT2SHORT    ", /*             147 */
	"LCMP         ", /*             148 */
	"FCMPL        ", /*             149 */
	"FCMPG        ", /*             150 */
	"DCMPL        ", /*             151 */
	"DCMPG        ", /*             152 */
	"IFEQ         ", /*             153 */
	"IFNE         ", /*             154 */
	"IFLT         ", /*             155 */
	"IFGE         ", /*             156 */
	"IFGT         ", /*             157 */
	"IFLE         ", /*             158 */
	"IF_ICMPEQ    ", /*             159 */
	"IF_ICMPNE    ", /*             160 */
	"IF_ICMPLT    ", /*             161 */
	"IF_ICMPGE    ", /*             162 */
	"IF_ICMPGT    ", /*             163 */
	"IF_ICMPLE    ", /*             164 */
	"IF_ACMPEQ    ", /*             165 */
	"IF_ACMPNE    ", /*             166 */
	"GOTO         ", /*             167 */
	"JSR          ", /*             168 */
	"RET          ", /*             169 */
	"TABLESWITCH  ", /*             170 */
	"LOOKUPSWITCH ", /*             171 */
	"IRETURN      ", /*             172 */
	"LRETURN      ", /*             173 */
	"FRETURN      ", /*             174 */
	"DRETURN      ", /*             175 */
	"ARETURN      ", /*             176 */
	"RETURN       ", /*             177 */
	"GETSTATIC    ", /*             178 */
	"PUTSTATIC    ", /*             179 */
	"GETFIELD     ", /*             180 */
	"PUTFIELD     ", /*             181 */
	"INVOKEVIRTUAL", /*             182 */
	"INVOKESPECIAL", /*             183 */
	"INVOKESTATIC ", /*             184 */
	"INVOKEINTERFACE",/*            185 */
	"CHECKASIZE   ", /* UNDEF186    186 */
	"NEW          ", /*             187 */
	"NEWARRAY     ", /*             188 */
	"ANEWARRAY    ", /*             189 */
	"ARRAYLENGTH  ", /*             190 */
	"ATHROW       ", /*             191 */
	"CHECKCAST    ", /*             192 */
	"INSTANCEOF   ", /*             193 */
	"MONITORENTER ", /*             194 */
	"MONITOREXIT  ", /*             195 */
	"WIDE         ", /* WIDE        196 */
	"MULTIANEWARRAY",/*             197 */
	"IFNULL       ", /*             198 */
	"IFNONNULL    ", /*             199 */
	"GOTO_W       ", /* GOTO_W      200 */
	"JSR_W        ", /* JSR_W       201 */
	"BREAKPOINT   ", /* BREAKPOINT  202 */

	                      "UNDEF203","UNDEF204","UNDEF205",
	"UNDEF206","UNDEF207","UNDEF208","UNDEF209","UNDEF210",
	"UNDEF","UNDEF","UNDEF","UNDEF","UNDEF",
	"UNDEF216","UNDEF217","UNDEF218","UNDEF219","UNDEF220",
	"UNDEF","UNDEF","UNDEF","UNDEF","UNDEF",
	"UNDEF226","UNDEF227","UNDEF228","UNDEF229","UNDEF230",
	"UNDEF","UNDEF","UNDEF","UNDEF","UNDEF",
	"UNDEF236","UNDEF237","UNDEF238","UNDEF239","UNDEF240",
	"UNDEF","UNDEF","UNDEF","UNDEF","UNDEF",
	"UNDEF246","UNDEF247","UNDEF248","UNDEF249","UNDEF250",
	"UNDEF251","UNDEF252",

	"BUILTIN3     ", /*             253 */
	"BUILTIN2     ", /*             254 */
	"BUILTIN1     "  /*             255 */
};


#if defined(USEBUILTINTABLE)

stdopdescriptor builtintable[] = {
	{ ICMD_LCMP,   TYPE_LONG, TYPE_LONG, TYPE_INT, ICMD_BUILTIN2,
	  (functionptr) builtin_lcmp , SUPPORT_LONG && SUPPORT_LONG_CMP, false },
	{ ICMD_LAND,   TYPE_LONG, TYPE_LONG, TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_land , SUPPORT_LONG && SUPPORT_LONG_LOG, false },
	{ ICMD_LOR,    TYPE_LONG, TYPE_LONG, TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lor , SUPPORT_LONG && SUPPORT_LONG_LOG, false },
	{ ICMD_LXOR,   TYPE_LONG, TYPE_LONG, TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lxor , SUPPORT_LONG && SUPPORT_LONG_LOG, false },
	{ ICMD_LSHL,   TYPE_LONG, TYPE_INT,  TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lshl , SUPPORT_LONG && SUPPORT_LONG_SHIFT, false },
	{ ICMD_LSHR,   TYPE_LONG, TYPE_INT,  TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lshr, SUPPORT_LONG && SUPPORT_LONG_SHIFT, false },
	{ ICMD_LUSHR,  TYPE_LONG, TYPE_INT,  TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lushr, SUPPORT_LONG && SUPPORT_LONG_SHIFT, false },
	{ ICMD_LADD,   TYPE_LONG, TYPE_LONG, TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_ladd , SUPPORT_LONG && SUPPORT_LONG_ADD, false },
	{ ICMD_LSUB,   TYPE_LONG, TYPE_LONG, TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lsub , SUPPORT_LONG && SUPPORT_LONG_ADD, false },
	{ ICMD_LNEG,   TYPE_LONG, TYPE_VOID, TYPE_LONG, ICMD_BUILTIN1, 
	  (functionptr) builtin_lneg, SUPPORT_LONG && SUPPORT_LONG_ADD, true },
	{ ICMD_LMUL,   TYPE_LONG, TYPE_LONG, TYPE_LONG, ICMD_BUILTIN2,
	  (functionptr) builtin_lmul , SUPPORT_LONG && SUPPORT_LONG_MUL, false },
	{ ICMD_I2F,    TYPE_INT, TYPE_VOID, TYPE_FLOAT, ICMD_BUILTIN1,
	  (functionptr) builtin_i2f, SUPPORT_FLOAT && SUPPORT_IFCVT, true },
	{ ICMD_I2D,    TYPE_INT, TYPE_VOID, TYPE_DOUBLE, ICMD_BUILTIN1, 
	  (functionptr) builtin_i2d, SUPPORT_DOUBLE && SUPPORT_IFCVT, true },
	{ ICMD_L2F,    TYPE_LONG, TYPE_VOID, TYPE_FLOAT, ICMD_BUILTIN1,
	  (functionptr) builtin_l2f, SUPPORT_LONG && SUPPORT_FLOAT && SUPPORT_LONG_FCVT, true },
	{ ICMD_L2D,    TYPE_LONG, TYPE_VOID, TYPE_DOUBLE, ICMD_BUILTIN1, 
	  (functionptr) builtin_l2d, SUPPORT_LONG && SUPPORT_DOUBLE && SUPPORT_LONG_FCVT, true },
	{ ICMD_F2L,    TYPE_FLOAT, TYPE_VOID, TYPE_LONG, ICMD_BUILTIN1,
	  (functionptr) builtin_f2l, SUPPORT_FLOAT && SUPPORT_LONG && SUPPORT_LONG_ICVT, true },
	{ ICMD_D2L,    TYPE_DOUBLE, TYPE_VOID, TYPE_LONG, ICMD_BUILTIN1,
	  (functionptr) builtin_d2l, SUPPORT_DOUBLE && SUPPORT_LONG && SUPPORT_LONG_ICVT, true },
	{ ICMD_F2I,    TYPE_FLOAT, TYPE_VOID, TYPE_INT, ICMD_BUILTIN1,
	  (functionptr) builtin_f2i, SUPPORT_FLOAT && SUPPORT_FICVT, true },
	{ ICMD_D2I,    TYPE_DOUBLE, TYPE_VOID, TYPE_INT, ICMD_BUILTIN1,
	  (functionptr) builtin_d2i, SUPPORT_DOUBLE && SUPPORT_FICVT, true },
  	{ 255, 0, 0, 0, 0, NULL, true, false },
};

static int builtintablelen;

#endif /* USEBUILTINTABLE */


/* include compiler subsystems ************************************************/

/* from codegen.inc */
extern int dseglen;


/* dummy function, used when there is no JavaVM code available                */

static void* do_nothing_function()
{
	return NULL;
}


/* jit_compile *****************************************************************

	jit_compile, new version of compiler, translates one method to machine code

*******************************************************************************/

methodptr jit_compile(methodinfo *m)
{
	int dumpsize;
	long starttime = 0;
	long stoptime  = 0;

	count_jit_calls++;

	/* if method has been already compiled return immediately */

	if (m->entrypoint)
		return m->entrypoint;

	count_methods++;

	intsDisable();      /* disable interrupts */

	regs_ok = false;

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* measure time */

	if (getcompilingtime)
		starttime = getcputime();

	/* if there is no javacode print error message and return empty method    */

	if (!m->jcode) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "No code given for: ");
		utf_sprint(logtext+strlen(logtext), m->class->name);
		strcpy(logtext+strlen(logtext), ".");
		utf_sprint(logtext+strlen(logtext), m->name);
		utf_sprint(logtext+strlen(logtext), m->descriptor);
		dolog(logtext);
		intsRestore();                             /* enable interrupts again */
		return (methodptr) do_nothing_function;    /* return empty method     */
	}

	/* print log message for compiled method */

	if (compileverbose) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Compiling: ");
		utf_sprint(logtext+strlen(logtext), m->class->name);
		strcpy(logtext+strlen(logtext), ".");
		utf_sprint(logtext+strlen(logtext), m->name);
		utf_sprint(logtext+strlen(logtext), m->descriptor);
		dolog(logtext);
	}


	/* initialisation of variables and subsystems */

	isleafmethod = true;

	method = m;
	class = m->class;
	descriptor = m->descriptor;
	maxstack = m->maxstack;
	maxlocals = m->maxlocals;
	jcodelength = m->jcodelength;
	jcode = m->jcode;
	exceptiontablelength = m->exceptiontablelength;
	raw_extable = m->exceptiontable;

#ifdef STATISTICS
	count_tryblocks += exceptiontablelength;
	count_javacodesize += jcodelength + 18;
	count_javaexcsize += exceptiontablelength * POINTERSIZE;
#endif

	/* initialise parameter type descriptor */

	descriptor2types(m);
	mreturntype = m->returntype;
	mparamcount = m->paramcount;
	mparamtypes = m->paramtypes;

	/* initialize class list with class the compiled method belongs to */

	uninitializedclasses = chain_new(); 
	compiler_addinitclass(m->class);


	/* call the compiler passes ***********************************************/

	/* must be call before reg_init, because it can change maxlocals */
	if (useinlining)
		inlining_init(m);

	reg_init(m);

	codegen_init();

	parse();
	analyse_stack();
   
#ifdef CACAO_TYPECHECK
	/* print log message for compiled method */

	if (compileverbose) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Typechecking: ");
		utf_sprint(logtext+strlen(logtext), m->class->name);
		strcpy(logtext+strlen(logtext), ".");
		utf_sprint(logtext+strlen(logtext), m->name);
		utf_sprint(logtext+strlen(logtext), m->descriptor);
		dolog(logtext);
	}

	typecheck();
	
	if (compileverbose) {
		dolog("Typechecking done.");
	}
#endif
	
	if (opt_loops) {
		depthFirst();
		analyseGraph();
		optimize_loops();
	}
   
#ifdef SPECIALMEMUSE
	preregpass();
#endif

	regalloc();
	regs_ok = true;
	codegen();

	/* intermediate and assembly code listings ********************************/
		
	if (showintermediate)
		show_icmd_method();
	else if (showdisassemble)
		disassemble((void*) (m->mcode + dseglen), m->mcodelength - dseglen);

	if (showddatasegment)
		dseg_display((void*) (m->mcode));

	/* release dump area */

	dump_release(dumpsize);

	/* measure time */

	if (getcompilingtime) {
		stoptime = getcputime();
		compilingtime += (stoptime-starttime); 
	}

	/* initialize all used classes */
	/* because of reentrant code global variables are not allowed here        */

	{
		chain *ul = uninitializedclasses;   /* list of uninitialized classes      */ 
		classinfo *c;                       /* single class                       */

		while ((c = chain_first(ul)) != NULL) {
			chain_remove(ul);
			class_init(c);                  /* may again call the compiler        */
		}
		chain_free(ul);
	}

	intsRestore();    /* enable interrupts again */

	/* return pointer to the methods entry point */
	
	return m->entrypoint;
} 



/* functions for compiler initialisation and finalisation *********************/

#ifdef USEBUILTINTABLE

static int stdopcompare(const void *a, const void *b)
{
	stdopdescriptor *o1 = (stdopdescriptor *) a;
	stdopdescriptor *o2 = (stdopdescriptor *) b;
	if (!o1->supported && o2->supported)
		return -1;
	if (o1->supported && !o2->supported)
		return 1;
	return (o1->opcode < o2->opcode) ? -1 : (o1->opcode > o2->opcode);
}


static inline void sort_builtintable()
{
	int len;

	len = sizeof(builtintable) / sizeof(stdopdescriptor);
	qsort(builtintable, len, sizeof(stdopdescriptor), stdopcompare);

	for (--len; len>=0 && builtintable[len].supported; len--);
	builtintablelen = ++len;

#if 0
	{
		int i;
		for (i=0; i<len; i++)
			if (!builtintable[i].supported)
				printf("%s\n", icmd_names[builtintable[i].opcode]);
	}
#endif
}


stdopdescriptor *find_builtin(int icmd)
{
	stdopdescriptor *first = builtintable;
	stdopdescriptor *last = builtintable + builtintablelen;
	int len = last - first;
	int half;
	stdopdescriptor *middle;

	while (len > 0) {
		half = len / 2;
		middle = first + half;
		if (middle->opcode < icmd) {
			first = middle + 1;
			len -= half + 1;
		} else
			len = half;
	}
	return first != last ? first : NULL;
}

#endif



void jit_init()
{
	int i;

#ifdef USEBUILTINTABLE
	sort_builtintable();
#endif

#if defined(__ALPHA__)
	has_ext_instr_set = ! has_no_x_instr_set();
#endif

	for (i = 0; i < 256; i++)
		stackreq[i] = 1;

	stackreq[JAVA_NOP]          = 0;
	stackreq[JAVA_ISTORE]       = 0;
	stackreq[JAVA_LSTORE]       = 0;
	stackreq[JAVA_FSTORE]       = 0;
	stackreq[JAVA_DSTORE]       = 0;
	stackreq[JAVA_ASTORE]       = 0;
	stackreq[JAVA_ISTORE_0]     = 0;
	stackreq[JAVA_ISTORE_1]     = 0;
	stackreq[JAVA_ISTORE_2]     = 0;
	stackreq[JAVA_ISTORE_3]     = 0;
	stackreq[JAVA_LSTORE_0]     = 0;
	stackreq[JAVA_LSTORE_1]     = 0;
	stackreq[JAVA_LSTORE_2]     = 0;
	stackreq[JAVA_LSTORE_3]     = 0;
	stackreq[JAVA_FSTORE_0]     = 0;
	stackreq[JAVA_FSTORE_1]     = 0;
	stackreq[JAVA_FSTORE_2]     = 0;
	stackreq[JAVA_FSTORE_3]     = 0;
	stackreq[JAVA_DSTORE_0]     = 0;
	stackreq[JAVA_DSTORE_1]     = 0;
	stackreq[JAVA_DSTORE_2]     = 0;
	stackreq[JAVA_DSTORE_3]     = 0;
	stackreq[JAVA_ASTORE_0]     = 0;
	stackreq[JAVA_ASTORE_1]     = 0;
	stackreq[JAVA_ASTORE_2]     = 0;
	stackreq[JAVA_ASTORE_3]     = 0;
	stackreq[JAVA_IASTORE]      = 0;
	stackreq[JAVA_LASTORE]      = 0;
	stackreq[JAVA_FASTORE]      = 0;
	stackreq[JAVA_DASTORE]      = 0;
	stackreq[JAVA_AASTORE]      = 0;
	stackreq[JAVA_BASTORE]      = 0;
	stackreq[JAVA_CASTORE]      = 0;
	stackreq[JAVA_SASTORE]      = 0;
	stackreq[JAVA_POP]          = 0;
	stackreq[JAVA_POP2]         = 0;
	stackreq[JAVA_IINC]         = 0;
	stackreq[JAVA_IFEQ]         = 0;
	stackreq[JAVA_IFNE]         = 0;
	stackreq[JAVA_IFLT]         = 0;
	stackreq[JAVA_IFGE]         = 0;
	stackreq[JAVA_IFGT]         = 0;
	stackreq[JAVA_IFLE]         = 0;
	stackreq[JAVA_IF_ICMPEQ]    = 0;
	stackreq[JAVA_IF_ICMPNE]    = 0;
	stackreq[JAVA_IF_ICMPLT]    = 0;
	stackreq[JAVA_IF_ICMPGE]    = 0;
	stackreq[JAVA_IF_ICMPGT]    = 0;
	stackreq[JAVA_IF_ICMPLE]    = 0;
	stackreq[JAVA_IF_ACMPEQ]    = 0;
	stackreq[JAVA_IF_ACMPNE]    = 0;
	stackreq[JAVA_GOTO]         = 0;
	stackreq[JAVA_RET]          = 0;
	stackreq[JAVA_TABLESWITCH]  = 0;
	stackreq[JAVA_LOOKUPSWITCH] = 0;
	stackreq[JAVA_IRETURN]      = 0;
	stackreq[JAVA_LRETURN]      = 0;
	stackreq[JAVA_FRETURN]      = 0;
	stackreq[JAVA_DRETURN]      = 0;
	stackreq[JAVA_ARETURN]      = 0;
	stackreq[JAVA_RETURN]       = 0;
	stackreq[JAVA_PUTSTATIC]    = 0;
	stackreq[JAVA_PUTFIELD]     = 0;
	stackreq[JAVA_MONITORENTER] = 0;
	stackreq[JAVA_MONITOREXIT]  = 0;
	stackreq[JAVA_WIDE]         = 0;
	stackreq[JAVA_IFNULL]       = 0;
	stackreq[JAVA_IFNONNULL]    = 0;
	stackreq[JAVA_GOTO_W]       = 0;
	stackreq[JAVA_BREAKPOINT]   = 0;

	stackreq[JAVA_SWAP] = 2;
	stackreq[JAVA_DUP2] = 2;
	stackreq[JAVA_DUP_X1] = 3;
	stackreq[JAVA_DUP_X2] = 4;
	stackreq[JAVA_DUP2_X1] = 3;
	stackreq[JAVA_DUP2_X2] = 4;
	
	init_exceptions();
}



void jit_close()
{
	codegen_close();
	reg_close();
}


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
