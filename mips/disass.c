/* disass.c ********************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	A very primitive disassembler for MIPS machine code for easy debugging.

	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1998/11/12

*******************************************************************************/

/*  The disassembler uses four tables for decoding the instructions. The first
	table (ops) is used to classify the instructions based on the op code and
	contains the instruction names for instructions which don't used the
	function codes. This table is indexed by the op code (6 bit, 64 entries).
	The other tables are either indexed by the function code or some special
	format and branch codes.
*/

#define ITYPE_UNDEF  0          /* undefined instructions (illegal opcode)    */
#define ITYPE_JMP    1          /* jump instructions                          */
#define ITYPE_IMM    2          /* immediate instructions                     */
#define ITYPE_MEM    3          /* memory instructions                        */
#define ITYPE_BRA    4          /* branch instructions                        */
#define ITYPE_RIMM   5          /* special/branch instructions                */
#define ITYPE_OP     6          /* integer instructions                       */
#define ITYPE_TRAP   7          /* trap instructions                          */
#define ITYPE_DIVMUL 8          /* integer divide/multiply instructions       */
#define ITYPE_MTOJR  9          /* move to and jump register instructions     */
#define ITYPE_MFROM 10          /* move from instructions                     */
#define ITYPE_SYS   11          /* operating system instructions              */
#define ITYPE_FOP   12          /* floating point instructions                */
#define ITYPE_FOP2  13          /* 2 operand floating point instructions      */
#define ITYPE_FCMP  14          /* floating point compare instructions        */


/* instruction decode table for 6 bit op codes                                */

static struct {char *name; int itype;} ops[] = {

	/* 0x00 */  {"SPECIAL ",    ITYPE_OP},
	/* 0x01 */  {"REGIMM  ",  ITYPE_RIMM},
	/* 0x02 */  {"J       ",   ITYPE_JMP},
	/* 0x03 */  {"JAL     ",   ITYPE_JMP},
	/* 0x04 */  {"BEQ     ",   ITYPE_BRA},
	/* 0x05 */  {"BNE     ",   ITYPE_BRA},
	/* 0x06 */  {"BLEZ    ",   ITYPE_BRA},
	/* 0x07 */  {"BGTZ    ",   ITYPE_BRA},

	/* 0x08 */  {"ADDI    ",   ITYPE_IMM},
	/* 0x09 */  {"ADDIU   ",   ITYPE_IMM},
	/* 0x0a */  {"SLTI    ",   ITYPE_IMM},
	/* 0x0b */  {"SLTIU   ",   ITYPE_IMM},
	/* 0x0c */  {"ANDI    ",   ITYPE_IMM},
	/* 0x0d */  {"ORI     ",   ITYPE_IMM},
	/* 0x0e */  {"XORI    ",   ITYPE_IMM},
	/* 0x0f */  {"LUI     ",   ITYPE_IMM},

	/* 0x10 */  {"COP0    ",    ITYPE_OP},
	/* 0x11 */  {"COP1    ",   ITYPE_FOP},
	/* 0x12 */  {"COP2    ",    ITYPE_OP},
	/* 0x13 */  {"",         ITYPE_UNDEF},
	/* 0x14 */  {"BEQL    ",   ITYPE_BRA},
	/* 0x15 */  {"BNEL    ",   ITYPE_BRA},
	/* 0x16 */  {"BLEZL   ",   ITYPE_BRA},
	/* 0x17 */  {"BGTZL   ",   ITYPE_BRA},

	/* 0x18 */  {"DADDI   ",   ITYPE_IMM},
	/* 0x19 */  {"DADDIU  ",   ITYPE_IMM},
	/* 0x1a */  {"LDL     ",   ITYPE_MEM},
	/* 0x1b */  {"LDR     ",   ITYPE_MEM},
	/* 0x1c */  {"",         ITYPE_UNDEF},
	/* 0x1d */  {"",         ITYPE_UNDEF},
	/* 0x1e */  {"",         ITYPE_UNDEF},
	/* 0x1f */  {"",         ITYPE_UNDEF},

