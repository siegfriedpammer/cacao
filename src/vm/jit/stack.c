/* jit/stack.c *****************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Parser for JavaVM to intermediate code translation
	
	Authors: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/11/18

*******************************************************************************/

#ifdef STATISTICS
#define COUNT(cnt) cnt++
#else
#define COUNT(cnt)
#endif

#define STACKRESET {curstack=0;stackdepth=0;}

#define TYPEPANIC  {show_icmd_method();panic("Stack type mismatch");}
#define CURKIND    curstack->varkind
#define CURTYPE    curstack->type

#define NEWSTACK(s,v,n) {new->prev=curstack;new->type=s;new->flags=0;\
                        new->varkind=v;new->varnum=n;curstack=new;new++;}
#define NEWSTACKn(s,n)  NEWSTACK(s,UNDEFVAR,n)
#define NEWSTACK0(s)    NEWSTACK(s,UNDEFVAR,0)
#define NEWXSTACK   {NEWSTACK(TYPE_ADR,STACKVAR,0);curstack=0;}

#define SETDST      {iptr->dst=curstack;}
#define POP(s)      {if(s!=curstack->type){TYPEPANIC;}\
                     if(curstack->varkind==UNDEFVAR)curstack->varkind=TEMPVAR;\
                     curstack=curstack->prev;}
#define POPANY      {if(curstack->varkind==UNDEFVAR)curstack->varkind=TEMPVAR;\
                     curstack=curstack->prev;}
#define COPY(s,d)   {(d)->flags=0;(d)->type=(s)->type;\
                     (d)->varkind=(s)->varkind;(d)->varnum=(s)->varnum;}

#define PUSHCONST(s){NEWSTACKn(s,stackdepth);SETDST;stackdepth++;}
#define LOAD(s,v,n) {NEWSTACK(s,v,n);SETDST;stackdepth++;}
#define STORE(s)    {POP(s);SETDST;stackdepth--;}
#define OP1_0(s)    {POP(s);SETDST;stackdepth--;}
#define OP1_0ANY    {POPANY;SETDST;stackdepth--;}
#define OP0_1(s)    {NEWSTACKn(s,stackdepth);SETDST;stackdepth++;}
#define OP1_1(s,d)  {POP(s);NEWSTACKn(d,stackdepth-1);SETDST;}
#define OP2_0(s)    {POP(s);POP(s);SETDST;stackdepth-=2;}
#define OPTT2_0(t,b){POP(t);POP(b);SETDST;stackdepth-=2;}
#define OP2_1(s)    {POP(s);POP(s);NEWSTACKn(s,stackdepth-2);SETDST;stackdepth--;}
#define OP2IAT_1(s) {POP(TYPE_INT);POP(TYPE_ADR);NEWSTACKn(s,stackdepth-2);\
                     SETDST;stackdepth--;}
#define OP2IT_1(s)  {POP(TYPE_INT);POP(s);NEWSTACKn(s,stackdepth-2);\
                     SETDST;stackdepth--;}
#define OPTT2_1(s,d){POP(s);POP(s);NEWSTACKn(d,stackdepth-2);SETDST;stackdepth--;}
#define OP2_2(s)    {POP(s);POP(s);NEWSTACKn(s,stackdepth-2);\
                     NEWSTACKn(s,stackdepth-1);SETDST;}
#define OP3TIA_0(s) {POP(s);POP(TYPE_INT);POP(TYPE_ADR);SETDST;stackdepth-=3;}
#define OP3_0(s)    {POP(s);POP(s);POP(s);SETDST;stackdepth-=3;}
#define POPMANY(i)  {stackdepth-=i;while(--i>=0){POPANY;}SETDST;}
#define DUP         {NEWSTACK(CURTYPE,CURKIND,curstack->varnum);SETDST;\
                    stackdepth++;}
#define SWAP        {COPY(curstack,new);POPANY;COPY(curstack,new+1);POPANY;\
                    new[0].prev=curstack;new[1].prev=new;\
                    curstack=new+1;new+=2;SETDST;}
#define DUP_X1      {COPY(curstack,new);COPY(curstack,new+2);POPANY;\
                    COPY(curstack,new+1);POPANY;new[0].prev=curstack;\
                    new[1].prev=new;new[2].prev=new+1;\
                    curstack=new+2;new+=3;SETDST;stackdepth++;}
#define DUP2_X1     {COPY(curstack,new+1);COPY(curstack,new+4);POPANY;\
                    COPY(curstack,new);COPY(curstack,new+3);POPANY;\
                    COPY(curstack,new+2);POPANY;new[0].prev=curstack;\
                    new[1].prev=new;new[2].prev=new+1;\
                    new[3].prev=new+2;new[4].prev=new+3;\
                    curstack=new+4;new+=5;SETDST;stackdepth+=2;}
#define DUP_X2      {COPY(curstack,new);COPY(curstack,new+3);POPANY;\
                    COPY(curstack,new+2);POPANY;COPY(curstack,new+1);POPANY;\
                    new[0].prev=curstack;new[1].prev=new;\
                    new[2].prev=new+1;new[3].prev=new+2;\
                    curstack=new+3;new+=4;SETDST;stackdepth++;}
#define DUP2_X2     {COPY(curstack,new+1);COPY(curstack,new+5);POPANY;\
                    COPY(curstack,new);COPY(curstack,new+4);POPANY;\
                    COPY(curstack,new+3);POPANY;COPY(curstack,new+2);POPANY;\
                    new[0].prev=curstack;new[1].prev=new;\
                    new[2].prev=new+1;new[3].prev=new+2;\
                    new[4].prev=new+3;new[5].prev=new+4;\
                    curstack=new+5;new+=6;SETDST;stackdepth+=2;}

#define COPYCURSTACK(copy) {\
	int d;\
	stackptr s;\
	if(curstack){\
		s=curstack;\
		new+=stackdepth;\
		d=stackdepth;\
		copy=new;\
		while(s){\
			copy--;d--;\
			copy->prev=copy-1;\
			copy->type=s->type;\
			copy->flags=0;\
			copy->varkind=STACKVAR;\
			copy->varnum=d;\
			s=s->prev;\
			}\
		copy->prev=NULL;\
		copy=new-1;\
		}\
	else\
		copy=NULL;\
}


#define BBEND(s,i){\
	i=stackdepth-1;\
	copy=s;\
	while(copy){\
		if((copy->varkind==STACKVAR)&&(copy->varnum>i))\
			copy->varkind=TEMPVAR;\
		else {\
			copy->varkind=STACKVAR;\
			copy->varnum=i;\
			}\
		interfaces[i][copy->type].type = copy->type;\
		interfaces[i][copy->type].flags |= copy->flags;\
		i--;copy=copy->prev;\
		}\
	i=bptr->indepth-1;\
	copy=bptr->instack;\
	while(copy){\
		interfaces[i][copy->type].type = copy->type;\
		if(copy->varkind==STACKVAR){\
			if (copy->flags & SAVEDVAR)\
				interfaces[i][copy->type].flags |= SAVEDVAR;\
			}\
		i--;copy=copy->prev;\
		}\
}

	
#define MARKREACHED(b,c) {\
	if(b->flags<0)\
		{COPYCURSTACK(c);b->flags=0;b->instack=c;b->indepth=stackdepth;}\
	else {stackptr s=curstack;stackptr t=b->instack;\
		if(b->indepth!=stackdepth)\
			{show_icmd_method();panic("Stack depth mismatch");}\
		while(s){if (s->type!=t->type)\
				TYPEPANIC\
			s=s->prev;t=t->prev;\
			}\
		}\
}


static void show_icmd_method();

