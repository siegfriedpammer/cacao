/* disass.c ********************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	A very primitive disassembler for Alpha machine code for easy debugging.

	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1998/11/18

*******************************************************************************/

/*  #include "i386.h" */


#define GETIMM32(dst) \
    do { \
        ((s4) code)++; \
        (dst) = *(code); \
        ((s4) code) += 3; \
        pos += 4; \
    } while (0)


#define GETIMM8(dst) \
    do { \
        ((s4) code)++; \
        (dst) = (u1) *(code); \
        pos++; \
    } while (0)


/* name table for 8 integer registers */

static char *regs[] = {
	"eax",
	"ecx",
	"edx",
	"ebx",
	"esp",
	"ebp",
	"esi",
	"edi"
};


/* name table for x86 conditional jumps */

static char *i386_jcc[] = {
	"o ",
	"no",
	"b ",
	"ae",
	"e ",
	"ne",
	"be",
	"a ",
	"s ",
	"ns",
	"p ",
	"np",
	"l ",
	"ge",
	"le",
	"g "
};


/* name table for x86 alu ops */

static char *i386_alu[] = {
	"add",
	"or ",
	"adc",
	"sbb",
	"and",
	"sub",
	"xor",
	"cmp"
};


/* name table for x86 shift ops */

static char *i386_sxx[] = {
	"",
	"",
	"",
	"",
	"shl",
	"shr",
	"",
	"sar"
};


/* function disassinstr ********************************************************

	outputs a disassembler listing of one machine code instruction on 'stdout'
	c:   instructions machine code
	pos: instructions address relative to method start

*******************************************************************************/


/*
 * quick hack
 */
typedef union {
    s4 val;
    u1 b[4];
} i386_imm_buf2;


static s4 *codestatic = 0;
static int pstatic = 0;


static void i386_decode_modrm(s4 *code, int *pos, u1 *mod, u1 *reg, u1 *rm)
{
	u1 modrm;
	modrm = (u1) *(code);
	*pos += 1;
	printf(" %02x", modrm);
	
    *mod = modrm >> 6;
    *reg = (modrm >> 3) & 0x7;
    *rm = modrm & 0x7;
}


static void i386_decode_sib(s4 *code, int *pos, u1 *ss, u1 *index, u1 *base)
{
	u1 sib;
	sib = (u1) *(code);
	*pos += 1;
	printf(" %02x", sib);
	
    *ss = sib >> 6;
    *index = (sib >> 3) & 0x7;
    *base = sib & 0x7;
}


