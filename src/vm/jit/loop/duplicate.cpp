#include "toolbox/logging.hpp"
#include "vm/jit/ir/icmd.hpp"

#include "duplicate.hpp"

#include <cstring>

namespace
{
	basicblock* duplicateBasicblock(const basicblock* block);
	basicblock* createBasicblock(jitdata* jd, s4 numInstructions);
	void deleteDuplicatedBasicblock(basicblock* block);
	void duplicateLoop(LoopContainer* loop);
	void deleteDuplicatedLoop(jitdata* jd);
	bool findLoop(jitdata* jd, basicblock** beforeLoopPtr, basicblock** lastBlockInLoopPtr);
	void buildBasicblockList(jitdata* jd, LoopContainer* loop, basicblock* beforeLoop, basicblock* lastBlockInLoop, basicblock* loopSwitch1, basicblock* loopTrampoline);
	void buildBasicblockList(jitdata* jd, LoopContainer* loop, basicblock* beforeLoop, basicblock* lastBlockInLoop, basicblock* loopSwitch1, basicblock* loopSwitch2, basicblock* loopTrampoline);
	void removeChecks(LoopContainer* loop, s4 array, s4 index);
	basicblock* createLoopTrampoline(basicblock* target);
	void redirectJumps(jitdata* jd, basicblock* loopSwitch);
	void optimizeLoop(jitdata* jd, LoopContainer* loop, s4 stackSlot);


	//                                                    //
	// Functions that generate intermediate instructions. //
	//                                                    //

	inline void opcode_ALOAD(instruction* instr, s4 src, s4 dst)
	{
		instr->opc = ICMD_ALOAD;
		instr->s1.varindex = src;
		instr->dst.varindex = dst;
	}

	inline void opcode_ILOAD(instruction* instr, s4 src, s4 dst)
	{
		instr->opc = ICMD_ILOAD;
		instr->s1.varindex = src;
		instr->dst.varindex = dst;
	}

	inline void opcode_ARRAYLENGTH(instruction* instr, s4 src, s4 dst)
	{
		instr->opc = ICMD_ARRAYLENGTH;
		instr->s1.varindex = src;
		instr->dst.varindex = dst;
	}

	inline void opcode_IADDCONST(instruction* instr, s4 src, s4 constant, s4 dst)
	{
		instr->opc = ICMD_IADDCONST;
		instr->s1.varindex = src;
		instr->sx.val.i = constant;
		instr->dst.varindex = dst;
	}
/*
	inline void opcode_ISUBCONST(instruction* instr, s4 src, s4 constant, s4 dst)
	{
		instr->opc = ICMD_ISUBCONST;
		instr->s1.varindex = src;
		instr->sx.val.i = constant;
		instr->dst.varindex = dst;
	}
*/

	inline void opcode_GOTO(instruction* instr, basicblock* dst)
	{
		instr->opc = ICMD_GOTO;
		instr->dst.block = dst;
	}

	inline void opcode_IFGT(instruction* instr, s4 src, s4 constant, basicblock* dst)
	{
		instr->opc = ICMD_IFGT;
		instr->s1.varindex = src;
		instr->sx.val.i = constant;
		instr->dst.block = dst;
	}

	inline void opcode_IFLE(instruction* instr, s4 src, s4 constant, basicblock* dst)
	{
		instr->opc = ICMD_IFLE;
		instr->s1.varindex = src;
		instr->sx.val.i = constant;
		instr->dst.block = dst;
	}

	inline void opcode_IF_ICMPGE(instruction* instr, s4 src1, s4 src2, basicblock* dst)
	{
		instr->opc = ICMD_IF_ICMPGE;
		instr->s1.varindex = src1;
		instr->sx.s23.s2.varindex = src2;
		instr->dst.block = dst;
	}

	////////////////////////////////////////////////////////


