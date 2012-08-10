#include "analyze.hpp"
#include "Interval.hpp"
#include "toolbox/logging.hpp"
#include "vm/jit/ir/icmd.hpp"

#include <sstream>

namespace
{
	void analyzeLoops(jitdata* jd);
	void analyzeBasicblockInLoop(basicblock* block, LoopContainer* loop);
	void analyzeFooter(jitdata* jd, LoopContainer* loop);
	void findLeaves(jitdata* jd);

	bool isRegularPredecessor(jitdata* jd, basicblock* node, basicblock* pred);
	LoopContainer* getLoopContainer(jitdata* jd, basicblock* header);
	IntervalMap analyze(basicblock* node, basicblock* target);

	bool isLocalIntVar(jitdata* jd, s4 varIndex);
	bool isLocalIntVar(jitdata* jd, s4 var0, s4 var1);

	/**
	 * Analyzes a single basicblock that belongs to a loop and finds
	 *
	 *   -) all written variables.
	 */
	void analyzeBasicblockInLoop(basicblock* block, LoopContainer* loop)
	{
		for (instruction* instr = block->iinstr; instr != block->iinstr + block->icount; instr++)
		{
			if (instruction_has_dst(instr))
			{
				switch (instr->opc)
				{
					case ICMD_COPY:
					case ICMD_MOVE:
					case ICMD_ILOAD:
					case ICMD_LLOAD:
					case ICMD_FLOAD:
					case ICMD_DLOAD:
					case ICMD_ALOAD:

						// The dst-variable is changed only if src != dst.
						if (instr->s1.varindex != instr->dst.varindex)
							loop->writtenVariables.insert(instr->dst.varindex);
						break;

					default:

						// Store changed variable.
						loop->writtenVariables.insert(instr->dst.varindex);
						break;
				}
			}
		}
	}

	/**
	 * Analyzes the footer of the specified loop (Only the first footer is considered) and
	 * finds all counter variables.
	 *
	 * It is important that analyzeBasicblockInLoop has been called for every basicblock
	 * in this loop except the footer.
	 */
	void analyzeFooter(jitdata* jd, LoopContainer* loop)
	{
		assert(!loop->footers.empty());
		
		basicblock* footer = loop->footers.front();
		bool jumpFound = false;

		for (s4 i = footer->icount - 1; i >= 0; i--)
		{
			instruction* instr = &footer->iinstr[i];

			if (jumpFound)
			{
				s4 var = instr->dst.varindex;
				if (instr->opc == ICMD_IINC &&   // For simplicity only IINC-instructions are considered.
					isLocalIntVar(jd, var) &&
					!loop->writtenVariables.contains(var) &&   // A counter variable is written only once in the whole loop.
					!loop->counterVariables.contains(var))   // A counter variable cannot be incremented multiple times.
				{
					loop->counterVariables.insert(var);
				}
				else
				{
					break;
				}
			}
			else
			{
				// Skip NOPs at the end of the basicblock until the jump instruction is found.
				if (icmd_table[instr->opc].controlflow != CF_NORMAL)
					jumpFound = true;
			}
		}
	}

	/**
	 * Returns true if there is no back-edge from pred to node, otherwise false.
	 */
	bool isRegularPredecessor(jitdata* jd, basicblock* node, basicblock* pred)
	{
		for (std::vector<Edge>::iterator it = jd->ld->loopBackEdges.begin(); it != jd->ld->loopBackEdges.end(); ++it)
		{
			if (it->from == pred && it->to == node)
				return false;
		}
		return true;
	}

