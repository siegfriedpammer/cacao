/* -*- mode: c; tab-width: 4; c-basic-offset: 4 -*- */
/********************************* disass.c ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	A very primitive disassembler for Alpha machine code for easy debugging.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/10/22

*******************************************************************************/


#define ITYPE_UNDEF 0
#define ITYPE_JMP   1
#define ITYPE_MEM   2
#define ITYPE_BRA   3
#define ITYPE_OP    4
#define ITYPE_FOP   5

static struct { int op; char *name; int itype; } ops[] = {
	{ 0x00, "",      ITYPE_UNDEF},
	{ 0x01, "",      ITYPE_UNDEF},
	{ 0x02, "",      ITYPE_UNDEF},
	{ 0x03, "",      ITYPE_UNDEF},
	{ 0x04, "",      ITYPE_UNDEF},
	{ 0x05, "",      ITYPE_UNDEF},
	{ 0x06, "",      ITYPE_UNDEF},
	{ 0x07, "",      ITYPE_UNDEF},
	{ 0x08, "LDA    ", ITYPE_MEM},
	{ 0x09, "LDAH   ", ITYPE_MEM},
	{ 0x0a, "LDB    ", ITYPE_MEM},
	{ 0x0b, "LDQ_U  ", ITYPE_MEM},
	{ 0x0c, "LDW    ", ITYPE_MEM},
	{ 0x0d, "STW    ", ITYPE_MEM},
	{ 0x0e, "STB    ", ITYPE_MEM},
	{ 0x0f, "STQ_U  ", ITYPE_MEM},
	{ 0x10, "OP     ",  ITYPE_OP},
	{ 0x11, "OP     ",  ITYPE_OP},
	{ 0x12, "OP     ",  ITYPE_OP},
	{ 0x13, "OP     ",  ITYPE_OP},
	{ 0x14, "",      ITYPE_UNDEF},
	{ 0x15, "",      ITYPE_UNDEF},
	{ 0x16, "FOP    ", ITYPE_FOP},
	{ 0x17, "FOP    ", ITYPE_FOP},
	{ 0x18, "MEMFMT ", ITYPE_MEM},
	{ 0x19, "",      ITYPE_UNDEF},
	{ 0x1a, "JMP    ", ITYPE_JMP},
	{ 0x1b, "",      ITYPE_UNDEF},
	{ 0x1c, "OP     ",  ITYPE_OP},
	{ 0x1d, "",      ITYPE_UNDEF},
	{ 0x1e, "",      ITYPE_UNDEF},
	{ 0x1f, "",      ITYPE_UNDEF},
	{ 0x20, "LDF    ", ITYPE_MEM},
	{ 0x21, "LDG    ", ITYPE_MEM},
	{ 0x22, "LDS    ", ITYPE_MEM},
	{ 0x23, "LDT    ", ITYPE_MEM},
	{ 0x24, "STF    ", ITYPE_MEM},
	{ 0x25, "STG    ", ITYPE_MEM},
	{ 0x26, "STS    ", ITYPE_MEM},
	{ 0x27, "STT    ", ITYPE_MEM},
	{ 0x28, "LDL    ", ITYPE_MEM},
	{ 0x29, "LDQ    ", ITYPE_MEM},
	{ 0x2a, "LDL_L  ", ITYPE_MEM},
	{ 0x2b, "LDQ_L  ", ITYPE_MEM},
	{ 0x2c, "STL    ", ITYPE_MEM},
	{ 0x2d, "STQ    ", ITYPE_MEM},
	{ 0x2e, "STL_C  ", ITYPE_MEM},
	{ 0x2f, "STQ_C  ", ITYPE_MEM},
	{ 0x30, "BR     ", ITYPE_BRA},
	{ 0x31, "FBEQ   ", ITYPE_BRA},
	{ 0x32, "FBLT   ", ITYPE_BRA},
	{ 0x33, "FBLE   ", ITYPE_BRA},
	{ 0x34, "BSR    ", ITYPE_BRA},
	{ 0x35, "FBNE   ", ITYPE_BRA},
	{ 0x36, "FBGE   ", ITYPE_BRA},
	{ 0x37, "FBGT   ", ITYPE_BRA},
	{ 0x38, "BLBC   ", ITYPE_BRA},
	{ 0x39, "BEQ    ", ITYPE_BRA},
	{ 0x3a, "BLT    ", ITYPE_BRA},
	{ 0x3b, "BLE    ", ITYPE_BRA},
	{ 0x3c, "BLBS   ", ITYPE_BRA},
	{ 0x3d, "BNE    ", ITYPE_BRA},
	{ 0x3e, "BGE    ", ITYPE_BRA},
	{ 0x3f, "BGT    ", ITYPE_BRA}
};
		
