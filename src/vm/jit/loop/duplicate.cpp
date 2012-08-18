#include "toolbox/logging.hpp"
#include "vm/jit/ir/icmd.hpp"

#include "duplicate.hpp"

#include <cstring>

namespace
{
	basicblock* copyBasicblock(const basicblock* block);
	void deleteDuplicatedBasicblock(basicblock* block);
	void duplicateLoop(LoopContainer* loop);
	void deleteDuplicatedLoop(jitdata* jd);
	bool findLoop(jitdata* jd, basicblock** beforeLoopPtr, basicblock** lastBlockInLoopPtr);
	void buildBasicblockList(jitdata* jd, LoopContainer* loop, basicblock* beforeLoop, basicblock* lastBlockInLoop, basicblock* loopSwitch, basicblock* loopTrampoline);
	void removeChecks(LoopContainer* loop, s4 array, s4 index);
	basicblock* createLoopTrampoline(basicblock* target);
	void redirectJumps(jitdata* jd, basicblock* loopSwitch);
	void optimizeLoop(jitdata* jd, LoopContainer* loop, s4 stackSlot);

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

	void duplicateLoop(LoopContainer* loop)
	{
		log_text("duplicateLoop started");

		// copy header
		loop->header->ld->copiedTo = duplicateBasicblock(loop->header);

		// copy other basicblocks
		for (std::vector<basicblock*>::iterator it = loop->nodes.begin(); it != loop->nodes.end(); ++it)
		{
			(*it)->ld->copiedTo = duplicateBasicblock(*it);
		}

		log_text("duplicateLoop finished");
	}

	void deleteDuplicatedLoop(jitdata* jd)
	{
		log_text("deleteDuplicatedLoop started");

		for (basicblock* block = jd->basicblocks; block; block = block->next)
		{
			deleteDuplicatedBasicblock(block->ld->copiedTo);

			// prepare for next loop duplication
			block->ld->copiedTo = 0;
		}

		log_text("deleteDuplicatedLoop finished");
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
		log_text("findLoop started");

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
					log_text("findLoop finished: hole");
					return false;
				}

				// The first block is not the header.
				if (block->ld->loop == 0)
				{
					log_text("findLoop finished: header");
					return false;
				}

