/***************************** ncomp/nblock.c **********************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Basic block handling functions.

	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/11/05

*******************************************************************************/


/******************** function determine_basic_blocks **************************

	Scans the JavaVM code of a method and marks each instruction which is the
	start of a basic block.
	
*******************************************************************************/

static void allocate_literals()
{
	int  p, nextp;
	int  opcode, i;

	p = 0;
	while (p < jcodelength) {

		opcode = jcode[p];
		nextp = p + jcommandsize[opcode];

		switch (opcode) {
			case JAVA_WIDE:
				switch (code_get_u1(p + 1)) {
					case JAVA_RET:  nextp = p + 4;
					                break;
					case JAVA_IINC: nextp = p + 6;
					                break;
					default:        nextp = p + 4;
					                break;
					}
				break;
							
			case JAVA_LOOKUPSWITCH:
				{
				s4 num;

				nextp = ALIGN((p + 1), 4);
				num = code_get_u4(nextp + 4);
				nextp = nextp + 8 + 8 * num;
				break;
				}

			case JAVA_TABLESWITCH:
				{
				s4 num;

				nextp = ALIGN ((p + 1),4);
				num = code_get_s4(nextp + 4);
				num = code_get_s4(nextp + 8) - num;
				nextp = nextp + 16 + 4 * num;
				break;
				}	

			case JAVA_LDC1:
				i = code_get_u1(p+1);
				goto pushconstantitem;
			case JAVA_LDC2:
			case JAVA_LDC2W:
				i = code_get_u2(p + 1);
			pushconstantitem:
				if (class_constanttype(class, i) == CONSTANT_String) {
					unicode *s;
					s = class_getconstant(class, i, CONSTANT_String);
					(void) literalstring_new(s);
					}
				break;
			} /* end switch */

		p = nextp;

		} /* end while */

}