	/* 0x20 */  {"LB      ",   ITYPE_MEM},
	/* 0x21 */  {"LH      ",   ITYPE_MEM},
	/* 0x22 */  {"LWL     ",   ITYPE_MEM},
	/* 0x23 */  {"LW      ",   ITYPE_MEM},
	/* 0x24 */  {"LBU     ",   ITYPE_MEM},
	/* 0x25 */  {"LHU     ",   ITYPE_MEM},
	/* 0x26 */  {"LWR     ",   ITYPE_MEM},
	/* 0x27 */  {"LWU     ",   ITYPE_MEM},

	/* 0x28 */  {"SB      ",   ITYPE_MEM},
	/* 0x29 */  {"SH      ",   ITYPE_MEM},
	/* 0x2a */  {"SWL     ",   ITYPE_MEM},
	/* 0x2b */  {"SW      ",   ITYPE_MEM},
	/* 0x2c */  {"SDL     ",   ITYPE_MEM},
	/* 0x2d */  {"SDR     ",   ITYPE_MEM},
	/* 0x2e */  {"SWR     ",   ITYPE_MEM},
	/* 0x2f */  {"CACHE   ",   ITYPE_MEM},

	/* 0x30 */  {"LL      ",   ITYPE_MEM},
	/* 0x31 */  {"LWC1    ",   ITYPE_MEM},
	/* 0x32 */  {"LWC2    ",   ITYPE_MEM},
	/* 0x33 */  {"",         ITYPE_UNDEF},
	/* 0x34 */  {"LLD     ",   ITYPE_MEM},
	/* 0x35 */  {"LDC1    ",   ITYPE_MEM},
	/* 0x36 */  {"LDC2    ",   ITYPE_MEM},
	/* 0x37 */  {"LD      ",   ITYPE_MEM},

	/* 0x38 */  {"SC      ",   ITYPE_MEM},
	/* 0x39 */  {"SWC1    ",   ITYPE_MEM},
	/* 0x3a */  {"SWC2    ",   ITYPE_MEM},
	/* 0x3b */  {"",         ITYPE_UNDEF},
	/* 0x3c */  {"SLD     ",   ITYPE_MEM},
	/* 0x3d */  {"SDC1    ",   ITYPE_MEM},
	/* 0x3e */  {"SDC2    ",   ITYPE_MEM},
	/* 0x3f */  {"SD      ",   ITYPE_MEM}
};


/* instruction decode table for 6 bit special function codes                  */

static struct {char *name; int ftype;} regops[] = {

	/* 0x00 */  {"SLL     ",   ITYPE_IMM},
	/* 0x01 */  {""        , ITYPE_UNDEF},
	/* 0x02 */  {"SRL     ",   ITYPE_IMM},
	/* 0x03 */  {"SRA     ",   ITYPE_IMM},
	/* 0x04 */  {"SLLV    ",    ITYPE_OP},
	/* 0x05 */  {""        , ITYPE_UNDEF},
	/* 0x06 */  {"SRLV    ",    ITYPE_OP},
	/* 0x07 */  {"SRAV    ",    ITYPE_OP},

	/* 0x08 */  {"JR      ", ITYPE_MTOJR},
	/* 0x09 */  {"JALR    ",   ITYPE_JMP},
	/* 0x0a */  {""        , ITYPE_UNDEF},
	/* 0x0b */  {""        , ITYPE_UNDEF},
	/* 0x0c */  {"SYSCALL ",   ITYPE_SYS},
	/* 0x0d */  {"BREAK   ",   ITYPE_SYS},
	/* 0x0e */  {""        , ITYPE_UNDEF},
	/* 0x0f */  {"SYNC    ",   ITYPE_SYS},

	/* 0x10 */  {"MFHI    ", ITYPE_MFROM},
	/* 0x11 */  {"MTHI    ", ITYPE_MTOJR},
	/* 0x12 */  {"MFLO    ", ITYPE_MFROM},
	/* 0x13 */  {"MTLO    ", ITYPE_MTOJR},
	/* 0x14 */  {"DSLLV   ",    ITYPE_OP},
	/* 0x15 */  {""        , ITYPE_UNDEF},
	/* 0x16 */  {"DSLRV   ",    ITYPE_OP},
	/* 0x17 */  {"DSRAV   ",    ITYPE_OP},