static void analyse_stack()
{
	int b_count, b_index;
	int stackdepth;
	stackptr curstack, new, copy;
	int opcode, i, len, loops;
	int superblockend, repeat, deadcode;
	instruction *iptr = instr;
	basicblock *bptr, *tbptr;
	s4  *s4ptr;
	
	arguments_num = 0;
	new = stack;
	loops = 0;
	block[0].flags = BBREACHED;
	block[0].instack = 0;
	block[0].indepth = 0;

	for (i = 0; i < exceptiontablelength; i++) {
		bptr = &block[block_index[extable[i].handlerpc]];
		bptr->flags = BBREACHED;
		bptr->type = BBTYPE_EXH;
		bptr->instack = new;
		bptr->indepth = 1;
		bptr->pre_count = 10000;
		STACKRESET;
		NEWXSTACK;
		}

#ifdef CONDITIONAL_LOADCONST
	b_count = block_count;
	bptr = block;
	while (--b_count >= 0) {
		if (bptr->icount != 0) {
			iptr = bptr->iinstr + bptr->icount - 1;
			switch (iptr->opc) {
				case ICMD_RET:
				case ICMD_RETURN:
				case ICMD_IRETURN:
				case ICMD_LRETURN:
				case ICMD_FRETURN:
				case ICMD_DRETURN:
				case ICMD_ARETURN:
				case ICMD_ATHROW:
					break;

				case ICMD_IFEQ:
				case ICMD_IFNE:
				case ICMD_IFLT:
				case ICMD_IFGE:
				case ICMD_IFGT:
				case ICMD_IFLE:

				case ICMD_IFNULL:
				case ICMD_IFNONNULL:

				case ICMD_IF_ICMPEQ:
				case ICMD_IF_ICMPNE:
				case ICMD_IF_ICMPLT:
				case ICMD_IF_ICMPGE:
				case ICMD_IF_ICMPGT:
				case ICMD_IF_ICMPLE:

				case ICMD_IF_ACMPEQ:
				case ICMD_IF_ACMPNE:
					bptr[1].pre_count++;
				case ICMD_GOTO:
					block[block_index[iptr->op1]].pre_count++;
					break;

				case ICMD_TABLESWITCH:
					s4ptr = iptr->val.a;
					block[block_index[*s4ptr++]].pre_count++;   /* default */
					i = *s4ptr++;                               /* low     */
					i = *s4ptr++ - i + 1;                       /* high    */
					while (--i >= 0) {
						block[block_index[*s4ptr++]].pre_count++;
						}
					break;
					
				case ICMD_LOOKUPSWITCH:
					s4ptr = iptr->val.a;
					block[block_index[*s4ptr++]].pre_count++;   /* default */
					i = *s4ptr++;                               /* count   */
					while (--i >= 0) {
						block[block_index[s4ptr[1]]].pre_count++;
						s4ptr += 2;
						}
					break;
				default:
					bptr[1].pre_count++;
					break;
				}
			}
		bptr++;
		}
#endif


	do {
		loops++;
		b_count = block_count;
		bptr = block;
		superblockend = true;
		repeat = false;
		STACKRESET;
		deadcode = true;
		while (--b_count >= 0) {
			if (bptr->flags == BBDELETED) {
				/* do nothing */
				}
			else if (superblockend && (bptr->flags < BBREACHED))
				repeat = true;
			else if (bptr->flags <= BBREACHED) {
				if (superblockend)
					stackdepth = bptr->indepth;
				else if (bptr->flags < BBREACHED) {
					COPYCURSTACK(copy);
					bptr->instack = copy;
					bptr->indepth = stackdepth;
					}
				else if (bptr->indepth != stackdepth) {
					show_icmd_method();
					panic("Stack depth mismatch");
					
					}
				curstack = bptr->instack;
				deadcode = false;
				superblockend = false;
				bptr->flags = BBFINISHED;
				len = bptr->icount;
				iptr = bptr->iinstr;
				b_index = bptr - block;
				while (--len >= 0)  {
					opcode = iptr->opc;
					switch (opcode) {

						/* pop 0 push 0 */

						case ICMD_NOP:
						case ICMD_CHECKASIZE:

						case ICMD_IFEQ_ICONST:
						case ICMD_IFNE_ICONST:
						case ICMD_IFLT_ICONST:
						case ICMD_IFGE_ICONST:
						case ICMD_IFGT_ICONST:
						case ICMD_IFLE_ICONST:
						case ICMD_ELSE_ICONST:
							SETDST;
							break;

						case ICMD_RET:
							locals[iptr->op1][TYPE_ADR].type = TYPE_ADR;
						case ICMD_RETURN:
							COUNT(count_pcmd_return);
							SETDST;
							superblockend = true;
							break;

						/* pop 0 push 1 const */
						
						case ICMD_ICONST:
							COUNT(count_pcmd_load);
							if (len > 0) {
								switch (iptr[1].opc) {
									case ICMD_IADD:
										iptr[0].opc = ICMD_IADDCONST;
icmd_iconst_tail:
										iptr[1].opc = ICMD_NOP;
										OP1_1(TYPE_INT,TYPE_INT);
										COUNT(count_pcmd_op);
										break;
									case ICMD_ISUB:
										iptr[0].opc = ICMD_ISUBCONST;
										goto icmd_iconst_tail;
									case ICMD_IMUL:
										iptr[0].opc = ICMD_IMULCONST;
										goto icmd_iconst_tail;
									case ICMD_IDIV:
										if (iptr[0].val.i == 0x00000002)
											iptr[0].val.i = 1;
										else if (iptr[0].val.i == 0x00000004)
											iptr[0].val.i = 2;
										else if (iptr[0].val.i == 0x00000008)
											iptr[0].val.i = 3;
										else if (iptr[0].val.i == 0x00000010)
											iptr[0].val.i = 4;
										else if (iptr[0].val.i == 0x00000020)
											iptr[0].val.i = 5;
										else if (iptr[0].val.i == 0x00000040)
											iptr[0].val.i = 6;
										else if (iptr[0].val.i == 0x00000080)
											iptr[0].val.i = 7;
										else if (iptr[0].val.i == 0x00000100)
											iptr[0].val.i = 8;
										else if (iptr[0].val.i == 0x00000200)
											iptr[0].val.i = 9;
										else if (iptr[0].val.i == 0x00000400)
											iptr[0].val.i = 10;
										else if (iptr[0].val.i == 0x00000800)
											iptr[0].val.i = 11;
										else if (iptr[0].val.i == 0x00001000)
											iptr[0].val.i = 12;
										else if (iptr[0].val.i == 0x00002000)
											iptr[0].val.i = 13;
										else if (iptr[0].val.i == 0x00004000)
											iptr[0].val.i = 14;
										else if (iptr[0].val.i == 0x00008000)
											iptr[0].val.i = 15;
										else if (iptr[0].val.i == 0x00010000)
											iptr[0].val.i = 16;
										else if (iptr[0].val.i == 0x00020000)
											iptr[0].val.i = 17;
										else if (iptr[0].val.i == 0x00040000)
											iptr[0].val.i = 18;
										else if (iptr[0].val.i == 0x00080000)
											iptr[0].val.i = 19;
										else if (iptr[0].val.i == 0x00100000)
											iptr[0].val.i = 20;
										else if (iptr[0].val.i == 0x00200000)
											iptr[0].val.i = 21;
										else if (iptr[0].val.i == 0x00400000)
											iptr[0].val.i = 22;
										else if (iptr[0].val.i == 0x00800000)
											iptr[0].val.i = 23;
										else if (iptr[0].val.i == 0x01000000)
											iptr[0].val.i = 24;
										else if (iptr[0].val.i == 0x02000000)
											iptr[0].val.i = 25;
										else if (iptr[0].val.i == 0x04000000)
											iptr[0].val.i = 26;
										else if (iptr[0].val.i == 0x08000000)
											iptr[0].val.i = 27;
										else if (iptr[0].val.i == 0x10000000)
											iptr[0].val.i = 28;
										else if (iptr[0].val.i == 0x20000000)
											iptr[0].val.i = 29;
										else if (iptr[0].val.i == 0x40000000)
											iptr[0].val.i = 30;
										else if (iptr[0].val.i == 0x80000000)
											iptr[0].val.i = 31;
										else {
											PUSHCONST(TYPE_INT);
											break;
											}
										iptr[0].opc = ICMD_IDIVPOW2;
										goto icmd_iconst_tail;
									case ICMD_IREM:
										if (iptr[0].val.i == 0x10001) {
											iptr[0].opc = ICMD_IREM0X10001;
											goto icmd_iconst_tail;
											}
										if ((iptr[0].val.i == 0x00000002) ||
										    (iptr[0].val.i == 0x00000004) ||
										    (iptr[0].val.i == 0x00000008) ||
										    (iptr[0].val.i == 0x00000010) ||
										    (iptr[0].val.i == 0x00000020) ||
										    (iptr[0].val.i == 0x00000040) ||
										    (iptr[0].val.i == 0x00000080) ||
										    (iptr[0].val.i == 0x00000100) ||
										    (iptr[0].val.i == 0x00000200) ||
										    (iptr[0].val.i == 0x00000400) ||
										    (iptr[0].val.i == 0x00000800) ||
										    (iptr[0].val.i == 0x00001000) ||
										    (iptr[0].val.i == 0x00002000) ||
										    (iptr[0].val.i == 0x00004000) ||
										    (iptr[0].val.i == 0x00008000) ||
										    (iptr[0].val.i == 0x00010000) ||
										    (iptr[0].val.i == 0x00020000) ||
										    (iptr[0].val.i == 0x00040000) ||
										    (iptr[0].val.i == 0x00080000) ||
										    (iptr[0].val.i == 0x00100000) ||
										    (iptr[0].val.i == 0x00200000) ||
										    (iptr[0].val.i == 0x00400000) ||
										    (iptr[0].val.i == 0x00800000) ||
										    (iptr[0].val.i == 0x01000000) ||
										    (iptr[0].val.i == 0x02000000) ||
										    (iptr[0].val.i == 0x04000000) ||
										    (iptr[0].val.i == 0x08000000) ||
										    (iptr[0].val.i == 0x10000000) ||
										    (iptr[0].val.i == 0x20000000) ||
										    (iptr[0].val.i == 0x40000000) ||
										    (iptr[0].val.i == 0x80000000)) {
											iptr[0].opc = ICMD_IREMPOW2;
											iptr[0].val.i -= 1;
											goto icmd_iconst_tail;
											}
										PUSHCONST(TYPE_INT);
										break;
									case ICMD_IAND:
										iptr[0].opc = ICMD_IANDCONST;
										goto icmd_iconst_tail;
									case ICMD_IOR:
										iptr[0].opc = ICMD_IORCONST;
										goto icmd_iconst_tail;
									case ICMD_IXOR:
										iptr[0].opc = ICMD_IXORCONST;
										goto icmd_iconst_tail;
									case ICMD_ISHL:
										iptr[0].opc = ICMD_ISHLCONST;
										goto icmd_iconst_tail;
									case ICMD_ISHR:
										iptr[0].opc = ICMD_ISHRCONST;
										goto icmd_iconst_tail;
									case ICMD_IUSHR:
										iptr[0].opc = ICMD_IUSHRCONST;
										goto icmd_iconst_tail;
									case ICMD_IF_ICMPEQ:
										iptr[0].opc = ICMD_IFEQ;
icmd_if_icmp_tail:
										iptr[0].op1 = iptr[1].op1;
										bptr->icount--;
										len--;
										/* iptr[1].opc = ICMD_NOP; */
										OP1_0(TYPE_INT);
										tbptr = block + block_index[iptr->op1];
										MARKREACHED(tbptr, copy);
										COUNT(count_pcmd_bra);
										break;
									case ICMD_IF_ICMPLT:
										iptr[0].opc = ICMD_IFLT;
										goto icmd_if_icmp_tail;
									case ICMD_IF_ICMPLE:
										iptr[0].opc = ICMD_IFLE;
										goto icmd_if_icmp_tail;
									case ICMD_IF_ICMPNE:
										iptr[0].opc = ICMD_IFNE;
										goto icmd_if_icmp_tail;
									case ICMD_IF_ICMPGT:
										iptr[0].opc = ICMD_IFGT;
										goto icmd_if_icmp_tail;
									case ICMD_IF_ICMPGE:
										iptr[0].opc = ICMD_IFGE;
										goto icmd_if_icmp_tail;
									default:
										PUSHCONST(TYPE_INT);
									}
								}
							else
								PUSHCONST(TYPE_INT);
							break;
						case ICMD_LCONST:
							COUNT(count_pcmd_load);
							if (len > 0) {
								switch (iptr[1].opc) {
									case ICMD_LADD:
										iptr[0].opc = ICMD_LADDCONST;
icmd_lconst_tail:
										iptr[1].opc = ICMD_NOP;
										OP1_1(TYPE_LNG,TYPE_LNG);
										COUNT(count_pcmd_op);
										break;
									case ICMD_LSUB:
										iptr[0].opc = ICMD_LSUBCONST;
										goto icmd_lconst_tail;
									case ICMD_LMUL:
										iptr[0].opc = ICMD_LMULCONST;
										goto icmd_lconst_tail;
									case ICMD_LDIV:
										if (iptr[0].val.l == 0x00000002)
											iptr[0].val.l = 1;
										else if (iptr[0].val.l == 0x00000004)
											iptr[0].val.l = 2;
										else if (iptr[0].val.l == 0x00000008)
											iptr[0].val.l = 3;
										else if (iptr[0].val.l == 0x00000010)
											iptr[0].val.l = 4;
										else if (iptr[0].val.l == 0x00000020)
											iptr[0].val.l = 5;
										else if (iptr[0].val.l == 0x00000040)
											iptr[0].val.l = 6;
										else if (iptr[0].val.l == 0x00000080)
											iptr[0].val.l = 7;
										else if (iptr[0].val.l == 0x00000100)
											iptr[0].val.l = 8;
										else if (iptr[0].val.l == 0x00000200)
											iptr[0].val.l = 9;
										else if (iptr[0].val.l == 0x00000400)
											iptr[0].val.l = 10;
										else if (iptr[0].val.l == 0x00000800)
											iptr[0].val.l = 11;
										else if (iptr[0].val.l == 0x00001000)
											iptr[0].val.l = 12;
										else if (iptr[0].val.l == 0x00002000)
											iptr[0].val.l = 13;
										else if (iptr[0].val.l == 0x00004000)
											iptr[0].val.l = 14;
										else if (iptr[0].val.l == 0x00008000)
											iptr[0].val.l = 15;
										else if (iptr[0].val.l == 0x00010000)
											iptr[0].val.l = 16;
										else if (iptr[0].val.l == 0x00020000)
											iptr[0].val.l = 17;
										else if (iptr[0].val.l == 0x00040000)
											iptr[0].val.l = 18;
										else if (iptr[0].val.l == 0x00080000)
											iptr[0].val.l = 19;
										else if (iptr[0].val.l == 0x00100000)
											iptr[0].val.l = 20;
										else if (iptr[0].val.l == 0x00200000)
											iptr[0].val.l = 21;
										else if (iptr[0].val.l == 0x00400000)
											iptr[0].val.l = 22;
										else if (iptr[0].val.l == 0x00800000)
											iptr[0].val.l = 23;
										else if (iptr[0].val.l == 0x01000000)
											iptr[0].val.l = 24;
										else if (iptr[0].val.l == 0x02000000)
											iptr[0].val.l = 25;
										else if (iptr[0].val.l == 0x04000000)
											iptr[0].val.l = 26;
										else if (iptr[0].val.l == 0x08000000)
											iptr[0].val.l = 27;
										else if (iptr[0].val.l == 0x10000000)
											iptr[0].val.l = 28;
										else if (iptr[0].val.l == 0x20000000)
											iptr[0].val.l = 29;
										else if (iptr[0].val.l == 0x40000000)
											iptr[0].val.l = 30;
										else if (iptr[0].val.l == 0x80000000)
											iptr[0].val.l = 31;
										else {
											PUSHCONST(TYPE_LNG);
											break;
											}
										iptr[0].opc = ICMD_LDIVPOW2;
										goto icmd_lconst_tail;
									case ICMD_LREM:
										if (iptr[0].val.l == 0x10001) {
											iptr[0].opc = ICMD_LREM0X10001;
											goto icmd_lconst_tail;
											}
										if ((iptr[0].val.l == 0x00000002) ||
										    (iptr[0].val.l == 0x00000004) ||
										    (iptr[0].val.l == 0x00000008) ||
										    (iptr[0].val.l == 0x00000010) ||
										    (iptr[0].val.l == 0x00000020) ||
										    (iptr[0].val.l == 0x00000040) ||
										    (iptr[0].val.l == 0x00000080) ||
										    (iptr[0].val.l == 0x00000100) ||
										    (iptr[0].val.l == 0x00000200) ||
										    (iptr[0].val.l == 0x00000400) ||
										    (iptr[0].val.l == 0x00000800) ||
										    (iptr[0].val.l == 0x00001000) ||
										    (iptr[0].val.l == 0x00002000) ||
										    (iptr[0].val.l == 0x00004000) ||
										    (iptr[0].val.l == 0x00008000) ||
										    (iptr[0].val.l == 0x00010000) ||
										    (iptr[0].val.l == 0x00020000) ||
										    (iptr[0].val.l == 0x00040000) ||
										    (iptr[0].val.l == 0x00080000) ||
										    (iptr[0].val.l == 0x00100000) ||
										    (iptr[0].val.l == 0x00200000) ||
										    (iptr[0].val.l == 0x00400000) ||
										    (iptr[0].val.l == 0x00800000) ||
										    (iptr[0].val.l == 0x01000000) ||
										    (iptr[0].val.l == 0x02000000) ||
										    (iptr[0].val.l == 0x04000000) ||
										    (iptr[0].val.l == 0x08000000) ||
										    (iptr[0].val.l == 0x10000000) ||
										    (iptr[0].val.l == 0x20000000) ||
										    (iptr[0].val.l == 0x40000000) ||
										    (iptr[0].val.l == 0x80000000)) {
											iptr[0].opc = ICMD_LREMPOW2;
											iptr[0].val.l -= 1;
											goto icmd_lconst_tail;
											}
										PUSHCONST(TYPE_LNG);
										break;
									case ICMD_LAND:
										iptr[0].opc = ICMD_LANDCONST;
										goto icmd_lconst_tail;
									case ICMD_LOR:
										iptr[0].opc = ICMD_LORCONST;
										goto icmd_lconst_tail;
									case ICMD_LXOR:
										iptr[0].opc = ICMD_LXORCONST;
										goto icmd_lconst_tail;
									case ICMD_LSHL:
										iptr[0].opc = ICMD_LSHLCONST;
										goto icmd_lconst_tail;
									case ICMD_LSHR:
										iptr[0].opc = ICMD_LSHRCONST;
										goto icmd_lconst_tail;
									case ICMD_LUSHR:
										iptr[0].opc = ICMD_LUSHRCONST;
										goto icmd_lconst_tail;
									case ICMD_LCMP:
										if ((len > 1) && (iptr[2].val.i == 0)) {
											switch (iptr[2].opc) {
											case ICMD_IFEQ:
												iptr[0].opc = ICMD_IF_LEQ;
icmd_lconst_lcmp_tail:
												iptr[0].op1 = iptr[2].op1;
												bptr->icount -= 2;
												len -= 2;
												/* iptr[1].opc = ICMD_NOP;
												iptr[2].opc = ICMD_NOP; */
												OP1_0(TYPE_LNG);
												tbptr = block + block_index[iptr->op1];
												MARKREACHED(tbptr, copy);
												COUNT(count_pcmd_bra);
												COUNT(count_pcmd_op);
												break;
											case ICMD_IFNE:
												iptr[0].opc = ICMD_IF_LNE;
												goto icmd_lconst_lcmp_tail;
											case ICMD_IFLT:
												iptr[0].opc = ICMD_IF_LLT;
												goto icmd_lconst_lcmp_tail;
											case ICMD_IFGT:
												iptr[0].opc = ICMD_IF_LGT;
												goto icmd_lconst_lcmp_tail;
											case ICMD_IFLE:
												iptr[0].opc = ICMD_IF_LLE;
												goto icmd_lconst_lcmp_tail;
											case ICMD_IFGE:
												iptr[0].opc = ICMD_IF_LGE;
												goto icmd_lconst_lcmp_tail;
											default:
												PUSHCONST(TYPE_LNG);
											} /* switch (iptr[2].opc) */
											} /* if (iptr[2].val.i == 0) */
										else
											PUSHCONST(TYPE_LNG);
										break;
									default:
										PUSHCONST(TYPE_LNG);
									}
								}
							else
								PUSHCONST(TYPE_LNG);
							break;
						case ICMD_FCONST:
							COUNT(count_pcmd_load);
							PUSHCONST(TYPE_FLT);
							break;
						case ICMD_DCONST:
							COUNT(count_pcmd_load);
							PUSHCONST(TYPE_DBL);
							break;
						case ICMD_ACONST:
							COUNT(count_pcmd_load);
							PUSHCONST(TYPE_ADR);
							break;

						/* pop 0 push 1 load */
						
						case ICMD_ILOAD:
						case ICMD_LLOAD:
						case ICMD_FLOAD:
						case ICMD_DLOAD:
						case ICMD_ALOAD:
							COUNT(count_load_instruction);
							i = opcode-ICMD_ILOAD;
							locals[iptr->op1][i].type = i;
							LOAD(i, LOCALVAR, iptr->op1);
							break;

						/* pop 2 push 1 */

						case ICMD_IALOAD:
						case ICMD_LALOAD:
						case ICMD_FALOAD:
						case ICMD_DALOAD:
						case ICMD_AALOAD:
							COUNT(count_check_null);
							COUNT(count_check_bound);
							COUNT(count_pcmd_mem);
							OP2IAT_1(opcode-ICMD_IALOAD);
							break;

						case ICMD_BALOAD:
						case ICMD_CALOAD:
						case ICMD_SALOAD:
							COUNT(count_check_null);
							COUNT(count_check_bound);
							COUNT(count_pcmd_mem);
							OP2IAT_1(TYPE_INT);
							break;

						/* pop 0 push 0 iinc */

						case ICMD_IINC:
#ifdef STATISTICS
							i = stackdepth;
							if (i >= 10)
								count_store_depth[10]++;
							else
								count_store_depth[i]++;
#endif
							copy = curstack;
							i = stackdepth - 1;
							while (copy) {
								if ((copy->varkind == LOCALVAR) &&
								    (copy->varnum == curstack->varnum)) {
									copy->varkind = TEMPVAR;
									copy->varnum = i;
									}
								i--;
								copy = copy->prev;
								}
							SETDST;
							break;

						/* pop 1 push 0 store */

						case ICMD_ISTORE:
						case ICMD_LSTORE:
						case ICMD_FSTORE:
						case ICMD_DSTORE:
						case ICMD_ASTORE:
							i = opcode-ICMD_ISTORE;
							locals[iptr->op1][i].type = i;
#ifdef STATISTICS
							count_pcmd_store++;
							i = new - curstack;
							if (i >= 20)
								count_store_length[20]++;
							else
								count_store_length[i]++;
							i = stackdepth - 1;
							if (i >= 10)
								count_store_depth[10]++;
							else
								count_store_depth[i]++;
#endif
							copy = curstack->prev;
							i = stackdepth - 2;
							while (copy) {
								if ((copy->varkind == LOCALVAR) &&
								    (copy->varnum == curstack->varnum)) {
									copy->varkind = TEMPVAR;
									copy->varnum = i;
									}
								i--;
								copy = copy->prev;
								}
							if ((new - curstack) == 1) {
								curstack->varkind = LOCALVAR;
								curstack->varnum = iptr->op1;
								};
							STORE(opcode-ICMD_ISTORE);
							break;

						/* pop 3 push 0 */

						case ICMD_IASTORE:
						case ICMD_LASTORE:
						case ICMD_FASTORE:
						case ICMD_DASTORE:
						case ICMD_AASTORE:
							COUNT(count_check_null);
							COUNT(count_check_bound);
							COUNT(count_pcmd_mem);
							OP3TIA_0(opcode-ICMD_IASTORE);
							break;
						case ICMD_BASTORE:
						case ICMD_CASTORE:
						case ICMD_SASTORE:
							COUNT(count_check_null);
							COUNT(count_check_bound);
							COUNT(count_pcmd_mem);
							OP3TIA_0(TYPE_INT);
							break;

						/* pop 1 push 0 */

						case ICMD_POP:
							OP1_0ANY;
							break;

						case ICMD_IRETURN:
						case ICMD_LRETURN:
						case ICMD_FRETURN:
						case ICMD_DRETURN:
						case ICMD_ARETURN:
							COUNT(count_pcmd_return);
							OP1_0(opcode-ICMD_IRETURN);
							superblockend = true;
							break;

						case ICMD_ATHROW:
							COUNT(count_check_null);
							OP1_0(TYPE_ADR);
							STACKRESET;
							SETDST;
							superblockend = true;
							break;

						case ICMD_PUTSTATIC:
							COUNT(count_pcmd_mem);
							OP1_0(iptr->op1);
							break;

						/* pop 1 push 0 branch */

						case ICMD_IFNULL:
						case ICMD_IFNONNULL:
							COUNT(count_pcmd_bra);
							OP1_0(TYPE_ADR);
							tbptr = block + block_index[iptr->op1];
							MARKREACHED(tbptr, copy);
							break;

						case ICMD_IFEQ:
						case ICMD_IFNE:
						case ICMD_IFLT:
						case ICMD_IFGE:
						case ICMD_IFGT:
						case ICMD_IFLE:
							COUNT(count_pcmd_bra);
#ifdef CONDITIONAL_LOADCONST
							{
							tbptr = block + b_index;
							if ((b_count >= 3) &&
							    ((b_index + 2) == block_index[iptr[0].op1]) &&
							    (tbptr[1].pre_count == 1) &&
							    (iptr[1].opc == ICMD_ICONST) &&
							    (iptr[2].opc == ICMD_GOTO)   &&
							    ((b_index + 3) == block_index[iptr[2].op1]) &&
							    (tbptr[2].pre_count == 1) &&
							    (iptr[3].opc == ICMD_ICONST)) {
								OP1_1(TYPE_INT, TYPE_INT);
								switch (iptr[0].opc) {
									case ICMD_IFEQ:
										iptr[0].opc = ICMD_IFNE_ICONST;
										break;
									case ICMD_IFNE:
										iptr[0].opc = ICMD_IFEQ_ICONST;
										break;
									case ICMD_IFLT:
										iptr[0].opc = ICMD_IFGE_ICONST;
										break;
									case ICMD_IFGE:
										iptr[0].opc = ICMD_IFLT_ICONST;
										break;
									case ICMD_IFGT:
										iptr[0].opc = ICMD_IFLE_ICONST;
										break;
									case ICMD_IFLE:
										iptr[0].opc = ICMD_IFGT_ICONST;
										break;
									}
								iptr[0].val.i = iptr[1].val.i;
								iptr[1].opc = ICMD_ELSE_ICONST;
								iptr[1].val.i = iptr[3].val.i;
								iptr[2].opc = ICMD_NOP;
								iptr[3].opc = ICMD_NOP;
								tbptr[1].flags = BBDELETED;
								tbptr[2].flags = BBDELETED;
								tbptr[1].icount = 0;
								tbptr[2].icount = 0;
								if (tbptr[3].pre_count == 2) {
									len += tbptr[3].icount + 3;
									bptr->icount += tbptr[3].icount + 3;
									tbptr[3].flags = BBDELETED;
									tbptr[3].icount = 0;
									b_index++;
									}
								else {
									bptr->icount++;
									len ++;
									}
								b_index += 2;
								break;
								}
							}
#endif
							OP1_0(TYPE_INT);
							tbptr = block + block_index[iptr->op1];
							MARKREACHED(tbptr, copy);
							break;

						/* pop 0 push 0 branch */

						case ICMD_GOTO:
							COUNT(count_pcmd_bra);
							tbptr = block + block_index[iptr->op1];
							MARKREACHED(tbptr, copy);
							SETDST;
							superblockend = true;
							break;

						/* pop 1 push 0 table branch */

						case ICMD_TABLESWITCH:
							COUNT(count_pcmd_table);
							OP1_0(TYPE_INT);
							s4ptr = iptr->val.a;
							tbptr = block + block_index[*s4ptr++]; /* default */
							MARKREACHED(tbptr, copy);
							i = *s4ptr++;                          /* low     */
							i = *s4ptr++ - i + 1;                  /* high    */
							while (--i >= 0) {
								tbptr = block + block_index[*s4ptr++];
								MARKREACHED(tbptr, copy);
								}
							SETDST;
							superblockend = true;
							break;
							
						/* pop 1 push 0 table branch */

						case ICMD_LOOKUPSWITCH:
							COUNT(count_pcmd_table);
							OP1_0(TYPE_INT);
							s4ptr = iptr->val.a;
							tbptr = block + block_index[*s4ptr++]; /* default */
							MARKREACHED(tbptr, copy);
							i = *s4ptr++;                          /* count   */
							while (--i >= 0) {
								tbptr = block + block_index[s4ptr[1]];
								MARKREACHED(tbptr, copy);
								s4ptr += 2;
								}
							SETDST;
							superblockend = true;
							break;

						case ICMD_NULLCHECKPOP:
						case ICMD_MONITORENTER:
							COUNT(count_check_null);
						case ICMD_MONITOREXIT:
							OP1_0(TYPE_ADR);
							break;

						/* pop 2 push 0 branch */

						case ICMD_IF_ICMPEQ:
						case ICMD_IF_ICMPNE:
						case ICMD_IF_ICMPLT:
						case ICMD_IF_ICMPGE:
						case ICMD_IF_ICMPGT:
						case ICMD_IF_ICMPLE:
							COUNT(count_pcmd_bra);
							OP2_0(TYPE_INT);
							tbptr = block + block_index[iptr->op1];
							MARKREACHED(tbptr, copy);
							break;

						case ICMD_IF_ACMPEQ:
						case ICMD_IF_ACMPNE:
							COUNT(count_pcmd_bra);
							OP2_0(TYPE_ADR);
							tbptr = block + block_index[iptr->op1];
							MARKREACHED(tbptr, copy);
							break;

						/* pop 2 push 0 */

						case ICMD_PUTFIELD:
							COUNT(count_check_null);
							COUNT(count_pcmd_mem);
							OPTT2_0(iptr->op1,TYPE_ADR);
							break;

						case ICMD_POP2:
							if (! IS_2_WORD_TYPE(curstack->type)) {
								OP1_0ANY;                /* second pop */
								}
							else
								iptr->opc = ICMD_POP;
							OP1_0ANY;
							break;

						/* pop 0 push 1 dup */
						
						case ICMD_DUP:
							COUNT(count_dup_instruction);
							DUP;
							break;

						case ICMD_DUP2:
							if (IS_2_WORD_TYPE(curstack->type)) {
								iptr->opc = ICMD_DUP;
								DUP;
								}
							else {
								copy = curstack;
								NEWSTACK(copy[-1].type, copy[-1].varkind,
								         copy[-1].varnum);
								NEWSTACK(copy[ 0].type, copy[ 0].varkind,
								         copy[ 0].varnum);
								SETDST;
								stackdepth+=2;
								}
							break;

						/* pop 2 push 3 dup */
						
						case ICMD_DUP_X1:
							DUP_X1;
							break;

						case ICMD_DUP2_X1:
							if (IS_2_WORD_TYPE(curstack->type)) {
								iptr->opc = ICMD_DUP_X1;
								DUP_X1;
								}
							else {
								DUP2_X1;
								}
							break;

						/* pop 3 push 4 dup */
						
						case ICMD_DUP_X2:
							if (IS_2_WORD_TYPE(curstack[-1].type)) {
								iptr->opc = ICMD_DUP_X1;
								DUP_X1;
								}
							else {
								DUP_X2;
								}
							break;

						case ICMD_DUP2_X2:
							if (IS_2_WORD_TYPE(curstack->type)) {
								if (IS_2_WORD_TYPE(curstack[-1].type)) {
									iptr->opc = ICMD_DUP_X1;
									DUP_X1;
									}
								else {
									iptr->opc = ICMD_DUP_X2;
									DUP_X2;
									}
								}
							else
								if (IS_2_WORD_TYPE(curstack[-2].type)) {
									iptr->opc = ICMD_DUP2_X1;
									DUP2_X1;
									}
								else {
									DUP2_X2;
									}
							break;

						/* pop 2 push 2 swap */
						
						case ICMD_SWAP:
							SWAP;
							break;

						/* pop 2 push 1 */
						
						case ICMD_IDIV:
							if (!(SUPPORT_DIVISION)) {
								iptr[0].opc = ICMD_BUILTIN2;
								iptr[0].op1 = TYPE_INT;
								iptr[0].val.a = (functionptr) asm_builtin_idiv;
								isleafmethod = false;
								goto builtin2;
								}

						case ICMD_LDIV:
							if (!(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_MULDIV)) {
								iptr[0].opc = ICMD_BUILTIN2;
								iptr[0].op1 = TYPE_LNG;
								iptr[0].val.a = (functionptr) asm_builtin_ldiv;
								isleafmethod = false;
								goto builtin2;
								}

						case ICMD_IREM:
							if (!(SUPPORT_DIVISION)) {
								iptr[0].opc = ICMD_BUILTIN2;
								iptr[0].op1 = TYPE_INT;
								iptr[0].val.a = (functionptr) asm_builtin_irem;
								isleafmethod = false;
								goto builtin2;
								}

						case ICMD_LREM:
							if (!(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_MULDIV)) {
								iptr[0].opc = ICMD_BUILTIN2;
								iptr[0].op1 = TYPE_LNG;
								iptr[0].val.a = (functionptr) asm_builtin_lrem;
								isleafmethod = false;
								goto builtin2;
								}

						case ICMD_IADD:
						case ICMD_ISUB:
						case ICMD_IMUL:

						case ICMD_ISHL:
						case ICMD_ISHR:
						case ICMD_IUSHR:
						case ICMD_IAND:
						case ICMD_IOR:
						case ICMD_IXOR:
							COUNT(count_pcmd_op);
							OP2_1(TYPE_INT);
							break;

						case ICMD_LADD:
						case ICMD_LSUB:
						case ICMD_LMUL:

						case ICMD_LOR:
						case ICMD_LAND:
						case ICMD_LXOR:
							COUNT(count_pcmd_op);
							OP2_1(TYPE_LNG);
							break;

						case ICMD_LSHL:
						case ICMD_LSHR:
						case ICMD_LUSHR:
							COUNT(count_pcmd_op);
							OP2IT_1(TYPE_LNG);
							break;

						case ICMD_FADD:
						case ICMD_FSUB:
						case ICMD_FMUL:
						case ICMD_FDIV:
						case ICMD_FREM:
							COUNT(count_pcmd_op);
							OP2_1(TYPE_FLT);
							break;

						case ICMD_DADD:
						case ICMD_DSUB:
						case ICMD_DMUL:
						case ICMD_DDIV:
						case ICMD_DREM:
							COUNT(count_pcmd_op);
							OP2_1(TYPE_DBL);
							break;

						case ICMD_LCMP:
							COUNT(count_pcmd_op);
							if ((len > 0) && (iptr[1].val.i == 0)) {
								switch (iptr[1].opc) {
									case ICMD_IFEQ:
										iptr[0].opc = ICMD_IF_LCMPEQ;
icmd_lcmp_if_tail:
										iptr[0].op1 = iptr[1].op1;
										len--;
										bptr->icount--;
										/* iptr[1].opc = ICMD_NOP; */
										OP2_0(TYPE_LNG);
										tbptr = block + block_index[iptr->op1];
										MARKREACHED(tbptr, copy);
										COUNT(count_pcmd_bra);
										break;
									case ICMD_IFNE:
										iptr[0].opc = ICMD_IF_LCMPNE;
										goto icmd_lcmp_if_tail;
									case ICMD_IFLT:
										iptr[0].opc = ICMD_IF_LCMPLT;
										goto icmd_lcmp_if_tail;
									case ICMD_IFGT:
										iptr[0].opc = ICMD_IF_LCMPGT;
										goto icmd_lcmp_if_tail;
									case ICMD_IFLE:
										iptr[0].opc = ICMD_IF_LCMPLE;
										goto icmd_lcmp_if_tail;
									case ICMD_IFGE:
										iptr[0].opc = ICMD_IF_LCMPGE;
										goto icmd_lcmp_if_tail;
									default:
										OPTT2_1(TYPE_LNG, TYPE_INT);
									}
								}
							else
								OPTT2_1(TYPE_LNG, TYPE_INT);
							break;
						case ICMD_FCMPL:
						case ICMD_FCMPG:
							COUNT(count_pcmd_op);
							OPTT2_1(TYPE_FLT, TYPE_INT);
							break;
						case ICMD_DCMPL:
						case ICMD_DCMPG:
							COUNT(count_pcmd_op);
							OPTT2_1(TYPE_DBL, TYPE_INT);
							break;

						/* pop 1 push 1 */
						
						case ICMD_INEG:
						case ICMD_INT2BYTE:
						case ICMD_INT2CHAR:
						case ICMD_INT2SHORT:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_INT, TYPE_INT);
							break;
						case ICMD_LNEG:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_LNG, TYPE_LNG);
							break;
						case ICMD_FNEG:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_FLT, TYPE_FLT);
							break;
						case ICMD_DNEG:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_DBL, TYPE_DBL);
							break;

						case ICMD_I2L:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_INT, TYPE_LNG);
							break;
						case ICMD_I2F:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_INT, TYPE_FLT);
							break;
						case ICMD_I2D:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_INT, TYPE_DBL);
							break;
						case ICMD_L2I:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_LNG, TYPE_INT);
							break;
						case ICMD_L2F:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_LNG, TYPE_FLT);
							break;
						case ICMD_L2D:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_LNG, TYPE_DBL);
							break;
						case ICMD_F2I:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_FLT, TYPE_INT);
							break;
						case ICMD_F2L:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_FLT, TYPE_LNG);
							break;
						case ICMD_F2D:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_FLT, TYPE_DBL);
							break;
						case ICMD_D2I:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_DBL, TYPE_INT);
							break;
						case ICMD_D2L:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_DBL, TYPE_LNG);
							break;
						case ICMD_D2F:
							COUNT(count_pcmd_op);
							OP1_1(TYPE_DBL, TYPE_FLT);
							break;

						case ICMD_CHECKCAST:
							OP1_1(TYPE_ADR, TYPE_ADR);
							break;

						case ICMD_ARRAYLENGTH:
						case ICMD_INSTANCEOF:
							OP1_1(TYPE_ADR, TYPE_INT);
							break;

						case ICMD_NEWARRAY:
						case ICMD_ANEWARRAY:
							OP1_1(TYPE_INT, TYPE_ADR);
							break;

						case ICMD_GETFIELD:
							COUNT(count_check_null);
							COUNT(count_pcmd_mem);
							OP1_1(TYPE_ADR, iptr->op1);
							break;

						/* pop 0 push 1 */
						
						case ICMD_GETSTATIC:
							COUNT(count_pcmd_mem);
							OP0_1(iptr->op1);
							break;

						case ICMD_NEW:
							OP0_1(TYPE_ADR);
							break;

						case ICMD_JSR:
							OP0_1(TYPE_ADR);
							tbptr = block + block_index[iptr->op1];
							tbptr->type=BBTYPE_SBR;
							MARKREACHED(tbptr, copy);
							OP1_0ANY;
							break;

						/* pop many push any */
						
						case ICMD_INVOKEVIRTUAL:
						case ICMD_INVOKESPECIAL:
						case ICMD_INVOKEINTERFACE:
						case ICMD_INVOKESTATIC:
							COUNT(count_pcmd_met);
							{
							methodinfo *m = iptr->val.a;
							if (m->flags & ACC_STATIC)
								{COUNT(count_check_null);}
							i = iptr->op1;
							if (i > arguments_num)
								arguments_num = i;
							copy = curstack;
							while (--i >= 0) {
								if (! (copy->flags & SAVEDVAR)) {
									copy->varkind = ARGVAR;
									copy->varnum = i;
									}
								copy = copy->prev;
								}
							while (copy) {
								copy->flags |= SAVEDVAR;
								copy = copy->prev;
								}
							i = iptr->op1;
							POPMANY(i);
							if (m->returntype != TYPE_VOID) {
								OP0_1(m->returntype);
								}
							break;
							}

						case ICMD_BUILTIN3:
							if (! (curstack->flags & SAVEDVAR)) {
								curstack->varkind = ARGVAR;
								curstack->varnum = 2;
								}
							OP1_0ANY;
						case ICMD_BUILTIN2:
