static void descriptor2types (methodinfo *m);

/*typedef struct {
        listnode linkage;
        instruction *iptr;
        } t_patchlistnode;*/


typedef struct {
        listnode linkage;

        methodinfo *method;
        int startgp;
        int stopgp;
        int firstlocal;

        bool *readonly;
        int  *label_index;
        
        list *inlinedmethods;
} inlining_methodinfo;

typedef struct {
        listnode linkage;
        
	// saved static compiler variables
        
        methodinfo *method;
        
	// restored through method

	// int jcodelength;
	// u1 *jcode;
	// classinfo *class;

	// descriptor never used
	// maxstack used outside of main for loop
	// maxlocals never used
	
	// exceptiontablelength
	// raw_extable used outside of main for loop
	// mreturntype used outside of main for loop
	// mparamcount used outside of main for loop
	// mparamtypes used outside of main for loop

	//local variables used in parse()  

	int  i;                     /* temporary for different uses (counters)    */
	int  p;                     /* java instruction counter                   */
	int  nextp;                 /* start of next java instruction             */
	int  opcode;                /* java opcode                                */

	inlining_methodinfo *inlinfo;

	/*	list *patchlist; */
} t_inlining_stacknode;

// checked functions and macros: LOADCONST code_get OP1 BUILTIN block_insert bound_check ALIGN

// replace jcodelength loops with correct number after main for loop in parse()!

static list *inlining_stack;
//static list *inlining_patchlist;
static bool isinlinedmethod;
static int cumjcodelength;         /* cumulative immediate intruction length      */
static int cumlocals;
static int cummaxstack;
static int cumextablelength;
static int cummethods;
static inlining_methodinfo *inlining_rootinfo;


void inlining_push_compiler_variables(int i, int p, int nextp, int opcode, inlining_methodinfo* inlinfo);
void inlining_pop_compiler_variables(int *i, int *p, int *nextp, int *opcode, inlining_methodinfo** inlinfo); 
void inlining_init(void);
inlining_methodinfo *inlining_analyse_method(methodinfo *m, int level, int gp, int firstlocal, int maxstackdepth);

#define inlining_save_compiler_variables() inlining_push_compiler_variables(i,p,nextp,opcode,inlinfo)
#define inlining_restore_compiler_variables() inlining_pop_compiler_variables(&i,&p,&nextp,&opcode,&inlinfo)
#define inlining_set_compiler_variables(i) p=nextp=0; inlining_set_compiler_variables_fun(i->method); inlinfo=i;
#define INLINING_MAXDEPTH 1
#define INLINING_MAXCODESIZE 32
#define INLINING_MAXMETHODS 8

void inlining_init(void) 
{
	inlining_stack = NULL;
	//	inlining_patchlist = NULL;
	isinlinedmethod = 0;
	cumjcodelength = 0;
	cumlocals = 0;
	cumextablelength = 0;
	cummaxstack = 0;
	cummethods = 0;

	inlining_stack = NEW(list);
	list_init(inlining_stack, OFFSET(t_inlining_stacknode, linkage));
	
	inlining_rootinfo = inlining_analyse_method(method, 0, 0, 0, 0);
	maxlocals = cumlocals;
	maxstack = cummaxstack;
}

void inlining_cleanup(void)
{
	FREE(inlining_stack, t_inlining_stacknode);
}

void inlining_push_compiler_variables(int i, int p, int nextp, int opcode, inlining_methodinfo *inlinfo) 
{
	t_inlining_stacknode *new = NEW(t_inlining_stacknode);

	new->i = i;
	new->p = p;
	new->nextp = nextp;
	new->opcode = opcode;
	new->method = method;
	//	new->patchlist = inlining_patchlist;
	new->inlinfo = inlinfo;
	
	list_addfirst(inlining_stack, new);
	isinlinedmethod++;
}

void inlining_pop_compiler_variables(int *i, int *p, int *nextp, int *opcode, inlining_methodinfo **inlinfo) 
{
	t_inlining_stacknode *tmp = (t_inlining_stacknode *) list_first(inlining_stack);

	if (!isinlinedmethod) panic("Attempting to pop from inlining stack in toplevel method!\n");

	*i = tmp->i;
	*p = tmp->p;
	*nextp = tmp->nextp;
	*opcode = tmp->opcode;
	*inlinfo = tmp->inlinfo;

	method = tmp->method;
	class = method->class;
	jcodelength = method->jcodelength;
	jcode = method->jcode;
	//	inlining_patchlist = tmp->patchlist;

	list_remove(inlining_stack, tmp);
	FREE(tmp, t_inlining_stacknode);
	isinlinedmethod--;
}

