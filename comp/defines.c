/******************************** comp/defines.c *******************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	defines all the constants and data structures of the compiler 
	
	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Michael Gschwind    EMAIL: cacao@complang.tuwien.ac.at
                  
	Last Change: 1997/09/22

*******************************************************************************/

#include "types.h"

/*********************** resolve typedef-cycles *******************************/

typedef struct stackinfo stackinfo;
typedef struct basicblock basicblock;
typedef struct varinfo varinfo;
typedef struct subroutineinfo subroutineinfo;


/***************************** basic block structure **************************/

#define BLOCKTYPE_JAVA        1
#define BLOCKTYPE_EXCREATOR   2
#define BLOCKTYPE_EXFORWARDER 3

/* there are three kinds of basic blocks:
	JAVA  ........ block which contains JavaVM code (normal case)
	EXCREATOR  ... block which creates an exception and calls the handler
	EXFORWARDER .. block which does the dispatching to all possible handlers
*/

struct basicblock {
	u2  type;           /* block type */
	bool reached;       /* true, when block has been reached; the field stack
	                       contains the stack valid before entering block */
	bool finished;      /* true, if 'pcmdlist' is finished */
	subroutineinfo *subroutine;  /* info about blocks reachable by JSR */
	listnode linkage;	/* list chaining */
	u4 jpc;             /* JavaVM program counter at start of block */
	stackinfo *stack;   /* stack description */
	list pcmdlist;      /* list of pseudo commands */
	u4 mpc;             /* program counter within compiled machine code */
	java_objectheader *exproto;
	                    /* if (type==EXCREATOR) contains pointer to exception
	                       raising object */
	varinfo *exvar;     /* if (type==EXFORWARDER) contains exception variable */
	u4 throwpos;        /* if (type!=JAVA) contains position in program where
	                       the exception was raised */
	};


/***************************** subroutine structure ***************************/

struct subroutineinfo {
	bool returnfinished;     /* true if return allready has been processed */

	stackinfo *returnstack;  /* stack structure at position of return */
	chain *callers;          /* a list of all callers */
	u2 counter;
	};


/************************** stack element structure ***************************/

struct stackinfo {
	stackinfo *prev; /* pointer to next element towards bootom */
	varinfo *var;    /* pointer to variable which contains this value */
	};


/************************* pseudo variable structure **************************/

struct varinfo {
	listnode linkage;      /* list chaining */

	u2 type;               /* basic type of variable */
	u2 number;             /* sequential numbering (used for debugging) */

	bool saved;            /* true if variable sould survive subroutine calls */

	struct reginfo *reg;   /* registerused by variable */

	/* temporary fields used during parsing */

	list copies;           /* list of all variables which are copies */
	listnode copylink;     /* chaining for copy list */
	varinfo *original;     /* pointer to original variable (self reference
	                          is possible) */

	bool globalscope;      /* true if variable is activ in whole subroutine */
	bool active;           /* true if variable is currently active (parsing
	                          a block where the variable is valid ) */
	};


typedef varinfo *varid;

#define NOVAR NULL


/************************** pseudo command structure **************************/

/* pseudo command tags */

#define TAG_LOADCONST_I  1      /* load integer constant */
#define TAG_LOADCONST_L  2      /* load long constant    */
#define TAG_LOADCONST_F  3      /* load float constant   */
#define TAG_LOADCONST_D  4      /* load double constant  */
#define TAG_LOADCONST_A  5      /* load address constant */
#define TAG_MOVE         6
#define TAG_OP           7
#define TAG_MEM          8
#define TAG_BRA          9
#define TAG_TABLEJUMP   10
#define TAG_METHOD      11
#define TAG_DROP        12
#define TAG_ACTIVATE    13


