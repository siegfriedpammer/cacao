/****************************** comp/parse.c ***********************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Enth"alt den Parser f"ur die Bytecode-Darstellung der Methoden
	
	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/10/17

*******************************************************************************/


/* Kurzschreibweise f"ur oft verwendete Funktionen */

#define LOADCONST_I  pcmd_loadconst_i
#define LOADCONST_L  pcmd_loadconst_l
#define LOADCONST_F  pcmd_loadconst_f
#define LOADCONST_D  pcmd_loadconst_d
#define LOADCONST_A  pcmd_loadconst_a
#define MOVE         pcmd_move
#define IINC         pcmd_iinc
#define OP           pcmd_op
#define MEM          pcmd_mem
#define BRA          pcmd_bra
#define TABLEJUMP    pcmd_tablejump
#define METHOD       pcmd_method

#define BUILTIN1     pcmd_builtin1
#define BUILTIN2     pcmd_builtin2
#define BUILTIN3     pcmd_builtin3

#define DROP         pcmd_drop
#define ACTIVATE     pcmd_activate
#define BRA_N_DROP   pcmd_bra_n_drop
#define MOVE_N_DROP  pcmd_move_n_drop

#define OP1(opcode,s,d)         OP(opcode,s,NOVAR,NOVAR,d)
#define OP2(opcode,s1,s2,d)     OP(opcode,s1,s2,NOVAR,d)
#define OP3(opcode,s1,s2,s3,d)  OP(opcode,s1,s2,s3,d)


#define EXCREATOR(exclass)      block_createexcreator (exclass, p)
#define EXFORWARDER(exvar)      block_createexforwarder (exvar, p)




/****************** Funktion: addreturnlog ************************************

	f"ugt in den Code einen Aufruf der Methoden-R"uckkehr-Protokollierung
	ein.

******************************************************************************/

static void addreturnhandling()
{
	if (checksync && (method->flags & ACC_SYNCHRONIZED) ) {
		stack_makesaved ();
#ifdef USE_THREADS
		if (method->flags & ACC_STATIC) {
			varid v = var_create (TYPE_ADDRESS);
			LOADCONST_A (class, v);
			BUILTIN1 ( (functionptr) builtin_monitorexit, v, NOVAR);
			} 
		else {
			BUILTIN1 ( (functionptr) builtin_monitorexit,
			            local_get (0, TYPE_ADDRESS) , NOVAR);
			}
#endif
		}
}


/*************** Funktion: addreturnexceptionlog *****************************

	f"ugt in den Code einen Aufruf der Methoden-R"uckkehr-Protokollierung
	mit Exception ein.

******************************************************************************/

static void addreturnexceptionhandling()
{

	if (runverbose) {
		varid v;

		stack_makesaved ();
		v = var_create (TYPE_ADDRESS);
		LOADCONST_A (method, v);
		BUILTIN1 ( (functionptr) builtin_displaymethodexception, v, NOVAR);
		}
		
	if (checksync && (method->flags & ACC_SYNCHRONIZED) ) {
		stack_makesaved ();
#ifdef USE_THREADS
		if (method->flags & ACC_STATIC) {
			varid v = var_create (TYPE_ADDRESS);
			LOADCONST_A (class, v);
			BUILTIN1 ( (functionptr) builtin_monitorexit, v, NOVAR);
			} 
		else {
			BUILTIN1 ( (functionptr) builtin_monitorexit,
			            local_get (0, TYPE_ADDRESS) , NOVAR);
			}
#endif
		}
}



/******************************************************************************
*************** Funktion 'parse' zum Durcharbeiten des Bytecodes **************
******************************************************************************/