static void disassinstr(int c, int pos)
{
	s4 *code = (s4 *) c;

	u1 opcode = (u1) *code;
	u1 opcode2 = 0;
	
	u1 modrm = 0;
	u1 mod = 0;
	u1 reg = 0;
	u1 rm = 0;

	u1 sib = 0;
	u1 ss = 0;
	u1 index = 0;
	u1 base = 0;

	u1 func = 0;
		
	i386_imm_buf2 imb;
	i386_imm_buf2 imb2;

	imb.val = 0;
	imb2.val = 0;
	
	printf("%6x:   %02x", pos, opcode);
	
	switch (opcode) {
                /* alu  r32,r/m32 */
	case 0x01:	/* add  r32,r/m32 (01 /r) */
	case 0x09:	/* or   r32,r/m32 (0b /r) */
	case 0x11:	/* adc  r32,r/m32 (11 /r) */
	case 0x19:	/* sbb  r32,r/m32 (0b /r) */
	case 0x21:  /* and  r32,r/m32 (21 /r) */
	case 0x29:	/* sub  r32,r/m32 (29 /r) */
	case 0x31:	/* xor  r32,r/m32 (31 /r) */
	case 0x39:	/* cmp  r32,r/m32 (39 /r) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

		func = (u1) (opcode >> 3);

		switch (mod) {
		case 0: /* disp == 0 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				printf("\t\t%s    %%%s,(%%%s,1)\n", i386_alu[func], regs[reg], regs[base]);
				break;

			default:
				printf("\t\t%s    %%%s,(%%%s)\n", i386_alu[func], regs[reg], regs[rm]);
			}
			break;

		case 1: /* disp == 8 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\t%s    %%%s,0x%x(%%%s,1)\n", i386_alu[func], regs[reg], (s1) imb.b[0], regs[base]);
				break;

			default:
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\t%s    %%%s,0x%x(%%%s)\n", i386_alu[func], regs[reg], (s1) imb.b[0], regs[rm]);
			}
			break;

		case 2: /* disp == 32 */
			GETIMM32(imb.val);
			printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
			printf("\t%s    %%%s,0x%x(%%%s)\n", i386_alu[func], regs[reg], imb.val, regs[rm]);
			break;

		case 3:
			printf("\t\t\t%s    %%%s,%%%s\n", i386_alu[func], regs[reg], regs[rm]);
			break;
		}
		break;

		
                /* alu  r/m32,r32 */
	case 0x03:	/* add  r/m32,r32 (03 /r) */
	case 0x0b:	/* or   r/m32,r32 (0b /r) */
	case 0x13:	/* adc  r/m32,r32 (13 /r) */
	case 0x1b:	/* sbb  r/m32,r32 (1b /r) */
	case 0x23:	/* and  r/m32,r32 (23 /r) */
	case 0x2b:	/* sub  r/m32,r32 (2b /r) */
	case 0x33:	/* xor  r/m32,r32 (33 /r) */
	case 0x3b:	/* cmp  r/m32,r32 (3b /r) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

		func = (u1) (opcode >> 3);

		switch (mod) {
		case 0: /* disp == 0 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				printf("\t\t%s    (%%%s,1),%%%s\n", i386_alu[func], regs[base], regs[reg]);
				break;

			default:
				printf("\t\t%s    (%%%s),%%%s\n", i386_alu[func], regs[rm], regs[reg]);
			}
			break;

		case 1: /* disp == 8 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\t%s    0x%x(%%%s,1),%%%s\n", i386_alu[func], (s1) imb.b[0], regs[rm], regs[reg]);
				break;

			default:
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\t%s    0x%x(%%%s),%%%s\n", i386_alu[func], (s1) imb.b[0], regs[rm], regs[reg]);
			}
			break;

		case 2: /* disp == 32 */
			GETIMM32(imb.val);
			printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
			printf("\t%s    0x%x(%%%s),%%%s\n", i386_alu[func], imb.val, regs[rm], regs[reg]);
			break;

		case 3:
			printf("\t\t\t%s    %%%s,%%%s\n", i386_alu[func], regs[rm], regs[reg]);
			break;
		}
		break;

		
	case 0x0f:	/* 2 byte opcode */
		((s4) code)++;
		pos++;
		opcode2 = (u1) *(code);
		printf(" %02x", opcode2);
		
		switch (opcode2) {
		case 0x80:		/* jcc  rel32 */
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x8a:
		case 0x8b:
		case 0x8c:
		case 0x8d:
		case 0x8e:
		case 0x8f:
			func = opcode2 - 0x80;
			GETIMM32(imb.val);
			printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
			printf("\tj%s    $0x%x\n", i386_jcc[func], (pos + 1 + imb.val));
			break;
			
		case 0xa5:		/* shld  cl,r32,r/m32 (0f a5) */
			((s4) code)++;
			i386_decode_modrm(code, &pos, &mod, &reg, &rm);

			switch (mod) {
			case 0: /* disp == 0 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					printf("\t\tshld   %%cl,%%%s,(%%%s,1)\n", regs[reg], regs[base]);
					break;

				default:
					printf("\t\tshld   %%cl,%%%s,(%%%s)\n", regs[reg], regs[rm]);
				}
				break;

			case 1:	/* disp == imm8 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\tshld   %%cl,%%%s,0x%x(%%%s,1)\n", regs[reg], (s1) imb.b[0], regs[base]);
					break;

				default:
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\tshld   %%cl,%%%s,0x%x(%%%s)\n", regs[reg], (s1) imb.b[0], regs[rm]);
				}
				break;

			case 2:	/* disp == imm32 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\tshld   %%cl,%%%s,0x%x(%%%s)\n", regs[reg], imb.val, regs[base]);
					break;

				default:
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\tshld   %%cl,%%%s,0x%x(%%%s)\n", regs[reg], imb.val, regs[rm]);
				}
				break;

			case 3:
				printf("\t\tshld   %%cl,%%%s,%%%s\n", regs[reg], regs[rm]);
				break;

			default:
				printf("\t\tUNDEF\n");
			}
			break;

		case 0xad:		/* shrd  cl,r32,r/m32 (0f ad) */
			((s4) code)++;
			i386_decode_modrm(code, &pos, &mod, &reg, &rm);

			switch (mod) {
			case 0: /* disp == 0 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					printf("\t\tshrd   %%cl,%%%s,(%%%s,1)\n", regs[reg], regs[base]);
					break;

				default:
					printf("\t\tshrd   %%cl,%%%s,(%%%s)\n", regs[reg], regs[rm]);
				}
				break;

			case 1:	/* disp == imm8 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\tshrd   %%cl,%%%s,0x%x(%%%s,1)\n", regs[reg], (s1) imb.b[0], regs[base]);
					break;

				default:
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\tshrd   %%cl,%%%s,0x%x(%%%s)\n", regs[reg], (s1) imb.b[0], regs[rm]);
				}
				break;

			case 2:	/* disp == imm32 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\tshrd   %%cl,%%%s,0x%x(%%%s)\n", regs[reg], imb.val, regs[base]);
					break;

				default:
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\tshrd   %%cl,%%%s,0x%x(%%%s)\n", regs[reg], imb.val, regs[rm]);
				}
				break;

			case 3:
				printf("\t\tshrd   %%cl,%%%s,%%%s\n", regs[reg], regs[rm]);
				break;

			default:
				printf("\t\tUNDEF\n");
			}
			break;

		case 0xaf:		/* imul  r/m32,r32 (0f af /r) */
			((s4) code)++;
			i386_decode_modrm(code, &pos, &mod, &reg, &rm);

			switch (mod) {
			case 0: /* disp == 0 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					printf("\t\timul   (%%%s,1),%%%s\n", regs[base], regs[reg]);
					break;

				default:
					printf("\t\timul   (%%%s),%%%s\n", regs[rm], regs[reg]);
				}
				break;

			case 1:	/* disp == imm8 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\timul   0x%x(%%%s,1),%%%s\n", (s1) imb.b[0], regs[base], regs[reg]);
					break;

				default:
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\timul   0x%x(%%%s),%%%s\n", (s1) imb.b[0], regs[rm], regs[reg]);
				}
				break;

			case 2:	/* disp == imm32 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\timul   0x%x(%%%s),%%%s\n", imb.val, regs[base], regs[reg]);
					break;

				default:
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\timul   0x%x(%%%s),%%%s\n", imb.val, regs[rm], regs[reg]);
				}
				break;

			case 3:
				printf("\t\timul   %%%s,%%%s\n", regs[rm], regs[reg]);
				break;

			default:
				printf("\t\tUNDEF\n");
			}
			break;

		case 0xbe:      /* movsx  r/m8,r32 (0f be /r) */
			((s4) code)++;
			i386_decode_modrm(code, &pos, &mod, &reg, &rm);
			printf("\t\tmovsbl %%%s,%%%s\n", regs[rm], regs[reg]);
			break;

		default:
			printf("\t\t\tUNDEF\n");
		}
		break;

		
	case 0x40:  /* inc  r32 (40+ rd) */
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47:
		reg = opcode - 0x40;
		printf("\t\t\tinc    %%%s\n", regs[reg]);
		break;


	case 0x48:  /* dec  r32 (48+ rd) */
	case 0x49:
	case 0x4a:
	case 0x4b:
	case 0x4c:
	case 0x4d:
	case 0x4e:
	case 0x4f:
		reg = opcode - 0x48;
		printf("\t\t\tdec    %%%s\n", regs[reg]);
		break;


	case 0x50:	/* push  r32 (50+rd) */
	case 0x51:
	case 0x52:
	case 0x53:
	case 0x54:
	case 0x55:
	case 0x56:
	case 0x57:
		reg = opcode - 0x50;
		printf("\t\t\tpush   %%%s\n", regs[reg]);
		break;

		
	case 0x58:	/* pop  r32 (58+rd) */
	case 0x59:
	case 0x5a:
	case 0x5b:
	case 0x5c:
	case 0x5d:
	case 0x5e:
	case 0x5f:
		reg = opcode - 0x58;
		printf("\t\t\tpop    %%%s\n", regs[reg]);
		break;

		
	case 0x69:	/* imul  imm32,r/m32,r32 (69 /r id) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

		switch (mod) {
		case 0: /* disp == 0 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM32(imb2.val);
				printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
				printf("\timul   $0x%x,(%%%s,1),%%%s\n", imb2.val, regs[base], regs[reg]);
				break;

			default:
				GETIMM32(imb2.val);
				printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
				printf("\timul   $0x%x,(%%%s),%%%s\n", imb2.val, regs[rm], regs[reg]);
			}
			break;

		case 1:	/* disp == imm8 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				GETIMM32(imb2.val);
				printf(" %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2]);
				printf("\timul   $0x%x,0x%x(%%%s,1),%%%s\n", imb2.val, (s1) imb.b[0], regs[base], regs[reg]);
				printf("          %02x\n", imb2.b[3]);
				break;

			default:
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				GETIMM32(imb2.val);
				printf(" %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2]);
				printf("\timul   $0x%x,0x%x(%%%s),%%%s\n", imb2.val, (s1) imb.b[0], regs[rm], regs[reg]);
				printf("          %02x\n", imb2.b[3]);
			}
			break;

		case 2:	/* disp == imm32 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				GETIMM32(imb2.val);
				printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
				printf("\timul   $0x%x,0x%x(%%%s),%%%s\n", imb2.val, imb.val, regs[base], regs[reg]);
				break;

			default:
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				GETIMM32(imb2.val);
				printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
				printf("\timul   $0x%x,0x%x(%%%s),%%%s\n", imb2.val, imb.val, regs[rm], regs[reg]);
			}
			break;

		case 3:
			GETIMM32(imb2.val);
			printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
			printf("\timul   $0x%x,%%%s,%%%s\n", imb2.val, regs[rm], regs[reg]);
			break;

		default:
			printf("\t\tUNDEF\n");
		}
		break;

		
	case 0x6b:	/* imul  imm8,r/m32,r32 (6b /r ib) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

		switch (mod) {
		case 0: /* disp == 0 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb2.b[0]);
				printf(" %02x", imb2.b[0]);
				printf("\t\timul   $0x%x,(%%%s,1),%%%s\n", (s1) imb2.b[0], regs[base], regs[reg]);
				break;

			default:
				GETIMM8(imb2.b[0]);
				printf(" %02x", imb2.b[0]);
				printf("\t\timul   $0x%x,(%%%s),%%%s\n", (s1) imb2.b[0], regs[rm], regs[reg]);
			}
			break;

		case 1:	/* disp == imm8 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				GETIMM8(imb2.b[0]);
				printf(" %02x", imb2.b[0]);
				printf("\timul   $0x%x,0x%x(%%%s,1),%%%s\n", (s1) imb2.b[0], (s1) imb.b[0], regs[base], regs[reg]);
				break;

			default:
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				GETIMM8(imb2.b[0]);
				printf(" %02x", imb2.b[0]);
				printf("\timul   $0x%x,0x%x(%%%s),%%%s\n", (s1) imb2.b[0], (s1) imb.b[0], regs[rm], regs[reg]);
			}
			break;

		case 2:	/* disp == imm32 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				GETIMM8(imb2.b[0]);
				printf(" %02x", imb2.b[0]);
				printf("\timul   $0x%x,0x%x(%%%s),%%%s\n", (s1) imb2.b[0], imb.val, regs[base], regs[reg]);
				break;

			default:
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				GETIMM8(imb2.b[0]);
				printf(" %02x", imb2.b[0]);
				printf("\timul   $0x%x,0x%x(%%%s),%%%s\n", (s1) imb2.b[0], imb.val, regs[rm], regs[reg]);
			}
			break;

		case 3:
			GETIMM8(imb2.b[0]);
			printf(" %02x", imb2.b[0]);
			printf("\t\timul   $0x%x,%%%s,%%%s\n", (s1) imb2.b[0], regs[rm], regs[reg]);
			break;

		default:
			printf("\t\tUNDEF\n");
		}
		break;

		
	case 0x81:	/* alu  imm32,r/m32 (81 /digit id) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

			switch (mod) {
			case 0: /* disp == 0 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM32(imb2.val);
					printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
					printf("\t%sl   $0x%x,(%%%s,1)\n", i386_alu[reg], imb2.val, regs[base]);
					break;

				default:
					GETIMM32(imb2.val);
					printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
					printf("\t%sl   $0x%x,(%%%s)\n", i386_alu[reg], imb2.val, regs[rm]);
				}
				break;
			
			case 1: /* disp == 8 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					GETIMM32(imb2.val);
					printf(" %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2]);
					printf("\t%sl   $0x%x,0x%x(%%%s,1)\n", i386_alu[reg], imb2.val, (s1) imb.b[0], regs[base]);
					printf("          %02x\n", imb2.b[3]);
					break;

				default:
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					GETIMM32(imb2.val);
					printf(" %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2]);
					printf("\t\t%sl   $0x%x,0x%x(%%%s)\n", i386_alu[reg], imb2.val, (s1) imb.b[0], regs[rm]);
					printf("          %02x\n", imb2.b[3]);
				}
				break;
			
			case 2: /* disp == 3 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					GETIMM32(imb2.val);
					printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
					printf("\t%sl   $0x%x,0x%x(%%%s,1)\n", i386_alu[reg], imb2.val, imb.val, regs[base]);
					break;

				default:
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					GETIMM32(imb2.val);
					printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
					printf("\t\t%sl   $0x%x,0x%x(%%%s)\n", i386_alu[reg], imb2.val, imb.val, regs[rm]);
				}
				break;
			
			case 3:
				GETIMM32(imb2.val);
				printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
				printf("\t%s    $0x%x,%%%s\n", i386_alu[reg], imb2.val, regs[rm]);
			}
			break;

		
	case 0x83:	/* alu  imm8,r/m32 (83 /digit id) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

			switch (mod) {
			case 0: /* disp == 0 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM8(imb2.b[0]);
					printf(" %02x", imb2.b[0]);
					printf("\t\t%sl   $0x%x,(%%%s,1)\n", i386_alu[reg], (s1) imb2.b[0], regs[base]);
					break;

				default:
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\t\t%sl   $0x%x,(%%%s)\n", i386_alu[reg], (s1) imb.b[0], regs[rm]);
				}
				break;
			
			case 1: /* disp == 8 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					GETIMM8(imb2.b[0]);
					printf(" %02x", imb2.b[0]);
					printf("\t%sl   $0x%x,0x%x(%%%s,1)\n", i386_alu[reg], (s1) imb2.b[0], (s1) imb.b[0], regs[base]);
					break;

				default:
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					GETIMM8(imb2.b[0]);
					printf(" %02x", imb2.b[0]);
					printf("\t\t%sl   $0x%x,0x%x(%%%s)\n", i386_alu[reg], (s1) imb2.b[0], (s1) imb.b[0], regs[rm]);
				}
				break;
			
			case 2: /* disp == 32 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					GETIMM8(imb2.b[0]);
					printf(" %02x", imb2.b[0]);
					printf("\t%sl   $0x%x,0x%x(%%%s,1)\n", i386_alu[reg], (s1) imb2.b[0], imb.val, regs[base]);
					break;

				default:
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					GETIMM8(imb2.b[0]);
					printf(" %02x", imb2.b[0]);
					printf("\t\t%sl   $0x%x,0x%x(%%%s)\n", i386_alu[reg], (s1) imb2.b[0], imb.val, regs[rm]);
				}
				break;
			
			case 3:
				GETIMM8(imb2.b[0]);
				printf(" %02x", imb2.b[0]);
				printf("\t\t%s    $0x%x,%%%s\n", i386_alu[reg], (s1) imb2.b[0], regs[rm]);
				break;
			
			default:
				printf("\tUNDEF\n");
			}
			break;
			
		
	case 0x85:	/* test  r32,r/m32 (85 /r) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);
		
		switch (mod) {
		case 0: /* disp == 0 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				printf("\t\ttest   %%%s,(%%%s,1)\n", regs[reg], regs[base]);
				break;

			case 5: /* disp32 */
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\ttest   %%%s,0x%08x\n", regs[reg], imb.val);
				break;

			default:
				printf("\t\t\ttest   %%%s,(%%%s)\n", regs[reg], regs[rm]);
			}
			break;

		case 1:	/* disp == imm8 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\ttest   %%%s,0x%x(%%%s,1)\n", regs[reg], (s1) imb.b[0], regs[base]);
				break;

			default:
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\ttest   %%%s,0x%x(%%%s)\n", regs[reg], (s1) imb.b[0], regs[rm]);
			}
			break;

		case 2:	/* disp == imm32 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\ttest   %%%s,0x%x(%%%s,1)\n", regs[reg], imb.val, regs[base]);
				break;

			default:
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\ttest   %%%s,0x%x(%%%s)\n", regs[reg], imb.val, regs[rm]);
			}
			break;

		case 3:
			printf("\t\t\ttest   %%%s,%%%s\n", regs[reg], regs[rm]);
			break;

		default:
			printf("\t\tUNDEF\n");
		}
		break;

		
	case 0x89:	/* mov  r32,r/m32 (89 /r) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);
		
		switch (mod) {
		case 0: /* disp == 0 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);

				switch (index) {
				case 4: /* no scaled index */
					printf("\t\tmov    %%%s,(%%%s,%d)\n", regs[reg], regs[base], 1 << ss);
					break;

				default:
					printf("\t\tmov    %%%s,(%%%s,%%%s,%d)\n", regs[reg], regs[base], regs[index], 1 << ss);
					break;
				}
				break;

			case 5: /* disp32 */
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\tmov    %%%s,0x%08x\n", regs[reg], imb.val);
				break;

			default:
				printf("\t\t\tmov    %%%s,(%%%s)\n", regs[reg], regs[rm]);
			}
			break;

		case 1:	/* disp == imm8 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);

				switch (index) {
				case 4: /* no scaled index */
					printf("\t\tmov    %%%s,0x%x(%%%s,%d)\n", regs[reg], (s1) imb.b[0], regs[base], 1 << ss);
					break;
					
				default:
					printf("\t\tmov    %%%s,0x%x(%%%s,%%%s,%d)\n", regs[reg], (s1) imb.b[0], regs[base], regs[index], 1 << ss);
					break;
				}
				break;

			default:
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\tmov    %%%s,0x%x(%%%s)\n", regs[reg], (s1) imb.b[0], regs[rm]);
			}
			break;

		case 2:	/* disp == imm32 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);

				switch (index) {
				case 4: /* no scaled index */
					printf("\tmov    %%%s,0x%x(%%%s,%d)\n", regs[reg], imb.val, regs[base], 1 << ss);
					break;

				default:
					printf("\tmov    %%%s,0x%x(%%%s,%%%s,%d)\n", regs[reg], imb.val, regs[base], regs[index], 1 << ss);
					break;
				}
				break;

			default:
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\tmov    %%%s,0x%x(%%%s)\n", regs[reg], imb.val, regs[rm]);
			}
			break;

		case 3:
			printf("\t\t\tmov    %%%s,%%%s\n", regs[reg], regs[rm]);
			break;

		default:
			printf("\t\tUNDEF\n");
		}
		break;

		
	case 0x8b:	/* mov  r/m32,r32 (8b /r) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

		switch (mod) {
		case 0: /* disp == 0 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);

				switch (index) {
				case 4: /* no scaled index */
					printf("\t\tmov    (%%%s,%d),%%%s\n", regs[base], 1 << ss, regs[reg]);
					break;
					
				default:
					printf("\t\tmov    (%%%s,%%%s,%d),%%%s\n", regs[base], regs[index], 1 << ss, regs[reg]);
				}
				break;

			case 5: /* disp32 */
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\tmov    0x%x,%%%s\n", imb.val, regs[reg]);
				break;

			default:
				printf("\t\t\tmov    (%%%s),%%%s\n", regs[rm], regs[reg]);
			}
			break;

		case 1:	/* disp == imm8 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb2.b[0]);
				printf(" %02x", imb2.b[0]);

				switch (index) {
				case 4: /* no scaled index */
					printf("\t\tmov    0x%x(%%%s,%d),%%%s\n", (s1) imb2.b[0], regs[base], 1 << ss, regs[reg]);
					break;

				default:
					printf("\t\tmov    0x%x(%%%s,%%%s,%d),%%%s\n", (s1) imb2.b[0], regs[base], regs[index], 1 << ss, regs[reg]);
				}
				break;

			default:
				GETIMM8(imb2.b[0]);
				printf(" %02x", imb2.b[0]);
				printf("\t\tmov    0x%x(%%%s),%%%s\n", (s1) imb2.b[0], regs[rm], regs[reg]);
			}
			break;

		case 2:	/* disp == imm32 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM32(imb2.val);
				printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);

				switch (index) {
				case 4: /* no scaled index */
					printf("\t\tmov    0x%x(%%%s,%d),%%%s\n", imb2.val, regs[base], 1 << ss, regs[reg]);
					break;

				default:
					printf("\t\tmov    0x%x(%%%s,%%%s,%d),%%%s\n", imb2.val, regs[base], regs[index], 1 << ss, regs[reg]);
				}
				break;

			default:
				GETIMM32(imb2.val);
				printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
				printf("\tmov    0x%x(%%%s),%%%s\n", imb2.val, regs[rm], regs[reg]);
				break;
			}
			break;

		case 3:
			printf("\t\t\tmov    %%%s,%%%s\n", regs[rm], regs[reg]);
			break;

		default:
			printf("\t\tUNDEF\n");
		}
		break;

		
	case 0x90:	/* nop */
		printf("\t\t\tnop\n");
		break;


	case 0x99:	/* cltd */
		printf("\t\t\tcltd\n");
		break;


	case 0xb8:	/* mov  imm32,reg32 (b8+ rd) */
	case 0xb9:
	case 0xba:
	case 0xbb:
	case 0xbc:
	case 0xbd:
	case 0xbe:
	case 0xbf:
		reg = opcode - 0xb8;
		GETIMM32(imb.val);
		printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
		printf("\tmov    $0x%x,%%%s\n", imb.val, regs[reg]);
		break;


	case 0xc1:	/* sxx  imm8,r/m32 (c1 /reg ib) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

		switch (mod) {
		case 0: /* disp == 0 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\t%s    $0x%x,(%%%s,1)\n", i386_sxx[reg], imb.b[0], regs[base]);
				break;

			default:
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\t%s    $0x%x,(%%%s)\n", i386_sxx[reg], imb.b[0], regs[rm]);
			}
			break;
			
		case 1: /* disp == 8 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb.b[0]);
				GETIMM8(imb2.b[0]);
				printf(" %02x %02x", imb.b[0], imb2.b[0]);
				printf("\t%s    $0x%x,0x%x(%%%s,1)\n", i386_sxx[reg], imb2.b[0], (s1) imb.b[0], regs[base]);
				break;

			default:
				GETIMM8(imb.b[0]);
				GETIMM8(imb2.b[0]);
				printf(" %02x %02x", imb.b[0], imb2.b[0]);
				printf("\t%s    $0x%x,0x%x(%%%s)\n", i386_sxx[reg], imb2.b[0], (s1) imb.b[0], regs[rm]);
			}
			break;

		case 2: /* disp == 32 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM32(imb.val);
				GETIMM8(imb2.b[0]);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf(" %02x", imb2.b[0]);
				printf("\t%s    $0x%x,0x%x(%%%s,1)\n", i386_sxx[reg], imb2.b[0], imb.val, regs[base]);
				break;

			default:
				GETIMM32(imb.val);
				GETIMM8(imb2.b[0]);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf(" %02x", imb2.b[0]);
				printf("\t%s    $0x%x,0x%x(%%%s)\n", i386_sxx[reg], imb2.b[0], imb.val, regs[rm]);
			}
			break;

		case 3:
			GETIMM8(imb.b[0]);
			printf("\t\t\t%s    $0x%x,%%%s\n", i386_sxx[reg], imb.b[0], regs[rm]);
			break;

		default:
			printf("\t\t\tUNDEF\n");
		}
		break;
		
		
	case 0xc3:	/* ret (c3) */
		printf("\t\t\tret\n");
		break;


	case 0xc7:  /* mov  imm32,r/m32 (c7 /0) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

		switch (mod) {
		case 0: /* disp == 0 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\tmovl   $0x%x,(%%%s,1)\n", imb.val, regs[base]);
				break;

			default:
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\tmovl   $0x%x,(%%%s)\n", imb.val, regs[rm]);
			}
			break;

		case 1: /* disp == 8 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb2.b[0]);
				GETIMM32(imb.val);
				printf(" %02x", imb2.b[0]);
				printf(" %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2]);
				printf("\tmovl   $0x%x,0x%x(%%%s,1)\n", imb.val, (s1) imb2.b[0], regs[base]);
				printf("          %02x\n", imb.b[3]);
				break;

			default:
				GETIMM8(imb2.b[0]);
				GETIMM32(imb.val);
				printf(" %02x", imb2.b[0]);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\tmovl   $0x%x,0x%x(%%%s)\n", imb.val, (s1) imb2.b[0], regs[rm]);
			}
			break;

		case 2: /* disp == 32 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM32(imb2.val);
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
				printf(" %02x", imb.b[0]);
				printf("\tmovl   $0x%x,0x%x(%%%s,1)\n", imb.val, imb2.val, regs[rm]);
				printf("          %02x %02x %02x\n", imb.b[1], imb.b[2], imb.b[3]);
				break;

			default:
				GETIMM32(imb2.val);
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
				printf(" %02x", imb.b[0]);
				printf("\tmovl   $0x%x,0x%x(%%%s)\n", imb.val, imb2.val, regs[rm]);
				printf("          %02x %02x %02x\n", imb.b[1], imb.b[2], imb.b[3]);
			}
			break;

		default:
			printf("\t\t\tUNDEF\n");
		}
		break;


	case 0xc9:	/* leave (c9) */
		printf("\t\t\tleave\n");
		break;

		
	case 0xd1:	/* sxx  1,r/m32 (d1 /reg) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

		switch (mod) {
		case 0: /* disp == 0 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				printf("\t\t%s    (%%%s,1)\n", i386_sxx[reg], regs[base]);
				break;

			default:
				printf("\t\t%s    (%%%s)\n", i386_sxx[reg], regs[rm]);
			}
			break;

		case 1: /* disp == 8 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\t%s    0x%x(%%%s,1)\n", i386_sxx[reg], (s1) imb.b[0], regs[base]);
				break;

			default:
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\t%s    0x%x(%%%s,1)\n", i386_sxx[reg], (s1) imb.b[0], regs[rm]);
			}
			break;

		case 2: /* disp == 32 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\t\t%s    0x%x(%%%s,1)\n", i386_sxx[reg], imb.val, regs[base]);
				break;

			default:
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\t\t%s    0x%x(%%%s)\n", i386_sxx[reg], imb.val, regs[rm]);
			}
			break;

		case 3:
			printf("\t\t\t%s    %%%s\n", i386_sxx[reg], regs[rm]);
			break;

		default:
			printf("\t\t\tUNDEF\n");
		}
		break;
		
		
	case 0xd3:	/* sxx  cl,r/m32 (d3 /reg) */
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

		switch (mod) {
		case 0: /* disp == 0 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				printf("\t\t%s    %%cl,(%%%s,1)\n", i386_sxx[reg], regs[base]);
				break;

			default:
				printf("\t\t%s    %%cl,(%%%s)\n", i386_sxx[reg], regs[rm]);
			}
			break;

		case 1: /* disp == 8 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\t%s    %%cl,0x%x(%%%s,1)\n", i386_sxx[reg], (s1) imb.b[0], regs[base]);
				break;

			default:
				GETIMM8(imb.b[0]);
				printf(" %02x", imb.b[0]);
				printf("\t\t%s    %%cl,0x%x(%%%s,1)\n", i386_sxx[reg], (s1) imb.b[0], regs[rm]);
			}
			break;

		case 2: /* disp == 32 */
			switch (rm) {
			case 4:
				((s4) code)++;
				i386_decode_sib(code, &pos, &ss, &index, &base);
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\t\t%s    %%cl,0x%x(%%%s,1)\n", i386_sxx[reg], imb.val, regs[base]);
				break;

			default:
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\t\t%s    %%cl,0x%x(%%%s)\n", i386_sxx[reg], imb.val, regs[rm]);
			}
			break;

		case 3:
			printf("\t\t\t%s    %%cl,%%%s\n", i386_sxx[reg], regs[rm]);
			break;

		default:
			printf("\t\t\tUNDEF\n");
		}
		break;
		

	case 0xd8:  /* two byte opcodes */
		((s4) code)++;
		pos++;
		opcode2 = (u1) *(code);

		switch (opcode2) {
		default:
			i386_decode_modrm(code, &pos, &mod, &reg, &rm);

			switch (reg) {
			case 0:  /* fadd  m32real (d8 /0) */
				switch (mod) {
				case 0: /* disp == 0 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						printf("\t\tfadds  (%%%s,1)\n", regs[base]);
						break;

					default:
						printf("\t\tfadds  (%%%s)\n", regs[rm]);
					}
					break;

				case 1: /* disp == 8 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfadds  0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);
						break;

					default:
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfadds  0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
					}
					break;
				}
				break;

			case 4:  /* fstp  m32real (d8 /4) */
				switch (mod) {
				case 0: /* disp == 0 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						printf("\t\tfsubs  (%%%s,1)\n", regs[base]);
						break;

					default:
						printf("\t\tfsubs  (%%%s)\n", regs[rm]);
					}
					break;

				case 1: /* disp == 8 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfsubs  0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);
						break;

					default:
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfsubs  0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
					}
					break;
				}
				break;

			default:
				printf("\t\t\tUNDEF\n");
			}
		}
		break;


	case 0xd9:  /* two byte opcodes */
		((s4) code)++;
		pos++;
		opcode2 = (u1) *(code);

		switch (opcode2) {
		case 0xc8:  /* fxch  st(i) (d9 c8+i) */
		case 0xc9:
		case 0xca:
		case 0xcb:
		case 0xcc:
		case 0xcd:
		case 0xce:
		case 0xcf:
			reg = opcode2 - 0xc8;
			printf(" %02x", opcode2);
			printf("\t\t\tfxch   %%st(%d)\n", reg);
			break;

		case 0xe0:  /* fchs (d9 e0) */
			printf(" %02x", opcode2);
			printf("\t\t\tfchs\n");
			break;

		case 0xf5:  /* fprem1 (d9 f5) */
			printf(" %02x", opcode2);
			printf("\t\t\tfprem1\n");
			break;

		default:
			i386_decode_modrm(code, &pos, &mod, &reg, &rm);

			switch (reg) {
			case 0:  /* fld  m32real (d9 /0) */
				switch (mod) {
				case 0: /* disp == 0 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						printf("\t\tflds   (%%%s,1)\n", regs[base]);
						break;

					case 5: /* disp32 */
						GETIMM32(imb.val);
						printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
						printf("\tflds   0x%08x\n", imb.val);
						break;

					default:
						printf("\t\tflds   (%%%s)\n", regs[rm]);
					}
					break;

				case 1: /* disp == 8 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tflds   0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);
						break;

					default:
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tflds   0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
					}
					break;
				}
				break;

			case 3:  /* fstp  m32real (d9 /3) */
				switch (mod) {
				case 0: /* disp == 0 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						printf("\t\tfstps  (%%%s,1)\n", regs[base]);
						break;

					default:
						printf("\t\tfstps  (%%%s)\n", regs[rm]);
					}
					break;

				case 1: /* disp == 8 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfstps  0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);
						break;

					default:
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfstps  0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
					}
					break;
				}
				break;

			default:
				printf("\t\t\tUNDEF\n");
			}
		}
		break;


	case 0xda:  /* two byte opcodes */
		((s4) code)++;
		pos++;
		opcode2 = (u1) *(code);

		switch (opcode2) {
		case 0xe9:  /* fucompp (da e9) */
			printf(" %02x", opcode2);
			printf("\t\t\tfucompp\n");
			break;

		default:
			printf("\t\t\tUNDEF\n");
		}
		break;


	case 0xdb:  /* two byte opcodes */
		((s4) code)++;
		pos++;
		opcode2 = (u1) *(code);

		switch (opcode2) {
		default:
			i386_decode_modrm(code, &pos, &mod, &reg, &rm);

			switch (reg) {
			case 0:  /* fild  m32real (db /0) */
				switch (mod) {
				case 0: /* disp == 0 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						printf("\t\tfildl  (%%%s,1)\n", regs[base]);
						break;

					case 5: /* disp32 */
						GETIMM32(imb.val);
						printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
						printf("\tfildl  0x%08x\n", imb.val);
						break;

					default:
						printf("\t\tfildl  (%%%s)\n", regs[rm]);
					}
					break;

				case 1: /* disp == 8 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\tfildl  0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);
						break;

					default:
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfildl  0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
					}
					break;
				}
				break;

			case 3:  /* fistp  m32int (db /3) */
				switch (mod) {
				case 0: /* disp == 0 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						printf("\t\tfistpl (%%%s,1)\n", regs[base]);
						break;

					case 5: /* disp32 */
						GETIMM32(imb.val);
						printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
						printf("\tfistpl 0x%08x\n", imb.val);
						break;

					default:
						printf("\t\tfistpl (%%%s)\n", regs[rm]);
					}
					break;

				case 1: /* disp == 8 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\tfistpl 0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);
						break;

					default:
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfistpl 0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
					}
					break;
				}
				break;

			default:
				printf("\t\t\tUNDEF\n");
			}
		}
		break;


	case 0xdd:  /* two byte opcodes */
		((s4) code)++;
		pos++;
		opcode2 = (u1) *(code);

		switch (opcode2) {
		default:
			i386_decode_modrm(code, &pos, &mod, &reg, &rm);

			switch (reg) {
			case 0:  /* fld  m64real (dd /0) */
				switch (mod) {
				case 0: /* disp == 0 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						printf("\t\tfldl   (%%%s,1)\n", regs[base]);
						break;

					case 5: /* disp32 */
						GETIMM32(imb.val);
						printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
						printf("\tfldl   0x%08x\n", imb.val);
						break;

					default:
						printf("\t\tfldl   (%%%s)\n", regs[rm]);
					}
					break;

				case 1: /* disp == 8 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfldl   0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);
						break;

					default:
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfldl   0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
					}
					break;
				}
				break;

			case 3:  /* fstp  m64real (dd /3) */
				switch (mod) {
				case 0: /* disp == 0 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						printf("\t\tfstpl  (%%%s,1)\n", regs[base]);
						break;

					default:
						printf("\t\tfstpl  (%%%s)\n", regs[rm]);
					}
					break;

				case 1: /* disp == 8 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfstpl  0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);
						break;

					default:
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfstpl  0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
					}
					break;
				}
				break;

			default:
				printf("\t\t\tUNDEF\n");
			}
		}
		break;


	case 0xde:  /* two byte opcodes */
		((s4) code)++;
		pos++;
		opcode2 = (u1) *(code);

		switch (opcode2) {
		case 0xc1:  /* faddp (de c1) */
			printf(" %02x", opcode2);
			printf("\t\t\tfaddp\n");
			break;

		case 0xc9:  /* fmulp (de c9) */
			printf(" %02x", opcode2);
			printf("\t\t\tfmulp\n");
			break;

		case 0xe9:  /* fsubp (de e9) */
			printf(" %02x", opcode2);
			printf("\t\t\tfsubp\n");
			break;

		case 0xf9:  /* fdivp (de f9) */
			printf(" %02x", opcode2);
			printf("\t\t\tfdivp\n");
			break;

		default:
			i386_decode_modrm(code, &pos, &mod, &reg, &rm);

			switch (reg) {
			case 0:
			break;

			default:
				printf("\t\t\tUNDEF\n");
			}
		}
		break;


	case 0xdf:  /* two byte opcodes */
		((s4) code)++;
		pos++;
		opcode2 = (u1) *(code);

		switch (opcode2) {
		case 0xe0:  /* fnstsw  ax (df e0) */
			printf(" %02x", opcode2);
			printf("\t\t\tfnstsw %%ax\n");
			break;

		default:
			i386_decode_modrm(code, &pos, &mod, &reg, &rm);

			switch (reg) {
			case 5:  /* fild  m64real (df /5) */
				switch (mod) {
				case 0: /* disp == 0 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						printf("\t\tfildll (%%%s,1)\n", regs[base]);
						break;

					case 5: /* disp32 */
						GETIMM32(imb.val);
						printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
						printf("\tfildll 0x%08x\n", imb.val);
						break;

					default:
						printf("\t\tfildll (%%%s)\n", regs[rm]);
					}
					break;

				case 1: /* disp == 8 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfildll 0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);
						break;

					default:
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfildll 0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
					}
					break;
				}
				break;

			case 7:  /* fistp  m64int (df /7) */
				switch (mod) {
				case 0: /* disp == 0 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						printf("\t\tfistpll (%%%s,1)\n", regs[base]);
						break;

					case 5: /* disp32 */
						GETIMM32(imb.val);
						printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
						printf("\tfistpll 0x%08x\n", imb.val);
						break;

					default:
						printf("\t\tfistpll (%%%s)\n", regs[rm]);
					}
					break;

				case 1: /* disp == 8 */
					switch (rm) {
					case 4:
						((s4) code)++;
						i386_decode_sib(code, &pos, &ss, &index, &base);
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfistpll 0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);
						break;

					default:
						GETIMM8(imb.b[0]);
						printf(" %02x", imb.b[0]);
						printf("\t\tfistpll 0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
					}
					break;
				}
				break;

			default:
				printf("\t\t\tUNDEF\n");
			}
		}
		break;


	case 0xe9:	/* jmp  rel32 (e9 cd) */
		GETIMM32(imb.val);
		printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
		printf("\tjmp    $0x%x\n", (pos + 1 + imb.val));
		break;

		
	case 0xeb:	/* jmp  rel8 (eb cb) */
		GETIMM8(imb.b[0]);
		printf(" %02x", imb.b[0]);
		printf("\t\t\tjmp    $0x%x\n", (pos + 1 + imb.b[0]));
		break;

		
	case 0xf7:
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

		switch (reg) {
		case 0: /* test  imm32,r/m32 (f7 /0 id */
			switch (mod) {
			case 0: /* disp == 0 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM32(imb2.val);
					printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
					printf("\t\ttest   $0x%x,(%%%s,1)\n", imb2.val, regs[base]);
					break;

				default:
					GETIMM32(imb2.val);
					printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
					printf("\t\t\ttest   $0x%x,(%%%s)\n", imb2.val, regs[rm]);
				}
				break;

			case 1: /* disp == 8 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					GETIMM32(imb2.val);
					printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
					printf("\t\ttest   $0x%x,0x%x(%%%s,1)\n", imb2.val, imb.b[0], regs[base]);
					break;

				default:
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					GETIMM32(imb2.val);
					printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
					printf("\t\ttest   $0x%x,0x%x(%%%s)\n", imb2.val, imb.b[0], regs[rm]);
				}
				break;

			case 2: /* disp == 32 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					GETIMM32(imb2.val);
					printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
					printf("\t\ttest   $0x%x,0x%x(%%%s,1)\n", imb2.val, imb.val, regs[rm]);
					break;

				default:
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					GETIMM32(imb2.val);
					printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
					printf("\t\ttest   $0x%x,0x%x(%%%s,1)\n", imb2.val, imb.val, regs[rm]);
				}
				break;
			
			case 3:
				GETIMM32(imb2.val);
				printf(" %02x %02x %02x %02x", imb2.b[0], imb2.b[1], imb2.b[2], imb2.b[3]);
				printf("\ttest   $0x%x,%%%s\n", imb2.val, regs[rm]);
				break;
			}
			break;

		case 3: /* neg  r/m32 (f7 /3) */
			switch (mod) {
			case 0: /* disp == 0 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					printf("\t\tnegl   (%%%s,1)\n", regs[base]);
					break;

				default:
					printf("\t\tnegl   (%%%s)\n", regs[rm]);
				}
				break;

			case 1: /* disp == 8 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\t\tnegl   0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);
					break;

				default:
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\t\tnegl   0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
				}
				break;

			case 2: /* disp == 32 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\t\tnegl   0x%x(%%%s,1)\n", imb.val, regs[base]);
					break;

				default:
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\t\tnegl   0x%x(%%%s)\n", imb.val, regs[rm]);
				}
				break;
			
			case 3:
				printf("\t\t\tneg    %%%s\n", regs[rm]);
				break;
			}
			break;

		case 4: /* mul  r/m32,eax (f7 /4) */
			switch (mod) {
			case 0: /* disp == 0 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					printf("\t\tmull   (%%%s,1)\n", regs[base]);
					break;

				default:
					printf("\t\tmull   (%%%s)\n", regs[rm]);
				}
				break;

			case 1: /* disp == 8 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\t\tmull   0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);
					break;

				default:
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\t\tmull   0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
				}
				break;

			case 2: /* disp == 32 */
				switch (rm) {
				case 4:
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\t\tmull   0x%x(%%%s,1)\n", imb.val, regs[base]);
					break;

				default:
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\t\tmull   0x%x(%%%s)\n", imb.val, regs[rm]);
				}
				break;
			
			case 3:
				printf("\t\t\tmul    %%%s\n", regs[rm]);
				break;
			}
			break;

		case 7: /* idiv  r/m32 (f7 /7) */
			switch (mod) {
			case 0: /* disp == 0 */
				if (rm == 4) {
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					printf("\t\t\tidivl  (%%%s,1)\n", regs[base]);

				} else {
					printf("\t\t\tidivl  (%%%s)\n", regs[rm]);
				}
				break;

			case 1: /* disp == 8 */
				if (rm == 4) {
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\t\tidivl  0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);

				} else {
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\t\tidivl  0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
				}
				break;

			case 2: /* disp == 32 */
				if (rm == 4) {
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\t\tidivl  0x%x(%%%s,1)\n", imb.val, regs[rm]);

				} else {
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\t\tidivl  0x%x(%%%s,1)\n", imb.val, regs[rm]);
				}
				break;
			
			case 3:
				printf("\t\t\tidivl  %%%s\n", regs[rm]);
				break;
			}
			break;

		default:
			printf("\t\t\tUNDEF\n");
		}
		break;


	case 0xff:
		((s4) code)++;
		i386_decode_modrm(code, &pos, &mod, &reg, &rm);

		switch (reg) {
		case 0: /* inc  r/m32 (ff /0) */
			switch (mod) {
			case 0: /* disp == 0 */
				if (rm == 4) {
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					printf("\t\tinc    (%%%s,1)\n", regs[base]);

				} else {
					printf("\t\tinc    (%%%s)\n", regs[rm]);
				}
				break;

			case 1: /* disp == 8 */
				if (rm == 4) {
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\t\tinc    0x%x(%%%s,1)\n", (s1) imb.b[0], regs[base]);

				} else {
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\t\tinc    0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
				}
				break;

			case 2: /* disp == 32 */
				GETIMM32(imb.val);
				printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
				printf("\t\tinc    0x%x(%%%s)\n", imb.val, regs[rm]);
				break;

			default:
				printf("\t\tUNDEF\n");
			}
			break;

		case 1: /* dec  r/m32 (ff /1) */
			switch (mod) {
			case 0: /* disp == 0 */
				if (rm == 4) {
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					printf("\t\tdec    (%%%s,1)\n", regs[base]);

				} else {
					printf("\t\tdec    0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
				}
				break;
	
			case 1: /* disp == 8 */
				if (rm == 4) {
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\t\tdec    %d(%%%s,1)\n", (s1) imb.b[0], regs[base]);

				} else {
					GETIMM8(imb.b[0]);
					printf(" %02x", imb.b[0]);
					printf("\t\tdec    0x%x(%%%s)\n", (s1) imb.b[0], regs[rm]);
				}
				break;

			case 2: /* disp == 32 */
				if (rm == 4) {
					((s4) code)++;
					i386_decode_sib(code, &pos, &ss, &index, &base);
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\t\tdec    0x%x(%%%s,1)\n", imb.val, regs[base]);

				} else {
					GETIMM32(imb.val);
					printf(" %02x %02x %02x %02x", imb.b[0], imb.b[1], imb.b[2], imb.b[3]);
					printf("\t\tdec    0x%x(%%%s)\n", imb.val, regs[rm]);
				}
				break;

			default:
				printf("\t\tUNDEF\n");
			}
			break;

		case 2:	/* call  r/m32 (ff /2) */
			printf("\t\t\tcall   *%%%s\n", regs[rm]);
			break;

		default:
			printf("\t\t\tUNDEF\n");
		}
		break;

		
	default:
		printf("\t\t\tUNDEF\n");
	}

	codestatic = code;
	pstatic = pos;
}


/* function disassemble ********************************************************

	outputs a disassembler listing of some machine code on 'stdout'
	code: pointer to first instruction
	len:  code size (number of instructions * 4)

*******************************************************************************/

static void disassemble(int *code, int len)
{
	int p;
	
	printf("  --- disassembler listing ---\n");
	for (p = 0; p < len; p++, ((s4) code)++) {
 		disassinstr((s4) code, p);
		p = pstatic;
		code = codestatic;
	}
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
