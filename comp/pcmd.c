/********************************** comp/pcmd.c ********************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	contains the parser functions which generate pseude commands and
	eliminating unnecessary copy instructions

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/09/22

*******************************************************************************/


list *pcmdlist;  /* list of pseudo commands */


/***************************** support functions ******************************/

static void pcmd_init (list *pl)
{
	pcmdlist = pl;
}


static void pcmd_realmove (u2 type, varid source, varid dest)
{
	pcmd *c = DNEW(pcmd);
	c -> tag = TAG_MOVE;
	c -> opcode = CMD_MOVE;
	c -> source1 = source;
	c -> source2 = NOVAR;
	c -> source3 = NOVAR;
	c -> dest = dest;
	c -> u.move.type = type;
	list_addlast (pcmdlist, c);
#ifdef STATISTICS
	count_pcmd_move++;
#endif
}


static void pcmd_invalidatevar (varid v)
{
	varid c;
	
	if (!var_isoriginal(v) ) {
		var_unlinkcopy (v);
		}
	else {
		while ( (c = var_nextcopy (v)) != NULL) {
			pcmd_realmove (var_type(v), v,c);
			var_unlinkcopy (c);
			}
			
		}
}


static void pcmd_untievar (varid v)
{
	varid c;
	
	if (!var_isoriginal(v)) {
		pcmd_realmove (var_type(v), var_findoriginal(v), v);
		var_unlinkcopy (v);
		}
	else {
		while ( (c = var_nextcopy(v)) != NULL) {
			pcmd_realmove (var_type(v), v, c);
			var_unlinkcopy (c);
			}
		}
}


static void pcmd_untieall ()
{
	varid v;
	
	while ( (v = var_nextcopiedvar()) != NULL) {
	 	pcmd_realmove (var_type(v), var_findoriginal(v), v);
		var_unlinkcopy (v);
		}
}


/********************** generation of pseudo commands *************************/

static void pcmd_drop (varid var)
{
	pcmd *c = DNEW(pcmd);

	if (var_isoriginal(var)) {
		pcmd_invalidatevar (var);
	
		c -> tag = TAG_DROP;
		c -> opcode = CMD_DROP;
		c -> dest = var;
		c -> source1 = c -> source2 = c -> source3 = NOVAR;
		list_addlast (pcmdlist, c);
#ifdef STATISTICS
	count_pcmd_drop++;
#endif
		}
	else {
		pcmd_invalidatevar (var);
		}
}


static void pcmd_activate (varid var)
{
	pcmd *c = DNEW(pcmd);
	
	pcmd_untieall();

	c -> tag = TAG_ACTIVATE;
	c -> opcode = CMD_ACTIVATE;
	c -> dest = var;
	c -> source1 = c -> source2 = c -> source3 = NOVAR;
	list_addlast (pcmdlist, c);

#ifdef STATISTICS
	count_pcmd_activ++;
#endif
}


static void pcmd_loadconst_i (s4 val, varid var)
{
	pcmd *c = DNEW (pcmd);
	c -> tag = TAG_LOADCONST_I;
	c -> opcode = CMD_LOADCONST_I;
	c -> dest = var;
	c -> source1 = c -> source2 = c -> source3 = NOVAR;	
	c -> u.i.value = val;

	pcmd_invalidatevar (var);
	list_addlast (pcmdlist, c);

#ifdef STATISTICS
	if (val == 0)
		count_pcmd_zero++;
	count_pcmd_load++;
#endif
}


static void pcmd_loadconst_l (s8 val, varid var)
{
	pcmd *c = DNEW (pcmd);
	c -> tag = TAG_LOADCONST_L;
	c -> opcode = CMD_LOADCONST_L;
	c -> dest = var;
	c -> source1 = c -> source2 = c -> source3 = NOVAR;	
	c -> u.l.value = val;

	pcmd_invalidatevar (var);
	list_addlast (pcmdlist, c);

#ifdef STATISTICS
	if (val == 0)
		count_pcmd_zero++;
	count_pcmd_load++;
#endif
}