				loopFound = true;
				*beforeLoopPtr = lastBlock;
			}

			if (block->ld->copiedTo)
				*lastBlockInLoopPtr = block;

			lastBlock = block;
		}

		log_text("findLoop finished");

		return true;
	}

	/**
	 * Inserts the copied loop, the loop switch and the trampoline into the global basicblock list.
	 * It also inserts these nodes into the predecessor loops.
	 */
	void buildBasicblockList(jitdata* jd, LoopContainer* loop, basicblock* beforeLoop, basicblock* lastBlockInLoop, basicblock* loopSwitch, basicblock* loopTrampoline)
	{
		log_text("buildBasicblockList started");
		assert(beforeLoop);
		assert(lastBlockInLoop);
		assert(loopSwitch);
		assert(loopTrampoline);

		assert(loop->parent);   // loop is not the root

		// insert loop switch
		if (beforeLoop)
		{
			loopSwitch->next = beforeLoop->next;
			beforeLoop->next = loopSwitch;
		}
		else
		{
			loopSwitch->next = jd->basicblocks;
			jd->basicblocks = loopSwitch;
		}

		log_text("loop switch inserted");

		// insert trampoline after loop
		lastBlockInLoop->next = loopTrampoline;

		log_text("loop trampoline inserted");

		// insert copied loop after trampoline
		basicblock* end = loopTrampoline;
		for (basicblock* block = loopSwitch->next; block != loopTrampoline; block = block->next)
		{
			end->next = block->ld->copiedTo;
			end = block->ld->copiedTo;

			// prepare for next loop duplication
			block->ld->copiedTo = 0;

			log_println("block %d inserted", block->nr);
		}

		// end->next already points to the basicblock after the second loop.

		// Insert nodes into predecessor loops except the root loop.
		for (LoopContainer* pred = loop->parent; pred->parent; pred = pred->parent)
		{
			for (std::vector<basicblock*>::iterator it = loop->nodes.begin(); it != loop->nodes.end(); ++it)
			{
				pred->nodes.push_back(*it);
			}
			pred->nodes.push_back(loopSwitch);
			pred->nodes.push_back(loopTrampoline);
		}

		log_text("buildBasicblockList finished");
	}

	void removeChecks(LoopContainer* loop, s4 array, s4 index)
	{
		log_println("removeChecks started (array == %d, index == %d)", array, index);
		assert(loop);

		for (std::vector<basicblock*>::iterator it = loop->nodes.begin(); it != loop->nodes.end(); ++it)
		{
			for (instruction* instr = (*it)->iinstr; instr != (*it)->iinstr + (*it)->icount; instr++)
			{
//				log_println("instr->opc == %s", icmd_table[instr->opc].name);
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

		log_text("removeChecks finished");
	}

	/**
	 * Creates a basicblock with a single GOTO instruction.
	 */
	basicblock* createLoopTrampoline(jitdata* jd, basicblock* target)
	{
		log_text("createLoopTrampoline started");

		basicblock* loopTrampoline = new basicblock;
		memset(loopTrampoline, 0, sizeof(basicblock));
		loopTrampoline->ld = new BasicblockLoopData;
		loopTrampoline->method = jd->m;
		loopTrampoline->icount = 1;

		loopTrampoline->iinstr = new instruction[loopTrampoline->icount];
		memset(loopTrampoline->iinstr, 0, sizeof(instruction) * loopTrampoline->icount);
		loopTrampoline->iinstr->opc = ICMD_GOTO;
		loopTrampoline->iinstr->dst.block = target;

		log_text("createLoopTrampoline finished");

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
		log_text("redirectJumps started");

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
		
		log_text("redirectJumps finished");
	}

	void optimizeLoop(jitdata* jd, LoopContainer* loop, s4 stackSlot)
	{
		log_text("optimizeLoop started");

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
				if (loop->counterInterval.lower().lower() >= 0)
				{
					switch (loop->counterInterval.upper().instruction().kind())
					{
						case NumericInstruction::ARRAY_LENGTH:
							if (loop->counterInterval.upper().constant() < 0)
							{
								s4 array = loop->counterInterval.upper().instruction().variable();

								// The array variable must be invariant.
								if (!loop->invariantArrays.contains(array))
								{
									log_text("bad loop 1");
									log_text("optimizeLoop finished");
									return;
								}

								log_text("arraylength instruction");

								// duplicate loop
								duplicateLoop(loop);
								basicblock *beforeLoop, *lastBlockInLoop;
								if (!findLoop(jd, &beforeLoop, &lastBlockInLoop))
								{
									log_text("bad loop 2");

									// loop cannot be optimized
									deleteDuplicatedLoop(jd);

									log_text("optimizeLoop finished");
									return;
								}

								// The following assertion simplifies the code generation.
								if ((beforeLoop && beforeLoop->outdepth != 0) ||
									lastBlockInLoop->outdepth != 0)
								{
									log_text("bad loop 3");

									// loop cannot be optimized
									deleteDuplicatedLoop(jd);

									log_text("optimizeLoop finished");
									return;
								}

								// remove checks in original loop
								removeChecks(loop, array, loop->counterVariable);

								// create basicblock that jumps to the right loop
								basicblock* loopSwitch = new basicblock;
								memset(loopSwitch, 0, sizeof(basicblock));
								loopSwitch->ld = new BasicblockLoopData;
								loopSwitch->method = jd->m;
								loopSwitch->icount = 4;
								loopSwitch->iinstr = new instruction[loopSwitch->icount];
								memset(loopSwitch->iinstr, 0, sizeof(instruction) * loopSwitch->icount);

								log_text("loop switch created");

								// Fill instruction array of loop switch with:
								// if (array.length + upper_constant > MAX - increment) goto unoptimized_loop

								loopSwitch->iinstr[0].opc = ICMD_ALOAD;
								loopSwitch->iinstr[0].s1.varindex = array;
								loopSwitch->iinstr[0].dst.varindex = array;

								loopSwitch->iinstr[1].opc = ICMD_ARRAYLENGTH;
								loopSwitch->iinstr[1].s1.varindex = array;
								loopSwitch->iinstr[1].dst.varindex = stackSlot;

								loopSwitch->iinstr[2].opc = ICMD_IADDCONST;
								loopSwitch->iinstr[2].s1.varindex = stackSlot;
								loopSwitch->iinstr[2].sx.val.i = loop->counterInterval.upper().constant();
								loopSwitch->iinstr[2].dst.varindex = stackSlot;

								loopSwitch->iinstr[3].opc = ICMD_IFGT;
								loopSwitch->iinstr[3].s1.varindex = stackSlot;
								loopSwitch->iinstr[3].sx.val.i = Scalar::max() - loop->counterIncrement;
								loopSwitch->iinstr[3].dst.block = loop->header->ld->copiedTo;

								log_text("loop switch filled");

								// create basicblock that jumps over the second loop
								basicblock* loopTrampoline = createLoopTrampoline(jd, lastBlockInLoop->next);

								// Insert loop into basicblock list.
								redirectJumps(jd, loopSwitch);
								buildBasicblockList(jd, loop, beforeLoop, lastBlockInLoop, loopSwitch, loopTrampoline);

								log_text("arraylength finished");
							}
							break;
						case NumericInstruction::VARIABLE:
							break;
						case NumericInstruction::ZERO:
							break;
					}
				}
			}
		}

		log_text("optimizeLoop finished");
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
		log_text("loop duplication finished: no free variable available");
		return;
	}

	// Currently only methods without try blocks are allowed.
	if (jd->exceptiontablelength != 0)
	{
		log_text("loop duplication finished: exception handler");
		return;
	}

	// TODO Currently only methods with one loop will be optimized.
	if (jd->ld->rootLoop->children.size() == 1 && jd->ld->rootLoop->children[0]->children.size() == 0)
	{
		for (std::vector<LoopContainer*>::iterator it = jd->ld->rootLoop->children.begin(); it != jd->ld->rootLoop->children.end(); ++it)
		{
			optimizeLoop(jd, *it, stackSlot);
		}
	}

	log_text("loop duplication finished");
}