builtin2:
							if (! (curstack->flags & SAVEDVAR)) {
								curstack->varkind = ARGVAR;
								curstack->varnum = 1;
								}
							OP1_0ANY;
						case ICMD_BUILTIN1:
							if (! (curstack->flags & SAVEDVAR)) {
								curstack->varkind = ARGVAR;
								curstack->varnum = 0;
								}
							OP1_0ANY;
							copy = curstack;
							while (copy) {
								copy->flags |= SAVEDVAR;
								copy = copy->prev;
								}
							if (iptr->op1 != TYPE_VOID)
								OP0_1(iptr->op1);
							break;

						case ICMD_MULTIANEWARRAY:
							i = iptr->op1;
							if ((i + intreg_argnum) > arguments_num)
								arguments_num = i + intreg_argnum;
							copy = curstack;
							while (--i >= 0) {
								if (! (copy->flags & SAVEDVAR)) {
									copy->varkind = ARGVAR;
									copy->varnum = i + intreg_argnum;
									}
								copy = copy->prev;
								}
							while (copy) {
								copy->flags |= SAVEDVAR;
								copy = copy->prev;
								}
							i = iptr->op1;
							POPMANY(i);
							OP0_1(TYPE_ADR);
							break;

						default:
							printf("ICMD %d at %d\n", iptr->opc, (int)(iptr-instr));
							panic("Missing ICMD code during stack analysis");
						} /* switch */
					iptr++;
					} /* while instructions */
				bptr->outstack = curstack;
				bptr->outdepth = stackdepth;
				BBEND(curstack, i);
				} /* if */
			else
				superblockend = true;
			bptr++;
		} /* while blocks */
	} while (repeat && ! deadcode);