typedef struct pcmd {
	listnode linkage;           /* list chaining */
	int tag;                    /* kind of pseudo command */
	int opcode;                 /* opcode and kind of pseudo command */

	varinfo *dest;              /* optional destination operand */
	varinfo *source1;           /* 3 optional source operands */
	varinfo *source2;
	varinfo *source3;

	union {
		struct {                /* load integer constant */
			s4 value;
			} i;
		struct {                /* load long constant    */
			s8 value;
			} l;
		struct {                /* load float constant   */
			float value;
			} f;
		struct {                /* load double constant  */
			double value;
			} d;
		struct {                /* load address constant */
			voidptr value;
			} a;

		struct {                /* move */
			u2 type;                /* type of value */
			} move;

		struct {                /* memory access (load/store) */
			int type;               /* access type */
			u4  offset;             /* offset relative to base register */
			} mem;

     	struct {                /* branch */
			basicblock *target;     /* branch target */
			} bra;

		struct {                /* branch using table */
			u4 targetcount;         /* number of entries */
			basicblock **targets;   /* branch target */
			} tablejump;

		struct {                /* method call */
			methodinfo *method;     /* pointer to 'methodinfo'-structure */
			functionptr builtin;    /* C function pointer or NULL */
			u2 paramnum;            /* number of parameters */
			varid *params;          /* table of parameter variables */
			varid exceptionvar;     /* exception variable */
			} method;

		} u;

	} pcmd;






/***************** forward references in branch instructions ******************/

typedef struct mcodereference {
	listnode linkage;       /* list chaining */

	bool incode;            /* true if code address, false if data address */
	s4 msourcepos;          /* patching position in code/data segment */
	basicblock *target;     /* target basic block */
	} mcodereference;




/********** JavaVM operation codes (sortet) and instruction lengths ***********/

