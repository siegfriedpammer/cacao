/* jit/parse.c *****************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Parser for JavaVM to intermediate code translation
	
	Author: Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: $Id: parse.c 530 2003-10-23 21:18:38Z stefan $
                     include Rapid Type Analysis parse - 5/2003 - carolyn


*******************************************************************************/

#include "math.h"
#include "sets.h"
                                /* data about the currently parsed   method   */

static classinfo  *rt_class;    /* class the compiled method belongs to       */
static methodinfo *rt_method;   /* pointer to method info of compiled method  */
static utf      *rt_descriptor; /* type descriptor of compiled method         */
static int       rt_jcodelength; /*length of JavaVM-codes                     */
static u1       *rt_jcode;      /* pointer to start of JavaVM-code            */


/* macros for byte code fetching ***********************************************

	fetch a byte code of given size from position p in code array jcode

*******************************************************************************/

#define code_get_u1(p)  jcode[p]
#define code_get_s1(p)  ((s1)jcode[p])
#define code_get_u2(p)  ((((u2)jcode[p])<<8)+jcode[p+1])
#define code_get_s2(p)  ((s2)((((u2)jcode[p])<<8)+jcode[p+1]))
#define code_get_u4(p)  ((((u4)jcode[p])<<24)+(((u4)jcode[p+1])<<16)\
                           +(((u4)jcode[p+2])<<8)+jcode[p+3])
#define code_get_s4(p)  ((s4)((((u4)jcode[p])<<24)+(((u4)jcode[p+1])<<16)\
                           +(((u4)jcode[p+2])<<8)+jcode[p+3]))



/*INLINING*/
#include "inline.c"
/*#define debug_writebranch printf("op: %s i: %d label_index[i]: %d\n",icmd_names[opcode], i, label_index[i]);*/
#define debug_writebranch

/* functionc compiler_addinitclass *********************************************

	add class into the list of classes to initialize

*******************************************************************************/
                                
static void compiler_addinitclass (classinfo *c)
{
	classinfo *cl;

	if (c->initialized) return;
	
	cl = chain_first(uninitializedclasses);
	if (cl == c)
		return;
	
	if (cl == class)
		cl = chain_next(uninitializedclasses);
	for (;;) {
		if (cl == c)
			return;
		if (cl == NULL) {
			if (runverbose) {
				sprintf(logtext, "compiler_addinitclass: ");
				utf_sprint(logtext+strlen(logtext), c->name);
				dolog();
				}
			chain_addlast(uninitializedclasses, c);
			return;
			}
		if (c < cl) {
			if (runverbose) {
				sprintf(logtext, "compiler_addinitclass: ");
				utf_sprint(logtext+strlen(logtext), c->name);
				dolog();
				}
			chain_addbefore(uninitializedclasses, c);
			return;
			}
		cl = chain_next(uninitializedclasses);
		}
}                       


/* function descriptor2typesL ***************************************************

	decodes a already checked method descriptor. The parameter count, the
	return type and the argument types are stored in the passed methodinfo.
        gets and saves classptr for object ref.s

*******************************************************************************/		