	/**
	 * Removes all fully redundant array bound checks in node and its predecessors in the CFG.
	 * Returns the interval map for the edge from node to target.
	 */
	IntervalMap analyze(jitdata* jd, basicblock* node, basicblock* target)
	{
		if (node->ld->analyzed)
		{
			if (target == node->ld->jumpTarget)
				return node->ld->targetIntervals;
			else
				return node->ld->intervals;
		}

		// Prevent this node from being analyzed again.
		node->ld->analyzed = true;

		// Define shortcuts.
		IntervalMap& intervals = node->ld->intervals;
		IntervalMap& targetIntervals = node->ld->targetIntervals;

		// Initialize results. That is necessary because analyze can be called recursively.
		targetIntervals = IntervalMap(jd->varcount);
		intervals = IntervalMap(jd->varcount);

		// If node is a root, the variables (e.g. function arguments) can have any value.
		// Otherwise we can take the interval maps from the predecessors.
		if (node->predecessorcount > 0)
		{
			// Initialize intervals.
			if (isRegularPredecessor(jd, node, node->predecessors[0]))
				intervals = analyze(jd, node->predecessors[0], node);

			// Combine it with the other intervals.
			for (s4 i = 1; i < node->predecessorcount; i++)
			{
				if (isRegularPredecessor(jd, node, node->predecessors[i]))
					intervals.unionWith(analyze(jd, node->predecessors[i], node));
			}
		}

		// If node is the header of a loop L,
		// set every interval that belongs to a variable which is changed in L to [MIN,MAX].
		if (node->ld->loop)
		{
			for (VariableSet::iterator it = node->ld->loop->writtenVariables.begin(); it != node->ld->loop->writtenVariables.end(); ++it)
			{
				intervals[*it] = Interval();
			}
		}

		for (instruction* instr = node->iinstr; instr != node->iinstr + node->icount; instr++)
		{
			// arguments
			s4 dst_index = instr->dst.varindex;
			s4 s1_index = instr->s1.varindex;
			s4 s2_index = instr->sx.s23.s2.varindex;
			s4 value = instr->sx.val.i;

			// Update the variable intervals for this node.
			// For an unknown instruction, set the corresponding interval to [MIN,MAX].
			switch (instr->opc)
			{
				case ICMD_ICONST:
					if (isLocalIntVar(jd, dst_index))
					{
						intervals[dst_index] = Interval(Scalar(value));
					}
					break;

				case ICMD_COPY:
				case ICMD_MOVE:
				case ICMD_ILOAD:
				case ICMD_ISTORE:
					if (isLocalIntVar(jd, dst_index))
					{
						// The source variable does not have to be local because
						// the interval of a non-local variable is [MIN,MAX].

						if (s1_index != dst_index)
						{
							// Overwrite dst-interval by s1-interval.
							intervals[dst_index] = intervals[s1_index];
						}
					}
					break;

				case ICMD_ARRAYLENGTH:
					if (isLocalIntVar(jd, dst_index))
					{
						// Create interval [s1.length, s1.length]
						intervals[dst_index] = Interval(Scalar(NumericInstruction::newArrayLength(s1_index)));
					}
					break;

				case ICMD_IINC:
				case ICMD_IADDCONST:
					if (isLocalIntVar(jd, dst_index))
					{
						Scalar l = intervals[s1_index].lower();
						Scalar u = intervals[s1_index].upper();
						Scalar offset(value);

						if (l.tryAdd(offset) && u.tryAdd(offset))
						{
							intervals[dst_index] = Interval(l, u);
						}
						else   // overflow
						{
							intervals[dst_index] = Interval();
						}
					}
					break;

				case ICMD_ISUBCONST:
					if (isLocalIntVar(jd, dst_index))
					{
						Scalar l = intervals[s1_index].lower();
						Scalar u = intervals[s1_index].upper();
						Scalar offset(value);

						if (l.trySubtract(offset) && u.trySubtract(offset))
						{
							intervals[dst_index] = Interval(l, u);
						}
						else   // overflow
						{
							intervals[dst_index] = Interval();
						}
					}
					break;

				case ICMD_IALOAD:
				case ICMD_BALOAD:
				case ICMD_CALOAD:
				case ICMD_SALOAD:
				case ICMD_IASTORE:
				case ICMD_BASTORE:
				case ICMD_CASTORE:
				case ICMD_SASTORE:
				case ICMD_IASTORECONST:
				case ICMD_LASTORECONST:
				case ICMD_FASTORECONST:
				case ICMD_DASTORECONST:
				case ICMD_AASTORECONST:
				case ICMD_BASTORECONST:
				case ICMD_CASTORECONST:
				case ICMD_SASTORECONST:

					// Remove array bounds check.
					if (intervals[s2_index].isWithinBounds(s1_index))
						instr->flags.bits &= ~INS_FLAG_CHECK;
					break;

				case ICMD_IFEQ:   // if (S1 == INT_CONST) goto DST
				{
					node->ld->jumpTarget = instr->dst.block;

					Scalar s(value);
					Interval i(s);

					// TRUE BRANCH
					targetIntervals = intervals;
					targetIntervals[s1_index].intersectWith(i);

					// FALSE BRANCH
					intervals[s1_index].tryRemove(s);
				
					break;
				}
				case ICMD_IFNE:   // if (S1 != INT_CONST) goto DST
				{
					node->ld->jumpTarget = instr->dst.block;

					Scalar s(value);
					Interval i(s);

					// TRUE BRANCH
					targetIntervals = intervals;
					targetIntervals[s1_index].tryRemove(s);

					// FALSE BRANCH
					intervals[s1_index].intersectWith(i);
					
					break;
				}
				case ICMD_IFLT:   // if (S1 < INT_CONST) goto DST
				{
					node->ld->jumpTarget = instr->dst.block;

					// TRUE BRANCH
					{
						Scalar l(Scalar::Min());
						Scalar u(value);

						// interval [MIN,value-1]
						Interval i(l, u);
						i.tryRemove(u);

						targetIntervals = intervals;
						targetIntervals[s1_index].intersectWith(i);
					}

					// FALSE BRANCH
					{
						Scalar l(value);
						Scalar u(Scalar::Max());

						// interval [value,MAX].
						Interval i(l, u);

						intervals[s1_index].intersectWith(i);
					}
					
					break;
				}
				case ICMD_IFLE:   // if (S1 <= INT_CONST) goto DST
				{
					node->ld->jumpTarget = instr->dst.block;

					// TRUE BRANCH
					{
						Scalar l(Scalar::Min());
						Scalar u(value);

						// interval [MIN,value]
						Interval i(l, u);

						targetIntervals = intervals;
						targetIntervals[s1_index].intersectWith(i);
					}

					// FALSE BRANCH
					{
						Scalar l(value);
						Scalar u(Scalar::Max());

						// interval [value+1,MAX].
						Interval i(l, u);
						i.tryRemove(l);

						intervals[s1_index].intersectWith(i);
					}
					
					break;
				}
				case ICMD_IFGT:   // if (S1 > INT_CONST) goto DST
				{
					node->ld->jumpTarget = instr->dst.block;

					// TRUE BRANCH
					{
						Scalar l(value);
						Scalar u(Scalar::Max());

						// interval [value+1,MAX]
						Interval i(l, u);
						i.tryRemove(l);

						targetIntervals = intervals;
						targetIntervals[s1_index].intersectWith(i);
					}

					// FALSE BRANCH
					{
						Scalar l(Scalar::Min());
						Scalar u(value);

						// interval [MIN,value].
						Interval i(l, u);

						intervals[s1_index].intersectWith(i);
					}
					
					break;
				}
				case ICMD_IFGE:   // if (S1 >= INT_CONST) goto DST
				{
					node->ld->jumpTarget = instr->dst.block;

					// TRUE BRANCH
					{
						Scalar l(value);
						Scalar u(Scalar::Max());

						// interval [value,MAX]
						Interval i(l, u);

						targetIntervals = intervals;
						targetIntervals[s1_index].intersectWith(i);
					}

					// FALSE BRANCH
					{
						Scalar l(Scalar::Min());
						Scalar u(value);

						// interval [MIN,value-1].
						Interval i(l, u);
						i.tryRemove(u);

						intervals[s1_index].intersectWith(i);
					}
					
					break;
				}
				case ICMD_IF_ICMPEQ:   // if (S1 == S2) goto DST
				{
					node->ld->jumpTarget = instr->dst.block;

					// TRUE BRANCH

					// S1, S2 lie in the intersection of their intervals.
					targetIntervals = intervals;
					targetIntervals[s1_index].intersectWith(intervals[s2_index]);
					targetIntervals[s2_index] = targetIntervals[s1_index];

					// FALSE BRANCH

					const IntervalMap temp = intervals;

					// If the interval of S2 contains only one element A,
					// then A can be removed from the other interval.
					if (temp[s2_index].lower() == temp[s2_index].upper())
					{
						intervals[s1_index].tryRemove(temp[s2_index].lower());
					}

					// If the interval of S1 contains only one element A,
					// then A can be removed from the other interval.
					if (temp[s1_index].lower() == temp[s1_index].upper())
					{
						intervals[s2_index].tryRemove(temp[s1_index].lower());
					}

					break;
				}
				case ICMD_IF_ICMPNE:   // if (S1 != S2) goto DST
				{
					node->ld->jumpTarget = instr->dst.block;

					// TRUE BRANCH

					targetIntervals = intervals;

					// If the interval of S2 contains only one element A,
					// then A can be removed from the other interval.
					if (intervals[s2_index].lower() == intervals[s2_index].upper())
					{
						targetIntervals[s1_index].tryRemove(intervals[s2_index].lower());
					}

					// If the interval of S1 contains only one element A,
					// then A can be removed from the other interval.
					if (intervals[s1_index].lower() == intervals[s1_index].upper())
					{
						targetIntervals[s2_index].tryRemove(intervals[s1_index].lower());
					}

					// FALSE BRANCH

					// S1, S2 lie in the intersection of their intervals.
					intervals[s1_index].intersectWith(intervals[s2_index]);
					intervals[s2_index] = intervals[s1_index];

					break;
				}
				case ICMD_IF_ICMPLT:   // if (S1 < S2) goto DST
				{
					node->ld->jumpTarget = instr->dst.block;

					// TRUE BRANCH
					{
						targetIntervals = intervals;

						// Create interval containing all elements less than the biggest instance of S2.
						Interval lessThan;
						lessThan.upper(intervals[s2_index].upper());
						lessThan.tryRemove(intervals[s2_index].upper());

						// Create interval containing all elements greater than the smallest instance of S1.
						Interval greaterThan;
						greaterThan.lower(intervals[s1_index].lower());
						greaterThan.tryRemove(intervals[s1_index].lower());

						// S1 is less than the biggest instance of S2.
						targetIntervals[s1_index].intersectWith(lessThan);

						// S2 is greater than the smallest instance of S1.
						targetIntervals[s2_index].intersectWith(greaterThan);
					}

					// FALSE BRANCH
					{
						// Create interval containing all elements greater than or equal the smallest instance of S2.
						Interval greaterThan;
						greaterThan.lower(intervals[s2_index].lower());

						// Create interval containing all elements less than or equal the biggest instance of S1.
						Interval lessThan;
						lessThan.upper(intervals[s1_index].upper());

						// S1 is greater than or equal the smallest instance of S2.
						intervals[s1_index].intersectWith(greaterThan);

						// S2 is less than or equal the biggest instance of S1.
						intervals[s2_index].intersectWith(lessThan);
					}

					break;
				}
				case ICMD_IF_ICMPLE:   // if (S1 <= S2) goto DST
				{
					node->ld->jumpTarget = instr->dst.block;

					// TRUE BRANCH
					{
						targetIntervals = intervals;

						// Create interval containing all elements less than or equal the biggest instance of S2.
						Interval lessThan;
						lessThan.upper(intervals[s2_index].upper());

						// Create interval containing all elements greater than or equal the smallest instance of S1.
						Interval greaterThan;
						greaterThan.lower(intervals[s1_index].lower());

						// S1 is less than or equal the biggest instance of S2.
						targetIntervals[s1_index].intersectWith(lessThan);

						// S2 is greater than or equal the smallest instance of S1.
						targetIntervals[s2_index].intersectWith(greaterThan);
					}

					// FALSE BRANCH
					{
						// Create interval containing all elements greater than the smallest instance of S2.
						Interval greaterThan;
						greaterThan.lower(intervals[s2_index].lower());
						greaterThan.tryRemove(intervals[s2_index].lower());

						// Create interval containing all elements less than the biggest instance of S1.
						Interval lessThan;
						lessThan.upper(intervals[s1_index].upper());
						lessThan.tryRemove(intervals[s1_index].upper());

						// S1 is greater than the smallest instance of S2.
						intervals[s1_index].intersectWith(greaterThan);

						// S2 is less than the biggest instance of S1.
						intervals[s2_index].intersectWith(lessThan);
					}

					break;
				}
				case ICMD_IF_ICMPGT:   // if (S1 > S2) goto DST
				{
					node->ld->jumpTarget = instr->dst.block;

					// TRUE BRANCH
					{
						targetIntervals = intervals;

						// Create interval containing all elements greater than the smallest instance of S2.
						Interval greaterThan;
						greaterThan.lower(intervals[s2_index].lower());
						greaterThan.tryRemove(intervals[s2_index].lower());

						// Create interval containing all elements less than the biggest instance of S1.
						Interval lessThan;
						lessThan.upper(intervals[s1_index].upper());
						lessThan.tryRemove(intervals[s1_index].upper());

						// S1 is greater than the smallest instance of S2.
						targetIntervals[s1_index].intersectWith(greaterThan);

						// S2 is less than the biggest instance of S1.
						targetIntervals[s2_index].intersectWith(lessThan);
					}

					// FALSE BRANCH
					{
						// Create interval containing all elements less than or equal the biggest instance of S2.
						Interval lessThan;
						lessThan.upper(intervals[s2_index].upper());

						// Create interval containing all elements greater than or equal the smallest instance of S1.
						Interval greaterThan;
						greaterThan.lower(intervals[s1_index].lower());

						// S1 is less than or equal the biggest instance of S2.
						intervals[s1_index].intersectWith(lessThan);

						// S2 is greater than or equal the smallest instance of S1.
						intervals[s2_index].intersectWith(greaterThan);
					}

					break;
				}
				case ICMD_IF_ICMPGE:   // if (S1 >= S2) goto DST
				{
					node->ld->jumpTarget = instr->dst.block;

					// TRUE BRANCH
					{
						targetIntervals = intervals;

						// Create interval containing all elements greater than or equal the smallest instance of S2.
						Interval greaterThan;
						greaterThan.lower(intervals[s2_index].lower());

						// Create interval containing all elements less than or equal the biggest instance of S1.
						Interval lessThan;
						lessThan.upper(intervals[s1_index].upper());

						// S1 is greater than or equal the smallest instance of S2.
						targetIntervals[s1_index].intersectWith(greaterThan);

						// S2 is less than or equal the biggest instance of S1.
						targetIntervals[s2_index].intersectWith(lessThan);
					}

					// FALSE BRANCH
					{
						// Create interval containing all elements less than the biggest instance of S2.
						Interval lessThan;
						lessThan.upper(intervals[s2_index].upper());
						lessThan.tryRemove(intervals[s2_index].upper());

						// Create interval containing all elements greater than the smallest instance of S1.
						Interval greaterThan;
						greaterThan.lower(intervals[s1_index].lower());
						greaterThan.tryRemove(intervals[s1_index].lower());

						// S1 is less than the biggest instance of S2.
						intervals[s1_index].intersectWith(lessThan);

						// S2 is greater than the smallest instance of S1.
						intervals[s2_index].intersectWith(greaterThan);
					}

					break;
				}

				case ICMD_IMULCONST:
				case ICMD_IANDCONST:
				case ICMD_IORCONST:
				case ICMD_IXORCONST:
				case ICMD_ISHLCONST:
				case ICMD_ISHRCONST:
				case ICMD_IUSHRCONST:
				case ICMD_IDIVPOW2:
				case ICMD_IREMPOW2:

				case ICMD_IADD:
				case ICMD_ISUB:
				case ICMD_IMUL:
				case ICMD_IDIV:
				case ICMD_IREM:
				case ICMD_INEG:
				case ICMD_ISHL:
				case ICMD_ISHR:
				case ICMD_IUSHR:
				case ICMD_IAND:
				case ICMD_IOR:
				case ICMD_IXOR:

				case ICMD_L2I:
				case ICMD_F2I:
				case ICMD_D2I:
				case ICMD_INT2BYTE:
				case ICMD_INT2CHAR:
				case ICMD_INT2SHORT:

				case ICMD_LCMP:
				case ICMD_FCMPL:
				case ICMD_FCMPG:
				case ICMD_DCMPL:
				case ICMD_DCMPG:
				case ICMD_IF_ACMPEQ:
				case ICMD_IF_ACMPNE:
				case ICMD_GOTO:
				case ICMD_JSR:
				case ICMD_RET:
				case ICMD_TABLESWITCH:
				case ICMD_LOOKUPSWITCH:
				case ICMD_IRETURN:
				case ICMD_LRETURN:
				case ICMD_FRETURN:
				case ICMD_DRETURN:
				case ICMD_ARETURN:
				case ICMD_RETURN:

				case ICMD_GETSTATIC:
				case ICMD_PUTSTATIC:
				case ICMD_GETFIELD:
				case ICMD_PUTFIELD:
				case ICMD_INVOKEVIRTUAL:
				case ICMD_INVOKESPECIAL:
				case ICMD_INVOKESTATIC:
				case ICMD_INVOKEINTERFACE:
				case ICMD_NEW:
				case ICMD_NEWARRAY:
				case ICMD_ANEWARRAY:
				case ICMD_ATHROW:
				case ICMD_CHECKCAST:
				case ICMD_INSTANCEOF:
				case ICMD_MONITORENTER:
				case ICMD_MONITOREXIT:
				case ICMD_MULTIANEWARRAY:
				case ICMD_IFNULL:
				case ICMD_IFNONNULL:
				case ICMD_BREAKPOINT:
				case ICMD_PUTSTATICCONST:
				case ICMD_PUTFIELDCONST:
				case ICMD_IMULPOW2:
				case ICMD_LMULPOW2:
				case ICMD_GETEXCEPTION:
				case ICMD_PHI:
				case ICMD_INLINE_START:
				case ICMD_INLINE_END:
				case ICMD_INLINE_BODY:
				case ICMD_BUILTIN:

				default:
					if (instruction_has_dst(instr))
					{
						if (isLocalIntVar(jd, dst_index))   // Integer produced by an unknown instruction.
						{
							intervals[dst_index] = Interval();   // [MIN,MAX]
						}
						else
						{
							// If dst_index is an array, reset all intervals referencing that array.
							// TODO: [MIN,MAX] is too conservative!
							for (size_t i = 0; i < intervals.size(); i++)
							{
								if (intervals[i].lower().instruction().kind() == NumericInstruction::ARRAY_LENGTH &&
									intervals[i].lower().instruction().variable() == dst_index)
								{
									intervals[i] = Interval();
								}
								else if (intervals[i].upper().instruction().kind() == NumericInstruction::ARRAY_LENGTH &&
									intervals[i].upper().instruction().variable() == dst_index)
								{
									intervals[i] = Interval();
								}
							}
						}
					}
			}
		}

		/*
		std::stringstream str;
		str << "# " << node->nr << " #  [F] " << intervals << "| [T] " << targetIntervals;
		log_text(str.str().c_str());
		*/

		if (target == node->ld->jumpTarget)
			return targetIntervals;

		return intervals;
	}

