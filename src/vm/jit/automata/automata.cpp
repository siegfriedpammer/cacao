#include <stdio.h>
#include <assert.h>

#include "automata.hpp"

#define NODE_IS_VALID(n) ((n) != NULL)

int get_pop_count(instruction *iptr)
{
    methoddesc *md;
    switch (iptr->opc) {
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
    methoddesc *md;
    switch (iptr->opc) {
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
    n->has_side_effects = instruction_has_side_effects(iptr);
    n->kids[0] = NULL;
    n->kids[1] = NULL;
}

static void print_node(node n)
{
    printf("%s", burm_opname[n->op]);
    if (NODE_IS_VALID(n->kids[0]) && NODE_IS_VALID(n->kids[1])) {
        printf("(");
        print_node(n->kids[0]);
        printf(", ");
        print_node(n->kids[1]); 
        printf(")");
    } else if (NODE_IS_VALID(n->kids[0])) {
        printf("(");
        print_node(n->kids[0]);
        printf(")");
    }
}

void emit(node n) {
    printf("emitting ");
    print_node(n);
    printf("\n");
    burm_label(n);
    burm_reduce(n, 1);
}

void automaton_run(basicblock *bptr, jitdata *jd)
{
    printf("-------------------\n");
    
    // Walk through all instructions.
    int32_t len = bptr->icount;
    uint16_t currentline = 0;

    node_t nodes[32];
    node_t nop;
    node stack[8] = { nullptr };
    node *tos = &stack[0];
    int start = 0;
    int end = 0;
    int count = 0;
    codegendata*  cd   = jd->cd;

    memset(&nodes, 0, 32 * sizeof(node_t));
    memset(&nop, 0, sizeof(node_t));

    for (int i = 0; i < bptr->indepth; i++) {
        node root = &nodes[end++];
        root->has_side_effects = false;
        root->iptr = NULL;
        root->jd = jd;
        root->kids[0] = NULL;
        root->kids[1] = NULL;
        root->op = 300;
        end %= 32;
        count++;
        *(tos++) = root;
    }

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
        printf("%d (%s) %d -- %d\n", iptr->opc, burm_opname[iptr->opc], pop, push);

        // Build new tree
        node root = &nodes[end++];
        fill_node(root, iptr, jd);
        end %= 32;
        count++;
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
        for (node *n = tos - 1; n >= &stack[0]; n--) {
            printf("%ld: ", n - &stack[0]);
            print_node(*n);
            printf(" %s\n", (*n)->has_side_effects ? "impure" : "pure");
        }
        // emit instructions if tree is terminated
        if (push == 0 || pop > 2) {
            // handle side-effects in stack
            for (node *n = &stack[0]; n < tos - 1; n++) {
                if ((*n)->has_side_effects) {
                    emit(*n);
                    assert(get_push_count((*n)->iptr) == 1);
                    (*n)->op = 300; // RESULT
                    (*n)->kids[0] = NULL;
                    (*n)->kids[1] = NULL;
                    (*n)->has_side_effects = false;
                } else if (pop > 2 && n >= tos - pop - 1) {
                    printf("emitting slot %d...\n", n - &stack[0]);
                    emit(*n);
                }
            }
            if (pop > 2) {
                tos -= pop;
            }
            emit(root);
            *(--tos) = NULL;
        }

#if defined(ENABLE_REPLACEMENT)
        if (instruction_has_side_effects(iptr) && JITDATA_HAS_FLAG_DEOPTIMIZE(jd)) {
            codegen_set_replacement_point(cd);
        }
#endif

    } // for all instructions
    assert(tos == &stack[0]);
    printf("-------------------\n");
}