	basicblock* duplicateBasicblock(const basicblock* block)
	{
		// flat clone
		basicblock* clone = new basicblock(*block);
		clone->ld = new BasicblockLoopData;

		// clone instructions
		clone->iinstr = new instruction[block->icount];
		memcpy(clone->iinstr, block->iinstr, sizeof(instruction) * block->icount);

		// clone table- and lookup-switches
		for (s4 i = 0; i < block->icount; i++)
		{
			instruction* instr = &block->iinstr[i];
			switch (instr->opc)
			{
				case ICMD_TABLESWITCH:
				{
					// count = (tablehigh - tablelow + 1) + 1 [default branch]
					s4 count = instr->sx.s23.s3.tablehigh - instr->sx.s23.s2.tablelow + 2;

					// clone switch table
					clone->iinstr[i].dst.table = new branch_target_t[count];
					memcpy(clone->iinstr[i].dst.table, instr->dst.table, sizeof(branch_target_t) * count);
					break;
				}
				case ICMD_LOOKUPSWITCH:
				{
					s4 count = instr->sx.s23.s2.lookupcount;

					// clone lookup table
					clone->iinstr[i].dst.lookup = new lookup_target_t[count];
					memcpy(clone->iinstr[i].dst.lookup, instr->dst.lookup, sizeof(lookup_target_t) * count);
					break;
				}
			}
		}

		return clone;
	}

	void deleteDuplicatedBasicblock(basicblock* block)
	{
		if (block != 0)
		{
			// delete switch- and lookup-tables
			for (instruction* instr = block->iinstr; instr != block->iinstr + block->icount; instr++)
			{
				switch (instr->opc)
				{
					case ICMD_TABLESWITCH:
						delete[] instr->dst.table;
						break;
					case ICMD_LOOKUPSWITCH:
						delete[] instr->dst.lookup;
						break;
				}
			}

			delete[] block->iinstr;
			delete block->ld;
			delete block;
		}
	}

	basicblock* createBasicblock(jitdata* jd, s4 numInstructions)
	{
		basicblock* block = new basicblock;
		memset(block, 0, sizeof(basicblock));
		block->ld = new BasicblockLoopData;
		block->method = jd->m;
		block->icount = numInstructions;
		block->iinstr = new instruction[numInstructions];
		memset(block->iinstr, 0, sizeof(instruction) * numInstructions);
		return block;
	}

	void duplicateLoop(LoopContainer* loop)
	{
		// copy header
		loop->header->ld->copiedTo = duplicateBasicblock(loop->header);

		// copy other basicblocks
		for (std::vector<basicblock*>::iterator it = loop->nodes.begin(); it != loop->nodes.end(); ++it)
		{
			(*it)->ld->copiedTo = duplicateBasicblock(*it);
		}
	}

	void deleteDuplicatedLoop(jitdata* jd)
	{
		for (basicblock* block = jd->basicblocks; block; block = block->next)
		{
			deleteDuplicatedBasicblock(block->ld->copiedTo);

			// prepare for next loop duplication
			block->ld->copiedTo = 0;
		}
	}

	/**
	 * Searches the basicblock list for the duplicated loop and returns two basicblock pointers:
	 * One points to the basicblock preceding the loop, the other to the last basicblock in the loop.
	 * By the way it checks whether the loop has no hole and the first block is the loop header.
	 *
	 * Returns true if the loop has no hole and the first block is the loop header, false otherwise.
	 */
	bool findLoop(jitdata* jd, basicblock** beforeLoopPtr, basicblock** lastBlockInLoopPtr)
	{
		basicblock* lastBlock = 0;
		bool loopFound = false;

		*beforeLoopPtr = 0;
		*lastBlockInLoopPtr = 0;
		
		for (basicblock* block = jd->basicblocks; block; block = block->next)
		{
			assert(lastBlock == 0 || lastBlock->ld);
			assert(block->ld);

			// Is block the beginning of the duplicated loop?
			if ((lastBlock == 0 || lastBlock->ld->copiedTo == 0) && block->ld->copiedTo)
			{
				// The loop contains a hole.
				if (loopFound)
				{
					log_text("findLoop: hole");
					return false;
				}

				// The first block is not the header.
				if (block->ld->loop == 0)
				{
					log_text("findLoop: header");
					return false;
				}

				loopFound = true;
				*beforeLoopPtr = lastBlock;
			}

			if (block->ld->copiedTo)
				*lastBlockInLoopPtr = block;

			lastBlock = block;
		}

		return true;
	}

	/**
	 * Inserts the copied loop, the loop switch and the trampoline into the global basicblock list.
	 * It also inserts these nodes into the predecessor loops.
	 *
	 * loop: The original loop that has been duplicated.
	 */
	void buildBasicblockList(jitdata* jd, LoopContainer* loop, basicblock* beforeLoop, basicblock* lastBlockInLoop, basicblock* loopSwitch, basicblock* loopTrampoline)
	{
		buildBasicblockList(jd, loop, beforeLoop, lastBlockInLoop, loopSwitch, 0, loopTrampoline);
	}