void inlining_set_compiler_variables_fun(methodinfo *m)
{
	method = m;
	class = m->class;
	jcodelength = m->jcodelength;
	jcode = m->jcode;
	
	//	inlining_patchlist = DNEW(list);
	//	list_init(inlining_patchlist, OFFSET(t_patchlistnode, linkage));
}

/*void inlining_addpatch(instruction *iptr)  
  {
  t_patchlistnode *patch = DNEW(t_patchlistnode);
  patch->iptr = iptr;
  list_addlast(inlining_patchlist, patch);
  }*/


classinfo *first_occurence(classinfo* class, utf* name, utf* desc) {
	classinfo *first = class;
	
	for (; class->super != NULL ; class = class->super) {
		if (class_findmethod(class->super, name, desc) != NULL) {
			first = class->super;
		}			
	}
		return first;
}

bool is_unique_rec(classinfo *class, methodinfo *m, utf* name, utf* desc) {
	methodinfo *tmp = class_findmethod(class, name, desc);
	if ((tmp != NULL) && (tmp != m))
		return false;

	for (; class != NULL; class = class->nextsub) {
		if ((class->sub != NULL) && !is_unique_rec(class->sub, m, name, desc)) {
			return false; 
		}
	}
	return true;
}

bool is_unique_method(classinfo *class, methodinfo *m, utf* name, utf* desc) {
	classinfo *firstclass;
	
	/*	sprintf (logtext, "First occurence of: ");
	utf_sprint (logtext+strlen(logtext), m->class->name);
	strcpy (logtext+strlen(logtext), ".");
	utf_sprint (logtext+strlen(logtext), m->name);
	utf_sprint (logtext+strlen(logtext), m->descriptor);
	dolog (); */
	
	firstclass = first_occurence(class, name, desc);
	
	/*	sprintf (logtext, "\nis in class:");
	utf_sprint (logtext+strlen(logtext), firstclass->name);
	dolog (); */

	if (firstclass != class) return false;

	return is_unique_rec(class, m, name, desc);
}