classSetNode * descriptor2typesL (methodinfo *m)
{
int debugInfo = 0;
	int i;
	u1 *types, *tptr;
	int pcount, c;
	char *utf_ptr;
	classinfo** classtypes;
	char *class; 
	char *desc;
	classSetNode *p=NULL;
if (debugInfo >= 1) {
	printf("In descriptor2typesL >>>\t"); fflush(stdout);
	utf_display(m->class->name); printf(".");
	method_display(m);fflush(stdout);
	}

	pcount = 0;
	desc =       MNEW (char, 256); 
	types = DMNEW (u1, m->descriptor->blength); 
	classtypes = MNEW (classinfo*, m->descriptor->blength+1);
	m->returnclass = NULL;
	tptr = types;
	if (!(m->flags & ACC_STATIC)) {
		*tptr++ = TYPE_ADR;
		if (debugInfo >= 1) {
			printf("param #0 (this?) method class =");utf_display(m->class->name);printf("\n");
			}
		classtypes[pcount] = m->class;
		p = addClassCone(p,  m->class);
		pcount++;
		}

	utf_ptr = m->descriptor->text + 1;
	strcpy (desc,utf_ptr);
   
	while ((c = *desc++) != ')') {
		pcount++;
		switch (c) {
			case 'B':
			case 'C':
			case 'I':
			case 'S':
			case 'Z':  *tptr++ = TYPE_INT;
			           break;
			case 'J':  *tptr++ = TYPE_LNG;
			           break;
			case 'F':  *tptr++ = TYPE_FLT;
			           break;
			case 'D':  *tptr++ = TYPE_DBL;
			           break;
			case 'L':  *tptr++ = TYPE_ADR;
                                   /* get class string */
                                   class = strtok(desc,";");
				   desc = strtok(NULL,"\0");
                                   /* get/save classinfo ptr */
				   classtypes[pcount-1] = class_get(utf_new_char(class));
				   p = addClassCone(p,  class_get(utf_new_char(class)));
					if (debugInfo >= 1) {
						printf("LParam#%i 's class type is: %s\n",pcount-1,class);fflush(stdout);
						printf("Lclasstypes[%i]=",pcount-1);fflush(stdout);
						utf_display(classtypes[pcount-1]->name);
						}
			           break;
			case '[':  *tptr++ = TYPE_ADR;
			           while (c == '[')
			               c = *desc++;
                                   /* get class string */
				   if (c == 'L') {
                                   	class = strtok(desc,";");
                                   	desc = strtok(NULL,"\0");
                                   	/* get/save classinfo ptr */
				   	classtypes[pcount-1] = class_get(utf_new_char(class));
				        p= addClassCone(p,  class_get(utf_new_char(class)));
					if (debugInfo >= 1) {
						printf("[Param#%i 's class type is: %s\n",pcount-1,class);
						printf("[classtypes[%i]=",pcount-1);fflush(stdout);
						utf_display(classtypes[pcount-1]->name);
						printf("\n");
						}
				       	}
				   else
					classtypes[pcount-1] = NULL;
			           break;
			default:   
				panic("Ill formed methodtype-descriptor");
			}
		}

	/* compute return type */
	switch (*desc++) {
		case 'B':
		case 'C':
		case 'I':
		case 'S':
		case 'Z':  m->returntype = TYPE_INT;
		           break;
		case 'J':  m->returntype = TYPE_LNG;
		           break;
		case 'F':  m->returntype = TYPE_FLT;
		           break;
		case 'D':  m->returntype = TYPE_DBL;
		           break;
		case '[':
			   m->returntype = TYPE_ADR;
			   c = *desc;
			   while (c == '[')
			       c = *desc++;
			   if (c != 'L') break;
			   *(desc++);
			   
		case 'L':  
			   m->returntype = TYPE_ADR;
			  
                            /* get class string */
			    class = strtok(desc,";");
			    m->returnclass = class_get(utf_new_char(class));
			    if (m->returnclass == NULL) {
				printf("class=%s :\t",class);
				panic ("return class not found");
				}
		           break;
		case 'V':  m->returntype = TYPE_VOID;
		           break;

	default:   panic("Ill formed methodtype-descriptor-ReturnType");
		}

	m->paramcount = pcount;
	m->paramtypes = types;
	m->paramclass = classtypes;

if (debugInfo >=1) {
	if (pcount > 0) {
  		for (i=0; i< m->paramcount; i++) {
    			if ((m->paramtypes[i] == TYPE_ADR) && (m->paramclass[i] != NULL)) {
			      printf("Param #%i is:\t",i);
			      utf_display(m->paramclass[i]->name);
			      printf("\n");
			      }
		   	 }
               }
	if ((m->returntype == TYPE_ADR) && (m->returnclass != NULL)) { 
		  printf("\tReturn Type is:\t"); fflush(stdout);
		  utf_display(m->returnclass->name);
		  printf("\n");
		  }

	printf("params2types: START  results in a set \n");
	printf("param2types: A Set size=%i=\n",sizeOfSet(p));
	printSet(p);
	}

return p;
}

/* function descriptor2types ***************************************************

	decodes a already checked method descriptor. The parameter count, the
	return type and the argument types are stored in the passed methodinfo.

*******************************************************************************/		

