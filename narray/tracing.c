/* tracing.c *******************************************************************

        Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

        See file COPYRIGHT for information on usage and disclaimer of warranties.

        Contains the functions which create a trace. A trace is a structure, that
		contains the source of the arguments of a given instruction. For more
		details see function tracing(basicblock, int, int) below.

        Authors: Christopher Kruegel      EMAIL: cacao@complang.tuwien.ac.at

        Last Change: 1998/17/02

*******************************************************************************/

/*	Test function -> will be removed in final release
*/
void printTraceResult(struct Trace *p)
{
	printf("TRACE: ");

	switch (p->type) {
	case TRACE_UNKNOWN:
		printf("\tUnknown");
		break;
	case TRACE_ICONST:
		printf("\tconst - %d", p->constant);
		break;
	case TRACE_ALENGTH:
		printf("\tarray - (%d)%d - %d", p->neg, p->var, p->constant);
		break;
	case TRACE_IVAR:
		printf("\tivar - (%d)%d - %d", p->neg, p->var, p->constant);
		break;
	case TRACE_AVAR:
		printf("\tavar - %d", p->var);
		break;
		}
	
	printf("\n");
}

    
/*	A function that creates a new trace structure and initializes its values
*/
struct Trace* create_trace(int type, int var, int constant, int nr)
{
	struct Trace *t;
	if ((t = (struct Trace *) malloc(sizeof(struct Trace))) == NULL)
		c_mem_error();

	t->type = type;

	t->neg = 1;
	t->var = var;
	t->nr = nr;

	t->constant = constant;

	return t;
}

/*	When the function tracing(...) encounters an add instruction during its 
	backward scan over the instructions, it trys to identify the source of the
	arguments of this add function. The following function performs this task.
*/
struct Trace* add(struct Trace* a, struct Trace* b)
{
	switch (a->type) {			/* check the first argument of add. when it		*/
	case TRACE_UNKNOWN:			/* is unknown or array-address, return unknown	*/
	case TRACE_AVAR:
		return create_trace(TRACE_UNKNOWN, -1, 0, 0);

	case TRACE_ICONST:			/* when it is constant, check second argument	*/
		switch (b->type) {
		case TRACE_IVAR:			/* when second argument is a variable value */
		case TRACE_ALENGTH:			/* or array length, just add the constant	*/
			a->type = b->type;
			a->var  = b->var;
			a->neg  = b->neg;
			a->constant += b->constant;
			break;
		case TRACE_UNKNOWN:			/* when unknown/array ref. return unknown	*/
		case TRACE_AVAR:
		      return create_trace(TRACE_UNKNOWN, -1, 0, 0);
	    case TRACE_ICONST:			/* when both are constant, just add them	*/
			a->constant += b->constant;
			break;
			}
		break;

	case TRACE_IVAR:			/* when it is a variable value or array length, */
	case TRACE_ALENGTH:			/* check second argument						*/
		switch (b->type) {
		case TRACE_IVAR:			/* when it is not a constant return unknown	*/
		case TRACE_ALENGTH:
		case TRACE_UNKNOWN:
		case TRACE_AVAR:
			return create_trace(TRACE_UNKNOWN, -1, 0, 0);
		case TRACE_ICONST:			/* when it is a constant, just add it		*/
			a->constant += b->constant;
			break;
			}
		break;
		}

	return a;
}

/*	When the function tracing(...) encounters a neg instruction during its 
	backward scan over the instructions, it trys to identify the source of the
	argument of this neg function. The following function performs this task.
*/
struct Trace* negate(struct Trace* a)
{
	switch (a->type) {				/* check argument type						*/
	case TRACE_IVAR:				/* when it is variable/array length value	*/
	case TRACE_ALENGTH:					
		a->neg = -(a->neg);				/* invert negate flag					*/
		a->constant = -(a->constant);	/* and negate constant					*/
		break;
	
	case TRACE_ICONST:				/* when it is a constant, negate it			*/
		a->constant = -(a->constant);
		break;

	default:
		a->type = TRACE_UNKNOWN;	/* else return unknown						*/
		break;
		}