	/**
	 * Checks whether the specified variable is local and of type integer.
	 */
	bool isLocalIntVar(jitdata* jd, s4 varIndex)
	{
		varinfo *info = VAR(varIndex);
		return IS_INT_TYPE(info->type) && (var_is_local(jd, varIndex) || var_is_temp(jd, varIndex));
	}
	
	/**
	 * Checks whether the specified variables are local and of type integer.
	 */
	bool isLocalIntVar(jitdata* jd, s4 var0, s4 var1)
	{
		varinfo *info0 = VAR(var0);
		varinfo *info1 = VAR(var1);
		return IS_INT_TYPE(info0->type) && (var_is_local(jd, var0) || var_is_temp(jd, var0))
			&& IS_INT_TYPE(info1->type) && (var_is_local(jd, var1) || var_is_temp(jd, var1));
	}
}

/**
 * Analyzes all loops and for every loop it finds
 *
 *   -) all written variables,
 *   -) all counter variables.
 */
void analyzeLoops(jitdata* jd)
{
	for (std::vector<LoopContainer*>::iterator it = jd->ld->loops.begin(); it != jd->ld->loops.end(); ++it)
	{
		LoopContainer* loop = *it;

		assert(!loop->footers.empty());

		// For simplicity we consider only one back edge per loop
		basicblock* footer = loop->footers.front();

		// Analyze all blocks contained in this loop.
		analyzeBasicblockInLoop(loop->header, loop);
		for (std::vector<basicblock*>::iterator blockIt = loop->nodes.begin(); blockIt != loop->nodes.end(); ++blockIt)
		{
			if (*blockIt != footer)
				analyzeBasicblockInLoop(*blockIt, loop);
		}
		analyzeFooter(jd, loop);					// Find counter variables.
		analyzeBasicblockInLoop(footer, loop);	// The footer must be the last node to be analyzed.
	}
}

/**
 * Marks all basicblocks which are leaves.
 */
void findLeaves(jitdata* jd)
{
	for (std::vector<Edge>::const_iterator it = jd->ld->loopBackEdges.begin(); it != jd->ld->loopBackEdges.end(); ++it)
	{
		it->from->ld->outgoingBackEdgeCount++;
	}

	for (basicblock* b = jd->basicblocks; b != 0; b = b->next)
	{
		assert(b->successorcount >= b->ld->outgoingBackEdgeCount);
		b->ld->leaf = (b->successorcount == b->ld->outgoingBackEdgeCount);
	}
}

void removeFullyRedundantChecks(jitdata* jd)
{
	for (basicblock* block = jd->basicblocks; block != 0; block = block->next)
	{
		if (block->ld->leaf)
			analyze(jd, block, 0);
	}
}