static void descriptor2types (methodinfo *m)
{
	u1 *types, *tptr;
	int pcount, c;
	char *utf_ptr;
	pcount = 0;
	types = DMNEW (u1, m->descriptor->blength); 
    	
	tptr = types;
	if (!(m->flags & ACC_STATIC)) {
		*tptr++ = TYPE_ADR;
		pcount++;
		}

	utf_ptr = m->descriptor->text + 1;
   
	while ((c = *utf_ptr++) != ')') {
		pcount++;
		switch (c) {
			case 'B':
			case 'C':
			case 'I':
			case 'S':
			case 'Z':  *tptr++ = TYPE_INT;
			           break;
			case 'J':  *tptr++ = TYPE_LNG;
			           break;
			case 'F':  *tptr++ = TYPE_FLT;
			           break;
			case 'D':  *tptr++ = TYPE_DBL;
			           break;
			case 'L':  *tptr++ = TYPE_ADR;
			           while (*utf_ptr++ != ';');
			           break;
			case '[':  *tptr++ = TYPE_ADR;
			           while (c == '[')
			               c = *utf_ptr++;
			           if (c == 'L')
			               while (*utf_ptr++ != ';') /* skip */;
			           break;
			default:   panic ("Ill formed methodtype-descriptor");
			}
		}

	/* compute return type */

	switch (*utf_ptr++) {
		case 'B':
		case 'C':
		case 'I':
		case 'S':
		case 'Z':  m->returntype = TYPE_INT;
		           break;
		case 'J':  m->returntype = TYPE_LNG;
		           break;
		case 'F':  m->returntype = TYPE_FLT;
		           break;
		case 'D':  m->returntype = TYPE_DBL;
		           break;
		case '[':
		case 'L':  m->returntype = TYPE_ADR;
		           break;
		case 'V':  m->returntype = TYPE_VOID;
		           break;

		default:   panic ("Ill formed methodtype-descriptor");
		}

	m->paramcount = pcount;
	m->paramtypes = types;
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
#define bound_check(i)     {if((i< 0) || (i>=cumjcodelength)) \
                               panic("branch target out of code-boundary");}
#define bound_check1(i)    {if((i< 0) || (i>cumjcodelength)) \
                               panic("branch target out of code-boundary");}
/* FIXME really use cumjcodelength for the bound_checkers ? */

static xtable* fillextable (xtable* extable, exceptiontable *raw_extable, int exceptiontablelength, int *label_index, int *block_count)
{
	int b_count, i, p;
	
	if (exceptiontablelength == 0) 
		return extable;
	
	b_count = *block_count;
	for (i = 0; i < exceptiontablelength; i++) {
								 
   		p = raw_extable[i].startpc;
		if (label_index != NULL) p = label_index[p];
		extable[i].startpc = p;
		bound_check(p);
		block_insert(p);
		
		p = raw_extable[i].endpc;
		if (label_index != NULL) p = label_index[p];
		extable[i].endpc = p;
		bound_check1(p);
		if (p < cumjcodelength)
			block_insert(p);

		p = raw_extable[i].handlerpc;
		if (label_index != NULL) p = label_index[p];
		extable[i].handlerpc = p;
		bound_check(p);
		block_insert(p);

		extable[i].catchtype  = raw_extable[i].catchtype;

		extable[i].next = NULL;
		extable[i].down = &extable[i+1];
		}
	*block_count = b_count;
	return &extable[i];  /* return the next free xtable* */
}