u1 jcommandsize[256] = {

#define CMD_NOP				0
		1,
#define CMD_ACONST_NULL		1
		1,
#define CMD_ICONST_M1		2
		1,
#define CMD_ICONST_0 		3
		1,
#define CMD_ICONST_1 		4
		1,
#define CMD_ICONST_2 		5
		1,
#define CMD_ICONST_3 		6
		1,
#define CMD_ICONST_4 		7
		1,
#define CMD_ICONST_5 		8
		1,
#define CMD_LCONST_0 		9
		1,
#define CMD_LCONST_1 		10
		1,
#define CMD_FCONST_0 		11
		1,
#define CMD_FCONST_1 		12
		1,
#define CMD_FCONST_2 		13
		1,
#define CMD_DCONST_0 		14
		1,
#define CMD_DCONST_1 		15
		1,
#define CMD_BIPUSH 			16
		2,
#define CMD_SIPUSH 			17
		3,
#define CMD_LDC1 			18
		2,
#define CMD_LDC2 			19
		3,
#define CMD_LDC2W 			20
		3,
#define CMD_ILOAD 			21
		2,
#define CMD_LLOAD 			22
		2,
#define CMD_FLOAD 			23
		2,
#define CMD_DLOAD 			24
		2,
#define CMD_ALOAD 			25
		2,
#define CMD_ILOAD_0 		26
		1,
#define CMD_ILOAD_1 		27
		1,
#define CMD_ILOAD_2 		28
		1,
#define CMD_ILOAD_3 		29
		1,
#define CMD_LLOAD_0 		30
		1,
#define CMD_LLOAD_1 		31
		1,
#define CMD_LLOAD_2 		32
		1,
#define CMD_LLOAD_3 		33
		1,
#define CMD_FLOAD_0 		34
		1,
#define CMD_FLOAD_1 		35
		1,
#define CMD_FLOAD_2 		36
		1,
#define CMD_FLOAD_3 		37
		1,
#define CMD_DLOAD_0 		38
		1,
#define CMD_DLOAD_1 		39
		1,
#define CMD_DLOAD_2 		40
		1,
#define CMD_DLOAD_3 		41
		1,
#define CMD_ALOAD_0 		42
		1,
#define CMD_ALOAD_1 		43
		1,
#define CMD_ALOAD_2 		44
		1,
#define CMD_ALOAD_3 		45
		1,
#define CMD_IALOAD 			46
		1,
#define CMD_LALOAD			47
		1,
#define CMD_FALOAD 			48
		1,
#define CMD_DALOAD 			49
		1,
#define CMD_AALOAD 			50
		1,
#define CMD_BALOAD 			51
		1,
#define CMD_CALOAD 			52
		1,
#define CMD_SALOAD 			53
		1,
#define CMD_ISTORE 			54
		2,
#define CMD_LSTORE 			55
		2,
#define CMD_FSTORE 			56
		2,
#define CMD_DSTORE 			57
		2,
#define CMD_ASTORE 			58
		2,
#define CMD_ISTORE_0 		59
		1,
#define CMD_ISTORE_1 		60
		1,
#define CMD_ISTORE_2 		61
		1,
#define CMD_ISTORE_3 		62
		1,
#define CMD_LSTORE_0 		63
		1,
#define CMD_LSTORE_1 		64
		1,
#define CMD_LSTORE_2 		65
		1,
#define CMD_LSTORE_3 		66
		1,
#define CMD_FSTORE_0 		67
		1,
#define CMD_FSTORE_1 		68
		1,
#define CMD_FSTORE_2 		69
		1,
#define CMD_FSTORE_3 		70
		1,
#define CMD_DSTORE_0 		71
		1,
#define CMD_DSTORE_1 		72
		1,
#define CMD_DSTORE_2 		73
		1,
#define CMD_DSTORE_3 		74
		1,
#define CMD_ASTORE_0 		75
		1,
#define CMD_ASTORE_1 		76
		1,
#define CMD_ASTORE_2 		77
		1,
#define CMD_ASTORE_3 		78
		1,
#define CMD_IASTORE 		79
		1,
#define CMD_LASTORE 		80
		1,
#define CMD_FASTORE 		81
		1,
#define CMD_DASTORE 		82
		1,
#define CMD_AASTORE 		83
		1,
#define CMD_BASTORE		    84
		1,
#define CMD_CASTORE 		85
		1,
#define CMD_SASTORE 		86
		1,
#define CMD_POP 		    87
		1,
#define CMD_POP2 		    88
		1,
#define CMD_DUP 		    89
		1,
#define CMD_DUP_X1 		    90
		1,
#define CMD_DUP_X2 		    91
		1,
#define CMD_DUP2 		    92
		1,
#define CMD_DUP2_X1 		93
		1,
#define CMD_DUP2_X2 		94
		1,
#define CMD_SWAP 			95
		1,
#define CMD_IADD 			96
		1,
#define CMD_LADD 			97
		1,
#define CMD_FADD 			98
		1,
#define CMD_DADD 			99
		1,
#define CMD_ISUB 			100
		1,
#define CMD_LSUB 			101
		1,
#define CMD_FSUB 			102
		1,
#define CMD_DSUB 			103
		1,
#define CMD_IMUL 			104
		1,
#define CMD_LMUL 			105
		1,
#define CMD_FMUL 			106
		1,
#define CMD_DMUL 			107
		1,
#define CMD_IDIV 			108
		1,
#define CMD_LDIV 			109
		1,
#define CMD_FDIV 			110
		1,
#define CMD_DDIV 			111
		1,
#define CMD_IREM 			112
		1,
#define CMD_LREM 			113
		1,
#define CMD_FREM 			114
		1,
#define CMD_DREM 			115
		1,
#define CMD_INEG 			116
		1,
#define CMD_LNEG 			117
		1,
#define CMD_FNEG 			118
		1,
#define CMD_DNEG 			119
		1,
#define CMD_ISHL 			120
		1,
#define CMD_LSHL 			121
		1,
#define CMD_ISHR 			122
		1,
#define CMD_LSHR 			123
		1,
#define CMD_IUSHR 			124
		1,
#define CMD_LUSHR 			125
		1,
#define CMD_IAND 			126
		1,
#define CMD_LAND 			127
		1,
#define CMD_IOR 			128
		1,
#define CMD_LOR 			129
		1,
#define CMD_IXOR 			130
		1,
#define CMD_LXOR 			131
		1,
#define CMD_IINC 			132
		3,
#define CMD_I2L 			133
		1,
#define CMD_I2F 			134
		1,
#define CMD_I2D 			135
		1,
#define CMD_L2I 			136
		1,
#define CMD_L2F 			137
		1,
#define CMD_L2D 			138
		1,
#define CMD_F2I 			139
		1,
#define CMD_F2L 			140
		1,
#define CMD_F2D 		    141
		1,
#define CMD_D2I 		    142
		1,
#define CMD_D2L 		    143
		1,
#define CMD_D2F 		    144
		1,
#define CMD_INT2BYTE 		145
		1,
#define CMD_INT2CHAR 		146
		1,
#define CMD_INT2SHORT 		147
		1,
#define CMD_LCMP 		    148
		1,
#define CMD_FCMPL 		    149
		1,
#define CMD_FCMPG 		    150
		1,
#define CMD_DCMPL 		    151
		1,
#define CMD_DCMPG 		    152
		1,
#define CMD_IFEQ   		    153
		3,
#define CMD_IFNE 		    154
		3,
#define CMD_IFLT 		    155
		3,
#define CMD_IFGE 		    156
		3,
#define CMD_IFGT 		    157
		3,
#define CMD_IFLE 		    158
		3,
#define CMD_IF_ICMPEQ 		159
		3,
#define CMD_IF_ICMPNE 		160
		3,
#define CMD_IF_ICMPLT 		161
		3,
#define CMD_IF_ICMPGE 		162
		3,
#define CMD_IF_ICMPGT 		163
		3,
#define CMD_IF_ICMPLE 		164
		3,
#define CMD_IF_ACMPEQ 		165
		3,
#define CMD_IF_ACMPNE 		166
		3,
#define CMD_GOTO 		    167
		3,
#define CMD_JSR 		    168
		3,
#define CMD_RET  		    169
		2,
#define CMD_TABLESWITCH     170
		0,  /* length must be computed */
#define CMD_LOOKUPSWITCH    171
		0,  /* length must be computed */
#define CMD_IRETURN 		172
		1,
#define CMD_LRETURN 		173
		1,
#define CMD_FRETURN 		174
		1,
#define CMD_DRETURN 		175
		1,
#define CMD_ARETURN 		176
		1,
#define CMD_RETURN 		    177
		1,
#define CMD_GETSTATIC       178
		3,
#define CMD_PUTSTATIC       179
		3,
#define CMD_GETFIELD        180
		3,
#define CMD_PUTFIELD        181
		3,
#define CMD_INVOKEVIRTUAL   182
		3,
#define CMD_INVOKESPECIAL   183
		3,
#define CMD_INVOKESTATIC 	184
		3,
#define CMD_INVOKEINTERFACE 185
		5,
		1, /* unused */
#define CMD_NEW	            187
		3,
#define CMD_NEWARRAY 		188
		2,
#define CMD_ANEWARRAY 		189
		3,
#define CMD_ARRAYLENGTH 	190
		1,
#define CMD_ATHROW    		191
		1,
#define CMD_CHECKCAST		192
		3,
#define CMD_INSTANCEOF		193
		3,
#define CMD_MONITORENTER	194
		1,
#define CMD_MONITOREXIT		195
		1,
#define CMD_WIDE            196
		0,       /* length must be computed */
#define CMD_MULTIANEWARRAY	197
		4,
#define CMD_IFNULL          198
		3,
#define CMD_IFNONNULL 	    199
		3,
#define CMD_GOTO_W          200
		5,
#define CMD_JSR_W 	        201
		5,
#define CMD_BREAKPOINT      202
		1,

		1,1,1,1,1,1,1,1,1,1,		/* unused */
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,
		1,1,1
	};