#ifdef STATISTICS
	if (block_count > count_max_basic_blocks)
		count_max_basic_blocks = block_count;
	count_basic_blocks += block_count;
	if (instr_count > count_max_javainstr)
		count_max_javainstr = instr_count;
	count_javainstr += instr_count;
	if (stack_count > count_upper_bound_new_stack)
		count_upper_bound_new_stack = stack_count;
	if ((new - stack) > count_max_new_stack)
		count_max_new_stack = (new - stack);

	b_count = block_count;
	bptr = block;
	while (--b_count >= 0) {
		if (bptr->flags > BBREACHED) {
			if (bptr->indepth >= 10)
				count_block_stack[10]++;
			else
				count_block_stack[bptr->indepth]++;
			len = bptr->icount;
			if (len <= 10) 
				count_block_size_distribution[len - 1]++;
			else if (len <= 12)
				count_block_size_distribution[10]++;
			else if (len <= 14)
				count_block_size_distribution[11]++;
			else if (len <= 16)
				count_block_size_distribution[12]++;
			else if (len <= 18)
				count_block_size_distribution[13]++;
			else if (len <= 20)
				count_block_size_distribution[14]++;
			else if (len <= 25)
				count_block_size_distribution[15]++;
			else if (len <= 30)
				count_block_size_distribution[16]++;
			else
				count_block_size_distribution[17]++;
			}
		bptr++;
		}

	if (loops == 1)
		count_analyse_iterations[0]++;
	else if (loops == 2)
		count_analyse_iterations[1]++;
	else if (loops == 3)
		count_analyse_iterations[2]++;
	else if (loops == 4)
		count_analyse_iterations[3]++;
	else
		count_analyse_iterations[4]++;

	if (block_count <= 5)
		count_method_bb_distribution[0]++;
	else if (block_count <= 10)
		count_method_bb_distribution[1]++;
	else if (block_count <= 15)
		count_method_bb_distribution[2]++;
	else if (block_count <= 20)
		count_method_bb_distribution[3]++;
	else if (block_count <= 30)
		count_method_bb_distribution[4]++;
	else if (block_count <= 40)
		count_method_bb_distribution[5]++;
	else if (block_count <= 50)
		count_method_bb_distribution[6]++;
	else if (block_count <= 75)
		count_method_bb_distribution[7]++;
	else
		count_method_bb_distribution[8]++;
