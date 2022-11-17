#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <mutex>          // std::mutex, std::lock

#include "automata.hpp"

#define NODE_IS_VALID(n) ((n) != NULL)

FILE *output = NULL;
std::mutex l;

void log(const char *format, ...)
{
    va_list args;
    char buffer[8096];

    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    if (output == NULL) {
        output = fopen("/workspaces/masterarbeit/output.log", "a");
    }

    fprintf(output, "%s", buffer);
    fflush(output);
}

int get_pop_count(instruction *iptr)
{
    assert(iptr != NULL);
    methoddesc *md;
    switch (iptr->opc) {
        case ICMD_POP:
            return 1;
        case ICMD_POP2:
            return 2;
        case ICMD_BUILTIN:
            md = iptr->sx.s23.s3.bte->md;
            return md->paramcount;
        case ICMD_INVOKEINTERFACE:
        case ICMD_INVOKESPECIAL:
        case ICMD_INVOKESTATIC:
        case ICMD_INVOKEVIRTUAL:
			INSTRUCTION_GET_METHODDESC(iptr, md);
            return md->paramcount;
        case ICMD_COPY:
        case ICMD_MOVE:
            return 0;
        default:
            return pop_count_table[iptr->opc];
    }
}

int get_push_count(instruction *iptr)
{
    assert(iptr != NULL);
    methoddesc *md;
    switch ((int)iptr->opc) {
        case 300: // RESULT
            return 1;
        case ICMD_POP:
        case ICMD_POP2:
            return 0;
        case ICMD_BUILTIN:
            md = iptr->sx.s23.s3.bte->md;
            return md->returntype.type != TYPE_VOID ? 1 : 0;
        case ICMD_INVOKEINTERFACE:
        case ICMD_INVOKESPECIAL:
        case ICMD_INVOKESTATIC:
        case ICMD_INVOKEVIRTUAL:
			INSTRUCTION_GET_METHODDESC(iptr, md);
            return md->returntype.type != TYPE_VOID ? 1 : 0;
        case ICMD_COPY:
        case ICMD_MOVE:
            return 1;
        default:
            return push_count_table[iptr->opc];
    }
}

static void fill_node(node n, instruction *iptr, jitdata *jd)
{
    n->iptr = iptr;
    n->jd = jd;
    n->op = iptr->opc;
    switch (iptr->opc) {
        case ICMD_ACONST:
        case ICMD_ICONST:
        case ICMD_LCONST:
        case ICMD_FCONST:
        case ICMD_DCONST:
            n->has_side_effects = false;
            break;
        default:
            n->has_side_effects = true;
            break; 
    }
    n->kids[0] = NULL;
    n->kids[1] = NULL;
}

static void print_node(node n)
{
    assert(n != NULL);
    int arity = burm_arity[n->op];
    int push = n->iptr != NULL && n->op != 300 ? get_push_count(n->iptr) : 1;
    int pop = n->iptr != NULL && n->op != 300 ? get_pop_count(n->iptr) : 0;
    log("%d: %s[%d--%d]", n->offset, burm_opname[n->op], pop, push);
    if (arity > 0) { log("("); }
    switch (burm_arity[n->op]) {
        case 2:
            print_node(n->kids[0]);
            log(", ");
            print_node(n->kids[1]);
            break;
        case 1:
            print_node(n->kids[0]);
            break;
    }
    if (arity > 0) { log(")"); }
}

void emit(node n) {
    //log("emitting ");
    //print_node(n);
    //log("...\n");
    //log("start symbol: %s %d\n", burm_opname[n->op], n->op);
    burm_label(n);
    burm_reduce(n, 1);
}

bool ignore(ICMD opc) {
    switch (opc) {
        case ICMD_BUILTIN:
        case ICMD_INVOKEINTERFACE:
        case ICMD_INVOKESPECIAL:
        case ICMD_INVOKESTATIC:
        case ICMD_INVOKEVIRTUAL:
        case ICMD_MULTIANEWARRAY:
        case ICMD_POP2:
            return true;
        default:
            return false;
    }
}