static void pcmd_loadconst_f (float val, varid var)
{
	pcmd *c = DNEW (pcmd);
	c -> tag = TAG_LOADCONST_F;
	c -> opcode = CMD_LOADCONST_F;
	c -> dest = var;
	c -> source1 = c -> source2 = c -> source3 = NOVAR;	
	c -> u.f.value = val;

	pcmd_invalidatevar (var);
	list_addlast (pcmdlist, c);

#ifdef STATISTICS
	count_pcmd_load++;
#endif
}


static void pcmd_loadconst_d (double val, varid var)
{
	pcmd *c = DNEW (pcmd);
	c -> tag = TAG_LOADCONST_D;
	c -> opcode = CMD_LOADCONST_D;
	c -> dest = var;
	c -> source1 = c -> source2 = c -> source3 = NOVAR;	
	c -> u.d.value = val;

	pcmd_invalidatevar (var);
	list_addlast (pcmdlist, c);

#ifdef STATISTICS
	count_pcmd_load++;
#endif
}


static void pcmd_loadconst_a (void *val, varid var)
{
	pcmd *c = DNEW (pcmd);
	c -> tag = TAG_LOADCONST_A;
	c -> opcode = CMD_LOADCONST_A;
	c -> dest = var;
	c -> source1 = c -> source2 = c -> source3 = NOVAR;	
	c -> u.a.value = val;

	pcmd_invalidatevar (var);
	list_addlast (pcmdlist, c);

#ifdef STATISTICS
	if (val == NULL)
		count_pcmd_zero++;
	count_pcmd_load++;
#endif
}


static void pcmd_move (u2 type, varid source, varid dest)
{
	pcmd_invalidatevar (dest);
	var_makecopy ( var_findoriginal(source), dest);
}


static void pcmd_move_n_drop (u2 type, varid source, varid dest)
{
	pcmd *c;
	
	pcmd_untievar (source);
	pcmd_untievar (dest);

#ifdef STATISTICS
	count_pcmd_store++;
#endif
	c = list_last (pcmdlist);
	while (c) {
		switch (c->tag) {
		case TAG_LOADCONST_I:
			if (c->dest == source) { c->dest = dest; return; }
#ifdef STATISTICS
				count_pcmd_const_store++;
#endif
			goto nothingfound;
		case TAG_LOADCONST_L:
			if (c->dest == source) { c->dest = dest; return; }
			goto nothingfound;
		case TAG_LOADCONST_F:
			if (c->dest == source) { c->dest = dest; return; }
			goto nothingfound;
		case TAG_LOADCONST_D:
			if (c->dest == source) { c->dest = dest; return; }
			goto nothingfound;
		case TAG_LOADCONST_A:
			if (c->dest == source) { c->dest = dest; return; }
			goto nothingfound;
		case TAG_OP:	
			if (c->dest == source) { c->dest = dest; return; }
			goto nothingfound;
		case TAG_MEM:
			if (c->dest == source) { c->dest = dest; return; }
			goto nothingfound;

		case TAG_DROP:
			break;
		default:
			goto nothingfound;
		} /* end switch */
		
		c = list_prev (pcmdlist, c);
		}

nothingfound:
	var_makecopy ( source, dest);
	pcmd_drop (source);
#ifdef STATISTICS
	count_pcmd_store_comb++;
#endif
}


static void pcmd_iinc (s4 val, varid sourcedest)
{
	pcmd *c = DNEW (pcmd);

	pcmd_invalidatevar (sourcedest);

	c -> tag = TAG_OP;
	c -> opcode = CMD_IINC;
	c -> source1 = var_findoriginal(sourcedest);
	c -> source2 = NOVAR;
	c -> source3 = NOVAR;	
	c -> dest = sourcedest;
	c -> u.i.value = val;

	list_addlast (pcmdlist, c);

#ifdef STATISTICS
	count_pcmd_op++;
#endif
}


