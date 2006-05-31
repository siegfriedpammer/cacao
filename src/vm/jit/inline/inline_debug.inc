#include <ctype.h>

#define DEBUG_SLOT(slot)  ((int)((slot) ? ((slot) - iln->ctx->n_debug_stackbase) : (-1)))

#if 0
		printf("linenumbertable_entry %p: pc=%p line=%08x, lntsize=%d, looking for pc=%p\n",
				(void*)lntentry,(void*)lntentry->pc,lntentry->line,lntsize,(void*)pc);
#endif

#if 0
						printf("inline entry %p: pc=%p line=%08x, lntsize=%d, looking for pc=%p\n",
								(void*)lntinline,(void*)lntinline->pc,lntinline->line,lntsize,(void*)pc);
						printf("\trecurse %p into %p ",(void*)lntinline,(void*)lntinline->pc); method_println((methodinfo *)lntinline->pc);
#endif

#if 0
	if (oa) {
	    int i;

	    for (i=0; i<oa->header.size; ++i) {
		printf("\t%i: %p ",i,(void*)oa->data[i]);
		if (oa->data[i])
		    class_println((classinfo *)oa->data[i]);
		else
		    printf("\n");
	    }
	}
	printf("GOT CLASS: %p\n",c);
	if (c) {
	    class_println(c);
	}
	printf("GOT CLASSLOADER: %p\n",cl);
	if (cl) {
	    class_println(cl->vftbl->class);
	}
#endif

static void debug_dump_inline_context(inline_node *iln)
{
	inline_stack_translation *tr;
	
	printf("inline_context @%p: stackbase=%p transstart=%p translationlimit=%p\n",
			(void*)iln->ctx,(void*)iln->ctx->n_debug_stackbase,
			(void*)iln->ctx->stacktranslationstart,(void*)iln->ctx->o_translationlimit);
	tr = iln->ctx->stacktranslationstart;
	while (tr >= iln->ctx->stacktranslation) {
		printf("\ttranslate %p -> %d (%p)\n",
				(void*)tr->o_sp,DEBUG_SLOT(tr->n_sp),(void*)tr->n_sp);
		tr--;
	}
}

static void debug_dump_stack(stackptr sp)
{
	while (sp) {
		printf("%p (%d) (%01d:%02d:%01x) -> ",(void*)sp,sp->type,sp->varkind,sp->varnum,sp->flags);
		sp = sp->prev;
	}
	printf("%p",(void*)NULL);
}

static void dump_inline_tree(inline_node *iln)
{
	char indent[100];
	int i;
	inline_node *child;

	if (!iln) {
		printf("(inline_node *)null\n");
		return;
	}

	for (i=0; i<iln->depth; ++i)
		indent[i] = '\t';
	indent[i] = 0;

	assert(iln->m);
	if (iln->depth) {
		if (!iln->parent) {
			printf("parent unset");
		}
		else {
			printf("%s[%d] L%03d %d-%d (ins %d,st %d) (sd=%d,cs=%p,lofs=%d) cum(ins %d,st %d,bb %d) ",
					indent,iln->depth,iln->callerblock->debug_nr,
					iln->callerpc,(int)(iln->callerblock->iinstr - iln->parent->m->basicblocks->iinstr),
					iln->instructioncount,iln->stackcount,iln->n_callerstackdepth,
					(void*)iln->n_callerstack,
					iln->localsoffset,
					iln->cumul_instructioncount,iln->cumul_stackcount,iln->cumul_basicblockcount);
		}
	}
	else {
		printf("%s[%d] MAIN (ins %d,st %d) (cs=%p) cum(ins %d,st %d,bb %d) ",indent,iln->depth,
				iln->instructioncount,iln->stackcount,
				(void*)iln->n_callerstack,
				iln->cumul_instructioncount,iln->cumul_stackcount,iln->cumul_basicblockcount);
	}
	method_println(iln->m);

	child = iln->children;
	if (child) {
		do {
			dump_inline_tree(child);
		}
		while ((child = child->next) != iln->children);
	}
}



static stackptr first_stackslot_of_block(basicblock *block)
{
	int len;
	instruction *iptr;
	stackptr sp;
	
	assert(block);
	if (block->instack)
		return block->instack - (block->indepth-1);
	
	len = block->icount;
	iptr = block->iinstr;
	while (len--) {
		if (iptr->dst) {
			sp = iptr->dst;
			while (sp->prev) {
				sp = sp->prev;
			}
			return sp;
		}
		iptr++;
	}

	return NULL;
}