inlining_methodinfo *inlining_analyse_method(methodinfo *m, int level, int gp, int firstlocal, int maxstackdepth)
{
	inlining_methodinfo *newnode = DNEW(inlining_methodinfo);
	u1 *jcode = m->jcode;
	int jcodelength = m->jcodelength;
	int p;
	int nextp;
	int opcode;
	int i;
/*  	int lastlabel = 0; */
	bool iswide = false, oldiswide;
	bool *readonly = NULL;
	int  *label_index = NULL;
	bool isnotrootlevel = (level > 0);
	bool isnotleaflevel = (level < INLINING_MAXDEPTH);

	//	if (level == 0) gp = 0;
	/*
	sprintf (logtext, "Performing inlining analysis of: ");
	utf_sprint (logtext+strlen(logtext), m->class->name);
	strcpy (logtext+strlen(logtext), ".");
	utf_sprint (logtext+strlen(logtext), m->name);
	utf_sprint (logtext+strlen(logtext), m->descriptor);
	dolog (); */

	if (isnotrootlevel) {
		newnode->readonly = readonly = DMNEW(bool, m->maxlocals); //FIXME only paramcount entrys necessary
		for (i = 0; i < m->maxlocals; readonly[i++] = true);
		isnotrootlevel = true;
	} else {
		readonly = NULL;
	}
	
	label_index = DMNEW(int, jcodelength);

	newnode->inlinedmethods = DNEW(list);
	list_init(newnode->inlinedmethods, OFFSET(inlining_methodinfo, linkage));

	newnode->method = m;
	newnode->startgp = gp;
	newnode->readonly = readonly;
	newnode->label_index = label_index;
	newnode->firstlocal = firstlocal;
	cumjcodelength += jcodelength + m->paramcount + 1 + 5;
	if ((firstlocal + m->maxlocals) > cumlocals) cumlocals = firstlocal + m->maxlocals;
	if ((maxstackdepth + m->maxstack) >  cummaxstack) cummaxstack = maxstackdepth + m->maxstack;
	cumextablelength += m->exceptiontablelength;
   

	for (p = 0; p < jcodelength; gp += (nextp - p), p = nextp) {
		opcode = code_get_u1 (p);
		nextp = p + jcommandsize[opcode];
		oldiswide = iswide;

		/* figure out nextp */

		switch (opcode) {

		case JAVA_ILOAD:
		case JAVA_LLOAD:
		case JAVA_FLOAD:
		case JAVA_DLOAD:
		case JAVA_ALOAD: 

		case JAVA_ISTORE:
		case JAVA_LSTORE:
		case JAVA_FSTORE:
		case JAVA_DSTORE:
		case JAVA_ASTORE: 

		case JAVA_RET:

			if (iswide) { nextp = p + 3; iswide = false; }
			break;

		case JAVA_IINC:
			if (iswide) { nextp = p + 5; iswide = false; }
			break;

		case JAVA_WIDE:
			iswide = true;
			nextp = p + 1;
			break;

		case JAVA_LOOKUPSWITCH:
			nextp = ALIGN((p + 1), 4) + 4;
			nextp += code_get_u4(nextp) * 8 + 4;
			break;

		case JAVA_TABLESWITCH:
			nextp = ALIGN((p + 1), 4) + 4;
			nextp += (code_get_u4(nextp+4) - code_get_u4(nextp) + 1) * 4 + 4;
			break;

		}
		/* detect readonly variables in inlined methods */
		
		if (isnotrootlevel) { 
			bool iswide = oldiswide;
			
			switch (opcode) {
				
			case JAVA_ISTORE:
			case JAVA_LSTORE:
			case JAVA_FSTORE:
			case JAVA_DSTORE:
			case JAVA_ASTORE: 
				if (!iswide)
					i = code_get_u1(p+1);
				else {
					i = code_get_u2(p+1);
				}
				readonly[i] = false;
				break;

			case JAVA_ISTORE_0:
			case JAVA_LSTORE_0:
			case JAVA_FSTORE_0:
			case JAVA_ASTORE_0:
				readonly[0] = false;
				break;

			case JAVA_ISTORE_1:
			case JAVA_LSTORE_1:
			case JAVA_FSTORE_1:
			case JAVA_ASTORE_1:
				readonly[1] = false;
				break;

			case JAVA_ISTORE_2:
			case JAVA_LSTORE_2:
			case JAVA_FSTORE_2:
			case JAVA_ASTORE_2:
				readonly[2] = false;
				break;

			case JAVA_ISTORE_3:
			case JAVA_LSTORE_3:
			case JAVA_FSTORE_3:
			case JAVA_ASTORE_3:
				readonly[3] = false;
				break;

			case JAVA_IINC:
				
				if (!iswide) {
					i = code_get_u1(p + 1);
				}
				else {
					i = code_get_u2(p + 1);
				}
				readonly[i] = false;
				break;
			}
			
			
		}

		/*		for (i=lastlabel; i<=p; i++) label_index[i] = gp; 
		//		printf("lastlabel=%d p=%d gp=%d\n",lastlabel, p, gp);
		lastlabel = p+1; */
		for (i=p; i < nextp; i++) label_index[i] = gp;

		if (isnotleaflevel) { 

			switch (opcode) {
			case JAVA_INVOKEVIRTUAL:
				if (!inlinevirtuals) break;
			case JAVA_INVOKESTATIC:
				i = code_get_u2(p + 1);
				{
					constant_FMIref *imr;
					methodinfo *imi;

					imr = class_getconstant (m->class, i, CONSTANT_Methodref);
					imi = class_findmethod (imr->class, imr->name, imr->descriptor);

					if (opcode == JAVA_INVOKEVIRTUAL) 
						{
							if (!is_unique_method(imi->class, imi, imr->name ,imr->descriptor))
								break;
						}

					if ( (cummethods < INLINING_MAXMETHODS) &&
						 (! (imi->flags & ACC_NATIVE)) &&  
						 ( !inlineoutsiders || (class == imr->class)) && 
						 (imi->jcodelength < INLINING_MAXCODESIZE) && 
						 (imi->jcodelength > 0) && 
						 (!inlineexceptions || (imi->exceptiontablelength == 0)) ) //FIXME: eliminate empty methods?
						{
							inlining_methodinfo *tmp;
							descriptor2types(imi);

							cummethods++;

							/*	sprintf (logtext, "Going to inline: ");
							utf_sprint (logtext+strlen(logtext), imi->class->name);
							strcpy (logtext+strlen(logtext), ".");
							utf_sprint (logtext+strlen(logtext), imi->name);
							utf_sprint (logtext+strlen(logtext), imi->descriptor);
							dolog (); */

							tmp = inlining_analyse_method(imi, level+1, gp, firstlocal + m->maxlocals, maxstackdepth + m->maxstack);
							list_addlast(newnode->inlinedmethods, tmp);
							gp = tmp->stopgp;
							p = nextp;
						}
				}
				break;
			}
		}  
		
	} /* for */
	
	newnode->stopgp = gp;

	/*
	sprintf (logtext, "Result of inlining analysis of: ");
	utf_sprint (logtext+strlen(logtext), m->class->name);
	strcpy (logtext+strlen(logtext), ".");
	utf_sprint (logtext+strlen(logtext), m->name);
	utf_sprint (logtext+strlen(logtext), m->descriptor);
	dolog ();
	sprintf (logtext, "label_index[0..%d]->", jcodelength);
	for (i=0; i<jcodelength; i++) sprintf (logtext, "%d:%d ", i, label_index[i]);
	sprintf(logtext,"stopgp : %d\n",newnode->stopgp); */

    return newnode;
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

	