#define CMD_TRACEBUILT      253     /* internal opcode */
#define CMD_IFEQL           254     /* internal opcode */
#define CMD_IF_UCMPGE       255     /* internal opcode */

#define CMD_LOADCONST_I (CMD_IF_UCMPGE+TAG_LOADCONST_I)   /* internal opcodes */
#define CMD_LOADCONST_L (CMD_IF_UCMPGE+TAG_LOADCONST_L)
#define CMD_LOADCONST_F (CMD_IF_UCMPGE+TAG_LOADCONST_F)
#define CMD_LOADCONST_D (CMD_IF_UCMPGE+TAG_LOADCONST_D)
#define CMD_LOADCONST_A (CMD_IF_UCMPGE+TAG_LOADCONST_A)
#define CMD_MOVE        (CMD_IF_UCMPGE+TAG_MOVE)

#define CMD_TABLEJUMP   (CMD_IF_UCMPGE+TAG_TABLEJUMP)
#define CMD_BUILTIN     (CMD_IF_UCMPGE+TAG_METHOD)
#define CMD_DROP        (CMD_IF_UCMPGE+TAG_DROP)
#define CMD_ACTIVATE    (CMD_IF_UCMPGE+TAG_ACTIVATE)


/******************* description of JavaVM instructions ***********************/