#endif
}


static void print_stack(stackptr s) {
	int i, j;
	stackptr t;

	i = maxstack;
	t = s;
	
	while (t) {
		i--;
		t = t->prev;
		}
	j = maxstack - i;
	while (--i >= 0)
		printf("    ");
	while (s) {
		j--;
		if (s->flags & SAVEDVAR)
			switch (s->varkind) {
				case TEMPVAR:
					if (s->flags & INMEMORY)
						printf(" M%02d", s->regoff);
					else if ((s->type == TYPE_FLT) || (s->type == TYPE_DBL))
						printf(" F%02d", s->regoff);
					else
						printf(" %3s", regs[s->regoff]);
					break;
				case STACKVAR:
					printf(" I%02d", s->varnum);
					break;
				case LOCALVAR:
					printf(" L%02d", s->varnum);
					break;
				case ARGVAR:
					printf(" A%02d", s->varnum);
					break;
				default:
					printf(" !%02d", j);
				}
		else
			switch (s->varkind) {
				case TEMPVAR:
					if (s->flags & INMEMORY)
						printf(" m%02d", s->regoff);
					else if ((s->type == TYPE_FLT) || (s->type == TYPE_DBL))
						printf(" f%02d", s->regoff);
					else
						printf(" %3s", regs[s->regoff]);
					break;
				case STACKVAR:
					printf(" i%02d", s->varnum);
					break;
				case LOCALVAR:
					printf(" l%02d", s->varnum);
					break;
				case ARGVAR:
					printf(" a%02d", s->varnum);
					break;
				default:
					printf(" ?%02d", j);
				}
		s = s->prev;
		}
}


