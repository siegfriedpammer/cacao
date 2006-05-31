/* src/vm/jit/jit.c - calls the code generation functions

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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

   Contact: cacao@cacaojvm.org

   Authors: Andreas Krall
            Reinhard Grafl

   Changes: Edwin Steiner
            Christian Thalinger
            Christian Ullrich

   $Id: jit.c 5000 2006-05-31 21:31:29Z edwin $

*/


#include "config.h"
#include "vm/types.h"

#include <assert.h>

#include "mm/memory.h"
#include "native/native.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/loader.h"
#include "vm/method.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/disass.h"
#include "vm/jit/dseg.h"
#include "vm/jit/jit.h"
#include "vm/jit/show.h"


#include "vm/jit/parse.h"
#include "vm/jit/reg.h"
#include "vm/jit/stack.h"

#include "vm/jit/allocator/simplereg.h"
#if defined(ENABLE_LSRA)
# include "vm/jit/allocator/lsra.h"
#endif

#if defined(ENABLE_IFCONV)
# include "vm/jit/ifconv/ifconv.h"
#endif

#include "vm/jit/loop/analyze.h"
#include "vm/jit/loop/graph.h"
#include "vm/jit/loop/loop.h"
#include "vm/jit/verify/typecheck.h"
#include "vm/rt-timing.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif


/* debug macros ***************************************************************/

#if !defined(NDEBUG)
#define DEBUG_JIT_COMPILEVERBOSE(x) \
    do { \
        if (compileverbose) { \
            log_message_method(x, m); \
        } \
    } while (0)
#else
#define DEBUG_JIT_COMPILEVERBOSE(x)    /* nothing */
#endif

 
/* global switches ************************************************************/

int stackreq[256];

                                
int jcommandsize[256] = {

#define JAVA_NOP               0
#define ICMD_NOP               0
	1,
#define JAVA_ACONST_NULL       1
#define ICMD_ACONST            1        /* val.a = constant                   */
	1,
#define JAVA_ICONST_M1         2
#define ICMD_CHECKNULL         2
	1,
#define JAVA_ICONST_0          3
#define ICMD_ICONST            3        /* val.i = constant                   */
	1,
#define JAVA_ICONST_1          4
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
	                                    /* equal to order of TYPE_*   defines */
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
	                                    /* equal to order of TYPE_* defines   */
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
/* UNDEF186 */
	1,
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
/* UNDEF 203 */
	1,
#define ICMD_IASTORECONST     204
	1,
#define ICMD_LASTORECONST     205
	1,
#define ICMD_FASTORECONST     206
	1,
#define ICMD_DASTORECONST     207
	1,
#define ICMD_AASTORECONST     208
	1,
#define ICMD_BASTORECONST     209
	1,
#define ICMD_CASTORECONST     210
	1,
#define ICMD_SASTORECONST     211
	1,
#define ICMD_PUTSTATICCONST   212
	1,
#define ICMD_PUTFIELDCONST    213
	1,
#define ICMD_IMULPOW2         214
	1,
#define ICMD_LMULPOW2         215
	1,

#define ICMD_IF_FCMPEQ        216
	1,
#define ICMD_IF_FCMPNE        217

#define ICMD_IF_FCMPL_LT      218
	1,
#define ICMD_IF_FCMPL_GE      219
	1,
#define ICMD_IF_FCMPL_GT      220
	1,
#define ICMD_IF_FCMPL_LE      221
	1,

#define ICMD_IF_FCMPG_LT      222
	1,
#define ICMD_IF_FCMPG_GE      223
	1,
#define ICMD_IF_FCMPG_GT      224
	1,
#define ICMD_IF_FCMPG_LE      225
	1,

#define ICMD_IF_DCMPEQ        226
	1,
#define ICMD_IF_DCMPNE        227
	1,

#define ICMD_IF_DCMPL_LT      228
	1,
#define ICMD_IF_DCMPL_GE      229
	1,
#define ICMD_IF_DCMPL_GT      230
	1,
#define ICMD_IF_DCMPL_LE      231
	1,

#define ICMD_IF_DCMPG_LT      232
	1,
#define ICMD_IF_DCMPG_GE      233
	1,
#define ICMD_IF_DCMPG_GT      234
	1,
#define ICMD_IF_DCMPG_LE      235
	1,

	/* unused */
	        1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1
};