typedef struct {
	u1 opcode;
	u1 type_s1;
	u1 type_s2;
	u1 type_d;	
	functionptr builtin;
	bool supported;
	bool isfloat;
} stdopdescriptor;

stdopdescriptor *stdopdescriptors[256];

stdopdescriptor stdopdescriptortable[] = {
	{ CMD_IADD,   TYPE_INT, TYPE_INT, TYPE_INT, NULL, true, false },
	{ CMD_ISUB,   TYPE_INT, TYPE_INT, TYPE_INT, NULL, true, false },
	{ CMD_IMUL,   TYPE_INT, TYPE_INT, TYPE_INT, NULL, true, false },
	{ CMD_ISHL,   TYPE_INT, TYPE_INT, TYPE_INT, NULL, true, false },
	{ CMD_ISHR,   TYPE_INT, TYPE_INT, TYPE_INT, NULL, true, false },
	{ CMD_IUSHR,  TYPE_INT, TYPE_INT, TYPE_INT, NULL, true, false },
	{ CMD_IAND,   TYPE_INT, TYPE_INT, TYPE_INT, NULL, true, false },
	{ CMD_IOR,    TYPE_INT, TYPE_INT, TYPE_INT, NULL, true, false },
	{ CMD_IXOR,   TYPE_INT, TYPE_INT, TYPE_INT, NULL, true, false },
	{ CMD_INEG,   TYPE_INT, TYPE_VOID,TYPE_INT, NULL, true, false },

	{ CMD_LADD,   TYPE_LONG, TYPE_LONG, TYPE_LONG, 
	       (functionptr) builtin_ladd , SUPPORT_LONG && SUPPORT_LONG_ADD, false },
	{ CMD_LSUB,   TYPE_LONG, TYPE_LONG, TYPE_LONG,
	       (functionptr) builtin_lsub , SUPPORT_LONG && SUPPORT_LONG_ADD, false },
	{ CMD_LMUL,   TYPE_LONG, TYPE_LONG, TYPE_LONG,
	       (functionptr) builtin_lmul , SUPPORT_LONG && SUPPORT_LONG_MULDIV, false },
	{ CMD_LSHL,   TYPE_LONG, TYPE_INT,  TYPE_LONG,
	       (functionptr) builtin_lshl , SUPPORT_LONG && SUPPORT_LONG_SHIFT, false },
	{ CMD_LSHR,   TYPE_LONG, TYPE_INT,  TYPE_LONG,
	       (functionptr) builtin_lshr, SUPPORT_LONG && SUPPORT_LONG_SHIFT, false },
	{ CMD_LUSHR,  TYPE_LONG, TYPE_INT,  TYPE_LONG,
	       (functionptr) builtin_lushr, SUPPORT_LONG && SUPPORT_LONG_SHIFT, false },
	{ CMD_LAND,   TYPE_LONG, TYPE_LONG, TYPE_LONG,
	       (functionptr) builtin_land, SUPPORT_LONG && SUPPORT_LONG_LOG, false },
	{ CMD_LOR,    TYPE_LONG, TYPE_LONG, TYPE_LONG,
	       (functionptr) builtin_lor , SUPPORT_LONG && SUPPORT_LONG_LOG, false },
	{ CMD_LXOR,   TYPE_LONG, TYPE_LONG, TYPE_LONG,
	       (functionptr) builtin_lxor, SUPPORT_LONG && SUPPORT_LONG_LOG, false },
	{ CMD_LNEG,   TYPE_LONG, TYPE_VOID, TYPE_LONG,
	       (functionptr) builtin_lneg, SUPPORT_LONG && SUPPORT_LONG_ADD, false },
	{ CMD_LCMP,   TYPE_LONG, TYPE_LONG, TYPE_INT,
	       (functionptr) builtin_lcmp, SUPPORT_LONG && SUPPORT_LONG_CMP, false },

	{ CMD_FADD,   TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, 
	       (functionptr) builtin_fadd, SUPPORT_FLOAT, true },
	{ CMD_FSUB,   TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, 
	       (functionptr) builtin_fsub, SUPPORT_FLOAT, true },
	{ CMD_FMUL,   TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, 
	       (functionptr) builtin_fmul, SUPPORT_FLOAT, true },
	{ CMD_FDIV,   TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, 
	       (functionptr) builtin_fdiv, SUPPORT_FLOAT, true },
	{ CMD_FREM,   TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, 
	       (functionptr) builtin_frem, SUPPORT_FLOAT, true },
	{ CMD_FNEG,   TYPE_FLOAT, TYPE_VOID,  TYPE_FLOAT, 
	       (functionptr) builtin_fneg, SUPPORT_FLOAT, true },
 	{ CMD_FCMPL,  TYPE_FLOAT, TYPE_FLOAT, TYPE_INT,   
 	       (functionptr) builtin_fcmpl, SUPPORT_FLOAT, true },
	{ CMD_FCMPG,  TYPE_FLOAT, TYPE_FLOAT, TYPE_INT,   
	       (functionptr) builtin_fcmpg, SUPPORT_FLOAT, true },

	{ CMD_DADD,   TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, 
	       (functionptr) builtin_dadd, SUPPORT_DOUBLE, true },
	{ CMD_DSUB,   TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, 
	       (functionptr) builtin_dsub, SUPPORT_DOUBLE, true },
	{ CMD_DMUL,   TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, 
	       (functionptr) builtin_dmul, SUPPORT_DOUBLE, true },
	{ CMD_DDIV,   TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, 
	       (functionptr) builtin_ddiv, SUPPORT_DOUBLE, true },
	{ CMD_DREM,   TYPE_DOUBLE, TYPE_DOUBLE, TYPE_DOUBLE, 
	       (functionptr) builtin_drem, SUPPORT_DOUBLE, true },
	{ CMD_DNEG,   TYPE_DOUBLE, TYPE_VOID,  TYPE_DOUBLE, 
	       (functionptr) builtin_dneg, SUPPORT_DOUBLE, true },
	{ CMD_DCMPL,  TYPE_DOUBLE, TYPE_DOUBLE, TYPE_INT, 
	       (functionptr) builtin_dcmpl, SUPPORT_DOUBLE, true },
	{ CMD_DCMPG,  TYPE_DOUBLE, TYPE_DOUBLE, TYPE_INT, 
	       (functionptr) builtin_dcmpg, SUPPORT_DOUBLE, true },

	{ CMD_INT2BYTE, TYPE_INT, TYPE_VOID, TYPE_INT, NULL, true,false },
	{ CMD_INT2CHAR, TYPE_INT, TYPE_VOID, TYPE_INT, NULL, true,false },
	{ CMD_INT2SHORT, TYPE_INT, TYPE_VOID, TYPE_INT, NULL, true,false },
	{ CMD_I2L,    TYPE_INT,  TYPE_VOID, TYPE_LONG,   
           (functionptr) builtin_i2l, SUPPORT_LONG && SUPPORT_LONG_ICVT, false },
	{ CMD_I2F,    TYPE_INT,  TYPE_VOID, TYPE_FLOAT,  
	       (functionptr) builtin_i2f, SUPPORT_FLOAT, true },
	{ CMD_I2D,    TYPE_INT,  TYPE_VOID, TYPE_DOUBLE, 
	       (functionptr) builtin_i2d, SUPPORT_DOUBLE, true },
	{ CMD_L2I,    TYPE_LONG, TYPE_VOID, TYPE_INT,    
	       (functionptr) builtin_l2i, SUPPORT_LONG && SUPPORT_LONG_ICVT, false },
	{ CMD_L2F,    TYPE_LONG, TYPE_VOID, TYPE_FLOAT,  
	       (functionptr) builtin_l2f, SUPPORT_LONG && SUPPORT_FLOAT && SUPPORT_LONG_FCVT, true },
	{ CMD_L2D,    TYPE_LONG, TYPE_VOID, TYPE_DOUBLE, 
	       (functionptr) builtin_l2d, SUPPORT_LONG && SUPPORT_DOUBLE && SUPPORT_LONG_FCVT, true },
	{ CMD_F2I,    TYPE_FLOAT, TYPE_VOID, TYPE_INT,   
	       (functionptr) builtin_f2i, SUPPORT_FLOAT, true },
	{ CMD_F2L,    TYPE_FLOAT, TYPE_VOID, TYPE_LONG,   
	       (functionptr) builtin_f2l, SUPPORT_FLOAT && SUPPORT_LONG && SUPPORT_LONG_FCVT, true },
	{ CMD_F2D,    TYPE_FLOAT, TYPE_VOID, TYPE_DOUBLE, 
	       (functionptr) builtin_f2d, SUPPORT_FLOAT && SUPPORT_DOUBLE, true },
	{ CMD_D2I,    TYPE_DOUBLE, TYPE_VOID, TYPE_INT,   
	       (functionptr) builtin_d2i, SUPPORT_DOUBLE, true },
	{ CMD_D2L,    TYPE_DOUBLE, TYPE_VOID, TYPE_LONG,   
	       (functionptr) builtin_d2l, SUPPORT_DOUBLE && SUPPORT_LONG && SUPPORT_LONG_FCVT, true },
	{ CMD_D2F,    TYPE_DOUBLE, TYPE_VOID, TYPE_FLOAT, 
	       (functionptr) builtin_d2f, SUPPORT_DOUBLE && SUPPORT_FLOAT, true },
	
};



/***************************** register types *********************************/

#define REG_RES   0         /* reserved register for OS or code generator */
#define REG_RET   1         /* return value register */
#define REG_EXC   2         /* exception value register */
#define REG_SAV   3         /* (callee) saved register */
#define REG_TMP   4         /* scratch temporary register (caller saved) */
#define REG_ARG   5         /* argument register (caller saved) */

#define REG_END   -1        /* last entry in tables */
 
#define PARAMMODE_NUMBERED  0 
#define PARAMMODE_STUFFED   1

/***************************** register info block ****************************/

extern int regdescint[];    /* description of integer registers */
extern int regdescfloat[];  /* description of floating point registers */

extern int reg_parammode;