static void debug_print_stack(inline_node *iln,stackptr sp,int validstackmin,int validstackmax)
{
	int i;
	int idx;
	char typechar;
	char kindchar;
	
	printf("[");
	for (i=0; i<iln->cumul_maxstack; ++i) {
		if (sp) {
			idx = sp - iln->n_inlined_stack;
			switch (sp->type) {
				case TYPE_ADR: typechar = 'a'; break;
				case TYPE_INT: typechar = 'i'; break;
				case TYPE_LNG: typechar = 'l'; break;
				case TYPE_FLT: typechar = 'f'; break;
				case TYPE_DBL: typechar = 'd'; break;
				default:       typechar = '?';
			}
            switch (sp->varkind) {
                case STACKVAR: kindchar = 's'; break;
                case LOCALVAR: kindchar = 'l'; break;
                case TEMPVAR : kindchar = 't'; break;
                case UNDEFVAR: kindchar = 'u'; break;
                case ARGVAR  : kindchar = 'a'; break;
                default:       kindchar = '_'; break;
            }
            if (sp->flags & SAVEDVAR)
                kindchar = toupper(kindchar);
			printf("%c%-3d(%c%-2d:%01x) ",typechar,idx,kindchar,sp->varnum,sp->flags);
			sp = sp->prev;

			assert(idx >= validstackmin);
			assert(idx <= validstackmax);
#if 0
			if (idx < validstackmin || idx > validstackmax) {
				printf("INVALID STACK INDEX: %d\n",idx);
			}
#endif
		}
		else {
			printf("----        ");
		}
	}
	printf("] ");
}

static void debug_dump_inlined_code(inline_node *iln,methodinfo *newmethod,codegendata *cd,registerdata *rd)
{
	basicblock *bptr;
	instruction *iptr;
	stackptr curstack;
	stackptr dst;
	basicblock *nextblock;
	int len;
	int validstackmin;
	int validstackmax;
	int i;
	int type;

	printf("INLINED CODE: maxstack=%d maxlocals=%d leafmethod=%d\n",
			newmethod->maxstack,newmethod->maxlocals,newmethod->isleafmethod);

	for (i=0; i<newmethod->maxlocals; ++i) {
	    for (type=0; type<5; ++type) {
	    	if (rd->locals[i][type].type < 0)
		    continue;
		printf("\tlocal %d type %d: flags %01x\n",i,type,rd->locals[i][type].flags);
	    }
	}

	for (i=0; i<newmethod->maxstack; ++i) {
	    for (type=0; type<5; ++type) {
	    	if (rd->interfaces[i][type].type < 0)
		    continue;
		printf("\tinterface %d type %d: flags %01x\n",i,type,rd->interfaces[i][type].flags);
	    }
	}

	printf("registerdata:\n");
	printf("\tmemuse = %d\n",rd->memuse);
	printf("\targintreguse = %d\n",rd->argintreguse);
	printf("\targfltreguse = %d\n",rd->argfltreguse);

	validstackmin = 0;

	bptr = iln->inlined_basicblocks;
	for (; bptr; bptr = bptr->next) {
		curstack = bptr->instack;
		iptr = bptr->iinstr;
		len = bptr->icount;

		nextblock = bptr->next;
find_stackmax:
		if (nextblock) {
			dst = first_stackslot_of_block(nextblock);
			if (dst) {
				validstackmax = (dst - iln->n_inlined_stack) - 1;
			}
			else {
				nextblock = nextblock->next;
				goto find_stackmax;
			}
		}
		else {
			validstackmax = 10000; /* XXX */
		}

		debug_print_stack(iln,curstack,validstackmin,validstackmax);
		printf("L%03d BLOCK %p indepth=%d outdepth=%d icount=%d stack=[%d,%d] type=%d flags=%d\n",
				bptr->debug_nr,(void*)bptr,bptr->indepth,bptr->outdepth,bptr->icount,
				validstackmin,validstackmax,
                bptr->type,bptr->flags);

		while (len--) {
			dst = iptr->dst;

			debug_print_stack(iln,dst,validstackmin,validstackmax);
			printf("     ");
			show_icmd(iptr,false); printf("\n");

			curstack = dst;
			iptr++;
		}
		printf("\n");

		/* next basic block */
		validstackmin = validstackmax + 1;
	}
}