char *icmd_names[256] = {
	"NOP            ", /*               0 */
	"ACONST         ", /*               1 */
	"CHECKNULL      ", /* ICONST_M1     2 */
	"ICONST         ", /*               3 */
	"UNDEF4         ", /* ICONST_1      4 */
	"IDIVPOW2       ", /* ICONST_2      5 */
	"LDIVPOW2       ", /* ICONST_3      6 */
	"UNDEF7         ", /* ICONST_4      7 */
	"UNDEF8         ", /* ICONST_5      8 */
	"LCONST         ", /*               9 */
	"LCMPCONST      ", /* LCONST_1     10 */
	"FCONST         ", /*              11 */
	"UNDEF12        ", /* FCONST_1     12 */
	"ELSE_ICONST    ", /* FCONST_2     13 */
	"DCONST         ", /*              14 */
	"IFEQ_ICONST    ", /* DCONST_1     15 */
	"IFNE_ICONST    ", /* BIPUSH       16 */
	"IFLT_ICONST    ", /* SIPUSH       17 */
	"IFGE_ICONST    ", /* LDC1         18 */
	"IFGT_ICONST    ", /* LDC2         19 */
	"IFLE_ICONST    ", /* LDC2W        20 */
	"ILOAD          ", /*              21 */
	"LLOAD          ", /*              22 */
	"FLOAD          ", /*              23 */
	"DLOAD          ", /*              24 */
	"ALOAD          ", /*              25 */
	"IADDCONST      ", /* ILOAD_0      26 */
	"ISUBCONST      ", /* ILOAD_1      27 */
	"IMULCONST      ", /* ILOAD_2      28 */
	"IANDCONST      ", /* ILOAD_3      29 */
	"IORCONST       ", /* LLOAD_0      30 */
	"IXORCONST      ", /* LLOAD_1      31 */
	"ISHLCONST      ", /* LLOAD_2      32 */
	"ISHRCONST      ", /* LLOAD_3      33 */
	"IUSHRCONST     ", /* FLOAD_0      34 */
	"IREMPOW2       ", /* FLOAD_1      35 */
	"LADDCONST      ", /* FLOAD_2      36 */
	"LSUBCONST      ", /* FLOAD_3      37 */
	"LMULCONST      ", /* DLOAD_0      38 */
	"LANDCONST      ", /* DLOAD_1      39 */
	"LORCONST       ", /* DLOAD_2      40 */
	"LXORCONST      ", /* DLOAD_3      41 */
	"LSHLCONST      ", /* ALOAD_0      42 */
	"LSHRCONST      ", /* ALOAD_1      43 */
	"LUSHRCONST     ", /* ALOAD_2      44 */
	"LREMPOW2       ", /* ALOAD_3      45 */
	"IALOAD         ", /*              46 */
	"LALOAD         ", /*              47 */
	"FALOAD         ", /*              48 */
	"DALOAD         ", /*              49 */
	"AALOAD         ", /*              50 */
	"BALOAD         ", /*              51 */
	"CALOAD         ", /*              52 */
	"SALOAD         ", /*              53 */
	"ISTORE         ", /*              54 */
	"LSTORE         ", /*              55 */
	"FSTORE         ", /*              56 */
	"DSTORE         ", /*              57 */
	"ASTORE         ", /*              58 */
	"IF_LEQ         ", /* ISTORE_0     59 */
	"IF_LNE         ", /* ISTORE_1     60 */
	"IF_LLT         ", /* ISTORE_2     61 */
	"IF_LGE         ", /* ISTORE_3     62 */
	"IF_LGT         ", /* LSTORE_0     63 */
	"IF_LLE         ", /* LSTORE_1     64 */
	"IF_LCMPEQ      ", /* LSTORE_2     65 */
	"IF_LCMPNE      ", /* LSTORE_3     66 */
	"IF_LCMPLT      ", /* FSTORE_0     67 */
	"IF_LCMPGE      ", /* FSTORE_1     68 */
	"IF_LCMPGT      ", /* FSTORE_2     69 */
	"IF_LCMPLE      ", /* FSTORE_3     70 */
	"UNDEF71        ", /* DSTORE_0     71 */
	"UNDEF72        ", /* DSTORE_1     72 */
	"UNDEF73        ", /* DSTORE_2     73 */
	"UNDEF74        ", /* DSTORE_3     74 */
	"UNDEF75        ", /* ASTORE_0     75 */
	"UNDEF76        ", /* ASTORE_1     76 */
	"UNDEF77        ", /* ASTORE_2     77 */
	"UNDEF78        ", /* ASTORE_3     78 */
	"IASTORE        ", /*              79 */
	"LASTORE        ", /*              80 */
	"FASTORE        ", /*              81 */
	"DASTORE        ", /*              82 */
	"AASTORE        ", /*              83 */
	"BASTORE        ", /*              84 */
	"CASTORE        ", /*              85 */
	"SASTORE        ", /*              86 */
	"POP            ", /*              87 */
	"POP2           ", /*              88 */
	"DUP            ", /*              89 */
	"DUP_X1         ", /*              90 */
	"DUP_X2         ", /*              91 */
	"DUP2           ", /*              92 */
	"DUP2_X1        ", /*              93 */
	"DUP2_X2        ", /*              94 */
	"SWAP           ", /*              95 */
	"IADD           ", /*              96 */
	"LADD           ", /*              97 */
	"FADD           ", /*              98 */
	"DADD           ", /*              99 */
	"ISUB           ", /*             100 */
	"LSUB           ", /*             101 */
	"FSUB           ", /*             102 */
	"DSUB           ", /*             103 */
	"IMUL           ", /*             104 */
	"LMUL           ", /*             105 */
	"FMUL           ", /*             106 */
	"DMUL           ", /*             107 */
	"IDIV           ", /*             108 */
	"LDIV           ", /*             109 */
	"FDIV           ", /*             110 */
	"DDIV           ", /*             111 */
	"IREM           ", /*             112 */
	"LREM           ", /*             113 */
	"FREM           ", /*             114 */
	"DREM           ", /*             115 */
	"INEG           ", /*             116 */
	"LNEG           ", /*             117 */
	"FNEG           ", /*             118 */
	"DNEG           ", /*             119 */
	"ISHL           ", /*             120 */
	"LSHL           ", /*             121 */
	"ISHR           ", /*             122 */
	"LSHR           ", /*             123 */
	"IUSHR          ", /*             124 */
	"LUSHR          ", /*             125 */
	"IAND           ", /*             126 */
	"LAND           ", /*             127 */
	"IOR            ", /*             128 */
	"LOR            ", /*             129 */
	"IXOR           ", /*             130 */
	"LXOR           ", /*             131 */
	"IINC           ", /*             132 */
	"I2L            ", /*             133 */
	"I2F            ", /*             134 */
	"I2D            ", /*             135 */
	"L2I            ", /*             136 */
	"L2F            ", /*             137 */
	"L2D            ", /*             138 */
	"F2I            ", /*             139 */
	"F2L            ", /*             140 */
	"F2D            ", /*             141 */
	"D2I            ", /*             142 */
	"D2L            ", /*             143 */
	"D2F            ", /*             144 */
	"INT2BYTE       ", /*             145 */
	"INT2CHAR       ", /*             146 */
	"INT2SHORT      ", /*             147 */
	"LCMP           ", /*             148 */
	"FCMPL          ", /*             149 */
	"FCMPG          ", /*             150 */
	"DCMPL          ", /*             151 */
	"DCMPG          ", /*             152 */
	"IFEQ           ", /*             153 */
	"IFNE           ", /*             154 */
	"IFLT           ", /*             155 */
	"IFGE           ", /*             156 */
	"IFGT           ", /*             157 */
	"IFLE           ", /*             158 */
	"IF_ICMPEQ      ", /*             159 */
	"IF_ICMPNE      ", /*             160 */
	"IF_ICMPLT      ", /*             161 */
	"IF_ICMPGE      ", /*             162 */
	"IF_ICMPGT      ", /*             163 */
	"IF_ICMPLE      ", /*             164 */
	"IF_ACMPEQ      ", /*             165 */
	"IF_ACMPNE      ", /*             166 */
	"GOTO           ", /*             167 */
	"JSR            ", /*             168 */
	"RET            ", /*             169 */
	"TABLESWITCH    ", /*             170 */
	"LOOKUPSWITCH   ", /*             171 */
	"IRETURN        ", /*             172 */
	"LRETURN        ", /*             173 */
	"FRETURN        ", /*             174 */
	"DRETURN        ", /*             175 */
	"ARETURN        ", /*             176 */
	"RETURN         ", /*             177 */
	"GETSTATIC      ", /*             178 */
	"PUTSTATIC      ", /*             179 */
	"GETFIELD       ", /*             180 */
	"PUTFIELD       ", /*             181 */
	"INVOKEVIRTUAL  ", /*             182 */
	"INVOKESPECIAL  ", /*             183 */
	"INVOKESTATIC   ", /*             184 */
	"INVOKEINTERFACE", /*             185 */
	"UNDEF186       ", /* UNDEF186    186 */
	"NEW            ", /*             187 */
	"NEWARRAY       ", /*             188 */
	"ANEWARRAY      ", /*             189 */
	"ARRAYLENGTH    ", /*             190 */
	"ATHROW         ", /*             191 */
	"CHECKCAST      ", /*             192 */
	"INSTANCEOF     ", /*             193 */
	"MONITORENTER   ", /*             194 */
	"MONITOREXIT    ", /*             195 */
	"UNDEF196       ", /* WIDE        196 */
	"MULTIANEWARRAY ", /*             197 */
	"IFNULL         ", /*             198 */
	"IFNONNULL      ", /*             199 */
	"UNDEF200       ", /* GOTO_W      200 */
	"UNDEF201       ", /* JSR_W       201 */
	"UNDEF202       ", /* BREAKPOINT  202 */
	"UNDEF203       ", /* UNDEF203    203 */
	"IASTORECONST   ", /*             204 */
	"LASTORECONST   ", /*             205 */
	"FASTORECONST   ", /*             206 */
	"DASTORECONST   ", /*             207 */
	"AASTORECONST   ", /*             208 */
	"BASTORECONST   ", /*             209 */
	"CASTORECONST   ", /*             210 */
	"SASTORECONST   ", /*             211 */
	"PUTSTATICCONST ", /*             212 */
	"PUTFIELDCONST  ", /*             213 */
	"IMULPOW2       ", /*             214 */
	"LMULPOW2       ", /*             215 */

	"IF_FCMPEQ      ", /*             216 */
	"IF_FCMPNE      ", /*             217 */

	"IF_FCMPL_LT    ", /*             218 */
	"IF_FCMPL_GE    ", /*             219 */
	"IF_FCMPL_GT    ", /*             220 */
	"IF_FCMPL_LE    ", /*             221 */

	"IF_FCMPG_LT    ", /*             222 */
	"IF_FCMPG_GE    ", /*             223 */
	"IF_FCMPG_GT    ", /*             224 */
	"IF_FCMPG_LE    ", /*             225 */

	"IF_DCMPEQ      ", /*             226 */
	"IF_DCMPNE      ", /*             227 */

	"IF_DCMPL_LT    ", /*             228 */
	"IF_DCMPL_GE    ", /*             229 */
	"IF_DCMPL_GT    ", /*             230 */
	"IF_DCMPL_LE    ", /*             231 */
	
	"IF_DCMPG_LT    ", /*             232 */
	"IF_DCMPG_GE    ", /*             233 */
	"IF_DCMPG_GT    ", /*             234 */
	"IF_DCMPG_LE    ", /*             235 */
	
	"UNDEF236", "UNDEF237", "UNDEF238", "UNDEF239", "UNDEF240",
	"UNDEF241", "UNDEF242", "UNDEF243", "UNDEF244", "UNDEF245",
	"UNDEF246", "UNDEF247", "UNDEF248", "UNDEF249", "UNDEF250",

	"INLINE_START   ", /*             251 */
	"INLINE_END     ", /*             252 */
	"INLINE_GOTO    ", /*             253 */

	"UNDEF254",

	"BUILTIN        "  /*             255 */
};