	/**
	 * Inserts the copied loop, the loop switches and the trampoline into the global basicblock list.
	 * It also inserts these nodes into the predecessor loops.
	 *
	 * loop: The original loop that has been duplicated.
	 * loopSwitch2: Will be inserted after the first loop switch if it is != 0.
	 */
	void buildBasicblockList(jitdata* jd, LoopContainer* loop, basicblock* beforeLoop, basicblock* lastBlockInLoop, basicblock* loopSwitch1, basicblock* loopSwitch2, basicblock* loopTrampoline)
	{
		assert(loopSwitch1);

		// insert first loop switch
		if (beforeLoop)
		{
			loopSwitch1->next = beforeLoop->next;
			beforeLoop->next = loopSwitch1;
		}
		else
		{
			loopSwitch1->next = jd->basicblocks;
			jd->basicblocks = loopSwitch1;
		}

		basicblock* lastLoopSwitch = loopSwitch1;

		// insert second loop switch
		if (loopSwitch2)
		{
			loopSwitch2->next = loopSwitch1->next;
			loopSwitch1->next = loopSwitch2;

			lastLoopSwitch = loopSwitch2;
		}

		// insert trampoline after loop
		lastBlockInLoop->next = loopTrampoline;

		// The basicblocks contained in the following container will be inserted into the predecessor loops.
		std::vector<basicblock*> duplicates;
		duplicates.reserve(loop->nodes.size() + 1);

		// insert copied loop after trampoline
		basicblock* end = loopTrampoline;
		for (basicblock* block = lastLoopSwitch->next; block != loopTrampoline; block = block->next)
		{
			end->next = block->ld->copiedTo;
			end = block->ld->copiedTo;

			// The copied basicblock must be inserted into the predecessor loops.
			duplicates.push_back(block->ld->copiedTo);

			// prepare for next loop duplication
			block->ld->copiedTo = 0;
		}

		// end->next already points to the basicblock after the second loop.

		// Insert nodes into predecessor loops except the root loop.
		for (LoopContainer* pred = loop->parent; pred->parent; pred = pred->parent)
		{
			for (std::vector<basicblock*>::iterator it = duplicates.begin(); it != duplicates.end(); ++it)
			{
				pred->nodes.push_back(*it);
			}
			pred->nodes.push_back(loopTrampoline);
			pred->nodes.push_back(loopSwitch1);
			if (loopSwitch2)
				pred->nodes.push_back(loopSwitch2);
		}
	}

	void removeChecks(LoopContainer* loop, s4 array, s4 index)
	{
		for (std::vector<basicblock*>::iterator it = loop->nodes.begin(); it != loop->nodes.end(); ++it)
		{
			for (instruction* instr = (*it)->iinstr; instr != (*it)->iinstr + (*it)->icount; instr++)
			{
				switch (instr->opc)
				{
					case ICMD_IALOAD:
					case ICMD_LALOAD:
					case ICMD_FALOAD:
					case ICMD_DALOAD:
					case ICMD_AALOAD:
					case ICMD_BALOAD:
					case ICMD_CALOAD:
					case ICMD_SALOAD:

					case ICMD_IASTORE:
					case ICMD_LASTORE:
					case ICMD_FASTORE:
					case ICMD_DASTORE:
					case ICMD_AASTORE:
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

						if (array == instr->s1.varindex && index == instr->sx.s23.s2.varindex)
							instr->flags.bits &= ~INS_FLAG_CHECK;
						break;
				}
			}
		}
	}

	/**
	 * Creates a basicblock with a single GOTO instruction.
	 */
	basicblock* createLoopTrampoline(jitdata* jd, basicblock* target)
	{
		basicblock* loopTrampoline = new basicblock;
		memset(loopTrampoline, 0, sizeof(basicblock));
		loopTrampoline->ld = new BasicblockLoopData;
		loopTrampoline->method = jd->m;
		loopTrampoline->icount = 1;

		loopTrampoline->iinstr = new instruction[loopTrampoline->icount];
		memset(loopTrampoline->iinstr, 0, sizeof(instruction) * loopTrampoline->icount);

		opcode_GOTO(loopTrampoline->iinstr, target);

		return loopTrampoline;
	}

