## src/vm/jit/compiler2/Makefile.am
##
## Copyright (C) 2013
## CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
##
## This file is part of CACAO.
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as
## published by the Free Software Foundation; either version 2, or (at
## your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
## 02110-1301, USA.


AM_CPPFLAGS += -I$(top_srcdir)/src \
	-I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) \
	-I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR) \
	-I$(top_srcdir)/src/vm/jit/compiler2/$(ARCH_DIR) \
	-I$(top_builddir)/src

LIBS =

SUBDIRS = \
	alloc \
	memory \
	$(ARCH_DIR)

noinst_LTLIBRARIES = \
	libcompiler2.la

libcompiler2_la_SOURCES = \
	Backend.cpp \
	Backend.hpp \
	BasicBlockSchedule.hpp \
	BasicBlockSchedulingPass.cpp \
	BasicBlockSchedulingPass.hpp \
	CFGConstructionPass.cpp \
	CFGConstructionPass.hpp \
	CFGMetaPass.cpp \
	CFGMetaPass.hpp \
	CodeGenPass.cpp \
	CodeGenPass.hpp \
	CodeMemory.cpp \
	CodeMemory.hpp \
	CodeSegment.cpp \
	CodeSegment.hpp \
	Compiler.cpp \
	Compiler.hpp \
	Conditional.cpp \
	Conditional.hpp \
	ConstantPropagationPass.cpp \
	ConstantPropagationPass.hpp \
	DeadCodeEliminationPass.cpp \
	DeadCodeEliminationPass.hpp \
	DisassemblerPass.cpp \
	DisassemblerPass.hpp \
	DataSegment.cpp \
	DataSegment.hpp \
	DominatorPass.cpp \
	DominatorPass.hpp \
	DomTreePrinterPass.cpp \
	DomTreePrinterPass.hpp \
	ExamplePass.cpp \
	ExamplePass.hpp \
	GlobalSchedule.hpp \
	GlobalValueNumberingPass.cpp \
	GlobalValueNumberingPass.hpp \
	GraphHelper.cpp \
	GraphHelper.hpp \
	ICMDPrinterPass.cpp \
	ICMDPrinterPass.hpp \
	Instruction.cpp \
	Instruction.hpp \
	InstructionMetaPass.cpp \
	InstructionMetaPass.hpp \
	InstructionSchedule.hpp \
	InstructionVisitor.cpp \
	InstructionVisitor.hpp \
	Instructions.hpp \
	JITData.cpp \
	JITData.hpp \
	LinearScanAllocatorPass.cpp \
	LinearScanAllocatorPass.hpp \
	ListSchedulingPass.cpp \
	ListSchedulingPass.hpp \
	LivetimeAnalysisPass.cpp \
	LivetimeAnalysisPass.hpp \
	LivetimeInterval.cpp \
	LivetimeInterval.hpp \
	Loop.cpp \
	Loop.hpp \
	LoopBase.hpp \
	LoopPass.cpp \
	LoopPass.hpp \
	LoopPassBase.hpp \
	LoopSimplificationPass.cpp \
	LoopSimplificationPass.hpp \
	LoopTreePrinterPass.cpp \
	LoopTreePrinterPass.hpp \
	MachineAddress.hpp \
	MachineBasicBlock.cpp \
	MachineBasicBlock.hpp \
	MachineInstruction.cpp \
	MachineInstruction.hpp \
	MachineInstructionPrinterPass.cpp \
	MachineInstructionPrinterPass.hpp \
	MachineInstructionSchedule.cpp \
	MachineInstructionSchedule.hpp \
	MachineInstructionSchedulingPass.cpp \
	MachineInstructionSchedulingPass.hpp \
	MachineInstructions.cpp \
	MachineInstructions.hpp \
	MachineLoop.cpp \
	MachineLoop.hpp \
	MachineLoopPass.cpp \
	MachineLoopPass.hpp \
	MachineOperand.cpp \
	MachineOperand.hpp \
	MachineRegister.hpp \
	Matcher.cpp \
	Matcher.hpp \
	MatcherDefs.hpp \
	MethodC2.cpp \
	MethodC2.hpp \
	MethodDescriptor.cpp \
	MethodDescriptor.hpp \
	NullCheckEliminationPass.cpp \
	NullCheckEliminationPass.hpp \
	ObjectFileWriterPass.cpp \
	ObjectFileWriterPass.hpp \
	ParserPass.cpp \
	ParserPass.hpp \
	Pass.cpp \
	Pass.hpp \
	PassDependencyGraphPrinter.cpp \
	PassDependencyGraphPrinter.hpp \
	PassManager.cpp \
	PassManager.hpp \
	PassUsage.hpp \
	RegisterAllocatorPass.cpp \
	RegisterAllocatorPass.hpp \
	ScheduleClickPass.cpp \
	ScheduleClickPass.hpp \
	ScheduleEarlyPass.cpp \
	ScheduleEarlyPass.hpp \
	ScheduleLatePass.cpp \
	ScheduleLatePass.hpp \
	Segment.cpp \
	Segment.hpp \
	SourceStateAttachmentPass.cpp \
	SourceStateAttachmentPass.hpp \
	SSAConstructionPass.cpp \
	SSAConstructionPass.hpp \
	SSAPrinterPass.cpp \
	SSAPrinterPass.hpp \
	StackAnalysisPass.cpp \
	StackAnalysisPass.hpp \
	StackSlotManager.cpp \
	StackSlotManager.hpp \
	Type.cpp \
	Type.hpp \
	Value.cpp \
	Value.hpp \
	VerifierPass.cpp \
	VerifierPass.hpp \
	\
	GrammarExcludedNodes.inc \
	InstructionDeclGen.inc \
	InstructionIDGen.inc \
	InstructionToInstGen.inc \
	InstructionNameSwitchGen.inc \
	InstructionVisitorGen.inc \
	InstructionVisitorImplGen.inc

libcompiler2_la_LIBADD = \
	alloc/liballoc.la \
	memory/libmemory.la \
	$(ARCH_DIR)/lib$(ARCH_DIR).la

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