static struct { u2 op,fun; char *name; }  op3s[] = {
	{ 0x10, 0x00,  "ADDL   " },
	{ 0x10, 0x40,  "ADDL/V " },
	{ 0x10, 0x20,  "ADDQ   " },
	{ 0x10, 0x60,  "ADDQ/V " },
	{ 0x10, 0x09,  "SUBL   " },
	{ 0x10, 0x49,  "SUBL/V " },
	{ 0x10, 0x29,  "SUBQ   " },
	{ 0x10, 0x69,  "SUBQ/V " },
	{ 0x10, 0x2D,  "CMPEQ  " },
	{ 0x10, 0x4D,  "CMPLT  " },
	{ 0x10, 0x6D,  "CMPLE  " },
	{ 0x10, 0x1D,  "CMPULT " },
	{ 0x10, 0x3D,  "CMPULE " },
	{ 0x10, 0x0F,  "CMPBGE " },
	{ 0x10, 0x02,  "S4ADDL " },
	{ 0x10, 0x0b,  "S4SUBL " },
	{ 0x10, 0x22,  "S4ADDQ " },
	{ 0x10, 0x2b,  "S4SUBQ " },
	{ 0x10, 0x12,  "S8ADDL " },
	{ 0x10, 0x1b,  "S8SUBL " },
	{ 0x10, 0x32,  "S8ADDQ " },
	{ 0x10, 0x3b,  "S8SUBQ " },
	{ 0x11, 0x00,  "AND    " },
	{ 0x11, 0x20,  "OR     " },
	{ 0x11, 0x40,  "XOR    " },
	{ 0x11, 0x08,  "ANDNOT " },
	{ 0x11, 0x28,  "ORNOT  " },
	{ 0x11, 0x48,  "XORNOT " },
	{ 0x11, 0x24,  "CMOVEQ " },
	{ 0x11, 0x44,  "CMOVLT " },
	{ 0x11, 0x64,  "CMOVLE " },
	{ 0x11, 0x26,  "CMOVNE " },
	{ 0x11, 0x46,  "CMOVGE " },
	{ 0x11, 0x66,  "CMOVGT " },
	{ 0x11, 0x14,  "CMOVLBS" },
	{ 0x11, 0x16,  "CMOVLBC" },
	{ 0x12, 0x39,  "SLL    " },
	{ 0x12, 0x3C,  "SRA    " },
	{ 0x12, 0x34,  "SRL    " },
	{ 0x12, 0x30,  "ZAP    " },
	{ 0x12, 0x31,  "ZAPNOT " },
	{ 0x12, 0x06,  "EXTBL  " },
	{ 0x12, 0x16,  "EXTWL  " },
	{ 0x12, 0x26,  "EXTLL  " },
	{ 0x12, 0x36,  "EXTQL  " },
	{ 0x12, 0x5a,  "EXTWH  " },
	{ 0x12, 0x6a,  "EXTLH  " },
	{ 0x12, 0x7a,  "EXTQH  " },
	{ 0x12, 0x0b,  "INSBL  " },
	{ 0x12, 0x1b,  "INSWL  " },
	{ 0x12, 0x2b,  "INSLL  " },
	{ 0x12, 0x3b,  "INSQL  " },
	{ 0x12, 0x57,  "INSWH  " },
	{ 0x12, 0x67,  "INSLH  " },
	{ 0x12, 0x77,  "INSQH  " },
	{ 0x12, 0x02,  "MSKBL  " },
	{ 0x12, 0x12,  "MSKWL  " },
	{ 0x12, 0x22,  "MSKLL  " },
	{ 0x12, 0x32,  "MSKQL  " },
	{ 0x12, 0x52,  "MSKWH  " },
	{ 0x12, 0x62,  "MSKLH  " },
	{ 0x12, 0x72,  "MSKQH  " },
	{ 0x13, 0x00,  "MULL   " },
	{ 0x13, 0x20,  "MULQ   " },
	{ 0x13, 0x40,  "MULL/V " },
	{ 0x13, 0x60,  "MULQ/V " },
	{ 0x13, 0x30,  "UMULH  " },
	{ 0x16, 0x080, "FADD   " },
	{ 0x16, 0x0a0, "DADD   " },
	{ 0x16, 0x081, "FSUB   " },
	{ 0x16, 0x0a1, "DSUB   " },
	{ 0x16, 0x082, "FMUL   " },
	{ 0x16, 0x0a2, "DMUL   " },
	{ 0x16, 0x083, "FDIV   " },
	{ 0x16, 0x0a3, "DDIV   " },
	{ 0x16, 0x580, "FADDS  " },
	{ 0x16, 0x5a0, "DADDS  " },
	{ 0x16, 0x581, "FSUBS  " },
	{ 0x16, 0x5a1, "DSUBS  " },
	{ 0x16, 0x582, "FMULS  " },
	{ 0x16, 0x5a2, "DMULS  " },
	{ 0x16, 0x583, "FDIVS  " },
	{ 0x16, 0x5a3, "DDIVS  " },
	{ 0x16, 0x0ac, "CVTDF  " },
	{ 0x16, 0x0bc, "CVTLF  " },
	{ 0x16, 0x0be, "CVTLD  " },
	{ 0x16, 0x0af, "CVTDL  " },
	{ 0x16, 0x02f, "CVTDLC " },
	{ 0x16, 0x5ac, "CVTDFS " },
	{ 0x16, 0x5af, "CVTDLS " },
	{ 0x16, 0x52f, "CVTDLCS" },
	{ 0x16, 0x0a4, "FCMPUN " },
	{ 0x16, 0x0a5, "FCMPEQ " },
	{ 0x16, 0x0a6, "FCMPLT " },
	{ 0x16, 0x0a7, "FCMPLE " },
	{ 0x16, 0x5a4, "FCMPUNS" },
	{ 0x16, 0x5a5, "FCMPEQS" },
	{ 0x16, 0x5a6, "FCMPLTS" },
	{ 0x16, 0x5a7, "FCMPLES" },
	{ 0x17, 0x020, "FMOV   " },
	{ 0x17, 0x021, "FMOVN  " },
	{ 0x1c, 0x0,   "BSEXT  " },
	{ 0x1c, 0x1,   "WSEXT  " },
	
