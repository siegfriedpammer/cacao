/****************************** ncomp/nparse.c *********************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Parser for JavaVM to intermediate code translation
	
	Author: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1998/05/07

*******************************************************************************/

#include "math.h"

/*********************** function allocate_literals ****************************

	Scans the JavaVM code of a method and allocates string literals. Needed
	to generate the same addresses as the old JIT compiler.
	
*******************************************************************************/

static void allocate_literals()
{
	int     p, nextp;
	int     opcode, i;
	s4      num;
	unicode *s;

	for (p = 0; p < jcodelength; p = nextp) {

		opcode = jcode[p];
		nextp = p + jcommandsize[opcode];

		switch (opcode) {
			case JAVA_WIDE:
				if (code_get_u1(p + 1) == JAVA_IINC)
					nextp = p + 6;
				else
					nextp = p + 4;
				break;
							
			case JAVA_LOOKUPSWITCH:
				nextp = ALIGN((p + 1), 4);
				num = code_get_u4(nextp + 4);
				nextp = nextp + 8 + 8 * num;
				break;

			case JAVA_TABLESWITCH:
				nextp = ALIGN ((p + 1),4);
				num = code_get_s4(nextp + 4);
				num = code_get_s4(nextp + 8) - num;
				nextp = nextp + 16 + 4 * num;
				break;

			case JAVA_LDC1:
				i = code_get_u1(p+1);
				goto pushconstantitem;
			case JAVA_LDC2:
			case JAVA_LDC2W:
				i = code_get_u2(p + 1);
			pushconstantitem:
				if (class_constanttype(class, i) == CONSTANT_String) {
					s = class_getconstant(class, i, CONSTANT_String);
					(void) literalstring_new(s);
					}
				break;
			} /* end switch */
		} /* end while */
}



/*******************************************************************************

	function 'parse' scans the JavaVM code and generates intermediate code

	During parsing the block index table is used to store at bit pos 0
	a flag which marks basic block starts and at position 1 to 31 the
	intermediate instruction index. After parsing the block index table
	is scanned, for marked positions a block is generated and the block
	number is stored in the block index table.

*******************************************************************************/

/* intermediate code generating macros */

#define PINC           iptr++;ipc++
#define LOADCONST_I(v) iptr->opc=ICMD_ICONST;iptr->op1=0;iptr->val.i=(v);PINC
#define LOADCONST_L(v) iptr->opc=ICMD_LCONST;iptr->op1=0;iptr->val.l=(v);PINC
#define LOADCONST_F(v) iptr->opc=ICMD_FCONST;iptr->op1=0;iptr->val.f=(v);PINC
#define LOADCONST_D(v) iptr->opc=ICMD_DCONST;iptr->op1=0;iptr->val.d=(v);PINC
#define LOADCONST_A(v) iptr->opc=ICMD_ACONST;iptr->op1=0;iptr->val.a=(v);PINC
#define OP(o)          iptr->opc=(o);iptr->op1=0;iptr->val.l=0;PINC
#define OP1(o,o1)      iptr->opc=(o);iptr->op1=(o1);iptr->val.l=(0);PINC
#define OP2I(o,o1,v)   iptr->opc=(o);iptr->op1=(o1);iptr->val.i=(v);PINC
#define OP2A(o,o1,v)   iptr->opc=(o);iptr->op1=(o1);iptr->val.a=(v);PINC
#define BUILTIN1(v,t)  isleafmethod=false;iptr->opc=ICMD_BUILTIN1;iptr->op1=t;\
                       iptr->val.a=(v);PINC
#define BUILTIN2(v,t)  isleafmethod=false;iptr->opc=ICMD_BUILTIN2;iptr->op1=t;\
                       iptr->val.a=(v);PINC
#define BUILTIN3(v,t)  isleafmethod=false;iptr->opc=ICMD_BUILTIN3;iptr->op1=t;\
                       iptr->val.a=(v);PINC


/* block generating and checking macros */