static void parse()
{
	int  p;                     /* java instruction counter                   */
	int  nextp;                 /* start of next java instruction             */
	int  opcode;                /* java opcode                                */
	int  i;                     /* temporary for different uses (counters)    */
	int  ipc = 0;               /* intermediate instruction counter           */
	int  b_count = 0;           /* basic block counter                        */
	int  s_count = 0;           /* stack element counter                      */
	bool blockend = false;      /* true if basic block end has been reached   */
	bool iswide = false;        /* true if last instruction was a wide        */
	instruction *iptr;          /* current pointer into instruction array     */
	int gp;                     /* global java instruction counter            */
				    /* inlining info for current method           */
	inlining_methodinfo *inlinfo = inlining_rootinfo, *tmpinlinf; 
	int nextgp = -1;            /* start of next method to be inlined         */
	int *label_index = NULL;    /* label redirection table                    */
	int firstlocal = 0;         /* first local variable of method             */
	xtable* nextex;             /* points next free entry in extable          */

	bool useinltmp;

/*INLINING*/
	if (useinlining)
		{
			label_index = inlinfo->label_index;
			maxstack = cummaxstack;
			exceptiontablelength=cumextablelength;
		}
	
	useinltmp = useinlining; /*FIXME remove this after debugging */
    /*useinlining = false;*/ 	 /* and merge the if-statements  */
	
	if (!useinlining) {
	  cumjcodelength = jcodelength;
	} else {
	  tmpinlinf = (inlining_methodinfo*) list_first(inlinfo->inlinedmethods);
	  if (tmpinlinf != NULL) nextgp = tmpinlinf->startgp;
	}

                /*RTAprint*/ if  ( ((opt_rt) ||(opt_xta) || (opt_vta)) && ((pOpcodes == 2) || (pOpcodes == 3)) )
                /*RTAprint*/    {printf("PARSE method name =");
                /*RTAprint*/    utf_display(method->class->name);printf(".");
                /*RTAprint*/    method_display(method); printf(">\n\n");fflush(stdout);}
	if ((opt_rt) || (opt_xta)) { 
            RT_jit_parse(method);
	    }
	else 	{
		if (opt_vta) 
			printf("VTA requested, but not yet implemented\n");
		}
	 

	/* allocate instruction array and block index table */
	
	/* 1 additional for end ipc and 3 for loop unrolling */
	
	block_index = DMNEW(int, cumjcodelength + 4);

	/* 1 additional for TRACEBUILTIN and 4 for MONITORENTER/EXIT */
	/* additional MONITOREXITS are reached by branches which are 3 bytes */
	
	iptr = instr = DMNEW(instruction, cumjcodelength + 5); 
	
	/* initialize block_index table (unrolled four times) */

	{
	int *ip;
	
	for (i = 0, ip = block_index; i <= cumjcodelength; i += 4, ip += 4) {
		ip[0] = 0;
		ip[1] = 0;
		ip[2] = 0;
		ip[3] = 0;
		}
	}

	/* compute branch targets of exception table */

	extable = DMNEW(xtable, exceptiontablelength + 1);
	/*
	for (i = 0; i < method->exceptiontablelength; i++) {

   		p = extable[i].startpc = raw_extable[i].startpc;
		if (useinlining) p = label_index[p];
		bound_check(p);
		block_insert(p);

		p = extable[i].endpc = raw_extable[i].endpc;
		if (useinlining) p = label_index[p];
		bound_check1(p);
		if (p < cumjcodelength)
			block_insert(p);

		p = extable[i].handlerpc = raw_extable[i].handlerpc;
		bound_check(p);
		block_insert(p);

		extable[i].catchtype  = raw_extable[i].catchtype;

		extable[i].next = NULL;
		extable[i].down = &extable[i+1];
		}
	*/

	nextex = fillextable(extable, raw_extable, method->exceptiontablelength, label_index, &b_count);

	s_count = 1 + exceptiontablelength; /* initialize stack element counter   */

#ifdef USE_THREADS
	if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
		isleafmethod=false;
		}			