static void pcmd_op (u1 opcode, 
             varid source1, varid source2, varid source3, varid dest)
{
	pcmd *c;
	
#ifdef STATISTICS
	c = list_last (pcmdlist);
	if (c && (c->tag == TAG_LOADCONST_I))
		switch (opcode) {
			case CMD_IASTORE:
			case CMD_BASTORE:
			case CMD_CASTORE:
			case CMD_SASTORE:
				count_pcmd_const_store++;
				break;
			case CMD_IADD:
			case CMD_ISUB:
			case CMD_IMUL:
			case CMD_ISHL:
			case CMD_ISHR:
			case CMD_IUSHR:
			case CMD_IAND:
			case CMD_IOR:
			case CMD_IXOR:
				count_pcmd_const_alu++;
			}
#endif
	c = DNEW(pcmd);
	if (dest) pcmd_invalidatevar (dest);
	
	c -> tag = TAG_OP;
	c -> opcode = opcode;
	c -> source1 = source1 ? var_findoriginal(source1) : NOVAR;
	c -> source2 = source2 ? var_findoriginal(source2) : NOVAR;
	c -> source3 = source3 ? var_findoriginal(source3) : NOVAR;
	c -> dest = dest;
	list_addlast (pcmdlist, c);
#ifdef STATISTICS
	count_pcmd_op++;
#endif
}


static void pcmd_mem (u1 opcode, u2 type, varid base, varid source, 
                        varid dest, u2 offset) 
{
	pcmd *c;

#ifdef STATISTICS
	c = list_last (pcmdlist);
	if (c && (c->tag == TAG_LOADCONST_I) && (opcode == CMD_PUTFIELD))
		count_pcmd_const_store++;
#endif
	c = DNEW(pcmd);
	if (dest) pcmd_invalidatevar (dest);
	
	c -> tag = TAG_MEM;
	c -> opcode = opcode;
	c -> source1 = base   ? var_findoriginal(base) : NOVAR;
	c -> source2 = source ? var_findoriginal(source) : NOVAR;
	c -> source3 = NOVAR;
	c -> dest = dest;
	c -> u.mem.type = type;
	c -> u.mem.offset = offset;
	list_addlast (pcmdlist, c);

#ifdef STATISTICS
	count_pcmd_mem++;
#endif
}


static void pcmd_bra (u1 opcode, varid s1, varid s2, varid dest, basicblock *target)
{
	pcmd *c;
	varid or1,or2;
	
#ifdef STATISTICS
	c = list_last (pcmdlist);
	if (c && (c->tag == TAG_LOADCONST_I))
		switch (opcode) {
			case CMD_IF_ICMPEQ:
			case CMD_IF_ICMPNE:
			case CMD_IF_ICMPLT:
			case CMD_IF_ICMPGT:
			case CMD_IF_ICMPLE:
			case CMD_IF_ICMPGE:
				count_pcmd_const_bra++;
			}
#endif
	c = DNEW(pcmd);
	or1 = s1 ? var_findoriginal (s1) : NOVAR;
	or2 = s2 ? var_findoriginal (s2) : NOVAR;
	
	pcmd_untieall();
	
	c -> tag = TAG_BRA;
	c -> opcode = opcode;
	c -> source1 = or1;
	c -> source2 = or2;
	c -> source3 = NOVAR;
	c -> dest = dest;
	c -> u.bra.target = target;
	list_addlast (pcmdlist, c);

#ifdef STATISTICS
	count_pcmd_bra++;
#endif
}


static void pcmd_trace (void *method)
{
	pcmd *c;
	
	isleafmethod = false;
	
	c = DNEW(pcmd);
	c -> tag = TAG_BRA;
	c -> opcode = CMD_TRACEBUILT;
	c -> source1 = NOVAR;
	c -> source2 = NOVAR;
	c -> source3 = NOVAR;
	c -> dest = NOVAR;
	c -> u.a.value = method;
	list_addlast (pcmdlist, c);
}