	/* 0x18 */  {"MULT    ",ITYPE_DIVMUL},
	/* 0x19 */  {"MULTU   ",ITYPE_DIVMUL},
	/* 0x1a */  {"DIV     ",ITYPE_DIVMUL},
	/* 0x1b */  {"DIVU    ",ITYPE_DIVMUL},
	/* 0x1c */  {"DMULT   ",ITYPE_DIVMUL},
	/* 0x1d */  {"DMULTU  ",ITYPE_DIVMUL},
	/* 0x1e */  {"DDIV    ",ITYPE_DIVMUL},
	/* 0x1f */  {"DDIVU   ",ITYPE_DIVMUL},

	/* 0x20 */  {"ADD     ",    ITYPE_OP},
	/* 0x21 */  {"ADDU    ",    ITYPE_OP},
	/* 0x22 */  {"SUB     ",    ITYPE_OP},
	/* 0x23 */  {"SUBU    ",    ITYPE_OP},
	/* 0x24 */  {"AND     ",    ITYPE_OP},
	/* 0x25 */  {"OR      ",    ITYPE_OP},
	/* 0x26 */  {"XOR     ",    ITYPE_OP},
	/* 0x27 */  {"NOR     ",    ITYPE_OP},

	/* 0x28 */  {""        , ITYPE_UNDEF},
	/* 0x29 */  {""        , ITYPE_UNDEF},
	/* 0x2a */  {"SLT     ",    ITYPE_OP},
	/* 0x2b */  {"SLTU    ",    ITYPE_OP},
	/* 0x2c */  {"DADD    ",    ITYPE_OP},
	/* 0x2d */  {"DADDU   ",    ITYPE_OP},
	/* 0x2e */  {"DSUB    ",    ITYPE_OP},
	/* 0x2f */  {"DSUBU   ",    ITYPE_OP},

	/* 0x30 */  {"TGE     ",  ITYPE_TRAP},
	/* 0x31 */  {"TGEU    ",  ITYPE_TRAP},
	/* 0x32 */  {"TLT     ",  ITYPE_TRAP},
	/* 0x33 */  {"TLTU    ",  ITYPE_TRAP},
	/* 0x34 */  {"TEQ     ",  ITYPE_TRAP},
	/* 0x35 */  {""        , ITYPE_UNDEF},
	/* 0x36 */  {"TNE     ",  ITYPE_TRAP},
	/* 0x37 */  {""        , ITYPE_UNDEF},

	/* 0x38 */  {"DSLL    ",   ITYPE_IMM},
	/* 0x39 */  {""        , ITYPE_UNDEF},
	/* 0x3a */  {"DSLR    ",   ITYPE_IMM},
	/* 0x3b */  {"DSRA    ",   ITYPE_IMM},
	/* 0x3c */  {"DSLL32  ",   ITYPE_IMM},
	/* 0x3d */  {""        , ITYPE_UNDEF},
	/* 0x3e */  {"DSLR32  ",   ITYPE_IMM},
	/* 0x3f */  {"DSRA32  ",   ITYPE_IMM}
};


/* instruction decode table for 5 bit reg immediate function codes            */

static struct {char *name; int ftype;} regimms[] = {

	/* 0x00 */  {"BLTZ   ",   ITYPE_BRA},
	/* 0x01 */  {"BGEZ   ",   ITYPE_BRA},
	/* 0x02 */  {"BLTZL  ",   ITYPE_BRA},
	/* 0x03 */  {"BGEZL  ",   ITYPE_BRA},
	/* 0x04 */  {"",        ITYPE_UNDEF},
	/* 0x05 */  {"",        ITYPE_UNDEF},
	/* 0x06 */  {"",        ITYPE_UNDEF},
	/* 0x07 */  {"",        ITYPE_UNDEF},

	/* 0x08 */  {"TGEI   ",   ITYPE_IMM},
	/* 0x09 */  {"DGEIU  ",   ITYPE_IMM},
	/* 0x0a */  {"TLTI   ",   ITYPE_IMM},
	/* 0x0b */  {"TLTIU  ",   ITYPE_IMM},
	/* 0x0c */  {"TEQI   ",   ITYPE_IMM},
	/* 0x0d */  {""       , ITYPE_UNDEF},
	/* 0x0e */  {"TNEI   ",   ITYPE_IMM},
	/* 0x0f */  {""       , ITYPE_UNDEF},