	/**
	 * Redirects all jumps that go to the old loop:
	 * -) A jump from outside of the loop will be redirected to the loop switch.
	 * -) A jump from the duplicate will be redirected to the duplicate.
	 *
	 * This function must be called before the duplicate is inserted into the linked list.
	 */
	void redirectJumps(jitdata* jd, basicblock* loopSwitch)
	{
		// All jumps that go from the outside of the loop to the loop header must be redirected to the loop switch.
		for (basicblock* block = jd->basicblocks; block; block = block->next)
		{
			// Check if this block is not part of the original loop.
			if (!block->ld->copiedTo)
			{
				for (instruction* instr = block->iinstr; instr != block->iinstr + block->icount; instr++)
				{
					// If instr is a jump into the original loop, redirect it to the loop switch.
					switch (icmd_table[instr->opc].controlflow)
					{
						case CF_IF:
						case CF_GOTO:
						case CF_RET:
							if (instr->dst.block->ld->copiedTo)
								instr->dst.block = loopSwitch;
							break;
						case CF_JSR:
							if (instr->sx.s23.s3.jsrtarget.block->ld->copiedTo)
								instr->sx.s23.s3.jsrtarget.block = loopSwitch;
							break;
						case CF_TABLE:
						{
							// count = (tablehigh - tablelow + 1) + 1 [default branch]
							s4 count = instr->sx.s23.s3.tablehigh - instr->sx.s23.s2.tablelow + 2;

							branch_target_t* target = instr->dst.table;
							while (--count >= 0)
							{
								if (target->block->ld->copiedTo)
									target->block = loopSwitch;
								target++;
							}
							break;
						}
						case CF_LOOKUP:
						{
							// default target
							if (instr->sx.s23.s3.lookupdefault.block->ld->copiedTo)
								instr->sx.s23.s3.lookupdefault.block = loopSwitch;

							// other targets
							lookup_target_t* entry = instr->dst.lookup;
							s4 count = instr->sx.s23.s2.lookupcount;
							while (--count >= 0)
							{
								if (entry->target.block->ld->copiedTo)
									entry->target.block = loopSwitch;
								entry++;
							}
							break;
						}
						case CF_END:
						case CF_NORMAL:
							// nothing
							break;
					}
				}
			}
			else   // This block is part of the original loop.
			{
				// Redirect jumps that go from the duplicate to the original loop.
				basicblock* dupBlock = block->ld->copiedTo;
				for (instruction* instr = dupBlock->iinstr; instr != dupBlock->iinstr + dupBlock->icount; instr++)
				{
					// If instr is a jump into the original loop, redirect it to the duplicated block.
					switch (icmd_table[instr->opc].controlflow)
					{
						case CF_IF:
						case CF_GOTO:
						case CF_RET:
							if (instr->dst.block->ld->copiedTo)
								instr->dst.block = instr->dst.block->ld->copiedTo;
							break;
						case CF_JSR:
							if (instr->sx.s23.s3.jsrtarget.block->ld->copiedTo)
								instr->sx.s23.s3.jsrtarget.block = instr->sx.s23.s3.jsrtarget.block->ld->copiedTo;
							break;
						case CF_TABLE:
						{
							// count = (tablehigh - tablelow + 1) + 1 [default branch]
							s4 count = instr->sx.s23.s3.tablehigh - instr->sx.s23.s2.tablelow + 2;

							branch_target_t* target = instr->dst.table;
							while (--count >= 0)
							{
								if (target->block->ld->copiedTo)
									target->block = target->block->ld->copiedTo;
								target++;
							}
							break;
						}
						case CF_LOOKUP:
						{
							// default target
							if (instr->sx.s23.s3.lookupdefault.block->ld->copiedTo)
								instr->sx.s23.s3.lookupdefault.block = instr->sx.s23.s3.lookupdefault.block->ld->copiedTo;

							// other targets
							lookup_target_t* entry = instr->dst.lookup;
							s4 count = instr->sx.s23.s2.lookupcount;
							while (--count >= 0)
							{
								if (entry->target.block->ld->copiedTo)
									entry->target.block = entry->target.block->ld->copiedTo;
								entry++;
							}
							break;
						}
						case CF_END:
						case CF_NORMAL:
							// nothing
							break;
					}
				}
			}
		}
	}