  return a;
}

/*	When the function tracing(...) encounters a sub instruction during its backward
	scan over the instructions, it trys to identify the source of the arguments of
	this sub function. The following function performs this task, by applaying the
	negate function on the second argument and then adds the values.
*/
struct Trace* sub(struct Trace* a, struct Trace* b)
{
	struct Trace *c = negate(b);
	return add(a, c);
}


/*	When the function tracing(...) encounters an array length instruction during
	its backward scan over the instructions, it trys to identify the source of 
	the argument ofthis array length function. The following function performs 
	this task.
*/
struct Trace* array_length(struct Trace* a)
{
	if (a->type == TRACE_AVAR)	/* if argument is an array ref., mark the type	*/
		a->type = TRACE_ALENGTH;	/* as array length of this array reference	*/
	else
		a->type = TRACE_UNKNOWN;	/* else it's unknown				        */

	return a;
}

/*	This function is used to identify the types of operands of an intermediate 
	code instruction.It is needed by functions, that analyze array accesses. If 
	something is stored into or loaded from an array, we have to find out, which 
	array really has been accessed. When a compare instruction is encountered at
	a loop header, the type of its operands have to be detected to construct
	dynamic bounds for some variables in the loop. This function returns a struct
	Trace (see loop.h for more details about this structure). block is the basic
	block to be examined, index holds the offset of the examined instruction in
	this block. The arguments are retrieved by using the stack structure, the 
	compilation process sets up. During the backwards scan of the code, it is 
	possible, that other instructions temporaray put or get values from the stack
	and hide the value, we are interested in below them. The value temp counts
	the number of values on the stack, the are located beyond the target value.
*/
struct Trace* tracing(basicblock *block, int index, int temp)
{
	int args, retval;
	instruction *ip;
	methodinfo *m;