	/* 0x10 */  {"BLTZAL ",   ITYPE_BRA},
	/* 0x11 */  {"BGEZAL ",   ITYPE_BRA},
	/* 0x12 */  {"BLTZALL",   ITYPE_BRA},
	/* 0x13 */  {"BGEZALL",   ITYPE_BRA},
	/* 0x14 */  {"",        ITYPE_UNDEF},
	/* 0x15 */  {"",        ITYPE_UNDEF},
	/* 0x16 */  {"",        ITYPE_UNDEF},
	/* 0x17 */  {"",        ITYPE_UNDEF},

	/* 0x18 */  {"",        ITYPE_UNDEF},
	/* 0x19 */  {"",        ITYPE_UNDEF},
	/* 0x1a */  {"",        ITYPE_UNDEF},
	/* 0x1b */  {"",        ITYPE_UNDEF},
	/* 0x1c */  {"",        ITYPE_UNDEF},
	/* 0x1d */  {"",        ITYPE_UNDEF},
	/* 0x1e */  {"",        ITYPE_UNDEF},
	/* 0x1f */  {"",        ITYPE_UNDEF}
};


/* instruction decode table for 6 bit floating point op codes                 */

static struct {char *name; char *fill; int ftype;} fops[] = {

	/* 0x00 */  {"ADD", "   ",   ITYPE_FOP},
	/* 0x01 */  {"SUB", "   ",   ITYPE_FOP},
	/* 0x02 */  {"MUL", "   ",   ITYPE_FOP},
	/* 0x03 */  {"DIV", "   ",   ITYPE_FOP},
	/* 0x04 */  {"SQRT", "  ",   ITYPE_FOP},
	/* 0x05 */  {"ABS", "   ",  ITYPE_FOP2},
	/* 0x06 */  {"MOV", "   ",  ITYPE_FOP2},
	/* 0x07 */  {"NEG", "   ",  ITYPE_FOP2},

	/* 0x08 */  {"ROUNDL", "",  ITYPE_FOP2},
	/* 0x09 */  {"TRUNCL", "",  ITYPE_FOP2},
	/* 0x0a */  {"CEILL", " ",  ITYPE_FOP2},
	/* 0x0b */  {"FLOORL", "",  ITYPE_FOP2},
	/* 0x0c */  {"ROUND", " ",  ITYPE_FOP2},
	/* 0x0d */  {"TRUNC", " ",  ITYPE_FOP2},
	/* 0x0e */  {"CEIL", "  ",  ITYPE_FOP2},
	/* 0x0f */  {"FLOOR", " ",  ITYPE_FOP2},

	/* 0x10 */  {"",       "", ITYPE_UNDEF},
	/* 0x11 */  {"",       "", ITYPE_UNDEF},
	/* 0x12 */  {"",       "", ITYPE_UNDEF},
	/* 0x13 */  {"",       "", ITYPE_UNDEF},
	/* 0x14 */  {"",       "", ITYPE_UNDEF},
	/* 0x15 */  {"",       "", ITYPE_UNDEF},
	/* 0x16 */  {"",       "", ITYPE_UNDEF},
	/* 0x17 */  {"",       "", ITYPE_UNDEF},

	/* 0x18 */  {"",       "", ITYPE_UNDEF},
	/* 0x19 */  {"RECIP", " ",  ITYPE_FOP2},
	/* 0x1a */  {"RSQRT", " ",  ITYPE_FOP2},
	/* 0x1b */  {"",       "", ITYPE_UNDEF},
	/* 0x1c */  {"",       "", ITYPE_UNDEF},
	/* 0x1d */  {"",       "", ITYPE_UNDEF},
	/* 0x1e */  {"",       "", ITYPE_UNDEF},
	/* 0x1f */  {"",       "", ITYPE_UNDEF},

	/* 0x20 */  {"CVTS", "  ",  ITYPE_FOP2},
	/* 0x21 */  {"CVTD", "  ",  ITYPE_FOP2},
	/* 0x22 */  {"CVTX", "  ",  ITYPE_FOP2},
	/* 0x23 */  {"CVTQ", "  ",  ITYPE_FOP2},
	/* 0x24 */  {"CVTW", "  ",  ITYPE_FOP2},
	/* 0x25 */  {"CVTL", "  ",  ITYPE_FOP2},
	/* 0x26 */  {"",       "", ITYPE_UNDEF},
	/* 0x27 */  {"",       "", ITYPE_UNDEF},

