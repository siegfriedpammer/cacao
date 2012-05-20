#include "analyze.hpp"

namespace
{
	void postOrderTraversal(jitdata* jd);
	void findAssignments(jitdata* jd, basicblock* block);
	void reverseDepthFirstTraversal(jitdata* jd, basicblock* next, basicblock* start);

	/**
	 * Performs a post-order-traversal of the dominator tree
	 * and calls findAssignments for every node except the root.
	 */
	void postOrderTraversal(jitdata* jd)
	{
		assert(!jd->ld->root->ld->children.empty());

		// The first block to be processed is the first leaf.
		basicblock* block = jd->ld->root;
		while (!block->ld->children.empty())
			block = block->ld->children.front();

		// Post-Order-Traversal.
		// The root is skipped.
		while (block != jd->ld->root)
		{
			// find variable assignments between block and idom(block)
			findAssignments(jd, block);

			if (block->ld->nextSibling)
			{
				// go to next sibling
				block = block->ld->nextSibling;
				while (!block->ld->children.empty())
					block = block->ld->children.front();
			}
			else
			{
				block = block->ld->dom;
			}
		}
	}

	/**
	 * Finds all variable assignments between block and idom(block) and stores
	 * the variable names in block.
	 */
	void findAssignments(jitdata* jd, basicblock* block)
	{
		log_println("findAssignments -- Block: %d", block->nr);

		// This node must not be visited in reverseDepthFirstTraversal.
		block->ld->analyzeVisited = block;

		for (s4 i = 0; i < block->predecessorcount; i++)
		{
			reverseDepthFirstTraversal(jd, block->predecessors[i], block);
		}
	}

