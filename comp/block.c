/***************************** comp/block.c ************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Basic block handling functions.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/01/18

*******************************************************************************/


static u2 creatornum;  /* Fortlaufende Nummerierung f"ur vom Compiler
                          generierte BasicBlocks (nur zu Debug-Zwecken) */

static subroutineinfo *actual_subroutine;
static u2 subroutinecounter=0;


/********************** Funktion: block_new ***********************************

	erzeugt eine neue 'basicblock'-Struktur und initialisiert alle 
	Komponenten 

******************************************************************************/

static basicblock *block_new (u2 type, u4 codepos)
{
	basicblock *b = DNEW (basicblock);
	
	b -> type = type;
	b -> reached = false;
	b -> finished = false;
	b -> subroutine = NULL;
	b -> jpc = codepos;
	b -> stack = NULL;
	list_init (&(b->pcmdlist), OFFSET (pcmd, linkage) );
	b -> mpc = 0;
	
	b -> exproto = NULL;
	b -> throwpos = 0;
	return b;
}



/******************* Funktion: block_find *************************************

	Sucht den Basicblock, der an einer gew"unschten Stelle im JavaVM-Code
	anf"angt.
	Wenn dort kein Block anf"angt  -> Fehler
	
******************************************************************************/

static basicblock *block_find (u4 codepos)
{
	basicblock *b = blocks[codepos];
	if (!b) panic ("Accessed JAVA-Command on no block boundary"); 
	return b;
}


/****************** Funktion: block_isany *************************************

	"uberpr"uft, ob an einer Stelle ein Basicblock anf"angt
	
******************************************************************************/

static bool block_isany (u4 codepos)
{
	return (blocks[codepos] != NULL);
}


/***************** Funktion: block_reach **************************************

	H"angt einen Basicblock in die Liste der schon erreichten Bl"ocke ein,
	und setzt seinen Blockeintrittsstack auf den aktuellen Stack und
	setzt den Unterprogramminfoblock auf das aktuelle Unterprogramminfo
	
	Wenn der Block bereits vorher als erreicht markiert war (und er deshalb
	schon einen definierten Stack hatte), dann wird im derzeit vom 
	Parser durchlaufenen Block (also dort, von wo dieser Block aus 
	angesprungen wird) ein entsprechende Codest"uck hinzugef"ugt, der
	die beiden Stacks aufeinander abstimmt.
	
******************************************************************************/

static void block_reach (basicblock *b)
{
	if (!b->reached) {
		list_addlast (&reachedblocks, b);
		b -> reached = true;
		b -> subroutine = actual_subroutine;
		b -> stack = stack_get();
		}
	else {
		if (b->subroutine != actual_subroutine) 
		   panic ("Try to merge different subroutines");
		stack_addjoincode (b->stack);
		}
}

/*********************** Funktion: subroutine_set *****************************

	setzt den aktuellen Subroutine-Infoblock 
	
*******************************************************************************/

static void subroutine_set (subroutineinfo *s)
{
	actual_subroutine = s;
}



/*********************** Funktion: subroutine_new *****************************

	erzeugt einen neuen Subroutine-Infoblock
	
*******************************************************************************/

static subroutineinfo *subroutine_new ()
{
	subroutineinfo *s = DNEW (subroutineinfo);
	s -> returnfinished = false;
	s -> returnstack = NULL;
	s -> callers = chain_dnew();
	s -> counter = subroutinecounter++;
	return s;
}






/********************** Funktion: block_insert ********************************

	Erzeugt einen neuen Block, der an einer gew"unschten JavaVM-Code- Stelle
	anf"angt.
	Der Zeiger auf diesen Block wird im (globalen) 'blocks'-Array 
	abgelegt.
	
******************************************************************************/	

static void block_insert (u4 codepos)
{
	if (codepos>=jcodelength) {
		sprintf (logtext,"Basic block border (%d) out of bounds",(int) codepos);
		error ();
		}

	if (blocks[codepos]) return;

	blocks[codepos] = block_new (BLOCKTYPE_JAVA, codepos);
}



/******************** Funktion: block_createexcreator *************************

	erzeugt einen neuen Basicblock vom Typ EXCREATOR (=Exception Creator)
	
******************************************************************************/

static basicblock *block_createexcreator (java_objectheader *exproto, u4 throwpos)
{
	basicblock *b;

	b = block_new (BLOCKTYPE_EXCREATOR, creatornum++);
	
	b -> reached = true;
	list_addlast (&reachedblocks, b);
	b -> subroutine = actual_subroutine;
	b -> exproto = exproto;
	b -> throwpos = throwpos;
	return b;
}


/******************* Funktion: block_createexforwarder ***********************

	erzeugt einen neuen Basicblock vom Typ EXFORWARDER (=Exception Forwarder)
	
*****************************************************************************/

static basicblock *block_createexforwarder (varid exvar, u4 throwpos)
{
	basicblock *b;

	b = block_new (BLOCKTYPE_EXFORWARDER, creatornum++);
	
	b -> reached = true;
	list_addlast (&reachedblocks, b);
	b -> subroutine = actual_subroutine;
	b -> exvar = exvar;
	b -> throwpos = throwpos;
	return b;
}