char *opcode_names[256] = {
	"NOP            ", /*               0 */
	"ACONST         ", /*               1 */
	"ICONST_M1      ", /* ICONST_M1     2 */
	"ICONST_0       ", /* ICONST_0      3 */
	"ICONST_1       ", /* ICONST_1      4 */
	"ICONST_2       ", /* ICONST_2      5 */
	"ICONST_3       ", /* ICONST_3      6 */
	"ICONST_4       ", /* ICONST_4      7 */
	"ICONST_5       ", /* ICONST_5      8 */
	"LCONST_0       ", /* LCONST_0      9 */
	"LCONST_1       ", /* LCONST_1     10 */
	"FCONST_0       ", /* FCONST_0     11 */
	"FCONST_1       ", /* FCONST_1     12 */
	"FCONST_2       ", /* FCONST_2     13 */
	"DCONST_0       ", /* DCONST_0     14 */
	"DCONST_1       ", /* DCONST_1     15 */
	"BIPUSH         ", /* BIPUSH       16 */
	"SIPUSH         ", /* SIPUSH       17 */
	"LDC            ", /* LDC          18 */
	"LDC_W          ", /* LDC_W        19 */
	"LDC2_W         ", /* LDC2_W       20 */
	"ILOAD          ", /*              21 */
	"LLOAD          ", /*              22 */
	"FLOAD          ", /*              23 */
	"DLOAD          ", /*              24 */
	"ALOAD          ", /*              25 */
	"ILOAD_0        ", /* ILOAD_0      26 */
	"ILOAD_1        ", /* ILOAD_1      27 */
	"ILOAD_2        ", /* ILOAD_2      28 */
	"ILOAD_3        ", /* ILOAD_3      29 */
	"LLOAD_0        ", /* LLOAD_0      30 */
	"LLOAD_1        ", /* LLOAD_1      31 */
	"LLOAD_2        ", /* LLOAD_2      32 */
	"LLOAD_3        ", /* LLOAD_3      33 */
	"FLOAD_0        ", /* FLOAD_0      34 */
	"FLOAD_1        ", /* FLOAD_1      35 */
	"FLOAD_2        ", /* FLOAD_2      36 */
	"FLOAD_3        ", /* FLOAD_3      37 */
	"DLOAD_0        ", /* DLOAD_0      38 */
	"DLOAD_1        ", /* DLOAD_1      39 */
	"DLOAD_2        ", /* DLOAD_2      40 */ 
	"DLOAD_3        ", /* DLOAD_3      41 */
	"ALOAD_0        ", /* ALOAD_0      42 */
	"ALOAD_1        ", /* ALOAD_1      43 */
	"ALOAD_2        ", /* ALOAD_2      44 */
	"ALOAD_3        ", /* ALOAD_3      45 */
	"IALOAD         ", /*              46 */
	"LALOAD         ", /*              47 */
	"FALOAD         ", /*              48 */
	"DALOAD         ", /*              49 */
	"AALOAD         ", /*              50 */
	"BALOAD         ", /*              51 */
	"CALOAD         ", /*              52 */
	"SALOAD         ", /*              53 */
	"ISTORE         ", /*              54 */
	"LSTORE         ", /*              55 */
	"FSTORE         ", /*              56 */
	"DSTORE         ", /*              57 */
	"ASTORE         ", /*              58 */
	"ISTORE_0       ", /* ISTORE_0     59 */
	"ISTORE_1       ", /* ISTORE_1     60 */
	"ISTORE_2       ", /* ISTORE_2     61 */
	"ISTORE_3       ", /* ISTORE_3     62 */
	"LSTORE_0       ", /* LSTORE_0     63 */
	"LSTORE_1       ", /* LSTORE_1     64 */
	"LSTORE_2       ", /* LSTORE_2     65 */
	"LSTORE_3       ", /* LSTORE_3     66 */
	"FSTORE_0       ", /* FSTORE_0     67 */
	"FSTORE_1       ", /* FSTORE_1     68 */
	"FSTORE_2       ", /* FSTORE_2     69 */
	"FSTORE_3       ", /* FSTORE_3     70 */
	"DSTORE_0       ", /* DSTORE_0     71 */
	"DSTORE_1       ", /* DSTORE_1     72 */
	"DSTORE_2       ", /* DSTORE_2     73 */
	"DSTORE_3       ", /* DSTORE_3     74 */
	"ASTORE_0       ", /* ASTORE_0     75 */
	"ASTORE_1       ", /* ASTORE_1     76 */
	"ASTORE_2       ", /* ASTORE_2     77 */
	"ASTORE_3       ", /* ASTORE_3     78 */
	"IASTORE        ", /*              79 */
	"LASTORE        ", /*              80 */
	"FASTORE        ", /*              81 */
	"DASTORE        ", /*              82 */
	"AASTORE        ", /*              83 */
	"BASTORE        ", /*              84 */
	"CASTORE        ", /*              85 */
	"SASTORE        ", /*              86 */
	"POP            ", /*              87 */
	"POP2           ", /*              88 */
	"DUP            ", /*              89 */
	"DUP_X1         ", /*              90 */
	"DUP_X2         ", /*              91 */
	"DUP2           ", /*              92 */
	"DUP2_X1        ", /*              93 */
	"DUP2_X2        ", /*              94 */
	"SWAP           ", /*              95 */
	"IADD           ", /*              96 */
	"LADD           ", /*              97 */
	"FADD           ", /*              98 */
	"DADD           ", /*              99 */
	"ISUB           ", /*             100 */
	"LSUB           ", /*             101 */
	"FSUB           ", /*             102 */
	"DSUB           ", /*             103 */
	"IMUL           ", /*             104 */
	"LMUL           ", /*             105 */
	"FMUL           ", /*             106 */
	"DMUL           ", /*             107 */
	"IDIV           ", /*             108 */
	"LDIV           ", /*             109 */
	"FDIV           ", /*             110 */
	"DDIV           ", /*             111 */
	"IREM           ", /*             112 */
	"LREM           ", /*             113 */
	"FREM           ", /*             114 */
	"DREM           ", /*             115 */
	"INEG           ", /*             116 */
	"LNEG           ", /*             117 */
	"FNEG           ", /*             118 */
	"DNEG           ", /*             119 */
	"ISHL           ", /*             120 */
	"LSHL           ", /*             121 */
	"ISHR           ", /*             122 */
	"LSHR           ", /*             123 */
	"IUSHR          ", /*             124 */
	"LUSHR          ", /*             125 */
	"IAND           ", /*             126 */
	"LAND           ", /*             127 */
	"IOR            ", /*             128 */
	"LOR            ", /*             129 */
	"IXOR           ", /*             130 */
	"LXOR           ", /*             131 */
	"IINC           ", /*             132 */
	"I2L            ", /*             133 */
	"I2F            ", /*             134 */
	"I2D            ", /*             135 */
	"L2I            ", /*             136 */
	"L2F            ", /*             137 */
	"L2D            ", /*             138 */
	"F2I            ", /*             139 */
	"F2L            ", /*             140 */
	"F2D            ", /*             141 */
	"D2I            ", /*             142 */
	"D2L            ", /*             143 */
	"D2F            ", /*             144 */
	"INT2BYTE       ", /*             145 */
	"INT2CHAR       ", /*             146 */
	"INT2SHORT      ", /*             147 */
	"LCMP           ", /*             148 */
	"FCMPL          ", /*             149 */
	"FCMPG          ", /*             150 */
	"DCMPL          ", /*             151 */
	"DCMPG          ", /*             152 */
	"IFEQ           ", /*             153 */
	"IFNE           ", /*             154 */
	"IFLT           ", /*             155 */
	"IFGE           ", /*             156 */
	"IFGT           ", /*             157 */
	"IFLE           ", /*             158 */
	"IF_ICMPEQ      ", /*             159 */
	"IF_ICMPNE      ", /*             160 */
	"IF_ICMPLT      ", /*             161 */
	"IF_ICMPGE      ", /*             162 */
	"IF_ICMPGT      ", /*             163 */
	"IF_ICMPLE      ", /*             164 */
	"IF_ACMPEQ      ", /*             165 */
	"IF_ACMPNE      ", /*             166 */
	"GOTO           ", /*             167 */
	"JSR            ", /*             168 */
	"RET            ", /*             169 */
	"TABLESWITCH    ", /*             170 */
	"LOOKUPSWITCH   ", /*             171 */
	"IRETURN        ", /*             172 */
	"LRETURN        ", /*             173 */
	"FRETURN        ", /*             174 */
	"DRETURN        ", /*             175 */
	"ARETURN        ", /*             176 */
	"RETURN         ", /*             177 */
	"GETSTATIC      ", /*             178 */
	"PUTSTATIC      ", /*             179 */
	"GETFIELD       ", /*             180 */
	"PUTFIELD       ", /*             181 */
	"INVOKEVIRTUAL  ", /*             182 */
	"INVOKESPECIAL  ", /*             183 */
	"INVOKESTATIC   ", /*             184 */
	"INVOKEINTERFACE", /*             185 */
	"UNDEF186       ", /*             186 */
	"NEW            ", /*             187 */
	"NEWARRAY       ", /*             188 */
	"ANEWARRAY      ", /*             189 */
	"ARRAYLENGTH    ", /*             190 */
	"ATHROW         ", /*             191 */
	"CHECKCAST      ", /*             192 */
	"INSTANCEOF     ", /*             193 */
	"MONITORENTER   ", /*             194 */
	"MONITOREXIT    ", /*             195 */
	"WIDE           ", /* WIDE        196 */
	"MULTIANEWARRAY ", /*             197 */
	"IFNULL         ", /*             198 */
	"IFNONNULL      ", /*             199 */
	"GOTO_W         ", /* GOTO_W      200 */
	"JSR_W          ", /* JSR_W       201 */
	"BREAKPOINT     ", /* BREAKPOINT  202 */

	                        "UNDEF203", "UNDEF204", "UNDEF205",
	"UNDEF206", "UNDEF207", "UNDEF208", "UNDEF209", "UNDEF210",
	"UNDEF211", "UNDEF212", "UNDEF213", "UNDEF214", "UNDEF215",
	"UNDEF216", "UNDEF217", "UNDEF218", "UNDEF219", "UNDEF220",
	"UNDEF221", "UNDEF222", "UNDEF223", "UNDEF224", "UNDEF225",
	"UNDEF226", "UNDEF227", "UNDEF228", "UNDEF229", "UNDEF230",
	"UNDEF231", "UNDEF232", "UNDEF233", "UNDEF234", "UNDEF235",
	"UNDEF236", "UNDEF237", "UNDEF238", "UNDEF239", "UNDEF240",
	"UNDEF241", "UNDEF242", "UNDEF243", "UNDEF244", "UNDEF245",
	"UNDEF246", "UNDEF247", "UNDEF248", "UNDEF249", "UNDEF250",
	"UNDEF251", "UNDEF252", "UNDEF253", "UNDEF254", "UNDEF255"
};