#if 0
static void print_reg(stackptr s) {
	if (s) {
		if (s->flags & SAVEDVAR)
			switch (s->varkind) {
				case TEMPVAR:
					if (s->flags & INMEMORY)
						printf(" tm%02d", s->regoff);
					else
						printf(" tr%02d", s->regoff);
					break;
				case STACKVAR:
					printf(" s %02d", s->varnum);
					break;
				case LOCALVAR:
					printf(" l %02d", s->varnum);
					break;
				case ARGVAR:
					printf(" a %02d", s->varnum);
					break;
				default:
					printf(" ! %02d", s->varnum);
				}
		else
			switch (s->varkind) {
				case TEMPVAR:
					if (s->flags & INMEMORY)
						printf(" Tm%02d", s->regoff);
					else
						printf(" Tr%02d", s->regoff);
					break;
				case STACKVAR:
					printf(" S %02d", s->varnum);
					break;
				case LOCALVAR:
					printf(" L %02d", s->varnum);
					break;
				case ARGVAR:
					printf(" A %02d", s->varnum);
					break;
				default:
					printf(" ? %02d", s->varnum);
				}
		}
	else
		printf("     ");
		
}
#endif


static char *builtin_name(functionptr bptr)
{
	builtin_descriptor *bdesc = builtin_desc;
	while ((bdesc->bptr != NULL) && (bdesc->bptr != bptr))
		bdesc++;
	return bdesc->name;
}