	/**
	 * Performs a depth first traversal from start to idom(start)
	 * and stores all variable assignments occuring between those two nodes in start.
	 */
	void reverseDepthFirstTraversal(jitdata* jd, basicblock* cur, basicblock* start)
	{
		log_println("reverseDepthFirstTraversal -- start: %d, cur: %d (visited: %s, dom: %d)", start->nr, cur->nr, (cur->ld->analyzeVisited == start) ? "true" : "false", start->ld->dom->nr);

		if (cur->ld->analyzeVisited == start)	// cur already visited.
			return;

		if (cur == start->ld->dom)   // Stop the traversal at idom(start).
			return;
	
		// Find all variable assignments in this block
		for (instruction* instr = cur->iinstr; instr != cur->iinstr + cur->icount; instr++)
		{
			switch (instr->opc)
			{
				case ICMD_ACONST:
				case ICMD_ICONST:
				case ICMD_IDIVPOW2:
				case ICMD_LDIVPOW2:
				case ICMD_LCONST:
				case ICMD_LCMPCONST:
				case ICMD_FCONST:
				case ICMD_DCONST:
				case ICMD_IADDCONST:
				case ICMD_ISUBCONST:
				case ICMD_IMULCONST:
				case ICMD_IANDCONST:
				case ICMD_IORCONST:
				case ICMD_IXORCONST:
				case ICMD_ISHLCONST:
				case ICMD_ISHRCONST:
				case ICMD_IUSHRCONST:
				case ICMD_IREMPOW2:
				case ICMD_LADDCONST:
				case ICMD_LSUBCONST:
				case ICMD_LMULCONST:
				case ICMD_LANDCONST:
				case ICMD_LORCONST:
				case ICMD_LXORCONST:
				case ICMD_LSHLCONST:
				case ICMD_LSHRCONST:
				case ICMD_LUSHRCONST:
				case ICMD_LREMPOW2:
				case ICMD_IALOAD:
				case ICMD_LALOAD:
				case ICMD_FALOAD:
				case ICMD_DALOAD:
				case ICMD_AALOAD:
				case ICMD_BALOAD:
				case ICMD_CALOAD:
				case ICMD_SALOAD:
				case ICMD_ISTORE:
				case ICMD_LSTORE:
				case ICMD_FSTORE:
				case ICMD_DSTORE:
				case ICMD_ASTORE:
				case ICMD_IADD:
				case ICMD_LADD:
				case ICMD_FADD:
				case ICMD_DADD:
				case ICMD_ISUB:
				case ICMD_LSUB:
				case ICMD_FSUB:
				case ICMD_DSUB:
				case ICMD_IMUL:
				case ICMD_LMUL:
				case ICMD_FMUL:
				case ICMD_DMUL:
				case ICMD_IDIV:
				case ICMD_LDIV:
				case ICMD_FDIV:
				case ICMD_DDIV:
				case ICMD_IREM:
				case ICMD_LREM:
				case ICMD_FREM:
				case ICMD_DREM:
				case ICMD_INEG:
				case ICMD_LNEG:
				case ICMD_FNEG:
				case ICMD_DNEG:
				case ICMD_ISHL:
				case ICMD_LSHL:
				case ICMD_ISHR:
				case ICMD_LSHR:
				case ICMD_IUSHR:
				case ICMD_LUSHR:
				case ICMD_IAND:
				case ICMD_LAND:
				case ICMD_IOR:
				case ICMD_LOR:
				case ICMD_IXOR:
				case ICMD_LXOR:
				case ICMD_IINC:
				case ICMD_I2L:
				case ICMD_I2F:
				case ICMD_I2D:
				case ICMD_L2I:
				case ICMD_L2F:
				case ICMD_L2D:
				case ICMD_F2I:
				case ICMD_F2L:
				case ICMD_F2D:
				case ICMD_D2I:
				case ICMD_D2L:
				case ICMD_D2F:
				case ICMD_INT2BYTE:
				case ICMD_INT2CHAR:
				case ICMD_INT2SHORT:
				case ICMD_LCMP:
				case ICMD_FCMPL:
				case ICMD_FCMPG:
				case ICMD_DCMPL:
				case ICMD_DCMPG:
				case ICMD_JSR:
				case ICMD_GETSTATIC:
				case ICMD_GETFIELD:
				case ICMD_NEW:
				case ICMD_NEWARRAY:
				case ICMD_ANEWARRAY:
				case ICMD_ARRAYLENGTH:
				case ICMD_CHECKCAST:
				case ICMD_INSTANCEOF:
				case ICMD_MULTIANEWARRAY:
				case ICMD_IMULPOW2:
				case ICMD_LMULPOW2:
				case ICMD_GETEXCEPTION:
					
					// Variable assignment found.
					start->ld->changedVariables.insert(instr->dst.varindex);
					break;

				case ICMD_COPY:
				case ICMD_MOVE:
				case ICMD_ILOAD:
				case ICMD_LLOAD:
				case ICMD_FLOAD:
				case ICMD_DLOAD:
				case ICMD_ALOAD:
					
					// The dst-variable is changed only if src != dst.
					if (instr->s1.varindex != instr->dst.varindex)
						start->ld->changedVariables.insert(instr->dst.varindex);
					break;

				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKEINTERFACE:
				{
					constant_FMIref *fmiref;
					INSTRUCTION_GET_METHODREF(instr, fmiref);

					// Return value of function must not be void.
					if (fmiref->parseddesc.md->returntype.type != TYPE_VOID)
						start->ld->changedVariables.insert(instr->dst.varindex);
					break;
				}
				case ICMD_BUILTIN:

					// Return value of function must not be void.
					if (instr->sx.s23.s3.bte->md->returntype.type != TYPE_VOID)
						start->ld->changedVariables.insert(instr->dst.varindex);
					break;
			}
		}

		cur->ld->analyzeVisited = start;

		for (s4 i = 0; i < cur->predecessorcount; i++)
		{
			reverseDepthFirstTraversal(jd, cur->predecessors[i], start);
		}
	}
}

void findVariableAssignments(jitdata* jd)
{
	postOrderTraversal(jd);
}