static void parse (basicblock *b)
{
	varid v,v1,v2,v3,ve;
	u4 poolindex;
	s4 type;
	u4 p,i;
	basicblock *target=NULL;
	bool iswide=false;

	stack_restore (b->stack);
	subroutine_set (b->subroutine);


	switch (b->type) {

		/* Code fuer einen Exception-Forwarder generieren */


	case BLOCKTYPE_EXFORWARDER:
		if (!compileall) {
			ACTIVATE (b->exvar);
			stack_repush (b->exvar);
			stack_makesaved ();

			for (i=0; i<exceptiontablelength; i++) {
				target = block_find (extable[i].handlerpc);
				if (   extable[i].startpc <= b->throwpos
				    && extable[i].endpc > b->throwpos) {

					if (!extable[i].catchtype) goto exceptionfits;

					stack_makesaved();
					v1 = var_create (TYPE_ADDRESS);
					v2 = var_create (TYPE_ADDRESS);
					v = var_create (TYPE_INT);
					MOVE (TYPE_ADDRESS, b->exvar, v1);
					LOADCONST_A (extable[i].catchtype, v2);
					BUILTIN2 ((functionptr) builtin_instanceof, v1,v2, v);

					block_reach (target);
					BRA_N_DROP (CMD_IFNE, v,NOVAR, NOVAR, target);
					}
				}
			target = NULL;
			}
		goto exceptionfits;


		/* Code fuer einen Exception-Creator generieren */
	case BLOCKTYPE_EXCREATOR:
		if (!compileall) {
			java_objectheader *o = b->exproto;
			LOADCONST_A (o, stack_push(TYPE_ADDRESS) );

			for (i=0; i<exceptiontablelength; i++) {
				target = block_find (extable[i].handlerpc);
				if (   extable[i].startpc <= b->throwpos
				    && extable[i].endpc > b->throwpos) {

					if (!extable[i].catchtype) goto exceptionfits;
					if (builtin_instanceof (o, extable[i].catchtype) ) goto exceptionfits;
					}
				}
			target = NULL;
			}

		/*** Der Sprung zum Exception-Handler (oder Methodenbeendigung) ***/

	  exceptionfits:
		if (!compileall) {
		   	if (target) {
				block_reach (target);
				BRA (CMD_GOTO, NOVAR,NOVAR,NOVAR, target);
				goto cleanup;
				}

#ifdef STATISTICS
	count_pcmd_returnx++;
#endif

			switch (mreturntype) {
				case TYPE_INT:
					addreturnexceptionhandling();

					v1 = var_create (TYPE_INT);
					LOADCONST_I (0, v1);
					BRA_N_DROP (CMD_IRETURN, v1,stack_pop(TYPE_ADDRESS), NOVAR, NULL);
					break;
				case TYPE_LONG:
					addreturnexceptionhandling();

					v1 = var_create (TYPE_LONG);
					LOADCONST_I (0, v1);
					BRA_N_DROP (CMD_LRETURN, v1,stack_pop(TYPE_ADDRESS), NOVAR, NULL);
					break;
				case TYPE_FLOAT:
					addreturnexceptionhandling();

					v1 = var_create (TYPE_FLOAT);
					LOADCONST_F (0.0, v1);
					BRA_N_DROP (CMD_FRETURN, v1,stack_pop(TYPE_ADDRESS), NOVAR, NULL);
					break;
				case TYPE_DOUBLE:
					addreturnexceptionhandling();

					v1 = var_create (TYPE_DOUBLE);
					LOADCONST_D (0.0, v1);
					BRA_N_DROP (CMD_DRETURN, v1,stack_pop(TYPE_ADDRESS), NOVAR, NULL);
					break;
				case TYPE_ADDRESS:
					addreturnexceptionhandling();

					v1 = var_create (TYPE_ADDRESS);
					LOADCONST_A (NULL, v1);
					BRA_N_DROP (CMD_ARETURN, v1,stack_pop(TYPE_ADDRESS), NOVAR, NULL);
					break;
				case TYPE_VOID:
					addreturnexceptionhandling();

					BRA (CMD_RETURN, NOVAR,stack_pop(TYPE_ADDRESS), NOVAR, NULL);
					break;
				}
			}
		goto cleanup;
		}


		/* Code fuer einen (normalen) JavaVM - Block generieren */

	p = b->jpc;


	
	if ( p==0) {
		/* Method call protocolling */

		if (runverbose) {
			stack_makesaved();
			pcmd_trace (method);
			}

		/* Synchronization */
		if (checksync && (method->flags & ACC_SYNCHRONIZED)) {
			stack_makesaved();
#ifdef USE_THREADS
			if (method->flags & ACC_STATIC) {
				varid v = var_create (TYPE_ADDRESS);
				LOADCONST_A (class, v);
				BUILTIN1 ( (functionptr) builtin_monitorenter, v, NOVAR);
			} 
			else {
				BUILTIN1 ( (functionptr) builtin_monitorenter,
				            local_get (0, TYPE_ADDRESS), NOVAR );
				}
#endif
			}			
		}

	for (;;) {
		u1 opcode;
		u4 nextp;

		opcode = code_get_u1 (p);
		nextp = p + jcommandsize[opcode];


		count_javainstr++;


		if (showstack) {
			printf ("PC: %3d  OPCODE: %3d   Stack: ",(int) p, (int) opcode);
			stack_display (stack_get());
			printf ("\n");
			}


		switch (opcode) {


			/*** Pushing constants onto the stack ***/

			case CMD_BIPUSH:
				LOADCONST_I (code_get_s1 (p+1), stack_push (TYPE_INT) );
				break;

			case CMD_SIPUSH:
				LOADCONST_I (code_get_s2 (p+1), stack_push (TYPE_INT) );
				break;

			case CMD_LDC1:  poolindex = code_get_u1 (p+1);
			                goto pushconstantitem;
			case CMD_LDC2:
			case CMD_LDC2W: poolindex = code_get_u2 (p+1);
			pushconstantitem:
				switch (class_constanttype(class, poolindex)) {
					case CONSTANT_Integer:
						{ constant_integer *c;
						c = class_getconstant (class, poolindex, CONSTANT_Integer);
						LOADCONST_I (c->value, stack_push(TYPE_INT) );
						}
						break;
					case CONSTANT_Long:
						{ constant_long *c;
						c = class_getconstant (class, poolindex, CONSTANT_Long);
						LOADCONST_L (c->value, stack_push(TYPE_LONG) );
						}
						break;
					case CONSTANT_Float:
						{ constant_float *c;
						c = class_getconstant (class, poolindex, CONSTANT_Float);
						LOADCONST_F (c->value, stack_push(TYPE_FLOAT) );
						}
						break;
					case CONSTANT_Double:
						{ constant_double *c;
						c = class_getconstant (class, poolindex, CONSTANT_Double);
						LOADCONST_D (c->value, stack_push(TYPE_DOUBLE) );
						}
						break;

					case CONSTANT_String:
						{ utf *s;

						s = class_getconstant (class, poolindex, CONSTANT_String);

						LOADCONST_A ( literalstring_new (s),
						              stack_push(TYPE_ADDRESS) );
						}
						break;

					default: panic ("Invalid constant type to push");
					}
				break;


			case CMD_ACONST_NULL:
				LOADCONST_A (0,  stack_push (TYPE_ADDRESS) );
				break;

			case CMD_ICONST_M1:
			case CMD_ICONST_0:
			case CMD_ICONST_1:
			case CMD_ICONST_2:
			case CMD_ICONST_3:
			case CMD_ICONST_4:
			case CMD_ICONST_5:
				LOADCONST_I (opcode - CMD_ICONST_0, stack_push (TYPE_INT) );
				break;

			case CMD_LCONST_0:
			case CMD_LCONST_1:
#if U8_AVAILABLE
				LOADCONST_L (opcode - CMD_LCONST_0, stack_push (TYPE_LONG) );
#else
				{ u8 v;
                  v.low = opcode - CMD_LCONST_0;
				  v.high = 0;
				  LOADCONST_L (v, stack_push(TYPE_LONG) );
				}
#endif
				break;

			case CMD_FCONST_0:
			case CMD_FCONST_1:
			case CMD_FCONST_2:
				LOADCONST_F (opcode - CMD_FCONST_0, stack_push (TYPE_FLOAT) );
				break;

			case CMD_DCONST_0:
			case CMD_DCONST_1:
				LOADCONST_D (opcode - CMD_DCONST_0, stack_push (TYPE_DOUBLE) );
				break;


			/*** Loading variables onto the Stack ***/

			case CMD_ILOAD:
				if (!iswide) {
					MOVE ( TYPE_INT,
					       local_get (code_get_u1 (p+1), TYPE_INT),
					       stack_push (TYPE_INT) );
					}
				else {
					MOVE ( TYPE_INT,
					       local_get (code_get_u2 (p+1), TYPE_INT),
					       stack_push (TYPE_INT) );
					nextp = p+3;
					iswide = false;
					}
				break;

			case CMD_ILOAD_0:
			case CMD_ILOAD_1:
			case CMD_ILOAD_2:
			case CMD_ILOAD_3:
				MOVE ( TYPE_INT,
				       local_get (opcode - CMD_ILOAD_0, TYPE_INT),
				       stack_push (TYPE_INT) );
				break;

			case CMD_LLOAD:
				if (!iswide) {
					MOVE ( TYPE_LONG,
					       local_get (code_get_u1 (p+1), TYPE_LONG),
					       stack_push (TYPE_LONG) );
					}
				else {
					MOVE ( TYPE_LONG,
					       local_get (code_get_u2 (p+1), TYPE_LONG),
					       stack_push (TYPE_LONG) );
					nextp = p+3;
					iswide = false;
					}
				break;

			case CMD_LLOAD_0:
			case CMD_LLOAD_1:
			case CMD_LLOAD_2:
			case CMD_LLOAD_3:
				MOVE ( TYPE_LONG,
				       local_get (opcode - CMD_LLOAD_0, TYPE_LONG),
				       stack_push (TYPE_LONG) );
				break;

			case CMD_FLOAD:
				if (!iswide) {
					MOVE ( TYPE_FLOAT,
					       local_get (code_get_u1 (p+1), TYPE_FLOAT),
					       stack_push (TYPE_FLOAT) );
					}
				else {
					MOVE ( TYPE_FLOAT,
					       local_get (code_get_u2 (p+1), TYPE_FLOAT),
					       stack_push (TYPE_FLOAT) );
					nextp = p+3;
					iswide = false;
					}
				break;

			case CMD_FLOAD_0:
			case CMD_FLOAD_1:
			case CMD_FLOAD_2:
			case CMD_FLOAD_3:
				MOVE ( TYPE_FLOAT,
				       local_get (opcode - CMD_FLOAD_0, TYPE_FLOAT),
				       stack_push (TYPE_FLOAT) );
				break;

			case CMD_DLOAD:
				if (!iswide) {
					MOVE ( TYPE_DOUBLE,
					       local_get (code_get_u1 (p+1), TYPE_DOUBLE),
					       stack_push (TYPE_DOUBLE) );
					}
				else {
					MOVE ( TYPE_DOUBLE,
					       local_get (code_get_u2 (p+1), TYPE_DOUBLE),
					       stack_push (TYPE_DOUBLE) );
					nextp = p+3;
					iswide = false;
					}
				break;

			case CMD_DLOAD_0:
			case CMD_DLOAD_1:
			case CMD_DLOAD_2:
			case CMD_DLOAD_3:
				MOVE ( TYPE_DOUBLE,
				       local_get (opcode - CMD_DLOAD_0, TYPE_DOUBLE),
				       stack_push (TYPE_DOUBLE) );
				break;

			case CMD_ALOAD:
				if (!iswide) {
					MOVE ( TYPE_ADDRESS,
					       local_get (code_get_u1 (p+1), TYPE_ADDRESS),
					       stack_push (TYPE_ADDRESS) );
					}
				else {
					MOVE ( TYPE_ADDRESS,
					       local_get (code_get_u2 (p+1), TYPE_ADDRESS),
					       stack_push (TYPE_ADDRESS) );
					nextp = p+3;
					iswide = false;
					}
				break;

			case CMD_ALOAD_0:
			case CMD_ALOAD_1:
			case CMD_ALOAD_2:
			case CMD_ALOAD_3:
				MOVE ( TYPE_ADDRESS,
				       local_get (opcode - CMD_ALOAD_0, TYPE_ADDRESS),
				       stack_push (TYPE_ADDRESS) );
				break;


			/*** Storing Stack Values into Local Variables ***/

			case CMD_ISTORE:
				v = stack_pop (TYPE_INT);
				if (!iswide) {
					MOVE_N_DROP (TYPE_INT, v,
					    local_get (code_get_u1 (p+1), TYPE_INT) );
					}
				else {
					MOVE_N_DROP (TYPE_INT, v,
					    local_get (code_get_u2 (p+1), TYPE_INT) );
					iswide=false;
					nextp = p+3;
					}
				break;

			case CMD_ISTORE_0:
			case CMD_ISTORE_1:
			case CMD_ISTORE_2:
			case CMD_ISTORE_3:
				v = stack_pop (TYPE_INT);
				MOVE_N_DROP (TYPE_INT,
				      v, local_get (opcode - CMD_ISTORE_0, TYPE_INT) );
				break;

			case CMD_LSTORE:
				v = stack_pop (TYPE_LONG);
				if (!iswide) {
					MOVE_N_DROP (TYPE_LONG, v,
					    local_get (code_get_u1 (p+1), TYPE_LONG) );
					}
				else {
					MOVE_N_DROP (TYPE_LONG, v,
					    local_get (code_get_u2 (p+1), TYPE_LONG) );
					iswide=false;
					nextp = p+3;
					}
				break;

			case CMD_LSTORE_0:
			case CMD_LSTORE_1:
			case CMD_LSTORE_2:
			case CMD_LSTORE_3:
				v = stack_pop (TYPE_LONG);
				MOVE_N_DROP (TYPE_LONG,
				      v, local_get (opcode - CMD_LSTORE_0, TYPE_LONG) );
				break;

			case CMD_FSTORE:
				v = stack_pop (TYPE_FLOAT);
				if (!iswide) {
					MOVE_N_DROP (TYPE_FLOAT, v,
					    local_get (code_get_u1 (p+1), TYPE_FLOAT) );
					}
				else {
					MOVE_N_DROP (TYPE_FLOAT, v,
					    local_get (code_get_u2 (p+1), TYPE_FLOAT) );
					iswide=false;
					nextp = p+3;
					}
				break;

			case CMD_FSTORE_0:
			case CMD_FSTORE_1:
			case CMD_FSTORE_2:
			case CMD_FSTORE_3:
				v = stack_pop (TYPE_FLOAT);
				MOVE_N_DROP (TYPE_FLOAT,
				      v, local_get (opcode - CMD_FSTORE_0, TYPE_FLOAT) );
				break;

			case CMD_DSTORE:
				v = stack_pop (TYPE_DOUBLE);
				if (!iswide) {
					MOVE_N_DROP (TYPE_DOUBLE, v,
					    local_get (code_get_u1 (p+1), TYPE_DOUBLE) );
					}
				else {
					MOVE_N_DROP (TYPE_DOUBLE, v,
					    local_get (code_get_u2 (p+1), TYPE_DOUBLE) );
					iswide=false;
					nextp = p+3;
					}
				break;

			case CMD_DSTORE_0:
			case CMD_DSTORE_1:
			case CMD_DSTORE_2:
			case CMD_DSTORE_3:
				v = stack_pop (TYPE_DOUBLE);
				MOVE_N_DROP (TYPE_DOUBLE,
				      v, local_get (opcode - CMD_DSTORE_0, TYPE_DOUBLE) );
				break;

			case CMD_ASTORE:
				v = stack_pop (TYPE_ADDRESS);
				if (!iswide) {
					MOVE_N_DROP (TYPE_ADDRESS, v,
					    local_get (code_get_u1 (p+1), TYPE_ADDRESS) );
					}
				else {
					MOVE_N_DROP (TYPE_ADDRESS, v,
					    local_get (code_get_u2 (p+1), TYPE_ADDRESS) );
					iswide=false;
					nextp = p+3;
					}
				break;

			case CMD_ASTORE_0:
			case CMD_ASTORE_1:
			case CMD_ASTORE_2:
			case CMD_ASTORE_3:
				v = stack_pop (TYPE_ADDRESS);
				MOVE_N_DROP (TYPE_ADDRESS,
				      v, local_get (opcode - CMD_ASTORE_0, TYPE_ADDRESS) );
				break;


			case CMD_IINC:
				if (!iswide) {
					v1 = local_get (code_get_u1 (p+1), TYPE_INT);
					IINC (code_get_s1 (p+2), v1 );
					DROP (v1);
					}
				else {
					v1 = local_get (code_get_u2 (p+1), TYPE_INT);
					IINC (code_get_s2 (p+3), v1 );
					DROP (v1);
					iswide = false;
					nextp = p+5;
					}
				break;


			/*** Wider index for Loading, Storing and Incrementing ***/

			case CMD_WIDE:
				iswide=true;
				nextp = p+1;
				break;


			/******************** Managing Arrays **************************/

			case CMD_NEWARRAY:
				v1 = stack_pop (TYPE_INT);
				BRA (CMD_IFLT, v1,NOVAR, NOVAR, EXCREATOR(proto_java_lang_NegativeArraySizeException) );

				stack_makesaved ();
				v = stack_push (TYPE_ADDRESS);
				switch ( code_get_s1 (p+1) ) {
				case 4: BUILTIN1 ((functionptr) builtin_newarray_boolean, v1, v);
					    break;
				case 5: BUILTIN1 ((functionptr) builtin_newarray_char, v1, v);
					    break;
				case 6: BUILTIN1 ((functionptr) builtin_newarray_float, v1, v);
					    break;
				case 7: BUILTIN1 ((functionptr) builtin_newarray_double, v1, v);
					    break;
				case 8: BUILTIN1 ((functionptr) builtin_newarray_byte, v1, v);
					    break;
				case 9: BUILTIN1 ((functionptr) builtin_newarray_short, v1, v);
					    break;
				case 10: BUILTIN1 ((functionptr) builtin_newarray_int, v1, v);
					    break;
				case 11: BUILTIN1 ((functionptr) builtin_newarray_long, v1, v);
					    break;
				default: panic ("Invalid array-type to create");
				}

				BRA (CMD_IFNULL, v,NOVAR, NOVAR, EXCREATOR(proto_java_lang_OutOfMemoryError) );

				break;

			case CMD_ANEWARRAY:
				poolindex = code_get_u2(p+1);
				if (class_constanttype (class, poolindex) == CONSTANT_Arraydescriptor) {
					/* anewarray mit Array-Typ! */
					constant_arraydescriptor *desc =
					  class_getconstant (class, poolindex, CONSTANT_Arraydescriptor);

					v1 = stack_pop (TYPE_INT);
					BRA (CMD_IFLT, v1,NOVAR, NOVAR, EXCREATOR(proto_java_lang_NegativeArraySizeException) );

					v2 = var_create (TYPE_ADDRESS);
					LOADCONST_A (desc, v2);

					stack_makesaved ();
					v = stack_push (TYPE_ADDRESS);
					BUILTIN2 ((functionptr) builtin_newarray_array, v1,v2, v);
					BRA (CMD_IFNULL, v,NOVAR, NOVAR, EXCREATOR(proto_java_lang_OutOfMemoryError) );
					}	
				else {
					classinfo *c = class_getconstant (class, poolindex, CONSTANT_Class);

					v1 = stack_pop (TYPE_INT);
					BRA (CMD_IFLT, v1,NOVAR, NOVAR, EXCREATOR(proto_java_lang_NegativeArraySizeException) );

					v2 = var_create (TYPE_ADDRESS);
					LOADCONST_A (c, v2);

					stack_makesaved ();
					v = stack_push (TYPE_ADDRESS);
					BUILTIN2 ((functionptr) builtin_anewarray, v1,v2, v);
					BRA (CMD_IFNULL, v,NOVAR, NOVAR, EXCREATOR(proto_java_lang_OutOfMemoryError) );
					}

				break;


			case CMD_MULTIANEWARRAY:
				{ constant_arraydescriptor *desc =
				    class_getconstant (class, code_get_u2(p+1), CONSTANT_Arraydescriptor);
				  int i, n = code_get_u1 (p+3);
				  varid dims =      var_create (TYPE_ADDRESS);  /* array for dimensions */
				  varid dimsdim =   var_create (TYPE_INT);      /* groesse des arrays */

				stack_makesaved ();
				LOADCONST_I (n, dimsdim);
				BUILTIN1 ((functionptr) builtin_newarray_int, dimsdim, dims);
				BRA (CMD_IFNULL, dims,NOVAR, NOVAR, EXCREATOR(proto_java_lang_OutOfMemoryError) );

				for (i=0; i<n; i++) {
					varid dimn = stack_pop (TYPE_INT);
					BRA (CMD_IFLT, dimn,NOVAR, NOVAR, EXCREATOR(proto_java_lang_NegativeArraySizeException) );

					LOADCONST_I ((n-i)-1, dimsdim);
					OP3 (CMD_IASTORE, dims, dimsdim, dimn, NOVAR);
					DROP (dimsdim);
					DROP (dimn);
					}

				v = stack_push (TYPE_ADDRESS);

				v1 = var_create (TYPE_ADDRESS);
				LOADCONST_A (desc, v1);

				BUILTIN2 ((functionptr) builtin_multianewarray, dims, v1, v);
				BRA (CMD_IFNULL, v,NOVAR, NOVAR, EXCREATOR(proto_java_lang_OutOfMemoryError) );

				}
				break;


			case CMD_ARRAYLENGTH:
				v = stack_pop (TYPE_ADDRESS);
				if (checknull) {
#ifdef STATISTICS
					count_check_null++;
#endif
					BRA (CMD_IFNULL, v,NOVAR, NOVAR,  EXCREATOR(proto_java_lang_NullPointerException) );
					}
				OP1 (opcode, v, stack_push (TYPE_INT) );
				DROP (v);
				break;

			case CMD_AALOAD:
				type = TYPE_ADDRESS; goto do_aXload;
			case CMD_LALOAD:
				type = TYPE_LONG; goto do_aXload;
			case CMD_FALOAD:
				type = TYPE_FLOAT;  goto do_aXload;
			case CMD_DALOAD:
				type = TYPE_DOUBLE; goto do_aXload;
			case CMD_IALOAD:
			case CMD_BALOAD:
			case CMD_CALOAD:
			case CMD_SALOAD:
				type = TYPE_INT; goto do_aXload;
			  do_aXload:
				v2 = stack_pop (TYPE_INT);
				v1 = stack_pop (TYPE_ADDRESS);
				if (checknull) {
#ifdef STATISTICS
					count_check_null++;
#endif
					BRA (CMD_IFNULL, v1,NOVAR, NOVAR, EXCREATOR(proto_java_lang_NullPointerException) );
					}
				if (checkbounds) {
#ifdef STATISTICS
					count_check_bound++;
#endif
					v = var_create (TYPE_INT);
					OP1 (CMD_ARRAYLENGTH, v1, v);
					BRA (CMD_IF_UCMPGE, v2,v, NOVAR, EXCREATOR(proto_java_lang_ArrayIndexOutOfBoundsException) );
					DROP (v);
					}
				OP2 (opcode, v1,v2, stack_push (type) );
				DROP (v1);
				DROP (v2);
                break;

			case CMD_AASTORE:
				type = TYPE_ADDRESS; goto do_aXstore;
			case CMD_LASTORE:
				type = TYPE_LONG; goto do_aXstore;
			case CMD_FASTORE:
				type = TYPE_FLOAT; goto do_aXstore;
			case CMD_DASTORE:
				type = TYPE_DOUBLE; goto do_aXstore;
			case CMD_IASTORE:
			case CMD_BASTORE:
			case CMD_CASTORE:
			case CMD_SASTORE:
				type = TYPE_INT; goto do_aXstore;
			  do_aXstore:
				v3 = stack_pop (type);
				v2 = stack_pop (TYPE_INT);
				v1 = stack_pop (TYPE_ADDRESS);
				if (checknull) {
#ifdef STATISTICS
					count_check_null++;
#endif
					BRA (CMD_IFNULL, v1,NOVAR, NOVAR, EXCREATOR(proto_java_lang_NullPointerException) );
					}
				if (checkbounds) {
#ifdef STATISTICS
					count_check_bound++;
#endif
					v = var_create (TYPE_INT);
					OP1 (CMD_ARRAYLENGTH, v1, v);
					BRA (CMD_IF_UCMPGE, v2,v, NOVAR, EXCREATOR(proto_java_lang_ArrayIndexOutOfBoundsException) );
					DROP (v);
					}

				if (opcode!=CMD_AASTORE) {
					OP3 (opcode, v1,v2,v3, NOVAR);
					DROP (v1);
					DROP (v2);
					DROP (v3);
				} else {
					stack_makesaved ();

					v = var_create (TYPE_INT);
					BUILTIN3 ((functionptr) builtin_aastore, v1,v2,v3, v);
					BRA (CMD_IFEQ, v,NOVAR, NOVAR, EXCREATOR(proto_java_lang_ArrayStoreException) );
					DROP (v);
					}

				break;



			/******************* Stack instructions **************************/

			case CMD_NOP:
				break;

			case CMD_POP:
				v1 = stack_popany (1);
				DROP (v1);
				break;

			case CMD_POP2:
				{ int varcount, i;
				  varid vararray[2];

				varcount = stack_popmany (vararray, 2);
				for (i=0; i<varcount; i++) DROP(vararray[i]);
				}
				break;

			case CMD_DUP:
				v1 = stack_popany(1);
				stack_repush (v1);
				v = stack_push ( var_type (v1) );
				MOVE (var_type(v1), v1,v);
				break;

			case CMD_DUP2:
				{ int varcount, i;
				  varid vararray[2];

				varcount = stack_popmany (vararray, 2);
				stack_repushmany (varcount, vararray);
				for (i=0; i<varcount; i++) {
					v = stack_push ( var_type(vararray[varcount-1-i]) );
					MOVE ( var_type(v), vararray[varcount-1-i], v);
					}
				}
				break;

			case CMD_DUP_X1:
				v1 = stack_popany(1);
				v2 = stack_popany(1);

				stack_repush (v1);
				stack_repush (v2);
				v = stack_push ( var_type (v1) );
				MOVE (var_type(v1), v1,v);
				break;

			case CMD_DUP2_X1:
				{ int varcount, i;
				  varid vararray[2];

				varcount = stack_popmany (vararray, 2);
				v3 = stack_popany (1);
				stack_repushmany (varcount, vararray);
				stack_repush (v3);
				for (i=0; i<varcount; i++) {
					v = stack_push ( var_type(vararray[varcount-1-i]) );
					MOVE ( var_type(v), vararray[varcount-1-i], v);
					}
				}
				break;

			case CMD_DUP_X2:
				{ int varcount;
				  varid vararray[2];

				v1 = stack_popany(1);
				varcount = stack_popmany (vararray, 2);
				stack_repush (v1);
				stack_repushmany (varcount, vararray);
				MOVE (var_type(v1), v1, stack_push(var_type(v1)) );
				}
				break;

			case CMD_DUP2_X2:
				{ int varcount1, varcount2, i;
				  varid vararray1[2],vararray2[2];

				varcount1 = stack_popmany (vararray1, 2);
				varcount2 = stack_popmany (vararray2, 2);
				stack_repushmany (varcount2, vararray2);
				stack_repushmany (varcount1, vararray1);
				for (i=0; i<varcount1; i++) {
				  	v = stack_push ( var_type(vararray1[varcount1-1-i]) );
				 	MOVE ( var_type(v), vararray1[varcount1-1-i], v);
					}
				}
				break;

            case CMD_SWAP:
				v1 = stack_popany (1);
				v2 = stack_popany (1);
				stack_repush (v1);
				stack_repush (v2);

			    break;


			/*** Arithmetic & logical instructions ***/

			case CMD_IDIV:
			case CMD_IREM:
				v2 = stack_pop (TYPE_INT);
				v1 = stack_pop (TYPE_INT);
				BRA (CMD_IFEQ, v2,NOVAR, NOVAR, EXCREATOR(proto_java_lang_ArithmeticException) );

				if (SUPPORT_DIVISION) {
					OP2 (opcode, v1,v2, stack_push(TYPE_INT));
					DROP (v1);
					DROP (v2);
					}
				else {
					stack_makesaved ();
					BUILTIN2 (
					   (opcode == CMD_IDIV) ? 
					        ((functionptr) builtin_idiv) 
					      : ((functionptr) builtin_irem) 
					  ,v1,v2, stack_push (TYPE_INT) );
					}
				break;

			case CMD_LDIV:
			case CMD_LREM:
				v2 = stack_pop (TYPE_LONG);
				v1 = stack_pop (TYPE_LONG);
				BRA (CMD_IFEQL, v2,NOVAR, NOVAR, EXCREATOR(proto_java_lang_ArithmeticException) );

				if (SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_MULDIV) {
					OP2 (opcode, v1,v2, stack_push(TYPE_LONG));
					DROP (v1);
					DROP (v2);
					}
				else {
					stack_makesaved ();
					BUILTIN2 (
					   (opcode == CMD_LDIV) ? 
					        ((functionptr) builtin_ldiv) 
					      : ((functionptr) builtin_lrem) 
					  ,v1,v2, stack_push (TYPE_LONG) );
					}
				break;


			/*** Control transfer instructions ***/

			case CMD_IFEQ:
			case CMD_IFLT:
			case CMD_IFLE:
			case CMD_IFNE:
			case CMD_IFGT:
			case CMD_IFGE:
				target = block_find (p + code_get_s2 (p+1) );
				v = stack_pop (TYPE_INT);
				block_reach (target);
				BRA_N_DROP (opcode, v,NOVAR, NOVAR, target);
				break;

			case CMD_IFNULL:
			case CMD_IFNONNULL:
				target = block_find (p + code_get_s2 (p+1));
				v = stack_pop (TYPE_ADDRESS);
				block_reach (target);
				BRA_N_DROP (opcode, v,NOVAR, NOVAR, target);
				break;

			case CMD_IF_ICMPEQ:
			case CMD_IF_ICMPNE:
			case CMD_IF_ICMPLT:
			case CMD_IF_ICMPGT:
			case CMD_IF_ICMPLE:
			case CMD_IF_ICMPGE:
				target = block_find (p + code_get_s2 (p+1) );
				v2 = stack_pop (TYPE_INT);
				v1 = stack_pop (TYPE_INT);
				block_reach (target);
				BRA_N_DROP (opcode, v1,v2, NOVAR, target );
				break;

			case CMD_IF_ACMPEQ:
			case CMD_IF_ACMPNE:
				target = block_find (p + code_get_s2 (p+1) );
				v2 = stack_pop (TYPE_ADDRESS);
				v1 = stack_pop (TYPE_ADDRESS);
				block_reach (target);
				BRA_N_DROP (opcode, v1,v2, NOVAR, target);
				break;


			case CMD_GOTO:
				target = block_find (p + code_get_s2 (p+1) );
				goto do_goto;
			case CMD_GOTO_W:
				target = block_find (p + code_get_s4 (p+1) );
			  do_goto:
				block_reach (target);
				BRA (CMD_GOTO, NOVAR,NOVAR, NOVAR, target );
				goto cleanup;


			case CMD_JSR:
				target = block_find (p + code_get_s2 (p+1) );
				goto do_jsr;
			case CMD_JSR_W:
				target = block_find (p + code_get_s4 (p+1) );
			  do_jsr:
			  	{
			    subroutineinfo *sub;

			  	ACTIVATE (stack_push (TYPE_ADDRESS) );

				sub = target->subroutine;
				if (!sub) {
					sub = subroutine_new();
					target->subroutine = sub;
					}

				subroutine_set (sub);
				block_reach (target);
				subroutine_set (b->subroutine);

				BRA (CMD_JSR, NOVAR,NOVAR, stack_pop(TYPE_ADDRESS), target );

				while (! stack_isempty() ) {
					v = stack_popany ( stack_topslots() );
					DROP (v);
					}

				if (sub->returnfinished) {
					stackinfo *s = sub->returnstack;
					stack_restore (s);
					while (s) {
						ACTIVATE (s->var);
						s = s->prev;
						}
					block_reach ( block_find(nextp) );
					}
				else {
					basicblock *n = block_find(nextp);
					n -> subroutine = b->subroutine;

					chain_addlast (sub->callers, n );
					}

				}
				goto cleanup;


			case CMD_RET:
				if (!iswide) {
					v = local_get (code_get_u1 (p+1), TYPE_ADDRESS);
					}
				else {
					v = local_get (code_get_u2 (p+1), TYPE_ADDRESS);
					nextp = p+3;
					iswide = false;
					}

				{
				subroutineinfo *sub;
				basicblock *bb;

				sub = b->subroutine;
				if (!sub) panic ("RET outside of subroutine");
				if (sub->returnfinished) panic ("Multiple RET in a subroutine");

				sub->returnfinished = true;
				sub->returnstack = stack_get() ;

				while ( (bb = chain_first(sub->callers)) ) {
					chain_remove (sub->callers);

					subroutine_set (bb->subroutine);
					block_reach ( bb );
					subroutine_set (sub);
					}

				BRA (CMD_RET, v,NOVAR, NOVAR, NULL);
				}
				goto cleanup;



			/*************** Function Return **************/

			case CMD_IRETURN:
				addreturnhandling();

				v = stack_pop (TYPE_INT);
				ve = var_create (TYPE_ADDRESS);
				LOADCONST_A (NULL, ve);
				BRA_N_DROP (opcode, v, ve, NOVAR, NULL);
#ifdef STATISTICS
	count_pcmd_return++;
#endif
				goto cleanup;

			case CMD_LRETURN:
				addreturnhandling();

				v = stack_pop (TYPE_LONG);
				var_proposereg (v, reg_parlistresult(TYPE_LONG) );
				ve = var_create (TYPE_ADDRESS);
				LOADCONST_A (NULL, ve);
				var_proposereg (ve, reg_parlistexception() );
				BRA_N_DROP (opcode, v, ve, NOVAR, NULL);
#ifdef STATISTICS
	count_pcmd_return++;
#endif
				goto cleanup;

			case CMD_FRETURN:
				addreturnhandling();

				v = stack_pop (TYPE_FLOAT);
				var_proposereg (v, reg_parlistresult(TYPE_FLOAT) );
				ve = var_create (TYPE_ADDRESS);
				LOADCONST_A (NULL, ve);
				var_proposereg (ve, reg_parlistexception() );
				BRA_N_DROP (opcode, v, ve, NOVAR, NULL);
#ifdef STATISTICS
	count_pcmd_return++;
#endif
				goto cleanup;

			case CMD_DRETURN:
				addreturnhandling();

				v = stack_pop (TYPE_DOUBLE);
				var_proposereg (v, reg_parlistresult(TYPE_DOUBLE) );
				ve = var_create (TYPE_ADDRESS);
				LOADCONST_A (NULL, ve);
				var_proposereg (ve, reg_parlistexception() );
				BRA_N_DROP (opcode, v, ve, NOVAR, NULL);
#ifdef STATISTICS
	count_pcmd_return++;
#endif
				goto cleanup;

			case CMD_ARETURN:
				addreturnhandling();

				v = stack_pop (TYPE_ADDRESS);
				var_proposereg (v, reg_parlistresult(TYPE_ADDRESS) );
				ve = var_create (TYPE_ADDRESS);
				LOADCONST_A (NULL, ve);
				var_proposereg (ve, reg_parlistexception() );
				BRA_N_DROP (opcode, v, ve, NOVAR, NULL);
				goto cleanup;
#ifdef STATISTICS
	count_pcmd_return++;
#endif

			case CMD_RETURN:
				addreturnhandling();

				ve = var_create (TYPE_ADDRESS);
				LOADCONST_A (NULL, ve);
				var_proposereg (ve, reg_parlistexception() );
				BRA_N_DROP (opcode, NOVAR,ve, NOVAR, NULL);
#ifdef STATISTICS
	count_pcmd_return++;
#endif
				goto cleanup;


			case CMD_BREAKPOINT:
				break;



			/**************** Table Jumping *****************/

			case CMD_LOOKUPSWITCH:
				{	u4 p2 = ALIGN((p+1), 4);
					basicblock *defaulttarget;
					u4 num, i;

					defaulttarget = block_find (p + code_get_s4 (p2) );
					num = code_get_s4 (p2+4);

					v = stack_pop (TYPE_INT);
					for (i=0; i<num; i++) {
						s4 value = code_get_s4 (p2 + 8 + 8*i);
						target = block_find (p + code_get_s4 (p2 + 8 + 4 + 8*i) );

						v1 = var_create (TYPE_INT);
						v2 = var_create (TYPE_INT);
						MOVE (TYPE_INT, v, v1);
						LOADCONST_I (value, v2);
						block_reach (target);
						BRA_N_DROP (CMD_IF_ICMPEQ, v1,v2, NOVAR, target );
						}

					DROP (v);
					block_reach (defaulttarget);
					BRA (CMD_GOTO, NOVAR,NOVAR, NOVAR, defaulttarget );

					nextp = p2 + 8 + 8*num;
					goto cleanup;
				}
				break;

			case CMD_TABLESWITCH:
				{	u4 p2 = ALIGN((p+1), 4);
					basicblock *target;
					basicblock **targets;
					s4 low,high, i;
					
					low = code_get_s4 (p2+4);
					high = code_get_s4 (p2+8);
					if (high<low) panic ("Tablejump range invalid");

					v = stack_pop (TYPE_INT);

					target = block_find (p + code_get_s4 (p2) );
					block_reach(target);

					v1 = var_create (TYPE_INT);
					LOADCONST_I (high, v1);
					BRA (CMD_IF_ICMPGT, v,v1, NOVAR, target);
					DROP (v1);
					
					if (low!=0) {
						v1 = var_create (TYPE_INT);
						v2 = var_create (TYPE_INT);
						LOADCONST_I (low, v1);
						OP2 (CMD_ISUB, v,v1, v2);
						DROP (v1);
						DROP (v);
						v = v2;
						} 
					BRA (CMD_IFLT, v,NOVAR, NOVAR, target);
						
					targets = DMNEW (basicblock*, (high-low)+1);
					for (i=0; i < (high-low)+1; i++) {
						target = block_find (p + code_get_s4 (p2 + 12 + 4*i) );
						block_reach (target);

						targets[i] = target;
						}

					TABLEJUMP (v, (high-low)+1, targets);
					DROP (v);

					nextp = p2 + 12 + 4 * ((high-low)+1);
					goto cleanup;
				}
				break;


			/************ Manipulating Object Fields ********/

			case CMD_PUTSTATIC:
			case CMD_GETSTATIC:
			case CMD_PUTFIELD:
			case CMD_GETFIELD:

				poolindex = code_get_u2 (p+1);
				{ constant_FMIref *fr;
				  fieldinfo *fi;

					fr = class_getconstant (class, poolindex, CONSTANT_Fieldref);
					fi = class_findfield (fr->class, fr->name, fr->descriptor);

					switch (opcode) {
					case CMD_PUTSTATIC:
						compiler_addinitclass (fr->class);

						v1 = var_create (TYPE_ADDRESS);
						v = stack_pop (fi->type);
						LOADCONST_A (&(fi->value), v1);

						MEM (CMD_PUTFIELD, fi->type, v1,v, NOVAR, 0);
						DROP (v);
						DROP (v1);
						break;

					case CMD_GETSTATIC:
						compiler_addinitclass (fr->class);

						v1 = var_create (TYPE_ADDRESS);
						v = stack_push (fi->type);
						LOADCONST_A (&(fi->value), v1);

						MEM (CMD_GETFIELD, fi->type, v1,NOVAR, v, 0);
						DROP (v1);
						break;


					case CMD_PUTFIELD:
						v = stack_pop (fi->type);
						v1 = stack_pop (TYPE_ADDRESS);
						if (checknull) {
#ifdef STATISTICS
					count_check_null++;
#endif
							BRA (CMD_IFNULL, v1,NOVAR, NOVAR, EXCREATOR(proto_java_lang_NullPointerException) );
							}
						MEM (CMD_PUTFIELD, fi->type, v1,v, NOVAR, fi->offset);
						DROP (v);
						DROP (v1);
						break;

					case CMD_GETFIELD:
						v1 = stack_pop (TYPE_ADDRESS);
						v = stack_push (fi->type);
						if (checknull) {
#ifdef STATISTICS
					count_check_null++;
#endif
							BRA (CMD_IFNULL, v1,NOVAR, NOVAR, EXCREATOR(proto_java_lang_NullPointerException) );
							}
						MEM (CMD_GETFIELD, fi->type, v1,NOVAR, v, fi->offset);
						DROP (v1);
						break;


					}
				}
				break;


			/*** Method invocation ***/

			case CMD_INVOKEVIRTUAL:
			case CMD_INVOKESPECIAL:
			case CMD_INVOKESTATIC:
			case CMD_INVOKEINTERFACE:
			
				count_calls ++;
			
				{ constant_FMIref *mr;
				  methodinfo *mi;
				  u4 i;
				  s4 paramnum;
				  u1 *paramtypes;
				  s4 returntype;
				  varid *params;
				  bool stat = (opcode == CMD_INVOKESTATIC);

				if (opcode==CMD_INVOKEINTERFACE) {
					poolindex = code_get_u2 (p+1);
					mr = class_getconstant (class, poolindex, CONSTANT_InterfaceMethodref);
					}
				else {
					poolindex = code_get_u2 (p+1);
					mr = class_getconstant (class, poolindex, CONSTANT_Methodref);
					}

				mi = class_findmethod (mr->class, mr->name, mr->descriptor);

				if ( ((mi->flags & ACC_STATIC) != 0) != stat)
					panic ("Static/Nonstatic mismatch on method call");

				descriptor2types (mi->descriptor, stat,
	                  &paramnum, &paramtypes, &returntype);
	            mi->paramcount = paramnum;

				params = DMNEW (varid, paramnum);
				for (i=0; i<paramnum; i++) {
					params[paramnum-i-1] = stack_pop (paramtypes[paramnum-i-1]);
					}

				stack_makesaved();

				if ((!stat) && checknull) {
#ifdef STATISTICS
					count_check_null++;
#endif
					BRA (CMD_IFNULL, params[0],NOVAR, NOVAR,  EXCREATOR(proto_java_lang_NullPointerException) );
					}

				ve = var_create (TYPE_ADDRESS);
				if (returntype != TYPE_VOID) {
					v = stack_push (returntype);
					METHOD (opcode, mi, NULL, paramnum, params, v,  ve);
					}
				else {
					METHOD (opcode, mi, NULL, paramnum, params, NOVAR, ve);
					}

				BRA (CMD_IFNONNULL, ve,NOVAR, NOVAR, EXFORWARDER(ve) );
				DROP (ve);

				}
				break;


			/********* Exception Handling *****************/

			case CMD_ATHROW:
				v = stack_pop (TYPE_ADDRESS);
				if (checknull) {
#ifdef STATISTICS
					count_check_null++;
#endif
					BRA (CMD_IFNULL, v,NOVAR, NOVAR,  EXCREATOR(proto_java_lang_NullPointerException) );
					}
				BRA (CMD_GOTO, NOVAR,NOVAR, NOVAR,  EXFORWARDER (v) );
				DROP (v);
				goto cleanup;


			/***** Miscellaneous Object Operations ****/

			case CMD_NEW:
				poolindex = code_get_u2 (p+1);
				{ classinfo *ci;

				ci = class_getconstant (class, poolindex, CONSTANT_Class);

				v1 = var_create (TYPE_ADDRESS);
				LOADCONST_A (ci, v1);

				stack_makesaved ();

				v = stack_push (TYPE_ADDRESS);
				BUILTIN1 ((functionptr) builtin_new, v1, v);
				BRA (CMD_IFNULL, v,NOVAR, NOVAR,  EXCREATOR(proto_java_lang_OutOfMemoryError) );

				}
				break;


			case CMD_CHECKCAST:
				poolindex = code_get_u2 (p+1);

				if (class_constanttype (class, poolindex) == CONSTANT_Arraydescriptor) {
					/* cast-check auf Array-Typ! */

					constant_arraydescriptor *desc =
					  class_getconstant (class, poolindex, CONSTANT_Arraydescriptor);

					v = stack_pop (TYPE_ADDRESS);
					stack_repush (v);
					v1 = var_create (TYPE_ADDRESS);
					MOVE (TYPE_ADDRESS, v,v1);
					v2 = var_create (TYPE_ADDRESS);
					LOADCONST_A (desc, v2);

					stack_makesaved ();
					ve = var_create (TYPE_INT);
					BUILTIN2 ((functionptr) builtin_checkarraycast, v1,v2, ve);

					BRA (CMD_IFEQ,ve,NOVAR, NOVAR,  EXCREATOR(proto_java_lang_ClassCastException) );
					DROP (ve);
					}
				else {
					/* cast-check auf Object-Typ */

					classinfo *ci = class_getconstant (class, poolindex, CONSTANT_Class);

					v = stack_pop (TYPE_ADDRESS);
					stack_repush (v);
					v1 = var_create (TYPE_ADDRESS);
					MOVE (TYPE_ADDRESS, v, v1);
					v2 = var_create (TYPE_ADDRESS);
					LOADCONST_A (ci, v2);

					stack_makesaved ();
					ve = var_create (TYPE_INT);
					BUILTIN2 ((functionptr) builtin_checkcast, v1,v2, ve);

					BRA (CMD_IFEQ,ve,NOVAR, NOVAR,  EXCREATOR(proto_java_lang_ClassCastException) );
					DROP (ve);
					}

				break;


			case CMD_INSTANCEOF:
				poolindex = code_get_u2 (p+1);

				if (class_constanttype (class, poolindex) == CONSTANT_Arraydescriptor) {
					/* cast-check auf Array-Typ! */

					constant_arraydescriptor *desc =
					  class_getconstant (class, poolindex, CONSTANT_Arraydescriptor);

					v = stack_pop (TYPE_ADDRESS);
					v1 = var_create (TYPE_ADDRESS);
					LOADCONST_A (desc, v1);

					stack_makesaved ();

					BUILTIN2 ((functionptr) builtin_arrayinstanceof, v,v1,
				    	              stack_push (TYPE_INT) );
					}
				else {
				 	classinfo *ci;	
				 	ci = class_getconstant (class, poolindex, CONSTANT_Class);

					v = stack_pop (TYPE_ADDRESS);
					v1 = var_create (TYPE_ADDRESS);
					LOADCONST_A (ci, v1);

					stack_makesaved ();

					BUILTIN2 ((functionptr) builtin_instanceof, v,v1,
				    	              stack_push (TYPE_INT) );
					}
				break;




			/*************** Monitors ************************/

			case CMD_MONITORENTER:
				if (checksync) {
					v = stack_pop (TYPE_ADDRESS);
					if (checknull) {
#ifdef STATISTICS
						count_check_null++;
#endif
						BRA (CMD_IFNULL, v, NOVAR, NOVAR,
						     EXCREATOR(proto_java_lang_NullPointerException));
						}
					BUILTIN1 ((functionptr) builtin_monitorenter, v, NOVAR);
				} else {
					DROP (stack_pop (TYPE_ADDRESS));
					}					
				break;

			case CMD_MONITOREXIT:
				if (checksync) {
					BUILTIN1 ((functionptr) builtin_monitorexit, 
					   stack_pop(TYPE_ADDRESS), NOVAR );
				} else {
					DROP (stack_pop (TYPE_ADDRESS));
					}					
				break;




			/************** any other Basic-Operation **********/


			default:
				{
				stdopdescriptor *s = stdopdescriptors[opcode];
				if (s) {
					v2 = NULL;
					if (s->type_s2 != TYPE_VOID) v2 = stack_pop(s->type_s2);
					v1 = stack_pop(s->type_s1);

					if (s->supported) {
						v = stack_push (s->type_d);
						if (v2) {
							OP2 (opcode, v1,v2, v);
							DROP (v1);
							DROP (v2);
							}
						else {
						  	OP1 (opcode, v1, v);
							DROP (v1);
							}
						}
					else {
						stack_makesaved ();
						v = stack_push (s->type_d);
						if (v2) {
							BUILTIN2 (s->builtin, v1,v2, v);
							}
						else {
							BUILTIN1 (s->builtin, v1, v);
							}
						}
					}
						

			/*************** invalid Opcode ***************/		
					
				else {
					sprintf (logtext, "Invalid opcode %d at position %ld",
			                           opcode, (long int) p);
					error ();
					}

				}
				
				break;
				

			} /* end switch */


		p = nextp;

		if ( block_isany (p) ) {
			block_reach ( block_find (p) );
			goto cleanup;
			}

		} /* end for */


cleanup:
	pcmd_untieall();
}