static char *jit_type[] = {
	"int",
	"lng",
	"flt",
	"dbl",
	"adr"
};


static void show_icmd_method()
{
	int b, i, j, last;
	int deadcode;
	s4  *s4ptr;
	instruction *iptr;
	
	printf("\n");
	unicode_fprint(stdout, class->name);
	printf(".");
	unicode_fprint(stdout, method->name);
	printf(" ");
	unicode_fprint(stdout, method->descriptor);
	printf ("\n\nMax locals: %d\n", (int) maxlocals);
	printf ("Max stack:  %d\n", (int) maxstack);

	printf ("Exceptions:\n");
	for (i = 0; i < exceptiontablelength; i++) {
		printf("    L%03d ... ", block_index[extable[i].startpc]);
		printf("L%03d = ", block_index[extable[i].endpc]);
		printf("L%03d\n", block_index[extable[i].handlerpc]);
		}
	
	printf ("Local Table:\n");
	for (i = 0; i < maxlocals; i++) {
		printf("   %3d: ", i);
		for (j = TYPE_INT; j <= TYPE_ADR; j++)
			if (locals[i][j].type >= 0) {
				printf("   (%s) ", jit_type[j]);
				if (locals[i][j].flags & INMEMORY)
					printf("m%2d", locals[i][j].regoff);
				else if ((j == TYPE_FLT) || (j == TYPE_DBL))
					printf("f%02d", locals[i][j].regoff);
				else
					printf("%3s", regs[locals[i][j].regoff]);
				}
		printf("\n");
		}
	printf("\n");

	printf ("Interface Table:\n");
	for (i = 0; i < maxstack; i++) {
		if ((interfaces[i][0].type >= 0) || (interfaces[i][1].type >= 0) ||
		    (interfaces[i][2].type >= 0) || (interfaces[i][3].type >= 0) ||
		    (interfaces[i][4].type >= 0)) {
			printf("   %3d: ", i);
			for (j = TYPE_INT; j <= TYPE_ADR; j++)
				if (interfaces[i][j].type >= 0) {
					printf("   (%s) ", jit_type[j]);
					if (interfaces[i][j].flags & SAVEDVAR) {
						if (interfaces[i][j].flags & INMEMORY)
							printf("M%2d", interfaces[i][j].regoff);
						else if ((j == TYPE_FLT) || (j == TYPE_DBL))
							printf("F%02d", interfaces[i][j].regoff);
						else
							printf("%3s", regs[interfaces[i][j].regoff]);
						}
					else {
						if (interfaces[i][j].flags & INMEMORY)
							printf("m%2d", interfaces[i][j].regoff);
						else if ((j == TYPE_FLT) || (j == TYPE_DBL))
							printf("f%02d", interfaces[i][j].regoff);
						else
							printf("%3s", regs[interfaces[i][j].regoff]);
						}
					}
			printf("\n");
			}
		}
	printf("\n");

	if (showdisassemble) {
		s4ptr = (s4 *) (method->mcode + dseglen);
		for (i = 0; i < block[0].mpc; i += 4, s4ptr++) {
			disassinstr(*s4ptr, i); 
			}
		printf("\n");
		}

	for (b = 0; b < block_count; b++)
		if (block[b].flags != BBDELETED) {
		deadcode = block[b].flags <= BBREACHED;
		printf("[");
		if (deadcode)
			for (j = maxstack; j > 0; j--)
				printf(" ?  ");
		else
			print_stack(block[b].instack);
		printf("] L%03d(%d):\n", b, block[b].pre_count);
		iptr = block[b].iinstr;
		i = iptr - instr;
		for (last = i + block[b].icount; i < last; i++, iptr++) {
			printf("[");
			if (deadcode) {
				for (j = maxstack; j > 0; j--)
					printf(" ?  ");
				}
			else
				print_stack(iptr->dst);
			printf("]     %4d  %s", i, icmd_names[iptr->opc]);
			switch ((int) iptr->opc) {
				case ICMD_IADDCONST:
				case ICMD_ISUBCONST:
				case ICMD_IMULCONST:
				case ICMD_IDIVPOW2:
				case ICMD_IREMPOW2:
				case ICMD_IREM0X10001:
				case ICMD_IANDCONST:
				case ICMD_IORCONST:
				case ICMD_IXORCONST:
				case ICMD_ISHLCONST:
				case ICMD_ISHRCONST:
				case ICMD_IUSHRCONST:
				case ICMD_ICONST:
				case ICMD_ELSE_ICONST:
				case ICMD_IFEQ_ICONST:
				case ICMD_IFNE_ICONST:
				case ICMD_IFLT_ICONST:
				case ICMD_IFGE_ICONST:
				case ICMD_IFGT_ICONST:
				case ICMD_IFLE_ICONST:
					printf(" %d", iptr->val.i);
					break;
				case ICMD_LADDCONST:
				case ICMD_LSUBCONST:
				case ICMD_LMULCONST:
				case ICMD_LDIVPOW2:
				case ICMD_LREMPOW2:
				case ICMD_LANDCONST:
				case ICMD_LORCONST:
				case ICMD_LXORCONST:
				case ICMD_LSHLCONST:
				case ICMD_LSHRCONST:
				case ICMD_LUSHRCONST:
				case ICMD_LCONST:
					printf(" %ld", iptr->val.l);
					break;
				case ICMD_FCONST:
					printf(" %f", iptr->val.f);
					break;
				case ICMD_DCONST:
					printf(" %f", iptr->val.d);
					break;
				case ICMD_ACONST:
					printf(" %p", iptr->val.a);
					break;
				case ICMD_GETFIELD:
				case ICMD_PUTFIELD:
					printf(" %d,", ((fieldinfo *) iptr->val.a)->offset);
				case ICMD_PUTSTATIC:
				case ICMD_GETSTATIC:
					printf(" ");
					unicode_fprint(stdout,
					                ((fieldinfo *) iptr->val.a)->name);
					break;
				case ICMD_IINC:
					printf(" %d + %d", iptr->op1, iptr->val.i);
					break;
				case ICMD_RET:
				case ICMD_ILOAD:
				case ICMD_LLOAD:
				case ICMD_FLOAD:
				case ICMD_DLOAD:
				case ICMD_ALOAD:
				case ICMD_ISTORE:
				case ICMD_LSTORE:
				case ICMD_FSTORE:
				case ICMD_DSTORE:
				case ICMD_ASTORE:
					printf(" %d", iptr->op1);
					break;
				case ICMD_NEW:
					printf(" ");
					unicode_fprint(stdout,
					               ((classinfo *) iptr->val.a)->name);
					break;
				case ICMD_NEWARRAY:
					switch (iptr->op1) {
						case 4:
							printf(" boolean");
							break;
						case 5:
							printf(" char");
							break;
						case 6:
							printf(" float");
							break;
						case 7:
							printf(" double");
							break;
						case 8:
							printf(" byte");
							break;
						case 9:
							printf(" short");
							break;
						case 10:
							printf(" int");
							break;
						case 11:
							printf(" long");
							break;
						}
					break;
				case ICMD_ANEWARRAY:
					if (iptr->op1) {
						printf(" ");
						unicode_fprint(stdout,
						               ((classinfo *) iptr->val.a)->name);
						}
					break;
				case ICMD_CHECKCAST:
				case ICMD_INSTANCEOF:
					if (iptr->op1) {
						classinfo *c = iptr->val.a;
						if (c->flags & ACC_INTERFACE)
							printf(" (INTERFACE) ");
						else
							printf(" (CLASS,%3d) ", c->vftbl->diffval);
						unicode_fprint(stdout, c->name);
						}
					break;
				case ICMD_BUILTIN3:
				case ICMD_BUILTIN2:
				case ICMD_BUILTIN1:
					printf(" %s", builtin_name((functionptr) iptr->val.a));
					break;
				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKEINTERFACE:
					printf(" ");
					unicode_fprint(stdout,
					               ((methodinfo *) iptr->val.a)->class->name);
					printf(".");
					unicode_fprint(stdout,
					               ((methodinfo *) iptr->val.a)->name);
					break;
				case ICMD_IFEQ:
				case ICMD_IFNE:
				case ICMD_IFLT:
				case ICMD_IFGE:
				case ICMD_IFGT:
				case ICMD_IFLE:
				case ICMD_IF_LEQ:
				case ICMD_IF_LNE:
				case ICMD_IF_LLT:
				case ICMD_IF_LGE:
				case ICMD_IF_LGT:
				case ICMD_IF_LLE:
					printf("(%d) L%03d", iptr->val.i, block_index[iptr->op1]);
					break;
				case ICMD_JSR:
				case ICMD_GOTO:
				case ICMD_IFNULL:
				case ICMD_IFNONNULL:
				case ICMD_IF_ICMPEQ:
				case ICMD_IF_ICMPNE:
				case ICMD_IF_ICMPLT:
				case ICMD_IF_ICMPGE:
				case ICMD_IF_ICMPGT:
				case ICMD_IF_ICMPLE:
				case ICMD_IF_LCMPEQ:
				case ICMD_IF_LCMPNE:
				case ICMD_IF_LCMPLT:
				case ICMD_IF_LCMPGE:
				case ICMD_IF_LCMPGT:
				case ICMD_IF_LCMPLE:
				case ICMD_IF_ACMPEQ:
				case ICMD_IF_ACMPNE:
					printf(" L%03d", block_index[iptr->op1]);
					break;
				case ICMD_TABLESWITCH:
					s4ptr = iptr->val.a;
					printf(" L%03d;", block_index[*s4ptr++]); /* default */
					j = *s4ptr++;                               /* low     */
					j = *s4ptr++ - j;                           /* high    */
					while (j >= 0) {
						printf(" L%03d", block_index[*s4ptr++]);
						j--;
						}
					break;
				case ICMD_LOOKUPSWITCH:
					s4ptr = iptr->val.a;
					printf(" L%d", block_index[*s4ptr++]);   /* default */
					j = *s4ptr++;                               /* count   */
					while (--j >= 0) {
						printf(" L%03d", block_index[s4ptr[1]]);
						s4ptr += 2;
						}
					break;
				}
			printf("\n");
			}

		if (showdisassemble && (!deadcode)) {
			printf("\n");
			i = block[b].mpc;
			s4ptr = (s4 *) (method->mcode + dseglen + i);
			for (; i < block[b + 1].mpc; i += 4, s4ptr++) {
				disassinstr(*s4ptr, i); 
				}
			printf("\n");
			}
	}
	i = block[b].mpc;
	s4ptr = (s4 *) (method->mcode + dseglen + i);
	if (showdisassemble && (s4ptr < (s4 *) (method->mcode + method->mcodelength))) {
		printf("\n");
		for (; s4ptr < (s4 *) (method->mcode + method->mcodelength); i += 4, s4ptr++) {
			disassinstr(*s4ptr, i); 
			}
		printf("\n");
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