	if (index >= 0) {
		ip = block->iinstr+index;

/*	printf("TRACING with %d %d %d\n", index, temp, ip->opc);
*/
		switch (ip->opc) {
		
		/* nop, nullcheckpop													*/
		case ICMD_NOP:				/* ...  ==> ...								*/
			return tracing(block, index-1, temp);
			break;
      
		case ICMD_NULLCHECKPOP:		/* ..., objectref  ==> ...					*/
			return tracing(block, index-1, temp+1);
			break;

		/* Constants															*/
		case ICMD_LCONST:				
		case ICMD_DCONST:
		case ICMD_FCONST:
		case ICMD_ACONST:
			if (temp > 0)				
				return tracing(block, index-1, temp-1);		
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;						

		case ICMD_ICONST:
			if (temp > 0)		/* if the target argument is not on top			*/
				return tracing(block, index-1, temp-1);	/* look further			*/
			else
				return create_trace(TRACE_ICONST, -1, ip->val.i, index);
			break;				/* else, return the value, found at this instr.	*/

		/* Load/Store															*/
        case ICMD_LLOAD:		
		case ICMD_DLOAD:    
		case ICMD_FLOAD:
			if (temp > 0)
				return tracing(block, index-1, temp-1);
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;
    
		case ICMD_ILOAD:
			if (temp > 0)
				return tracing(block, index-1, temp-1);
		    else
				return create_trace(TRACE_IVAR, ip->op1, 0, index);
			break;

		case ICMD_ALOAD:    
			if (temp > 0)
				return tracing(block, index-1, temp-1);
			else
				return create_trace(TRACE_AVAR, ip->op1, 0, index);			
			break;
		
		case ICMD_LSTORE:    
		case ICMD_DSTORE:    
		case ICMD_FSTORE:
		case ICMD_ISTORE:
		case ICMD_ASTORE:    
			return tracing(block, index-1, temp+1);
			break;
	

		/* pop/dup/swap															*/
		case ICMD_POP:      
			return tracing(block, index-1, temp+1);
			break;
     
		case ICMD_POP2:  
			return tracing(block, index-1, temp+2);
			break;

		case ICMD_DUP:
			if (temp > 0)
				return tracing(block, index-1, temp-1);
			else
				return tracing(block, index-1, temp);
			break;

		case ICMD_DUP_X1:			/* ..., a, b ==> ..., b, a, b				*/
			switch (temp) {
			case 0:					/* when looking for top or third element,	*/
			case 2:					/* just return top element					*/
				return tracing(block, index-1, 0);
			case 1:
				return tracing(block, index-1, 1);
			default:
				return tracing(block, index-1, temp-1);
				}
			break;

		case ICMD_DUP2:				/* ..., a, b ==> ..., a, b, a, b			*/
			switch (temp) { 
			case 0:					/* when looking for top or third element	*/
			case 2:					/* just return top element					*/
				return tracing(block, index-1, 0);
			case 1:					/* when looking for second or fourth element*/
			case 3:					/* just return second element				*/
				return tracing(block, index-1, 1);
			default:
				return tracing(block, index-1, temp-2);
				}
			break;

		case ICMD_DUP2_X1:			/* ..., a, b, c ==> ..., b, c, a, b, c		*/
			switch (temp) {
			case 0:
			case 3:
				return tracing(block, index-1, 0);
			case 1:
			case 4:
				return tracing(block, index-1, 1);
			case 2:
				return tracing(block, index-1, 2);
			default:
				return tracing(block, index-1, temp-2);
				}
			break;

		case ICMD_DUP_X2:			/* ..., a, b, c ==> ..., c, a, b, c			*/
			switch (temp) {    
			case 0:
			case 3:
				return tracing(block, index-1, 0);
			case 1:
			case 4:
				return tracing(block, index-1, 1);
			case 2:
				return tracing(block, index-1, 2);
			default:
				return tracing(block, index-1, temp-2);
				}
			break;

		case ICMD_DUP2_X2:			/* .., a, b, c, d ==> ..., c, d, a, b, c, d	*/
			switch (temp) {
			case 0:
			case 4:
				return tracing(block, index-1, 0);
			case 1:
			case 5:
				return tracing(block, index-1, 1);
			case 2:
				return tracing(block, index-1, 2);
			case 3:
				return tracing(block, index-1, 3);
			default:
				return tracing(block, index-1, temp-2);
				}
			break;

		case ICMD_SWAP:				/* ..., a, b ==> ..., b, a					*/
			switch (temp) {
			case 0:
				return tracing(block, index-1, 1);
			case 1:
				return tracing(block, index-1, 0);
			default:
				return tracing(block, index-1, temp);
				}
			break;
      
		/* Interger operations													*/
	    case ICMD_INEG:				/* ..., value  ==> ..., - value				*/
			if (temp > 0)
				return tracing(block, index-1, temp);
			else					/* if an inter neg. operation is found,		*/
									/* invokethe negate function				*/
				return negate(tracing(block, index-1, temp));
			break;

		case ICMD_LNEG:				/* ..., value  ==> ..., - value				*/
		case ICMD_I2L:				/* ..., value  ==> ..., value				*/
		case ICMD_L2I:				/* ..., value  ==> ..., value				*/
		case ICMD_INT2BYTE:			/* ..., value  ==> ..., value				*/
		case ICMD_INT2CHAR:			/* ..., value  ==> ..., value				*/
		case ICMD_INT2SHORT:		/* ..., value  ==> ..., value				*/
			if (temp > 0)
				return tracing(block, index-1, temp);
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;

		case ICMD_IADD:				/* ..., val1, val2  ==> ..., val1 + val2	*/
			if (temp > 0)
				return tracing(block, index-1, temp+1);
			else					/* when add is encountered, invoke add func	*/
				return add(tracing(block, index-1, 0), tracing(block, index-1, 1));
			break;

		case ICMD_IADDCONST:		/* ..., value  ==> ..., value + constant	*/
			if (temp > 0)
				return tracing(block, index-1, temp);
			else					/* when a constant is added, create a		*/
									/* constant-trace and use add function		*/
				return add(tracing(block, index-1, 0), create_trace(TRACE_ICONST, -1, ip->val.i, index));
			break;

		case ICMD_ISUB:				/* ..., val1, val2  ==> ..., val1 - val2	*/
			if (temp > 0)
				return tracing(block, index-1, temp+1);
			else					/* use sub function for sub instructions	*/
				return sub(tracing(block, index-1, 1), tracing(block, index-1, 0));
			break;

		case ICMD_ISUBCONST:		/* ..., value  ==> ..., value + constant	*/
			if (temp > 0)
				return tracing(block, index-1, temp);
			else
				return sub(tracing(block, index-1, 0), create_trace(TRACE_ICONST, -1, ip->val.i, index));
			break;

		case ICMD_LADD:				/* ..., val1, val2  ==> ..., val1 + val2	*/
		case ICMD_LSUB:				/* ..., val1, val2  ==> ..., val1 - val2	*/
		case ICMD_IMUL:				/* ..., val1, val2  ==> ..., val1 * val2	*/
		case ICMD_LMUL:				/* ..., val1, val2  ==> ..., val1 * val2	*/
		case ICMD_ISHL:				/* ..., val1, val2  ==> ..., val1 << val2	*/
		case ICMD_ISHR:				/* ..., val1, val2  ==> ..., val1 >> val2	*/
		case ICMD_IUSHR:			/* ..., val1, val2  ==> ..., val1 >>> val2	*/
		case ICMD_LSHL:				/* ..., val1, val2  ==> ..., val1 << val2	*/
		case ICMD_LSHR:				/* ..., val1, val2  ==> ..., val1 >> val2	*/
		case ICMD_LUSHR:			/* ..., val1, val2  ==> ..., val1 >>> val2	*/
		case ICMD_IAND:				/* ..., val1, val2  ==> ..., val1 & val2	*/
		case ICMD_LAND:  
		case ICMD_IOR:				/* ..., val1, val2  ==> ..., val1 | val2	*/
		case ICMD_LOR:
		case ICMD_IXOR:				/* ..., val1, val2  ==> ..., val1 ^ val2	*/
		case ICMD_LXOR:
			if (temp > 0)
				return tracing(block, index-1, temp+1);
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;
     
		case ICMD_LADDCONST:		/* ..., value  ==> ..., value + constant	*/
		case ICMD_LSUBCONST:		/* ..., value  ==> ..., value - constant	*/
		case ICMD_IMULCONST:		/* ..., value  ==> ..., value * constant	*/
		case ICMD_LMULCONST:		/* ..., value  ==> ..., value * constant	*/
		case ICMD_IDIVPOW2:			/* ..., value  ==> ..., value << constant	*/
		case ICMD_LDIVPOW2:			/* val.i = constant							*/
		case ICMD_ISHLCONST:		/* ..., value  ==> ..., value << constant	*/
		case ICMD_ISHRCONST:		/* ..., value  ==> ..., value >> constant	*/
		case ICMD_IUSHRCONST:		/* ..., value  ==> ..., value >>> constant	*/
		case ICMD_LSHLCONST:		/* ..., value  ==> ..., value << constant	*/
		case ICMD_LSHRCONST:		/* ..., value  ==> ..., value >> constant	*/
		case ICMD_LUSHRCONST:		/* ..., value  ==> ..., value >>> constant	*/
		case ICMD_IANDCONST:		/* ..., value  ==> ..., value & constant	*/
		case ICMD_IREMPOW2:			/* ..., value  ==> ..., value % constant	*/
		case ICMD_IREM0X10001:		/* ..., value  ==> ..., value % 0x100001	*/
		case ICMD_LANDCONST:		/* ..., value  ==> ..., value & constant	*/
		case ICMD_LREMPOW2:			/* ..., value  ==> ..., value % constant	*/
		case ICMD_LREM0X10001:		/* ..., value  ==> ..., value % 0x10001		*/
		case ICMD_IORCONST:			/* ..., value  ==> ..., value | constant	*/
		case ICMD_LORCONST:			/* ..., value  ==> ..., value | constant	*/  
		case ICMD_IXORCONST:		/* ..., value  ==> ..., value ^ constant	*/
		case ICMD_LXORCONST:		/* ..., value  ==> ..., value ^ constant	*/
		case ICMD_LCMP:				/* ..., val1, val2  ==> ..., val1 cmp val2	*/
			if (temp > 0)
				return tracing(block, index-1, temp);
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;

		case ICMD_IINC:				/* ..., value  ==> ..., value + constant	*/
			return tracing(block, index-1, temp);
			break;


		/* floating operations													*/
		case ICMD_FADD:				/* ..., val1, val2  ==> ..., val1 + val2	*/
		case ICMD_DADD:				/* ..., val1, val2  ==> ..., val1 + val2	*/
		case ICMD_FSUB:				/* ..., val1, val2  ==> ..., val1 - val2	*/
		case ICMD_DSUB:				/* ..., val1, val2  ==> ..., val1 - val2	*/
		case ICMD_FMUL:				/* ..., val1, val2  ==> ..., val1 * val2	*/
		case ICMD_DMUL:				/* ..., val1, val2  ==> ..., val1 *** val2	*/
		case ICMD_FDIV:				/* ..., val1, val2  ==> ..., val1 / val2	*/
		case ICMD_DDIV:				/* ..., val1, val2  ==> ..., val1 / val2	*/
		case ICMD_FREM:				/* ..., val1, val2  ==> ..., val1 % val2	*/
		case ICMD_DREM:				/* ..., val1, val2  ==> ..., val1 % val2	*/
		case ICMD_FCMPL:			/* .., val1, val2  ==> ..., val1 fcmpl val2	*/
		case ICMD_DCMPL:
		case ICMD_FCMPG:			/* .., val1, val2  ==> ..., val1 fcmpg val2	*/
		case ICMD_DCMPG:
			if (temp > 0)
				return tracing(block, index-1, temp+1);
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;

		case ICMD_FNEG:				/* ..., value  ==> ..., - value				*/
		case ICMD_DNEG:				/* ..., value  ==> ..., - value				*/  
		case ICMD_I2F:				/* ..., value  ==> ..., (float) value		*/
		case ICMD_L2F:
		case ICMD_I2D:				/* ..., value  ==> ..., (double) value		*/
		case ICMD_L2D:
		case ICMD_F2I:				/* ..., value  ==> ..., (int) value			*/
		case ICMD_D2I:
		case ICMD_F2L:				/* ..., value  ==> ..., (long) value		*/
		case ICMD_D2L:	
		case ICMD_F2D:				/* ..., value  ==> ..., (double) value		*/
		case ICMD_D2F:				/* ..., value  ==> ..., (double) value		*/
			if (temp > 0)
				return tracing(block, index-1, temp);
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;
   
		/* memory operations													*/
		 case ICMD_ARRAYLENGTH:		/* ..., arrayref  ==> ..., length			*/
			if (temp > 0)
				return tracing(block, index-1, temp);
			else
				return array_length(tracing(block, index-1, 0));
			break;

		case ICMD_AALOAD:			/* ..., arrayref, index  ==> ..., value		*/
		case ICMD_LALOAD:			/* ..., arrayref, index  ==> ..., value		*/  
		case ICMD_IALOAD:			/* ..., arrayref, index  ==> ..., value		*/
		case ICMD_FALOAD:			/* ..., arrayref, index  ==> ..., value		*/
		case ICMD_DALOAD:			/* ..., arrayref, index  ==> ..., value		*/
		case ICMD_CALOAD:			/* ..., arrayref, index  ==> ..., value		*/
		case ICMD_SALOAD:			/* ..., arrayref, index  ==> ..., value		*/
		case ICMD_BALOAD:			/* ..., arrayref, index  ==> ..., value		*/
			if (temp > 0)		
				return tracing(block, index-1, temp+1);
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;

		case ICMD_AASTORE:			/* ..., arrayref, index, value  ==> ...		*/
		case ICMD_LASTORE:			/* ..., arrayref, index, value  ==> ...		*/
		case ICMD_IASTORE:			/* ..., arrayref, index, value  ==> ...		*/
		case ICMD_FASTORE:			/* ..., arrayref, index, value  ==> ...		*/
		case ICMD_DASTORE:			/* ..., arrayref, index, value  ==> ...		*/
		case ICMD_CASTORE:			/* ..., arrayref, index, value  ==> ...		*/
		case ICMD_SASTORE:			/* ..., arrayref, index, value  ==> ...		*/
		case ICMD_BASTORE:			/* ..., arrayref, index, value  ==> ...		*/
			return tracing(block, index-1, temp+3);
			break;
 
		case ICMD_PUTSTATIC:		/* ..., value  ==> ...						*/
		case ICMD_PUTFIELD:			/* ..., value  ==> ...						*/
			return tracing(block, index-1, temp+1);
			break;
 
		case ICMD_GETSTATIC:		/* ...  ==> ..., value						*/
		case ICMD_GETFIELD:			/* ...  ==> ..., value						*/
			if (temp > 0)
				return tracing(block, index-1, temp-1);
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;


		/* branch: should not be encountered, but function calls possible		*/
		case ICMD_INVOKESTATIC:		/* ..., [arg1, [arg2 ...]] ==> ...			*/
			m = ip->val.a;			/* get method pointer and					*/
			args = ip->op1;			/* number of arguments						*/
			if (m->returntype != TYPE_VOID)
				retval = 1;			/* if function returns a value, it is on	*/
			else					/* top of stack								*/
				retval = 0;
      
			if (temp > 0)			/* temp is increased by number of arguments	*/
									/* less a possible result value				*/
				return tracing(block, index-1, temp+(args-retval));
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;

		case ICMD_INVOKESPECIAL:	/* ..., objectref, [arg1, [arg2 ...]] ==> .	*/
		case ICMD_INVOKEVIRTUAL:	/* ..., objectref, [arg1, [arg2 ...]] ==> .	*/
		case ICMD_INVOKEINTERFACE:	/* ..., objectref, [arg1, [arg2 ...]] ==> . */
			m = ip->val.a;
			args = ip->op1; 
			if (m->returntype != TYPE_VOID)
				retval = 1;
			else
				retval = 0;
			
			if (temp > 0)			/* same as above but add 1 for object ref	*/
				return tracing(block, index-1, temp+(args-retval+1));
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;
 
		/* special																*/  
		case ICMD_INSTANCEOF:		/* ..., objectref ==> ..., intresult		*/
		case ICMD_CHECKCAST:		/* ..., objectref ==> ..., objectref		*/
			if (temp > 0)
				return tracing(block, index-1, temp);
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;
      
		case ICMD_MULTIANEWARRAY:	/* ..., cnt1, [cnt2, ...] ==> ..., arrayref	*/
									/* op1 = dimension							*/ 

			if (temp > 0)			/* temp increased by number of dimensions	*/
									/* minus one for array ref					*/
				return tracing(block, index-1, temp+(ip->op1 - 1));
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;
       
		case ICMD_BUILTIN3:			/* ..., arg1, arg2, arg3 ==> ...			*/
			if (ip->op1 != TYPE_VOID)
				retval = 1;
			else
				retval = 0;
      
			if (temp > 0)			/* increase temp by 3 minus possible return	*/
									/* value									*/
				return tracing(block, index-1, temp+(3-retval));
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;

		case ICMD_BUILTIN2:			/* ..., arg1, arg2 ==> ...					*/
			if (ip->op1 != TYPE_VOID)
				retval = 1;
			else
				retval = 0;
			if (temp > 0)
				return tracing(block, index-1, temp+(2-retval));
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;

		case ICMD_BUILTIN1:     /* ..., arg1 ==> ...							*/
			if (ip->op1 != TYPE_VOID)
				retval = 1;
			else
				retval = 0;
			if (temp > 0)
				return tracing(block, index-1, temp+(1-retval));
			else
				return create_trace(TRACE_UNKNOWN, -1, 0, index);
			break;

		/* others																*/
		default:
			return create_trace(TRACE_UNKNOWN, -1, 0, index);
			}	/* switch	*/
		}		/* if		*/
	else
		return create_trace(TRACE_UNKNOWN, -1, 0, index);
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