	/* 0x28 */  {"",       "", ITYPE_UNDEF},
	/* 0x29 */  {"",       "", ITYPE_UNDEF},
	/* 0x2a */  {"",       "", ITYPE_UNDEF},
	/* 0x2b */  {"",       "", ITYPE_UNDEF},
	/* 0x2c */  {"",       "", ITYPE_UNDEF},
	/* 0x2d */  {"",       "", ITYPE_UNDEF},
	/* 0x2e */  {"",       "", ITYPE_UNDEF},
	/* 0x2f */  {"",       "", ITYPE_UNDEF},

	/* 0x30 */  {"C.F", "   ",  ITYPE_FCMP},
	/* 0x31 */  {"C.UN", "  ",  ITYPE_FCMP},
	/* 0x32 */  {"C.EQ", "  ",  ITYPE_FCMP},
	/* 0x33 */  {"C.UEQ", " ",  ITYPE_FCMP},
	/* 0x34 */  {"C.OLT", " ",  ITYPE_FCMP},
	/* 0x35 */  {"C.ULT", " ",  ITYPE_FCMP},
	/* 0x36 */  {"C.OLE", " ",  ITYPE_FCMP},
	/* 0x37 */  {"C.ULE", " ",  ITYPE_FCMP},

	/* 0x38 */  {"C.SF", "  ",  ITYPE_FCMP},
	/* 0x39 */  {"C.NGLE", "",  ITYPE_FCMP},
	/* 0x3a */  {"C.SEQ", " ",  ITYPE_FCMP},
	/* 0x3b */  {"C.NGL", " ",  ITYPE_FCMP},
	/* 0x3c */  {"C.LT", "  ",  ITYPE_FCMP},
	/* 0x3d */  {"C.NGE", " ",  ITYPE_FCMP},
	/* 0x3e */  {"C.LE", "  ",  ITYPE_FCMP},
	/* 0x3f */  {"C.NGT", " ",  ITYPE_FCMP}
};


/* format decode table for 3 bit floating point format codes                  */

static char *fmt[] = {
	/* 0x00 */  ".S",
	/* 0x01 */  ".D",
	/* 0x02 */  ".X",
	/* 0x03 */  ".Q",
	/* 0x04 */  ".W",
	/* 0x05 */  ".L",
	/* 0x06 */  ".?",
	/* 0x07 */  ".?"
};


/* format decode table for 2 bit floating point branch codes                  */

static char *fbra[] = {
	/* 0x00 */  "BC1F    ",
	/* 0x01 */  "BC1T    ",
	/* 0x02 */  "BC1FL   ",
	/* 0x03 */  "BC1TL   "
};


/* function disassinstr ********************************************************

	outputs a disassembler listing of one machine code instruction on 'stdout'
	c:   instructions machine code
	pos: instructions address relative to method start

*******************************************************************************/