/* jit_init ********************************************************************

   Initializes the JIT subsystem.

*******************************************************************************/

void jit_init(void)
{
	s4 i;

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
	stackreq[JAVA_IINC]         = 0;
	
	stackreq[JAVA_SWAP] = 2;
	stackreq[JAVA_DUP2] = 2;
	stackreq[JAVA_DUP_X1] = 3;
	stackreq[JAVA_DUP_X2] = 4;
	stackreq[JAVA_DUP2_X1] = 3;
	stackreq[JAVA_DUP2_X2] = 4;

	/* initialize stack analysis subsystem */

	(void) stack_init();

	/* initialize show subsystem */

#if !defined(NDEBUG)
	(void) show_init();
#endif

	/* initialize codegen subsystem */

	codegen_init();
}


/* jit_close *******************************************************************

   Close the JIT subsystem.

*******************************************************************************/

void jit_close(void)
{
	/* do nothing */
}


/* dummy function, used when there is no JavaVM code available                */

static u1 *do_nothing_function(void)
{
	return NULL;
}


/* jit_compile *****************************************************************

   Translates one method to machine code.

*******************************************************************************/

static u1 *jit_compile_intern(jitdata *jd);

u1 *jit_compile(methodinfo *m)
{
	u1      *r;
	jitdata *jd;
	s4       dumpsize;

	STATISTICS(count_jit_calls++);

	/* Initialize the static function's class. */

	/* ATTENTION: This MUST be done before the method lock is aquired,
	   otherwise we could run into a deadlock with <clinit>'s that
	   call static methods of it's own class. */

  	if ((m->flags & ACC_STATIC) && !(m->class->state & CLASS_INITIALIZED)) {
#if !defined(NDEBUG)
		if (initverbose)
			log_message_class("Initialize class ", m->class);
#endif

		if (!initialize_class(m->class))
			return NULL;

		/* check if the method has been compiled during initialization */

		if ((m->code != NULL) && (m->code->entrypoint != NULL))
			return m->code->entrypoint;
	}

#if defined(ENABLE_THREADS)
	/* enter a monitor on the method */

	builtin_monitorenter((java_objectheader *) m);
#endif

	/* if method has been already compiled return immediately */

	if (m->code) {
#if defined(ENABLE_THREADS)
		builtin_monitorexit((java_objectheader *) m);
#endif

		assert(m->code->entrypoint);
		return m->code->entrypoint;
	}

	STATISTICS(count_methods++);

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_start();
#endif

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* allocate jitdata structure and fill it */

	jd = DNEW(jitdata);

	jd->m     = m;
	jd->cd    = DNEW(codegendata);
	jd->rd    = DNEW(registerdata);
#if defined(ENABLE_LOOP)
	jd->ld    = DNEW(loopdata);
#endif
	jd->flags = 0;

	/* Allocate codeinfo memory from the heap as we need to keep them. */

	jd->code  = code_codeinfo_new(m); /* XXX check allocation */

	/* set the flags for the current JIT run */

	if (opt_ifconv)
		jd->flags |= JITDATA_FLAG_IFCONV;

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp)
# endif
		/* initialize the register allocator */

		reg_setup(jd);