static void pcmd_bra_n_drop (u1 opcode, varid s1, varid s2, varid dest, basicblock *target)
{
	pcmd *c;
	varid or1=NOVAR,or2=NOVAR;
	bool isor1=false,isor2=false;
	
#ifdef STATISTICS
	c = list_last (pcmdlist);
	if (c && (c->tag == TAG_LOADCONST_I))
		switch (opcode) {
			case CMD_IF_ICMPEQ:
			case CMD_IF_ICMPNE:
			case CMD_IF_ICMPLT:
			case CMD_IF_ICMPGT:
			case CMD_IF_ICMPLE:
			case CMD_IF_ICMPGE:
				count_pcmd_const_bra++;
			}
#endif
	c = DNEW(pcmd);
	if (s1!=NOVAR) {
		or1 = var_findoriginal (s1); 
	    if (! (isor1 = var_isoriginal(s1)) ) var_unlinkcopy (s1);
	   	}
	if (s2!=NOVAR) {
		or2 = var_findoriginal (s2);
		if (! (isor2 = var_isoriginal (s2)) ) var_unlinkcopy (s2);
		}

	pcmd_untieall();

	c -> tag = TAG_BRA;
	c -> opcode = opcode;
	c -> source1 = or1;
	c -> source2 = or2;
	c -> source3 = NOVAR;
	c -> dest = dest;
	c -> u.bra.target = target;
	list_addlast (pcmdlist, c);
	
	if (isor1) pcmd_drop (s1);
	if (isor2) pcmd_drop (s2);

#ifdef STATISTICS
	count_pcmd_bra++;
#endif
}


static void pcmd_tablejump (varid s, u4 targetcount, basicblock **targets)
{
	pcmd *c = DNEW(pcmd);
	
	pcmd_untieall();
	
	c -> tag = TAG_TABLEJUMP;
	c -> opcode = CMD_TABLEJUMP;
	c -> source1 = var_findoriginal(s);
	c -> source2 = NOVAR;
	c -> source3 = NOVAR;
	c -> dest = NOVAR;
	c -> u.tablejump.targetcount = targetcount;
	c -> u.tablejump.targets = targets;
	list_addlast (pcmdlist, c);

#ifdef STATISTICS
	count_pcmd_table++;
#endif
}


/******* ATTENTION: Method does DROP automatically !!!!! ****/

static void pcmd_method (int opcode, methodinfo *mi, functionptr builtin, 
                         int paramnum, varid *params, varid result, varid exceptionvar)
{
	varid v;
	int i;

	pcmd *c = DNEW(pcmd);
	
	pcmd_untieall();
					
	isleafmethod = false;

	reg_parlistinit();
	for (i = 0; i < paramnum; i++) {
		v = params[i];
		var_proposereg(v, reg_parlistpar(var_type(v)));
		}
		
	if (result) {
		var_proposereg(result, reg_parlistresult(var_type(result)));
		}
	if (exceptionvar) {
		exceptionvar -> reg = reg_parlistexception();
		}

	c -> tag = TAG_METHOD;
	c -> opcode = opcode;
	c -> source1 = c -> source2 = c -> source3 = NULL;
	c -> dest = result;
	c -> u.method.method = mi;
	c -> u.method.builtin = builtin;
	c -> u.method.paramnum = paramnum;
	c -> u.method.params = params;
	c -> u.method.exceptionvar = exceptionvar;
	list_addlast (pcmdlist, c);
	
	for (i = 0; i < paramnum; i++) pcmd_drop(params[i]);
	
#ifdef STATISTICS
	count_pcmd_met++;
#endif
}



static void pcmd_builtin1 (functionptr builtin, varid v1, varid result)
{
	varid *args = DMNEW (varid, 1);
	args[0] = v1;
	pcmd_method (CMD_BUILTIN, NULL, builtin, 1, args, result,NOVAR);
}

static void pcmd_builtin2 (functionptr builtin, varid v1, varid v2, varid result)
{
	varid *args = DMNEW (varid, 2);
	args[0] = v1;
	args[1] = v2;
	pcmd_method (CMD_BUILTIN, NULL, builtin, 2, args, result, NOVAR);
}

