#include "global.h"

void synchronize_caches() { }


void asm_call_jit_compiler () { }

java_objectheader *asm_calljavamethod (methodinfo *m, void *arg1, void*arg2,
                                       void*arg3, void*arg4) { }

java_objectheader *asm_calljavafunction (methodinfo *m, void *arg1, void*arg2,
                                         void*arg3, void*arg4) { }

methodinfo *asm_getcallingmethod () { }

void asm_dumpregistersandcall ( functionptr f) { }

void asm_builtin_aastore() { }
void asm_builtin_checkarraycast() { }
void asm_builtin_checkcast() { }
void asm_builtin_idiv() { }
void asm_builtin_irem() { }
void asm_builtin_ldiv() { }
void asm_builtin_lrem() { }
void asm_builtin_monitorenter() { }
void asm_builtin_monitorexit() { }

void asm_handle_exception() { }
int has_no_x_instr_set() { return 0; }
