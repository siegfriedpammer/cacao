/* src/vm/jit/compiler2/ObjectFileWriterPass.cpp - ObjectFileWriterPass

   Copyright (C) 2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

#include "vm/jit/compiler2/ObjectFileWriterPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/CodeGenPass.hpp"
#include "vm/jit/compiler2/MethodDescriptor.hpp"
#include "toolbox/logging.hpp"

#include "vm/jit/jit.hpp"
#include "vm/jit/code.hpp"

#include <bfd.h>
#include <string>

#define DEBUG_NAME "compiler2/ObjectFileWriter"

namespace {

template<typename T>
inline void check_bfd_error(T* t) {
	if (!t) {
		// error
		ABORT_MSG("ObjectFileWriterPass Error", "Error: " << bfd_errmsg(bfd_get_error()));
	}
}
inline void check_bfd_error(bfd_boolean b) {
	if (b == FALSE) {
		// error
		ABORT_MSG("ObjectFileWriterPass Error", "Error: " << bfd_errmsg(bfd_get_error()));
	}
}

} // end anonymous namespace

namespace cacao {
namespace jit {
namespace compiler2 {

bool ObjectFileWriterPass::run(JITData &JD) {
	Method *M = JD.get_Method();
	CodeGenPass *CG = get_Pass<CodeGenPass>();
	const CodeMemory &CM = CG->get_CodeMemory();

	const char* bfd_arch = "i386:x86-64";
	const char* bfd_target = "elf64-x86-64";
	std::string symbol_name(M->get_name_utf8().begin(),M->get_name_utf8().size());
	std::string class_name(M->get_class_name_utf8().begin(),M->get_class_name_utf8().size());
	std::string filename = class_name + "." + symbol_name + ".o";
	//JD.get_Method()->get_MethodDescriptor().
	//
	LOG("Object file name: " << filename.c_str() << nl);
	LOG(".text symbol name: " << symbol_name.c_str() << nl);

	if (DEBUG_COND) {
		const char ** target_list = bfd_target_list();
		while (*target_list) {
			LOG("target: " << *target_list << nl);
			++target_list;
		}
	}
	if (DEBUG_COND) {
		const char ** arch_list = bfd_arch_list();
		while (*arch_list) {
			LOG("arch: " << *arch_list << nl);
			++arch_list;
		}
	}
	// open file
	LOG2("open file" << nl);
	bfd *abfd = bfd_openw(filename.c_str(),bfd_target);
	check_bfd_error(abfd);

	// set architecture
	LOG2("set architecture" << nl);
	const bfd_arch_info_type *arch_info = bfd_scan_arch (bfd_arch);
	check_bfd_error(arch_info);

	bfd_set_arch_info(abfd,arch_info);
	// set format
	bfd_set_format(abfd, bfd_object);

	// create section
	LOG2("create section" << nl);
	asection *section = bfd_make_section_anyway_with_flags(abfd, ".text",
		SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_READONLY | SEC_CODE );
	check_bfd_error(section);
	// set section size
	LOG2("set section size" << nl);
	check_bfd_error(bfd_set_section_size(abfd, section,CM.size()));
	// set alignment
	section->alignment_power = 2;

	// create symbol
	LOG2("create symbol" << nl);
	asymbol *new_smbl = bfd_make_empty_symbol (abfd);
	new_smbl->name = symbol_name.c_str();
	new_smbl->section = section;
	new_smbl->flags = BSF_GLOBAL;

	// symbol list
	asymbol *ptrs[2];
	ptrs[0] = new_smbl;
	ptrs[1] = 0;

	// insert symbol list
	LOG2("insert symbol list" << nl);
	check_bfd_error(bfd_set_symtab(abfd, ptrs, 1));

	// set contents
	// this starts writing so do it at the end
	LOG2("set section contents" << nl);
	check_bfd_error(bfd_set_section_contents(abfd, section, CM.get_start(),0,CM.size()));

	// close file
	bfd_close(abfd);


	return true;
}

// pass usage
PassUsage& ObjectFileWriterPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(CodeGenPass::ID);
	return PU;
}

// the address of this variable is used to identify the pass
char ObjectFileWriterPass::ID = 0;

// register pass
static PassRegistery<ObjectFileWriterPass> X("ObjectFileWriterPass");

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