#define block_insert(i)    {if(!(block_index[i]&1))\
                               {b_count++;block_index[i] |= 1;}}
#define bound_check(i)     {if((i< 0) || (i>=jcodelength)) \
                               panic("branch target out of code-boundary");}
#define bound_check1(i)    {if((i< 0) || (i>jcodelength)) \
                               panic("branch target out of code-boundary");}


static void parse()
{
	int  p;                     /* java instruction counter                   */
	int  nextp;                 /* start of next java instruction             */
	int  opcode;                /* java opcode                                */
	int  i;                     /* temporary for different uses (counters)    */
	int  ipc = 0;               /* intermediate instruction counter           */
	int  b_count = 0;           /* basic block counter                        */
	int  s_count = 0;           /* stack element counter                      */
	bool blockend = false;      /* true if basic block end has reached        */
	bool iswide = false;        /* true if last instruction was a wide        */
	instruction *iptr;          /* current pointer into instruction array     */


	/* allocate instruction array and block index table */
	
	/* 1 additional for end ipc and 3 for loop unrolling */
	
	block_index = DMNEW(int, jcodelength + 3);

	/* 1 additional for TRACEBUILTIN and 4 for MONITORENTER/EXIT */
	/* additional MONITOREXITS are reached by branches which are 3 bytes */
	
	iptr = instr = DMNEW(instruction, jcodelength + 5);
	
	/* initialize block_index table (unrolled four times) */

	{
	int *ip;
	
	for (i = 0, ip = block_index; i <= jcodelength; i += 4, ip += 4) {
		ip[0] = 0;
		ip[1] = 0;
		ip[2] = 0;
		ip[3] = 0;
		}
	}

	/* compute branch targets of exception table */

	for (i = 0; i < exceptiontablelength; i++) {
		p = extable[i].startpc;
		bound_check(p);
		block_insert(p);
		p = extable[i].endpc;
		bound_check1(p);
		if (p < jcodelength)
			block_insert(p);
		p = extable[i].handlerpc;
		bound_check(p);
		block_insert(p);
		}

	s_count = 1 + exceptiontablelength; /* initialize stack element counter   */

	if (runverbose) {
/*		isleafmethod=false; */
		}

#ifdef USE_THREADS
	if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
		isleafmethod=false;
		}			