	void optimizeLoop(jitdata* jd, LoopContainer* loop, s4 stackSlot)
	{
		// Optimize inner loops.
		for (std::vector<LoopContainer*>::iterator it = loop->children.begin(); it != loop->children.end(); ++it)
		{
			optimizeLoop(jd, *it, stackSlot);
		}

		// Currently only loops with one footer are optimized.
		if (loop->footers.size() == 1)
		{
			if (loop->hasCounterVariable && loop->counterIncrement >= 0)
			{
				if (loop->counterInterval.lower().lower() >= 0)   // counterInterval: [L, ?], L >= 0
				{
					if (loop->counterInterval.upper().instruction().kind() == NumericInstruction::ARRAY_LENGTH &&
						loop->counterInterval.upper().constant() < 0)
					{
						// counterInterval: [L, array.length - c], L >= 0, c > 0

						log_text("optimizeLoop: [non-negative, arraylength]");

						s4 array = loop->counterInterval.upper().instruction().variable();

						// The array variable must be invariant.
						if (!loop->invariantArrays.contains(array))
						{
							log_println("optimizeLoop: %d not invariant", array);
							return;
						}

						// duplicate loop
						duplicateLoop(loop);
						basicblock *beforeLoop, *lastBlockInLoop;
						if (!findLoop(jd, &beforeLoop, &lastBlockInLoop))
						{
							log_text("optimizeLoop: hole or header");

							// loop cannot be optimized
							deleteDuplicatedLoop(jd);
							return;
						}

						// The following assertion simplifies the code generation.
						if ((beforeLoop && beforeLoop->outdepth != 0) ||
							lastBlockInLoop->outdepth != 0)
						{
							log_text("optimizeLoop: depth != 0");

							// loop cannot be optimized
							deleteDuplicatedLoop(jd);
							return;
						}

						// remove checks in original loop
						removeChecks(loop, array, loop->counterVariable);

						// create basicblock that jumps to the right loop
						basicblock* loopSwitch = createBasicblock(jd, 4);

						// Fill instruction array of loop switch with:
						// if (array.length + upper_constant > MAX - increment) goto unoptimized_loop

						instruction* instr = loopSwitch->iinstr;

						opcode_ALOAD(instr++, array, array);
						opcode_ARRAYLENGTH(instr++, array, stackSlot);
						opcode_IADDCONST(instr++, stackSlot, loop->counterInterval.upper().constant(), stackSlot);
						opcode_IFGT(instr++, stackSlot, Scalar::max() - loop->counterIncrement, loop->header->ld->copiedTo);

						assert(instr - loopSwitch->iinstr == loopSwitch->icount);

						// create basicblock that jumps over the second loop
						basicblock* loopTrampoline = createLoopTrampoline(jd, lastBlockInLoop->next);

						// Insert loop into basicblock list.
						redirectJumps(jd, loopSwitch);
						buildBasicblockList(jd, loop, beforeLoop, lastBlockInLoop, loopSwitch, loopTrampoline);

						// Adjust statistical data.
						jd->basicblockcount += loop->nodes.size() + 3;
					}
					else if (loop->counterInterval.upper().instruction().kind() == NumericInstruction::VARIABLE &&
						loop->counterInterval.upper().constant() == 0)
					{
						// counterInterval: [L, x], L >= 0

						log_text("optimizeLoop: [non-negative, invariant]");

						if (loop->invariantArrays.begin() == loop->invariantArrays.end())
						{
							log_text("optimizeLoop: no invariant array");
							return;
						}
						
						// TODO optimization: why take an arbitrary array variable?
						s4 array = *loop->invariantArrays.begin();

						// duplicate loop
						duplicateLoop(loop);
						basicblock *beforeLoop, *lastBlockInLoop;
						if (!findLoop(jd, &beforeLoop, &lastBlockInLoop))
						{
							log_text("optimizeLoop: hole or header");

							// loop cannot be optimized
							deleteDuplicatedLoop(jd);
							return;
						}

						// The following assertion simplifies the code generation.
						if ((beforeLoop && beforeLoop->outdepth != 0) ||
							lastBlockInLoop->outdepth != 0)
						{
							log_text("optimizeLoop: depth != 0");

							// loop cannot be optimized
							deleteDuplicatedLoop(jd);
							return;
						}

						// remove checks in original loop
						removeChecks(loop, array, loop->counterVariable);

						s4 invariantVariable = loop->counterInterval.upper().instruction().variable();

						// create loop switches that jump to the right loop
						basicblock* loopSwitch1 = createBasicblock(jd, 2);
						basicblock* loopSwitch2 = createBasicblock(jd, 4);
						instruction* instr;

						// Fill instruction array of first loop switch with:
						// if (x > MAX - increment) goto unoptimized_loop

						instr = loopSwitch1->iinstr;

						opcode_ILOAD(instr++, invariantVariable, invariantVariable);
						opcode_IFGT(instr++, invariantVariable, Scalar::max() - loop->counterIncrement, loop->header->ld->copiedTo);

						assert(instr - loopSwitch1->iinstr == loopSwitch1->icount);

						// Fill instruction array of second loop switch with:
						// if (x >= array.length) goto unoptimized_loop

						instr = loopSwitch2->iinstr;

						opcode_ILOAD(instr++, invariantVariable, invariantVariable);
						opcode_ALOAD(instr++, array, array);
						opcode_ARRAYLENGTH(instr++, array, stackSlot);
						opcode_IF_ICMPGE(instr++, invariantVariable, stackSlot, loop->header->ld->copiedTo);

						assert(instr - loopSwitch2->iinstr == loopSwitch2->icount);

						// create basicblock that jumps over the second loop
						basicblock* loopTrampoline = createLoopTrampoline(jd, lastBlockInLoop->next);

						// Insert loop into basicblock list.
						redirectJumps(jd, loopSwitch1);
						buildBasicblockList(jd, loop, beforeLoop, lastBlockInLoop, loopSwitch1, loopSwitch2, loopTrampoline);

						// Adjust statistical data.
						jd->basicblockcount += loop->nodes.size() + 4;
					}
					else if (loop->counterInterval.upper().instruction().kind() == NumericInstruction::ZERO &&
						loop->counterInterval.upper().constant() <= Scalar::max() - loop->counterIncrement)
					{
						// counterInterval: [L, c], L >= 0, c <= MAX - increment

						log_text("optimizeLoop: [non-negative, constant]");

						if (loop->invariantArrays.begin() == loop->invariantArrays.end())
						{
							log_text("optimizeLoop: no invariant array");
							return;
						}
						
						// TODO optimization: why take an arbitrary array variable?
						s4 array = *loop->invariantArrays.begin();

						// duplicate loop
						duplicateLoop(loop);
						basicblock *beforeLoop, *lastBlockInLoop;
						if (!findLoop(jd, &beforeLoop, &lastBlockInLoop))
						{
							log_text("optimizeLoop: hole or header");

							// loop cannot be optimized
							deleteDuplicatedLoop(jd);
							return;
						}

						// The following assertion simplifies the code generation.
						if ((beforeLoop && beforeLoop->outdepth != 0) ||
							lastBlockInLoop->outdepth != 0)
						{
							log_text("optimizeLoop: depth != 0");

							// loop cannot be optimized
							deleteDuplicatedLoop(jd);
							return;
						}

						// remove checks in original loop
						removeChecks(loop, array, loop->counterVariable);

						// create basicblock that jumps to the right loop
						basicblock* loopSwitch = createBasicblock(jd, 3);

						// Fill instruction array of loop switch with:
						// if (array.length <= upper_constant) goto unoptimized_loop

						instruction* instr = loopSwitch->iinstr;

						opcode_ALOAD(instr++, array, array);
						opcode_ARRAYLENGTH(instr++, array, stackSlot);
						opcode_IFLE(instr++, stackSlot, loop->counterInterval.upper().constant(), loop->header->ld->copiedTo);

						assert(instr - loopSwitch->iinstr == loopSwitch->icount);

						// create basicblock that jumps over the second loop
						basicblock* loopTrampoline = createLoopTrampoline(jd, lastBlockInLoop->next);

						// Insert loop into basicblock list.
						redirectJumps(jd, loopSwitch);
						buildBasicblockList(jd, loop, beforeLoop, lastBlockInLoop, loopSwitch, loopTrampoline);

						// Adjust statistical data.
						jd->basicblockcount += loop->nodes.size() + 3;
					}
				}
			}
		}
	}
}

void removePartiallyRedundantChecks(jitdata* jd)
{
	log_text("loop duplication started");

	// Reserve temporary variable.
	s4 stackSlot;
	if (jd->vartop < jd->varcount)
	{
		stackSlot = jd->vartop++;

		varinfo* v = VAR(stackSlot);
		v->type = TYPE_INT;
	}
	else
	{
		log_text("loop duplication: no free variable available");
		return;
	}

	// Currently only methods without try blocks are allowed.
	if (jd->exceptiontablelength != 0)
	{
		log_text("loop duplication: exception handler");
		return;
	}

	//if (jd->ld->rootLoop->children.size() == 1 && jd->ld->rootLoop->children[0]->children.size() == 0)
	//{
		for (std::vector<LoopContainer*>::iterator it = jd->ld->rootLoop->children.begin(); it != jd->ld->rootLoop->children.end(); ++it)
		{
			optimizeLoop(jd, *it, stackSlot);
		}
	//}

	log_text("loop duplication finished");
}