/********************** Funktion: block_genmcode ******************************

	generiert f"ur einen vom Parser fertig abgearbeiteten Block den
	Maschinencode.
	Hintereinanderliegende Bl"ocke durch die der Kontrollflu"s ohne
	Sprungbefehle durchgeht, m"ussen hier auch hintereinander 
	abgearbeitet werden.
	
******************************************************************************/

/* definition of block_genmcode moved to gen.c for inlining by andi          */


/************************ Funktion: block_firstscann **************************

	Liest den JavaVM-Code der ganzen Methode durch und erzeugt soviele
	neue Bl"ocke, wie es verschiedene Sprungziele gibt.
	
******************************************************************************/

static void block_firstscann ()
{
	u4 p,nextp;
	int i;
	u1 opcode;
	bool blockend;


	creatornum=10001;


	block_insert (0);

	for (i=0; i<exceptiontablelength; i++) block_insert(extable[i].handlerpc);

	p=0;
	blockend = false;
	while (p<jcodelength) {
		if (blockend) {
			block_insert (p);
			blockend = false;
			}

		opcode = jcode[p];
		nextp = p + jcommandsize[opcode];

		switch ( opcode ) {
			case CMD_IFEQ:
			case CMD_IFNULL:
			case CMD_IFLT:
			case CMD_IFLE:
			case CMD_IFNE:
			case CMD_IFNONNULL:
			case CMD_IFGT:
			case CMD_IFGE:
			case CMD_IF_ICMPEQ:
			case CMD_IF_ICMPNE:
			case CMD_IF_ICMPLT:
			case CMD_IF_ICMPGT:
			case CMD_IF_ICMPLE:
			case CMD_IF_ICMPGE:
			case CMD_IF_ACMPEQ:
			case CMD_IF_ACMPNE:
				block_insert ( p + code_get_s2 (p+1) );
				break;

			case CMD_GOTO:
				block_insert ( p + code_get_s2 (p+1) );
				blockend = true;
				break;

			case CMD_GOTO_W:
				block_insert ( p + code_get_s4 (p+1) );
				blockend = true;
				break;
				
			case CMD_JSR:
				block_insert ( p + code_get_s2 (p+1) );
				blockend = true;
				break;
			
			case CMD_JSR_W:
				block_insert ( p + code_get_s4 (p+1) );
				blockend = true;
				break;

			case CMD_RET:
				blockend = true;
				break;

			case CMD_IRETURN:
			case CMD_LRETURN:
			case CMD_FRETURN:
			case CMD_DRETURN:
			case CMD_ARETURN:
			case CMD_RETURN:
				blockend = true;
				break;

			case CMD_ATHROW:
				blockend = true;
				break;
				

			case CMD_WIDE:
				switch (code_get_u1(p+1)) {
				case CMD_RET:   nextp = p+4;
				                blockend = true;
				                break;
				case CMD_IINC:  nextp = p+6;
				                break;
				default:        nextp = p+4;
				                break;
				}
				break;
							
			case CMD_LOOKUPSWITCH:
				{ u4 num,p2,i;
					p2 = ALIGN ((p+1),4);
					num = code_get_u4 (p2+4);
					nextp = p2 + 8 + 8*num;

					block_insert ( p + code_get_s4(p2) );
					for (i=0; i<num; i++) 
						block_insert ( p + code_get_s4(p2+12+8*i) );
					
					blockend = true;
				}
				break;

			case CMD_TABLESWITCH:
				{ u4 num,p2,i;
					p2 = ALIGN ((p+1),4);
					num= code_get_u4(p2+8) - code_get_u4 (p2+4) + 1;
					nextp = p2 + 12 + 4*num;

					block_insert ( p + code_get_s4(p2) );
					
					for (i=0; i<num; i++) 
						block_insert ( p + code_get_s4(p2+12+4*i) );
						
					blockend = true;
				}	
				break;

			case CMD_LDC1:
				i = code_get_u1(p+1);
				goto pushconstantitem;
			case CMD_LDC2:
			case CMD_LDC2W:
				i = code_get_u2(p + 1);
			pushconstantitem:
				if (class_constanttype(class, i) == CONSTANT_String) {
					utf *s;
					s = class_getconstant(class, i, CONSTANT_String);
					(void) literalstring_new(s);
					}
				break;
			}

		if (nextp > jcodelength) panic ("Command-sequence crosses code-boundary");
		p = nextp;
		}
	
	if (!blockend) panic ("Code does not end with branch/return/athrow - stmt");	
	
}



/************* Funktion: block_display (nur zu Debug-Zwecken) ****************/

void block_display (basicblock *b)
{
	pcmd *p;
	
	if (b->subroutine) {
		printf ("\n%ld in subroutine: %d", (long int) b->jpc, (int) b->subroutine->counter);
		}
	else {
		printf ("\n%ld:", (long int) b->jpc);
		}

	switch (b->type) {
		case BLOCKTYPE_JAVA:        printf ("(JavaVM code)\n"); break;
		case BLOCKTYPE_EXCREATOR:   printf ("(Exception creator)\n"); break;
		case BLOCKTYPE_EXFORWARDER: printf ("(Exception forwarder)\n"); break;
		}
		
	printf ("binding of stack:  "); 
	stack_display (b->stack); printf ("\n");
	
	p = list_first (&(b->pcmdlist));
	while (p) {
		pcmd_display (p);
		p = list_next (&(b->pcmdlist), p);
		}
}


