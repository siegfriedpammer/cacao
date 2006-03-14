#include "vm/jit/code.h"
#include "mm/memory.h"


codeinfo *code_codeinfo_new(methodinfo *m)
{
	codeinfo *code;

	code = NEW(codeinfo);

	memset(code,0,sizeof(codeinfo));

	code->m = m;
	
	return code;
}

void code_codeinfo_free(codeinfo *code)
{
	if (!code)
		return;

	if (code->mcode)
		CFREE((void *) (ptrint) code->mcode, code->mcodelength);

	FREE(code,codeinfo);
}

void code_free_code_of_method(methodinfo *m)
{
	codeinfo *nextcode;
	codeinfo *code;
	
	nextcode = m->code;
	while (nextcode) {
		code = nextcode;
		nextcode = code->prev;
		code_codeinfo_free(code);
	}
}

/* vim: noet ts=4 sw=4
 */