	{ 0x00, 0x00,  NULL }
};


static void disasscmd (int c, int pos)
{
	int op, opfun, fopfun, i;
	int ra, rb, rc, lit, disp;

	op     = (c>>26) & 0x3f;
	opfun  = (c>>5)  & 0x7f;
	fopfun = (c>>5)  & 0x7ff;
	ra     = (c>>21) & 0x1f;
	rb     = (c>>16) & 0x1f;
	rc     = (c>>0)  & 0x1f;
	lit    = (c>>13) & 0xff;
	disp   = (c<<16) >> 16;

	printf ("%6x: %8x  ", pos, c);
	
	switch ( ops[op].itype ) {
	case ITYPE_JMP:
		switch ((c>>14) & 3) {
			case 0:
				printf ("JMP     "); 
				break;
			case 1:
				printf ("JSR     "); 
				break;
			case 2:
				printf ("RET     "); 
				break;
			case 3:
				printf ("JSR_CO  "); 
				break;
			}
		printf ("$%d,$%d\n", ra, rb); 
		break;

	case ITYPE_MEM:
		if (op == 0x18 && ra == 0 && ra == 0 && disp == 0)
			printf ("TRAPB\n"); 
		else
			printf ("%s $%d,$%d,%d\n", ops[op].name, ra, rb, disp); 
		break;

	case ITYPE_BRA:
		printf ("%s $%d,%x\n", ops[op].name, ra, pos + 4 + ((c << 11) >> 9));
		break;
		
	case ITYPE_FOP:
		if (op == 0x17 && fopfun == 0x020 && ra == rb) {
			if (ra == 31 && rc == 31)
				printf ("FNOP\n");
			else
				printf ("FMOV    $f%d,$f%d\n", ra, rc);
			return;
			}
		for (i = 0; op3s[i].name; i++)
			if (op3s[i].op == op && op3s[i].fun == fopfun) {
				printf ("%s $f%d,$f%d,$f%d\n", op3s[i].name, ra, rb,  rc);
				return;
				}
		printf ("%s%x $f%d,$f%d,$f%d\n", ops[op].name, fopfun, ra, rb, rc);
		break; 

	case ITYPE_OP:
		if (op == 0x11 && opfun == 0x20 && ra == rb && ~(c&0x1000)) {
			if (ra == 31 && rc == 31)
				printf ("NOP\n");
			else if (ra == 31)
				printf ("CLR     $%d\n", rc);
			else
				printf ("MOV     $%d,$%d\n", ra, rc);
			return;
			}
		for (i = 0; op3s[i].name; i++) {
			if (op3s[i].op == op && op3s[i].fun == opfun) {
				if (c&0x1000)
					printf ("%s $%d,#%d,$%d\n", op3s[i].name, ra, lit, rc);
				else
					printf ("%s $%d,$%d,$%d\n", op3s[i].name, ra, rb,  rc);
				return;
				}
			}
		/* fall through */
	default:
		if (c&0x1000)
			printf ("UNDEF  %x(%x) $%d,#%d,$%d\n", op, opfun, ra, lit, rc);
		else
			printf ("UNDEF  %x(%x) $%d,$%d,$%d\n", op, opfun, ra, rb,  rc);		
	}
}


/*********************** funktion disassemble **********************************

	outputs a disassembler listing of some machine code on 'stdout'

*******************************************************************************/

static void disassemble (int *code, int len)
{
	int p;

	printf ("  --- disassembler listing ---\n");	
	for (p = 0; p < len; p += 4, code++)
		disasscmd (*code, p); 
}