#endif

	/* scan all java instructions */

	for (p = 0, gp = 0; p < jcodelength; gp += (nextp - p), p = nextp) {
	  
	  /* DEBUG	  printf("p:%d gp:%d ",p,gp); */

/*INLINING*/
	  if ((useinlining) && (gp == nextgp)) {
		  u1 *tptr;
		  bool *readonly = NULL;

		  opcode = code_get_u1 (p);
		  nextp = p += jcommandsize[opcode];
		  tmpinlinf = list_first(inlinfo->inlinedmethods);
		  firstlocal = tmpinlinf->firstlocal;
		  label_index = tmpinlinf->label_index;
		  readonly = tmpinlinf->readonly;
		  for (i=0, tptr=tmpinlinf->method->paramtypes + tmpinlinf->method->paramcount - 1 ; i<tmpinlinf->method->paramcount; i++, tptr--)
			  {
				  int op;

				  if ( (i==0) && inlineparamopt) {
					  OP1(ICMD_CLEAR_ARGREN, firstlocal);
				  }

				  if ( !inlineparamopt || !readonly[i] )
					  op = ICMD_ISTORE;
				  else op = ICMD_READONLY_ARG;   

				  op += *tptr;
				  OP1(op, firstlocal + tmpinlinf->method->paramcount - 1 - i);

				  /* block_index[gp] |= (ipc << 1);*/  /*FIXME: necessary ? */
			  }
		  inlining_save_compiler_variables();
		  inlining_set_compiler_variables(tmpinlinf);
		  if (inlinfo->inlinedmethods == NULL) gp = -1;
		  else {
			  tmpinlinf = list_first(inlinfo->inlinedmethods);
			  nextgp = (tmpinlinf != NULL) ? tmpinlinf->startgp : -1;
		  }
		  if (method->exceptiontablelength > 0) 
			  nextex = fillextable(nextex, method->exceptiontable, method->exceptiontablelength, label_index, &b_count);
		  continue;
	  }
	  
	  opcode = code_get_u1 (p);           /* fetch op code                  */

	  
	  /*RTAprint*/ if  ((opt_rt) && ((pOpcodes == 2) || (pOpcodes == 3)) )
	  /*RTAprint*/    {printf("Parse<%i> p=%i<%i<   opcode=<%i> %s\n",
	  /*RTAprint*/            pOpcodes, p,rt_jcodelength,opcode,icmd_names[opcode]);}
	  
	  block_index[gp] |= (ipc << 1);       /* store intermediate count       */

	  if (blockend) {
		  block_insert(gp);                /* start new block                */
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
						LOADCONST_A(literalstring_new((utf*)
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
				OP1(opcode, i + firstlocal);
				break;

			case JAVA_ILOAD_0:
			case JAVA_ILOAD_1:
			case JAVA_ILOAD_2:
			case JAVA_ILOAD_3:
				OP1(ICMD_ILOAD, opcode - JAVA_ILOAD_0 + firstlocal);
				break;

			case JAVA_LLOAD_0:
			case JAVA_LLOAD_1:
			case JAVA_LLOAD_2:
			case JAVA_LLOAD_3:
				OP1(ICMD_LLOAD, opcode - JAVA_LLOAD_0 + firstlocal);
				break;

			case JAVA_FLOAD_0:
			case JAVA_FLOAD_1:
			case JAVA_FLOAD_2:
			case JAVA_FLOAD_3:
				OP1(ICMD_FLOAD, opcode - JAVA_FLOAD_0 + firstlocal);
				break;

			case JAVA_DLOAD_0:
			case JAVA_DLOAD_1:
			case JAVA_DLOAD_2:
			case JAVA_DLOAD_3:
				OP1(ICMD_DLOAD, opcode - JAVA_DLOAD_0 + firstlocal);
				break;

			case JAVA_ALOAD_0:
			case JAVA_ALOAD_1:
			case JAVA_ALOAD_2:
			case JAVA_ALOAD_3:
				OP1(ICMD_ALOAD, opcode - JAVA_ALOAD_0 + firstlocal);
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
				OP1(opcode, i + firstlocal);
				break;

			case JAVA_ISTORE_0:
			case JAVA_ISTORE_1:
			case JAVA_ISTORE_2:
			case JAVA_ISTORE_3:
				OP1(ICMD_ISTORE, opcode - JAVA_ISTORE_0 + firstlocal);
				break;

			case JAVA_LSTORE_0:
			case JAVA_LSTORE_1:
			case JAVA_LSTORE_2:
			case JAVA_LSTORE_3:
				OP1(ICMD_LSTORE, opcode - JAVA_LSTORE_0 + firstlocal);
				break;

			case JAVA_FSTORE_0:
			case JAVA_FSTORE_1:
			case JAVA_FSTORE_2:
			case JAVA_FSTORE_3:
				OP1(ICMD_FSTORE, opcode - JAVA_FSTORE_0 + firstlocal);
				break;

			case JAVA_DSTORE_0:
			case JAVA_DSTORE_1:
			case JAVA_DSTORE_2:
			case JAVA_DSTORE_3:
				OP1(ICMD_DSTORE, opcode - JAVA_DSTORE_0 + firstlocal);
				break;

			case JAVA_ASTORE_0:
			case JAVA_ASTORE_1:
			case JAVA_ASTORE_2:
			case JAVA_ASTORE_3:
				OP1(ICMD_ASTORE, opcode - JAVA_ASTORE_0 + firstlocal);
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
				OP2I(opcode, i + firstlocal, v);
				}
				break;

			/* wider index for loading, storing and incrementing */

			case JAVA_WIDE:
				iswide = true;
				nextp = p + 1;
				break;

			/* managing arrays ************************************************/

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
					s_count++;
					LOADCONST_A(class_getconstant(class, i,
					                              CONSTANT_Arraydescriptor));
#if defined(__I386__)
					BUILTIN2((functionptr) asm_builtin_newarray_array, TYPE_ADR);
#else
					BUILTIN2((functionptr)builtin_newarray_array, TYPE_ADR);
#endif
					}
				else {
				 	LOADCONST_A(class_getconstant(class, i, CONSTANT_Class));
					s_count++;
#if defined(__I386__)
					BUILTIN2((functionptr) asm_builtin_anewarray, TYPE_ADR);
#else
					BUILTIN2((functionptr)builtin_anewarray, TYPE_ADR);
#endif
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
				if (useinlining) { 
				  debug_writebranch
				  i = label_index[i];
				}
				bound_check(i);
				block_insert(i);
				blockend = true;
				OP1(opcode, i);
				break;
			case JAVA_GOTO_W:
			case JAVA_JSR_W:
				i = p + code_get_s4(p+1);
				if (useinlining) { 
				  debug_writebranch
				  i = label_index[i];
				}
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
				
				/*
				if (isinlinedmethod) {
				  OP1(ICMD_GOTO, inlinfo->stopgp);
				  break;
				  }*/

				OP1(opcode, i + firstlocal);
				break;

			case JAVA_IRETURN:
			case JAVA_LRETURN:
			case JAVA_FRETURN:
			case JAVA_DRETURN:
			case JAVA_ARETURN:
			case JAVA_RETURN:


				if (isinlinedmethod) {
/*  					if (p==jcodelength-1) {*/ /* return is at end of inlined method */
/*  						OP(ICMD_NOP); */
/*  						break; */
/*  					} */
					blockend = true;
					OP1(ICMD_GOTO, inlinfo->stopgp);
					break;
				}

				blockend = true;
				OP(opcode);
				break;

			case JAVA_ATHROW:
				blockend = true;
				OP(opcode);
				break;
				

			/* table jumps ********************************/

			case JAVA_LOOKUPSWITCH:
				{
				s4 num, j;
				s4 *tablep;

				blockend = true;
				nextp = ALIGN((p + 1), 4);
				if (!useinlining) {
					tablep = (s4*)(jcode + nextp);
				}
				else {
					num = code_get_u4(nextp + 4);
					tablep = DMNEW(s4, num * 2 + 2);
				}

				OP2A(opcode, 0, tablep);

				/* default target */

				j =  p + code_get_s4(nextp);
				if (useinlining) j = label_index[j];
				*tablep = j;     /* restore for little endian */
				tablep++;
				nextp += 4;
				bound_check(j);
				block_insert(j);

				/* number of pairs */

				num = code_get_u4(nextp);
				*tablep = num;
				tablep++;
				nextp += 4;

				for (i = 0; i < num; i++) {

					/* value */

					j = code_get_s4(nextp);
					*tablep = j; /* restore for little endian */
					tablep++;
					nextp += 4;

					/* target */

					j = p + code_get_s4(nextp);
					if (useinlining) j = label_index[j];
					*tablep = j; /* restore for little endian */
					tablep++;
					nextp += 4;
					bound_check(j);
					block_insert(j);
					}

				break;
				}


			case JAVA_TABLESWITCH:
				{
				s4 num, j;
				s4 *tablep;

				blockend = true;
				nextp = ALIGN((p + 1), 4);
				if (!useinlining) {
					tablep = (s4*)(jcode + nextp);
				}
				else {
					num = code_get_u4(nextp + 8) - code_get_u4(nextp + 4);
					tablep = DMNEW(s4, num + 1 + 3);
				}

				OP2A(opcode, 0, tablep);

				/* default target */

				j = p + code_get_s4(nextp);
				if (useinlining) j = label_index[j];
				*tablep = j;     /* restore for little endian */
				tablep++;
				nextp += 4;
				bound_check(j);
				block_insert(j);

				/* lower bound */

				j = code_get_s4(nextp);
				*tablep = j;     /* restore for little endian */
				tablep++;
				nextp += 4;

				/* upper bound */

				num = code_get_s4(nextp);
				*tablep = num;   /* restore for little endian */
				tablep++;
				nextp += 4;

				num -= j;

				for (i = 0; i <= num; i++) {
					j = p + code_get_s4(nextp);
					if (useinlining) j = label_index[j];
					*tablep = j; /* restore for little endian */
					tablep++;
					nextp += 4;
					bound_check(j);
					block_insert(j);
					}

				break;
				}


			/* load and store of object fields *******************/

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


			/* method invocation *****/

			case JAVA_INVOKESTATIC:
			        i = code_get_u2(p + 1);
				{
				constant_FMIref *mr;
				methodinfo *mi;
				
				mr = class_getconstant (class, i, CONSTANT_Methodref);
				mi = class_findmethod (mr->class, mr->name, mr->descriptor);
                			/*RTAprint*/ if (((pOpcodes == 2) || (pOpcodes == 3)) && opt_rt)
                                        /*RTAprint*/    {printf(" method name =");
                                        /*RTAprint*/    utf_display(mi->class->name); printf(".");
                                        /*RTAprint*/    utf_display(mi->name);printf("\tINVOKE STATIC\n");
                                        /*RTAprint*/    fflush(stdout);}
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
                			/*RTAprint*/ if (((pOpcodes == 2) || (pOpcodes == 3)) && opt_rt)
                                        /*RTAprint*/    {printf(" method name =");
							method_display(mi);
                                        /*RTAprint*/    utf_display(mi->class->name); printf(".");
                                        /*RTAprint*/    utf_display(mi->name);printf("\tINVOKE SPECIAL/VIRTUAL\n");
                                        /*RTAprint*/    fflush(stdout);}

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

			/* miscellaneous object operations *******/

			case JAVA_NEW:
				i = code_get_u2 (p+1);

				LOADCONST_A(class_getconstant(class, i, CONSTANT_Class));
				s_count++;
				BUILTIN1((functionptr) builtin_new, TYPE_ADR);
				break;

			case JAVA_CHECKCAST:
				i = code_get_u2(p+1);

				/* array type cast-check */
				if (class_constanttype (class, i) == CONSTANT_Arraydescriptor) {
					LOADCONST_A(class_getconstant(class, i, CONSTANT_Arraydescriptor));
					s_count++;
					BUILTIN2((functionptr) asm_builtin_checkarraycast, TYPE_ADR);
					}
				else { /* object type cast-check */
					/*
					LOADCONST_A(class_getconstant(class, i, CONSTANT_Class));
					s_count++;
					BUILTIN2((functionptr) asm_builtin_checkcast, TYPE_ADR);
					*/
				 	OP2A(opcode, 1, (class_getconstant(class, i, CONSTANT_Class)));
					}
				break;

			case JAVA_INSTANCEOF:
				i = code_get_u2(p+1);

				/* array type cast-check */
				if (class_constanttype (class, i) == CONSTANT_Arraydescriptor) {
					LOADCONST_A(class_getconstant(class, i, CONSTANT_Arraydescriptor));
					s_count++;
#if defined(__I386__)
					BUILTIN2((functionptr) asm_builtin_arrayinstanceof, TYPE_INT);
#else
					BUILTIN2((functionptr) builtin_arrayinstanceof, TYPE_INT);
#endif
					}
				else { /* object type cast-check */
					/*
					LOADCONST_A(class_getconstant(class, i, CONSTANT_Class));
					s_count++;
					BUILTIN2((functionptr) builtin_instanceof, TYPE_INT);
					*/
					OP2A(opcode, 1, (class_getconstant(class, i, CONSTANT_Class)));
					}
				break;

			case JAVA_MONITORENTER:
#ifdef USE_THREADS
				if (checksync) {
					BUILTIN1((functionptr) asm_builtin_monitorenter, TYPE_VOID);
				} else
#endif
					{
					OP(ICMD_NULLCHECKPOP);
					}
				break;

			case JAVA_MONITOREXIT:
#ifdef USE_THREADS
				if (checksync) {
					BUILTIN1((functionptr) asm_builtin_monitorexit, TYPE_VOID);
					}
				else
#endif
					{
					OP(ICMD_POP);
					}
				break;

			/* any other basic operation **************************************/

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
#if defined(__I386__)
  				OP(opcode);
#else
				BUILTIN2((functionptr) builtin_frem, TYPE_FLOAT);
#endif
				break;

			case JAVA_DREM:
#if defined(__I386__)
				OP(opcode);
#else
				BUILTIN2((functionptr) builtin_drem, TYPE_DOUBLE);
#endif
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
		
		/* INLINING */
		  
		if ((isinlinedmethod) && (p==jcodelength-1)) { /*end of an inlined method */
		  /*		  printf("setting gp from %d to %d\n",gp, inlinfo->stopgp); */
		  gp = inlinfo->stopgp; 
		  inlining_restore_compiler_variables();
		  list_remove(inlinfo->inlinedmethods, list_first(inlinfo->inlinedmethods));
		  if (inlinfo->inlinedmethods == NULL) nextgp = -1;
		  else {
			  tmpinlinf = list_first(inlinfo->inlinedmethods);
			  nextgp = (tmpinlinf != NULL) ? tmpinlinf->startgp : -1;
		  }
		  /*		  printf("nextpgp: %d\n", nextgp); */
		  label_index=inlinfo->label_index;
		  firstlocal = inlinfo->firstlocal;
		}

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
	c_debug_nr = 0;
	
	/* additional block if target 0 is not first intermediate instruction     */

	if (!block_index[0] || (block_index[0] > 1)) {
		bptr->iinstr = instr;
		bptr->mpc = -1;
		bptr->flags = -1;
		bptr->type = BBTYPE_STD;
		bptr->branchrefs = NULL;
		bptr->pre_count = 0;
		bptr->debug_nr = c_debug_nr++;
		bptr++;
		b_count++;
		(bptr - 1)->next = bptr;
	
		}

	/* allocate blocks */


	for (p = 0; p < cumjcodelength; p++)
		
		if (block_index[p] & 1) {
			bptr->iinstr = instr + (block_index[p] >> 1);
			bptr->debug_nr = c_debug_nr++;
			if (b_count != 0)
				(bptr - 1)->icount = bptr->iinstr - (bptr - 1)->iinstr;
			bptr->mpc = -1;
			bptr->flags = -1;
			bptr->lflags = 0;
			bptr->type = BBTYPE_STD;
			bptr->branchrefs = NULL;
			block_index[p] = b_count;
			bptr->pre_count = 0;
			bptr++;
			b_count++;

			(bptr - 1)->next = bptr;
			}

	/* allocate additional block at end */

	
	bptr->instack = bptr->outstack = NULL;
	bptr->indepth = bptr->outdepth = 0;
	bptr->iinstr = NULL;
	(bptr - 1)->icount = (instr + instr_count) - (bptr - 1)->iinstr;
	bptr->icount = 0;
	bptr->mpc = -1;
	bptr->flags = -1;
	bptr->lflags = 0;
	bptr->type = BBTYPE_STD;
	bptr->branchrefs = NULL;
	bptr->pre_count = 0;
	bptr->debug_nr = c_debug_nr++;
			
	(bptr - 1)->next = bptr;
	bptr->next = NULL;

	last_block = bptr;

	if (exceptiontablelength > 0)
		extable[exceptiontablelength-1].down = NULL;
	else
		extable = NULL;

	for (i = 0; i < exceptiontablelength; ++i) {
		p = extable[i].startpc;
		extable[i].start = block + block_index[p];

		p = extable[i].endpc;
		extable[i].end = block + block_index[p]; 

		p = extable[i].handlerpc;
		extable[i].handler = block + block_index[p];
	    }
	}
	
	if (useinlining) inlining_cleanup();
	useinlining = useinltmp;
}
#include "sets.c"
#include "parseRT.h"

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