#endif

	/* setup the codegendata memory */

	codegen_setup(jd);

	/* now call internal compile function */

	r = jit_compile_intern(jd);

	/* clear pointers to dump memory area */

	m->basicblocks     = NULL;
	m->basicblockindex = NULL;
	m->instructions    = NULL;
	m->stack           = NULL;

	/* release dump area */

	dump_release(dumpsize);

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_stop();
#endif


#if defined(ENABLE_THREADS)
	/* leave the monitor */

	builtin_monitorexit((java_objectheader *) m);
#endif

	if (r) {
		DEBUG_JIT_COMPILEVERBOSE("Running: ");

	} else {
		/* We had an exception! Finish stuff here if necessary. */

		/* Release memory for basic block profiling information. */

		if (opt_prof)
			if (m->bbfrequency)
				MFREE(m->bbfrequency, u4, m->basicblockcount);
	}

	/* return pointer to the methods entry point */

	return r;
}

/* jit_compile_intern **********************************************************

   Static internal function which does the actual compilation.

*******************************************************************************/

static u1 *jit_compile_intern(jitdata *jd)
{
	methodinfo  *m;
	codegendata *cd;
	codeinfo    *code;

#if defined(ENABLE_RT_TIMING)
	struct timespec time_start,time_checks,time_parse,time_stack,
					time_typecheck,time_loop,time_ifconv,time_alloc,
					time_rplpoints,time_codegen;
#endif
	
	RT_TIMING_GET_TIME(time_start);

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	
	/* print log message for compiled method */

	DEBUG_JIT_COMPILEVERBOSE("Compiling: ");

	/* handle native methods and create a native stub */

	if (m->flags & ACC_NATIVE) {
		functionptr f;

#if defined(WITH_STATIC_CLASSPATH)
		f = native_findfunction(m->class->name,	m->name, m->descriptor,
								(m->flags & ACC_STATIC));
		if (f == NULL)
			return NULL;
#else

		f = NULL;
#endif

		code = codegen_createnativestub(f, m);

		assert(!m->code); /* native methods are never recompiled */
		m->code = code;
		
		return code->entrypoint;
	}

	/* if there is no javacode, print error message and return empty method   */

	if (!m->jcode) {
		DEBUG_JIT_COMPILEVERBOSE("No code given for: ");

		code->entrypoint = (u1 *) (ptrint) do_nothing_function;
		m->code = code;

		return code->entrypoint;        /* return empty method                */
	}

	/* initialisation of variables and subsystems */

	m->isleafmethod = true;

#if defined(ENABLE_STATISTICS)
	if (opt_stat) {
		count_tryblocks    += m->exceptiontablelength;
		count_javacodesize += m->jcodelength + 18;
		count_javaexcsize  += m->exceptiontablelength * SIZEOF_VOID_P;
	}
#endif

	RT_TIMING_GET_TIME(time_checks);

	/* call the compiler passes ***********************************************/

	DEBUG_JIT_COMPILEVERBOSE("Parsing: ");

	/* call parse pass */

	if (!parse(jd)) {
		DEBUG_JIT_COMPILEVERBOSE("Exception while parsing: ");

		return NULL;
	}
	RT_TIMING_GET_TIME(time_parse);

	DEBUG_JIT_COMPILEVERBOSE("Parsing done: ");
	DEBUG_JIT_COMPILEVERBOSE("Analysing: ");

	/* call stack analysis pass */

	if (!stack_analyse(jd)) {
		DEBUG_JIT_COMPILEVERBOSE("Exception while analysing: ");

		return NULL;
	}
	RT_TIMING_GET_TIME(time_stack);

	DEBUG_JIT_COMPILEVERBOSE("Analysing done: ");

#ifdef ENABLE_VERIFIER
	if (opt_verify) {
		DEBUG_JIT_COMPILEVERBOSE("Typechecking: ");

		/* call typecheck pass */
		if (!typecheck(jd)) {
			DEBUG_JIT_COMPILEVERBOSE("Exception while typechecking: ");

			return NULL;
		}

		DEBUG_JIT_COMPILEVERBOSE("Typechecking done: ");
	}
#endif
	RT_TIMING_GET_TIME(time_typecheck);

#if defined(ENABLE_LOOP)
	if (opt_loops) {
		depthFirst(jd);
		analyseGraph(jd);
		optimize_loops(jd);
	}
#endif
	RT_TIMING_GET_TIME(time_loop);

#if defined(ENABLE_IFCONV)
	if (jd->flags & JITDATA_FLAG_IFCONV)
		if (!ifconv_static(jd))
			return NULL;
#endif
	RT_TIMING_GET_TIME(time_ifconv);

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp) {
# endif
		DEBUG_JIT_COMPILEVERBOSE("Allocating registers: ");

		/* allocate registers */
# if defined(ENABLE_LSRA)
		if (opt_lsra) {
			if (!lsra(jd))
				return NULL;

			STATISTICS(count_methods_allocated_by_lsra++);

		} else
# endif /* defined(ENABLE_LSRA) */
		{
			STATISTICS(count_locals_conflicts += (cd->maxlocals - 1) * (cd->maxlocals));

			regalloc(jd);
		}

		STATISTICS(reg_make_statistics(jd));

		DEBUG_JIT_COMPILEVERBOSE("Allocating registers done: ");
# if defined(ENABLE_INTRP)
	}