void automaton_run(basicblock *bptr, jitdata *jd)
{
    l.lock();
    log("-------------------\n");
    log("%s.%s%s\n", Utf8String(jd->m->clazz->name).begin(), Utf8String(jd->m->name).begin(), Utf8String(jd->m->descriptor).begin());
    log("instruction count: %d\n", bptr->icount);
    log("maxstack: %d\n", jd->stackcount);
    log("stack depth at start: %d\n", bptr->indepth);
    // Walk through all instructions.
    uint16_t currentline = 0;

    auto nodes = std::make_unique<node_t[]>(bptr->icount + bptr->indepth);
    auto stack = std::make_unique<node[]>(jd->stackcount);
    node_t nop;
    node *tos = &stack[0];
    int count = 0;
    codegendata* cd = jd->cd;

    memset(&nop, 0, sizeof(node_t));

    for (int i = 0; i < bptr->indepth; i++) {
        node root = &nodes[count++];
        root->has_side_effects = true;
        root->iptr = NULL;
        root->jd = jd;
        root->kids[0] = NULL;
        root->kids[1] = NULL;
        root->op = 300;
        *(tos++) = root;
    }

    int32_t len = bptr->icount;

    for (instruction* iptr = bptr->iinstr; len > 0; len--, iptr++) {
        // Add line number.
        if (iptr->line != currentline) {
            linenumbertable_list_entry_add(cd, iptr->line);
            currentline = iptr->line;
        }

        // An instruction usually needs < 64 words.
        // XXX Check if this is true for all archs.
        MCODECHECK(64);    // All
        MCODECHECK(128);   // PPC64
        MCODECHECK(1024);  // I386, X86_64, S390      /* 1kB should be enough */

        int pop = get_pop_count(iptr);
        int push = get_push_count(iptr);

        if (iptr->opc == ICMD_NOP)
            continue;

        // Build new tree
        node root = &nodes[count++];
        fill_node(root, iptr, jd);
        root->offset = iptr - bptr->iinstr;
        if (!ignore(iptr->opc) && pop != burm_arity[iptr->opc] && pop <= 2) {
            log("%s has incompatible pop count; expected %d; got arity %d\n", burm_opname[iptr->opc], pop, burm_arity[iptr->opc]);
            assert(false);
        }
        root->kids[0] = &nop;
        root->kids[1] = &nop;
        //emit(root);
        //continue;
        switch (pop) {
            case 0:
                break;
            case 1:
                root->kids[0] = *(--tos);
                root->kids[1] = &nop;
                root->has_side_effects = (root->has_side_effects || root->kids[0]->has_side_effects);
                break;
            case 2:
                root->kids[0] = *(tos - 2);
                root->kids[1] = *(tos - 1);
                tos -= 2;
                root->has_side_effects = (root->has_side_effects || root->kids[0]->has_side_effects || root->kids[1]->has_side_effects);
                break;
            default:
                // terminate tree
                root->kids[0] = &nop;
                root->kids[1] = &nop;
                break;
        }
        *(tos++) = root;
        // emit instructions if tree is terminated
        if (push == 0 || pop > 2) {
            // handle side-effects in stack
            assert((tos - &stack[0]) > 0);
            for (node *n = &stack[0]; n < tos - 1; n++) {
                assert(*n != NULL);
                if ((*n)->op == 300) { continue; }
                emit(*n);
                int push_n = get_push_count((*n)->iptr);
                if (push_n == 0) { continue; }
                assert(push_n == 1);
                (*n)->op = 300; // RESULT
                (*n)->kids[0] = NULL;
                (*n)->kids[1] = NULL;
                (*n)->has_side_effects = false;
            }
            if (pop > 2) {
                tos -= pop;
            }
            assert(root != NULL);
            emit(root);
            if (push == 0) {
                tos--;
            } else {
                assert(push == 1);
                root->op = 300; // RESULT
                root->kids[0] = NULL;
                root->kids[1] = NULL;
                root->has_side_effects = false;
            }
        }

#if defined(ENABLE_REPLACEMENT)
        if (instruction_has_side_effects(iptr) && JITDATA_HAS_FLAG_DEOPTIMIZE(jd)) {
            codegen_set_replacement_point(cd);
        }
#endif

    } // for all instructions

    // emit instructions remaining on stack
    for (node *n = &stack[0]; n < tos; n++) {
        assert(*n != NULL);
        if ((*n)->op == 300) { continue; }
        emit(*n);
    }
    log("-------------------\n");
    l.unlock();
}