static void pcmd_builtin3 (functionptr builtin, varid v1, varid v2, varid v3, varid result)
{
	varid *args = DMNEW (varid, 3);
	args[0] = v1;
	args[1] = v2;
	args[2] = v3;
	pcmd_method (CMD_BUILTIN, NULL, builtin, 3, args, result, NOVAR);
}


static void pcmd_display (pcmd *c)
{
	int i;

	switch (c -> tag) {
		case TAG_LOADCONST_I:
			printf ("   LOADCONST_I #%ld -> ", 
                                         (long int) c -> u.i.value);
			var_display (c -> dest);
			printf ("\n");
			break;
		case TAG_LOADCONST_L:  
#if U8_AVAILABLE
			printf ("   LOADCONST_L #%ld -> ", (long int) c -> u.l.value);
#else
			printf ("   LOADCONST_L #HI: %ld LO: %ld -> ", 
			  (long int) c->u.l.value.high, (long int) c->u.l.value.low );
#endif
			var_display (c -> dest);
			printf ("\n");
			break;
		case TAG_LOADCONST_F:  
			printf ("   LOADCONST_F #%f -> ", (double) c -> u.f.value);				
			var_display (c -> dest);
			printf ("\n");
			break;
		case TAG_LOADCONST_D:  
			printf ("   LOADCONST_D #%f -> ", c ->  u.d.value);
			var_display (c -> dest);
			printf ("\n");
			break;
		case TAG_LOADCONST_A:  
			printf ("   LOADCONST_A #%p -> ", c ->  u.a.value);
			var_display (c -> dest);
			printf ("\n");
			break;
			
		case TAG_MOVE:
			printf ("   MOVE_%1d      ", c -> u.move.type);
			var_display (c-> source1);	
			printf (" -> ");
			var_display (c -> dest);
			printf ("\n");
			break;
			
		case TAG_OP:
			printf ("   OP%3d       ", c -> opcode);
			var_display (c -> source1);
			printf (", ");
			var_display (c -> source2);
			printf (", ");
			var_display (c -> source3);
			printf (" -> ");
			var_display (c -> dest );
			printf ("\n");
			break;

		case TAG_BRA:
			printf ("   BRA%3d     (", c -> opcode);
			var_display (c -> source1);
			printf (", ");
			var_display (c -> source2);
			printf (" -> ");
			var_display (c -> dest);
			if (c->u.bra.target) {
				printf (") to pos %ld\n", 
				   (long int) c -> u.bra.target -> jpc );
				}
			else printf (")\n");
			break;
			
		case TAG_TABLEJUMP:
			printf ("   TABLEJUMP     ");
			var_display (c -> source1);
			printf ("\n");
			for (i=0; i < (int) c->u.tablejump.targetcount; i++) {
				printf ("      %d: to pos %ld\n", (int) i,
				   (long int) c ->u.tablejump.targets[i] -> jpc );
				}
			break;

		case TAG_MEM:
			printf ("   MEM%3d_%d    ",  c -> opcode, c -> u.mem.type);
			var_display (c -> source2);
			printf ("/");
			var_display (c -> dest);
			printf ("  <->  ");
			printf ("%d [", (int) (c -> u.mem.offset) );
			var_display (c -> source1 );
			printf ("]\n");
			break;

		case TAG_METHOD:
			printf ("   METHOD%3d   ", c -> opcode);
			for (i=0; i < c -> u.method.paramnum; i++) {
			   var_display (c -> u.method.params[i]);
			   }
			printf (" -> ");
			var_display (c -> dest);
			printf ("/");
			var_display (c->u.method.exceptionvar);
			printf ("\n");
			break;

		case TAG_DROP:
			printf ("   [ DROP      ");
			var_display (c-> dest);
			printf (" ]\n");
			break;
		case TAG_ACTIVATE:
			printf ("   [ ACTIVATE      ");
			var_display (c-> dest);
			printf (" ]\n");
			break;

		default:
			printf ("   ???\n");
			break;
		}

}