# endif
#endif /* defined(ENABLE_JIT) */
	RT_TIMING_GET_TIME(time_alloc);

	/* Allocate memory for basic block profiling information. This
	   _must_ be done after loop optimization and register allocation,
	   since they can change the basic block count. */

	if (opt_prof)
		m->bbfrequency = MNEW(u4, m->basicblockcount);

	DEBUG_JIT_COMPILEVERBOSE("Generating code: ");

	/* create the replacement points */

	if (!replace_create_replacement_points(jd))
		return NULL;
	RT_TIMING_GET_TIME(time_rplpoints);

	/* now generate the machine code */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp) {
		if (!intrp_codegen(jd)) {
			DEBUG_JIT_COMPILEVERBOSE("Exception while generating code: ");

			return NULL;
		}
	} else
# endif
		{
			if (!codegen(jd)) {
				DEBUG_JIT_COMPILEVERBOSE("Exception while generating code: ");

				return NULL;
			}
		}
#else
	if (!intrp_codegen(jd)) {
		DEBUG_JIT_COMPILEVERBOSE("Exception while generating code: ");

		return NULL;
	}
#endif
	RT_TIMING_GET_TIME(time_codegen);

	DEBUG_JIT_COMPILEVERBOSE("Generating code done: ");

#if !defined(NDEBUG)
	/* intermediate and assembly code listings */
		
	if (opt_showintermediate) {
		show_method(jd);

	} else if (opt_showdisassemble) {
# if defined(ENABLE_DISASSEMBLER)
		DISASSEMBLE(code->entrypoint,
					code->entrypoint + (code->mcodelength - cd->dseglen));
# endif
	}

	if (opt_showddatasegment)
		dseg_display(jd);