#endif

	/* scan all java instructions */

	for (p = 0; p < jcodelength; p = nextp) {

		opcode = code_get_u1 (p);           /* fetch op code                  */

		block_index[p] |= (ipc << 1);       /* store intermediate count       */

		if (blockend) {
			block_insert(p);                /* start new block                */
			blockend = false;
			}

		nextp = p + jcommandsize[opcode];   /* compute next instruction start */
		s_count += stackreq[opcode];      	/* compute stack element count    */

		switch (opcode) {

			case JAVA_NOP:
				break;

			/* pushing constants onto the stack p */

			case JAVA_BIPUSH:
				LOADCONST_I(code_get_s1(p+1));
				break;

			case JAVA_SIPUSH:
				LOADCONST_I(code_get_s2(p+1));
				break;

			case JAVA_LDC1:
				i = code_get_u1(p+1);
				goto pushconstantitem;
			case JAVA_LDC2:
			case JAVA_LDC2W:
				i = code_get_u2(p + 1);

			pushconstantitem:

				if (i >= class->cpcount) 
					panic ("Attempt to access constant outside range");

				switch (class->cptags[i]) {
					case CONSTANT_Integer:
						LOADCONST_I(((constant_integer*)
						             (class->cpinfos[i]))->value);
						break;
					case CONSTANT_Long:
						LOADCONST_L(((constant_long*)
						             (class->cpinfos[i]))->value);
						break;
					case CONSTANT_Float:
						LOADCONST_F(((constant_float*)
						             (class->cpinfos[i]))->value);
						break;
					case CONSTANT_Double:
						LOADCONST_D(((constant_double*)
						             (class->cpinfos[i]))->value);
						break;
					case CONSTANT_String:
						LOADCONST_A(literalstring_new((unicode*)
						                              (class->cpinfos[i])));
						break;
					default: panic("Invalid constant type to push");
					}
				break;

			case JAVA_ACONST_NULL:
				LOADCONST_A(NULL);
				break;

			case JAVA_ICONST_M1:
			case JAVA_ICONST_0:
			case JAVA_ICONST_1:
			case JAVA_ICONST_2:
			case JAVA_ICONST_3:
			case JAVA_ICONST_4:
			case JAVA_ICONST_5:
				LOADCONST_I(opcode - JAVA_ICONST_0);
				break;

			case JAVA_LCONST_0:
			case JAVA_LCONST_1:
				LOADCONST_L(opcode - JAVA_LCONST_0);
				break;

			case JAVA_FCONST_0:
			case JAVA_FCONST_1:
			case JAVA_FCONST_2:
				LOADCONST_F(opcode - JAVA_FCONST_0);
				break;

			case JAVA_DCONST_0:
			case JAVA_DCONST_1:
				LOADCONST_D(opcode - JAVA_DCONST_0);
				break;

			/* loading variables onto the stack */

			case JAVA_ILOAD:
			case JAVA_LLOAD:
			case JAVA_FLOAD:
			case JAVA_DLOAD:
			case JAVA_ALOAD:
				if (!iswide)
					i = code_get_u1(p+1);
				else {
					i = code_get_u2(p+1);
					nextp = p+3;
					iswide = false;
					}
				OP1(opcode, i);
				break;

			case JAVA_ILOAD_0:
			case JAVA_ILOAD_1:
			case JAVA_ILOAD_2:
			case JAVA_ILOAD_3:
				OP1(ICMD_ILOAD, opcode - JAVA_ILOAD_0);
				break;

			case JAVA_LLOAD_0:
			case JAVA_LLOAD_1:
			case JAVA_LLOAD_2:
			case JAVA_LLOAD_3:
				OP1(ICMD_LLOAD, opcode - JAVA_LLOAD_0);
				break;

			case JAVA_FLOAD_0:
			case JAVA_FLOAD_1:
			case JAVA_FLOAD_2:
			case JAVA_FLOAD_3:
				OP1(ICMD_FLOAD, opcode - JAVA_FLOAD_0);
				break;

			case JAVA_DLOAD_0:
			case JAVA_DLOAD_1:
			case JAVA_DLOAD_2:
			case JAVA_DLOAD_3:
				OP1(ICMD_DLOAD, opcode - JAVA_DLOAD_0);
				break;

			case JAVA_ALOAD_0:
			case JAVA_ALOAD_1:
			case JAVA_ALOAD_2:
			case JAVA_ALOAD_3:
				OP1(ICMD_ALOAD, opcode - JAVA_ALOAD_0);
				break;

			/* storing stack values into local variables */

			case JAVA_ISTORE:
			case JAVA_LSTORE:
			case JAVA_FSTORE:
			case JAVA_DSTORE:
			case JAVA_ASTORE:
				if (!iswide)
					i = code_get_u1(p+1);
				else {
					i = code_get_u2(p+1);
					iswide=false;
					nextp = p+3;
					}
				OP1(opcode, i);
				break;

			case JAVA_ISTORE_0:
			case JAVA_ISTORE_1:
			case JAVA_ISTORE_2:
			case JAVA_ISTORE_3:
				OP1(ICMD_ISTORE, opcode - JAVA_ISTORE_0);
				break;

			case JAVA_LSTORE_0:
			case JAVA_LSTORE_1:
			case JAVA_LSTORE_2:
			case JAVA_LSTORE_3:
				OP1(ICMD_LSTORE, opcode - JAVA_LSTORE_0);
				break;

			case JAVA_FSTORE_0:
			case JAVA_FSTORE_1:
			case JAVA_FSTORE_2:
			case JAVA_FSTORE_3:
				OP1(ICMD_FSTORE, opcode - JAVA_FSTORE_0);
				break;

			case JAVA_DSTORE_0:
			case JAVA_DSTORE_1:
			case JAVA_DSTORE_2:
			case JAVA_DSTORE_3:
				OP1(ICMD_DSTORE, opcode - JAVA_DSTORE_0);
				break;

			case JAVA_ASTORE_0:
			case JAVA_ASTORE_1:
			case JAVA_ASTORE_2:
			case JAVA_ASTORE_3:
				OP1(ICMD_ASTORE, opcode - JAVA_ASTORE_0);
				break;

			case JAVA_IINC:
				{
				int v;
				
				if (!iswide) {
					i = code_get_u1(p + 1);
					v = code_get_s1(p + 2);
					}
				else {
					i = code_get_u2(p + 1);
					v = code_get_s2(p + 3);
					iswide = false;
					nextp = p+5;
					}
				OP2I(opcode, i, v);
				}
				break;

			/* wider index for loading, storing and incrementing */

			case JAVA_WIDE:
				iswide = true;
				nextp = p + 1;
				break;

			/*********************** managing arrays **************************/

			case JAVA_NEWARRAY:
				OP2I(ICMD_CHECKASIZE, 0, 0);
				switch (code_get_s1(p+1)) {
					case 4:
						BUILTIN1((functionptr)builtin_newarray_boolean, TYPE_ADR);
						break;
					case 5:
						BUILTIN1((functionptr)builtin_newarray_char, TYPE_ADR);
						break;
					case 6:
						BUILTIN1((functionptr)builtin_newarray_float, TYPE_ADR);
						break;
					case 7:
						BUILTIN1((functionptr)builtin_newarray_double, TYPE_ADR);
						break;
					case 8:
						BUILTIN1((functionptr)builtin_newarray_byte, TYPE_ADR);
						break;
					case 9:
						BUILTIN1((functionptr)builtin_newarray_short, TYPE_ADR);
						break;
					case 10:
						BUILTIN1((functionptr)builtin_newarray_int, TYPE_ADR);
						break;
					case 11:
						BUILTIN1((functionptr)builtin_newarray_long, TYPE_ADR);
						break;
					default: panic("Invalid array-type to create");
					}
				break;

			case JAVA_ANEWARRAY:
				OP2I(ICMD_CHECKASIZE, 0, 0);
				i = code_get_u2(p+1);
				/* array or class type ? */
				if (class_constanttype (class, i) == CONSTANT_Arraydescriptor) {
					LOADCONST_A(class_getconstant(class, i,
					                              CONSTANT_Arraydescriptor));
					BUILTIN2((functionptr)builtin_newarray_array, TYPE_ADR);
					}
				else {
				 	LOADCONST_A(class_getconstant(class, i, CONSTANT_Class));
					BUILTIN2((functionptr)builtin_anewarray, TYPE_ADR);
					}
				break;

			case JAVA_MULTIANEWARRAY:
				isleafmethod=false;
				i = code_get_u2(p+1);
				{
				int v = code_get_u1(p+3);
				constant_arraydescriptor *desc =
				    class_getconstant (class, i, CONSTANT_Arraydescriptor);
				OP2A(opcode, v, desc);
				}
				break;

			case JAVA_IFEQ:
			case JAVA_IFLT:
			case JAVA_IFLE:
			case JAVA_IFNE:
			case JAVA_IFGT:
			case JAVA_IFGE:
			case JAVA_IFNULL:
			case JAVA_IFNONNULL:
			case JAVA_IF_ICMPEQ:
			case JAVA_IF_ICMPNE:
			case JAVA_IF_ICMPLT:
			case JAVA_IF_ICMPGT:
			case JAVA_IF_ICMPLE:
			case JAVA_IF_ICMPGE:
			case JAVA_IF_ACMPEQ:
			case JAVA_IF_ACMPNE:
			case JAVA_GOTO:
			case JAVA_JSR:
				i = p + code_get_s2(p+1);
				bound_check(i);
				block_insert(i);
				blockend = true;
				OP1(opcode, i);
				break;
			case JAVA_GOTO_W:
			case JAVA_JSR_W:
				i = p + code_get_s4(p+1);
				bound_check(i);
				block_insert(i);
				blockend = true;
				OP1(opcode, i);
				break;

			case JAVA_RET:
				if (!iswide)
					i = code_get_u1(p+1);
				else {
					i = code_get_u2(p+1);
					nextp = p+3;
					iswide = false;
					}
				blockend = true;
				OP1(opcode, i);
				break;

			case JAVA_IRETURN:
			case JAVA_LRETURN:
			case JAVA_FRETURN:
			case JAVA_DRETURN:
			case JAVA_ARETURN:
			case JAVA_RETURN:
				blockend = true;
				OP(opcode);
				break;

			case JAVA_ATHROW:
				blockend = true;
				OP(opcode);
				break;
				

			/**************** table jumps *****************/

			case JAVA_LOOKUPSWITCH:
				{
				s4 num, j;

				blockend = true;
				nextp = ALIGN((p + 1), 4);
				OP2A(opcode, 0, jcode + nextp);

				/* default target */

				j =  p + code_get_s4(nextp);
				*((s4*)(jcode + nextp)) = j;     /* restore for little endian */
				nextp += 4;
				bound_check(j);
				block_insert(j);

				/* number of pairs */

				num = code_get_u4(nextp);
				*((s4*)(jcode + nextp)) = num;
				nextp += 4;

				for (i = 0; i < num; i++) {

					/* value */

					j = code_get_s4(nextp);
					*((s4*)(jcode + nextp)) = j; /* restore for little endian */
					nextp += 4;

					/* target */

					j = p + code_get_s4(nextp);
					*((s4*)(jcode + nextp)) = j; /* restore for little endian */
					nextp += 4;
					bound_check(j);
					block_insert(j);
					}

				break;
				}


			case JAVA_TABLESWITCH:
				{
				s4 num, j;

				blockend = true;
				nextp = ALIGN((p + 1), 4);
				OP2A(opcode, 0, jcode + nextp);

				/* default target */

				j = p + code_get_s4(nextp);
				*((s4*)(jcode + nextp)) = j;     /* restore for little endian */
				nextp += 4;
				bound_check(j);
				block_insert(j);

				/* lower bound */

				j = code_get_s4(nextp);
				*((s4*)(jcode + nextp)) = j;     /* restore for little endian */
				nextp += 4;

				/* upper bound */

				num = code_get_s4(nextp);
				*((s4*)(jcode + nextp)) = num;   /* restore for little endian */
				nextp += 4;

				num -= j;

				for (i = 0; i <= num; i++) {
					j = p + code_get_s4(nextp);
					*((s4*)(jcode + nextp)) = j; /* restore for little endian */
					nextp += 4;
					bound_check(j);
					block_insert(j);
					}

				break;
				}


			/************ load and store of object fields ********/

			case JAVA_AASTORE:
				BUILTIN3((functionptr) asm_builtin_aastore, TYPE_VOID);
				break;

			case JAVA_PUTSTATIC:
			case JAVA_GETSTATIC:
				i = code_get_u2(p + 1);
				{
				constant_FMIref *fr;
				fieldinfo *fi;
				fr = class_getconstant (class, i, CONSTANT_Fieldref);
				fi = class_findfield (fr->class, fr->name, fr->descriptor);
				compiler_addinitclass (fr->class);
				OP2A(opcode, fi->type, fi);
				}
				break;
			case JAVA_PUTFIELD:
			case JAVA_GETFIELD:
				i = code_get_u2(p + 1);
				{
				constant_FMIref *fr;
				fieldinfo *fi;
				fr = class_getconstant (class, i, CONSTANT_Fieldref);
				fi = class_findfield (fr->class, fr->name, fr->descriptor);
				OP2A(opcode, fi->type, fi);
				}
				break;


			/*** method invocation ***/

			case JAVA_INVOKESTATIC:
				i = code_get_u2(p + 1);
				{
				constant_FMIref *mr;
				methodinfo *mi;
				
				mr = class_getconstant (class, i, CONSTANT_Methodref);
				mi = class_findmethod (mr->class, mr->name, mr->descriptor);
				if (! (mi->flags & ACC_STATIC))
					panic ("Static/Nonstatic mismatch calling static method");
				descriptor2types(mi);
				isleafmethod=false;
				OP2A(opcode, mi->paramcount, mi);
				}
				break;
			case JAVA_INVOKESPECIAL:
			case JAVA_INVOKEVIRTUAL:
				i = code_get_u2(p + 1);
				{
				constant_FMIref *mr;
				methodinfo *mi;
				
				mr = class_getconstant (class, i, CONSTANT_Methodref);
				mi = class_findmethod (mr->class, mr->name, mr->descriptor);
				if (mi->flags & ACC_STATIC)
					panic ("Static/Nonstatic mismatch calling static method");
				descriptor2types(mi);
				isleafmethod=false;
				OP2A(opcode, mi->paramcount, mi);
				}
				break;
			case JAVA_INVOKEINTERFACE:
				i = code_get_u2(p + 1);
				{
				constant_FMIref *mr;
				methodinfo *mi;
				
				mr = class_getconstant (class, i, CONSTANT_InterfaceMethodref);
				mi = class_findmethod (mr->class, mr->name, mr->descriptor);
				if (mi->flags & ACC_STATIC)
					panic ("Static/Nonstatic mismatch calling static method");
				descriptor2types(mi);
				isleafmethod=false;
				OP2A(opcode, mi->paramcount, mi);
				}
				break;

			/***** miscellaneous object operations ****/

			case JAVA_NEW:
				i = code_get_u2 (p+1);
				LOADCONST_A(class_getconstant(class, i, CONSTANT_Class));
				BUILTIN1((functionptr) builtin_new, TYPE_ADR);
				break;

			case JAVA_CHECKCAST:
				i = code_get_u2(p+1);

				/* array type cast-check */
				if (class_constanttype (class, i) == CONSTANT_Arraydescriptor) {
					LOADCONST_A(class_getconstant(class, i, CONSTANT_Arraydescriptor));
					BUILTIN2((functionptr) asm_builtin_checkarraycast, TYPE_ADR);
					}
				else { /* object type cast-check */
				 	OP2A(opcode, 1, (class_getconstant(class, i, CONSTANT_Class)));
					}
				break;

			case JAVA_INSTANCEOF:
				i = code_get_u2(p+1);

				/* array type cast-check */
				if (class_constanttype (class, i) == CONSTANT_Arraydescriptor) {
					LOADCONST_A(class_getconstant(class, i, CONSTANT_Arraydescriptor));
					BUILTIN2((functionptr) builtin_arrayinstanceof, TYPE_INT);
					}
				else { /* object type cast-check */
				 	OP2A(opcode, 1, (class_getconstant(class, i, CONSTANT_Class)));
					}
				break;

			case JAVA_MONITORENTER:
#ifdef USE_THREADS
				if (checksync) {
#ifdef SOFTNULLPTRCHECK
					if (checknull) {
						BUILTIN1((functionptr) asm_builtin_monitorenter, TYPE_VOID);
						}
					else {
/*						BUILTIN1((functionptr) builtin_monitorenter, TYPE_VOID); */
						BUILTIN1((functionptr) asm_builtin_monitorenter, TYPE_VOID);
						}
#else
					BUILTIN1((functionptr) builtin_monitorenter, TYPE_VOID);
#endif
					}
				else
#endif
					{
					OP(ICMD_NULLCHECKPOP);
					}
				break;

			case JAVA_MONITOREXIT:
#ifdef USE_THREADS
				if (checksync) {
					BUILTIN1((functionptr) builtin_monitorexit, TYPE_VOID);
					}
				else
#endif
					{
					OP(ICMD_POP);
					}
				break;

			/************** any other basic operation **********/

			case JAVA_IDIV:
				OP(opcode);
				break;

			case JAVA_IREM:
				OP(opcode);
				break;

			case JAVA_LDIV:
				OP(opcode);
				break;

			case JAVA_LREM:
				OP(opcode);
				break;

			case JAVA_FREM:
				BUILTIN2((functionptr) builtin_frem, TYPE_FLOAT);
				break;

			case JAVA_DREM:
				BUILTIN2((functionptr) builtin_drem, TYPE_DOUBLE);
				break;

			case JAVA_F2I:
				if (checkfloats) {
					BUILTIN1((functionptr) builtin_f2i, TYPE_INT);
					}
				else {
					OP(opcode);
					}
				break;

			case JAVA_F2L:
				if (checkfloats) {
					BUILTIN1((functionptr) builtin_f2l, TYPE_LONG);
					}
				else {
					OP(opcode);
					}
				break;

			case JAVA_D2I:
				if (checkfloats) {
					BUILTIN1((functionptr) builtin_d2i, TYPE_INT);
					}
				else {
					OP(opcode);
					}
				break;

			case JAVA_D2L:
				if (checkfloats) {
					BUILTIN1((functionptr) builtin_d2l, TYPE_LONG);
					}
				else {
					OP(opcode);
					}
				break;

			case JAVA_BREAKPOINT:
				panic("Illegal opcode Breakpoint encountered");
				break;

			case 203:
			case 204:
			case 205:
			case 206:
			case 207:
			case 208:
			case 209:
			case 210:
			case 211:
			case 212:
			case 213:
			case 214:
			case 215:
			case 216:
			case 217:
			case 218:
			case 219:
			case 220:
			case 221:
			case 222:
			case 223:
			case 224:
			case 225:
			case 226:
			case 227:
			case 228:
			case 229:
			case 230:
			case 231:
			case 232:
			case 233:
			case 234:
			case 235:
			case 236:
			case 237:
			case 238:
			case 239:
			case 240:
			case 241:
			case 242:
			case 243:
			case 244:
			case 245:
			case 246:
			case 247:
			case 248:
			case 249:
			case 250:
			case 251:
			case 252:
			case 253:
			case 254:
			case 255:
				printf("Illegal opcode %d at instr %d", opcode, ipc);
				panic("encountered");
				break;

			default:
				OP(opcode);
				break;

			} /* end switch */

		} /* end for */

	if (p != jcodelength)
		panic("Command-sequence crosses code-boundary");

	if (!blockend)
		panic("Code does not end with branch/return/athrow - stmt");	

	/* adjust block count if target 0 is not first intermediate instruction   */

	if (!block_index[0] || (block_index[0] > 1))
		b_count++;

	/* copy local to global variables   */

	instr_count = ipc;
	block_count = b_count;
	stack_count = s_count + block_count * maxstack;

	/* allocate stack table */

	stack = DMNEW(stackelement, stack_count);

	{
	basicblock  *bptr;

	bptr = block = DMNEW(basicblock, b_count + 1);    /* one more for end ipc */

	b_count = 0;
	
	/* additional block if target 0 is not first intermediate instruction     */

	if (!block_index[0] || (block_index[0] > 1)) {
		bptr->ipc = 0;
		bptr->mpc = -1;
		bptr->flags = -1;
		bptr->type = BBTYPE_STD;
		bptr->branchrefs = NULL;
		bptr->pre_count = 0;
		bptr++;
		b_count++;
		}

	/* allocate blocks */

	for (p = 0; p < jcodelength; p++)
		if (block_index[p] & 1) {
			bptr->ipc = block_index[p] >> 1;
			bptr->mpc = -1;
			bptr->flags = -1;
			bptr->type = BBTYPE_STD;
			bptr->branchrefs = NULL;
			block_index[p] = b_count;
			bptr->pre_count = 0;
			bptr++;
			b_count++;
			}

	/* allocate additional block at end */

	bptr->ipc = instr_count;
	bptr->mpc = -1;
	bptr->flags = -1;
	bptr->type = BBTYPE_STD;
	bptr->branchrefs = NULL;
	bptr->pre_count = 0;
	}
}