static void disassinstr(int c, int pos)
{
	int op;                     /* 6 bit op code                              */
	int opfun;                  /* 6 bit function code                        */
	int rs, rt, rd;             /* 5 bit integer register specifiers          */
	int fs, ft, fd;             /* 5 bit floating point register specifiers   */
	int shift;                  /* 5 bit unsigned shift amount                */

	op    = (c >> 26) & 0x3f;   /* 6 bit op code                              */
	opfun = (c >>  0) & 0x3f;   /* 6 bit function code                        */
	rs    = (c >> 21) & 0x1f;   /* 5 bit source register specifier            */
	rt    = (c >> 16) & 0x1f;   /* 5 bit source/destination register specifier*/
	rd    = (c >> 11) & 0x1f;   /* 5 bit destination register specifier       */
	shift = (c >>  6) & 0x1f;   /* 5 bit unsigned shift amount                */

	printf ("%6x: %#8x  ", pos, c);
	
	switch (ops[op].itype) {
		case ITYPE_JMP:                      /* 26 bit unsigned jump offset   */
			printf ("%s %#7x\n", ops[op].name, (c & 0x3ffffff) << 2); 
			break;

		case ITYPE_IMM:                      /* 16 bit signed immediate value */
			printf ("%s $%d,$%d,%d\n", ops[op].name, rt, rs, (c << 16) >> 16); 
			break;

		case ITYPE_MEM:                      /* 16 bit signed memory offset   */
			printf ("%s $%d,%d($%d)\n", ops[op].name, rt, (c << 16) >> 16, rs); 
			break;

		case ITYPE_BRA:                      /* 16 bit signed branch offset   */
			printf("%s $%d,$%d,%x\n", ops[op].name, rs, rt, 
			                          pos + 4 + ((c << 16) >> 14));
			break;
			
		case ITYPE_RIMM:
			if (regimms[rt].ftype == ITYPE_IMM)
				printf("%s $%d,#%d\n", regimms[rt].name, rs, (c << 16) >> 16);
			else if (regimms[rt].ftype == ITYPE_BRA)
				printf("%s $%d,%x\n", regimms[rt].name, rs,
				                                   pos + 4 + ((c << 16) >> 14));
			else
				printf("REGIMM %#2x,$%d,%d\n", rt, rs, (c << 16) >> 16);		
			break;

		case ITYPE_OP:
			if (opfun == 0x25 && rs == rt) {
				if (rs == 0 && rd == 0)
					printf("NOP\n");
				else if (rs == 0)
					printf("CLR      $%d\n", rd);
				else
					printf("MOV      $%d,$%d\n", rs, rd);
				return;
				}
			switch (regops[opfun].ftype) {
				case ITYPE_OP:
					printf("%s $%d,$%d,$%d\n", regops[opfun].name, rd, rs, rt);
					break;
				case ITYPE_IMM:  /* immediate instruction */
					printf("%s $%d,$%d,#%d\n",
					       regops[opfun].name, rd, rt, shift);
					break;
				case ITYPE_TRAP:
					printf("%s $%d,$%d,#%d\n",
					        regops[opfun].name, rs, rt, (c << 16) >> 22);
					break;
				case ITYPE_DIVMUL: /* div/mul instruction */
					printf("%s $%d,$%d\n", regops[opfun].name, rs, rt);
					break;
				case ITYPE_JMP:
					printf("%s $%d,$%d\n", regops[opfun].name, rd, rs);
					break;
				case ITYPE_MTOJR:
					printf("%s $%d\n", regops[opfun].name, rs);
					break;
				case ITYPE_MFROM:
					printf("%s $%d\n", regops[opfun].name, rd);
					break;
				case ITYPE_SYS:
					printf("%s\n", regops[opfun].name);
				default:
					printf("SPECIAL  (%#2x) $%d,$%d,$%d\n", opfun, rd, rs, rt);
				}		
			break;
		case ITYPE_FOP:
			fs    = (c >> 11) & 0x1f;   /* 5 bit source register              */
			ft    = (c >> 16) & 0x1f;   /* 5 bit source/destination register  */
			fd    = (c >>  6) & 0x1f;   /* 5 bit destination register         */

			if (rs == 8) {              /* floating point branch              */
				printf("%s %x\n", fbra[ft&3], pos + 4 + ((c << 16) >> 14));
				break;
				}

			if (rs == 1) {              /* double move from                   */
				printf("MFC1     $%d,$f%d\n", rt, fs);		
				break;
				}

			if (rs == 5) {              /* double move to                     */
				printf("MTC1     ,$%d,$f%d\n", rt, fs);		
				break;
				}

			rs    = rs & 7;             /* truncate to 3 bit format specifier */

			if (fops[opfun].ftype == ITYPE_FOP)
				printf("%s%s%s $%d,$%d,$%d\n", fops[opfun].name, fmt[rs],
				                               fops[opfun].fill, fd, fs, ft);
			else if (fops[opfun].ftype == ITYPE_FOP2)
				printf("%s%s%s $%d,$%d\n", fops[opfun].name, fmt[rs],
				                           fops[opfun].fill, fd, fs);
			else if (fops[opfun].ftype == ITYPE_FOP2)
				printf("%s%s%s $%d,$%d\n", fops[opfun].name, fmt[rs],
				                           fops[opfun].fill, fs, ft);
			else
				printf("COP1     (%#2x) $%d,$%d,$%d\n", opfun, fd, fs, ft);		

			break;

		default:
			printf("UNDEF    %#2x(%#2x) $%d,$%d,$%d\n", op, opfun, rd, rs, rt);		
		}
}


/* function disassemble ********************************************************

	outputs a disassembler listing of some machine code on 'stdout'
	code: pointer to first instruction
	len:  code size (number of instructions * 4)

*******************************************************************************/

static void disassemble(int *code, int len)
{
	int p;

	printf ("  --- disassembler listing ---\n");	
	for (p = 0; p < len; p += 4, code++)
		disassinstr(*code, p); 
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