#endif

	DEBUG_JIT_COMPILEVERBOSE("Compiling done: ");

	/* switch to the newly generated code */

	assert(code);
	assert(code->entrypoint);

	/* add the current compile version to the methodinfo */

	code->prev = m->code;
	m->code = code;

	RT_TIMING_TIME_DIFF(time_start,time_checks,RT_TIMING_JIT_CHECKS);
	RT_TIMING_TIME_DIFF(time_checks,time_parse,RT_TIMING_JIT_PARSE);
	RT_TIMING_TIME_DIFF(time_parse,time_stack,RT_TIMING_JIT_STACK);
	RT_TIMING_TIME_DIFF(time_stack,time_typecheck,RT_TIMING_JIT_TYPECHECK);
	RT_TIMING_TIME_DIFF(time_typecheck,time_loop,RT_TIMING_JIT_LOOP);
	RT_TIMING_TIME_DIFF(time_loop,time_alloc,RT_TIMING_JIT_ALLOC);
	RT_TIMING_TIME_DIFF(time_alloc,time_rplpoints,RT_TIMING_JIT_RPLPOINTS);
	RT_TIMING_TIME_DIFF(time_rplpoints,time_codegen,RT_TIMING_JIT_CODEGEN);
	RT_TIMING_TIME_DIFF(time_start,time_codegen,RT_TIMING_JIT_TOTAL);

	/* return pointer to the methods entry point */

	return code->entrypoint;
} 


/* jit_asm_compile *************************************************************

   This method is called from asm_vm_call_method and does things like:
   create stackframe info for exceptions, compile the method, patch
   the entrypoint of the method into the calculated address in the JIT
   code, and flushes the instruction cache.

*******************************************************************************/

u1 *jit_asm_compile(methodinfo *m, u1 *mptr, u1 *sp, u1 *ra)
{
	stackframeinfo  sfi;
	u1             *entrypoint;
	u1             *pa;
	ptrint         *p;

	/* create the stackframeinfo (XPC is equal to RA) */

	stacktrace_create_extern_stackframeinfo(&sfi, NULL, sp, ra, ra);

	/* actually compile the method */

	entrypoint = jit_compile(m);

	/* remove the stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	/* there was a problem during compilation */

	if (entrypoint == NULL)
		return NULL;

	/* get the method patch address */

	pa = md_get_method_patch_address(ra, &sfi, mptr);

	/* patch the method entry point */

	p = (ptrint *) pa;

	*p = (ptrint) entrypoint;

	/* flush the instruction cache */

	md_icacheflush(pa, SIZEOF_VOID_P);

	return entrypoint;
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
 * vim:noexpandtab:sw=4:ts=4:
 */
