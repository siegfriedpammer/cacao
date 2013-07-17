/* src/vm/jit/Patcher.cpp - Patcher class

   Copyright (C) 1996-2013
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


#include "vm/jit/Patcher.hpp"
#include "toolbox/logging.hpp"

#include "codegen.hpp"                   /* for PATCHER_CALL_SIZE */

#define DEBUG_NAME "Patcher"
namespace cacao{

Patcher::Patcher() {
	LOG2("Patcher ctor" << nl);
}

Patcher::~Patcher() {
	LOG2("Patcher dtor" << nl);
}

const char* Patcher::get_name() const {
	return "UnknownPatcher";
}

OStream& Patcher::print(OStream &OS) const {
	// Display information about patcher.
	OS << "\tpatcher";
	OS << " type:" << get_name();
	OS << " pc:0x" << (void*) get_mpc();
	#if PATCHER_CALL_SIZE == 4
	OS << " mcode:" << (uint32_t) get_mcode();
	#elif PATCHER_CALL_SIZE == 2
	OS << " mcode:" << (uint16_t) get_mcode();
	#else
	# error Unknown PATCHER_CALL_SIZE
	#endif
	// Display machine code of patched position.
#if 0 && defined(ENABLE_DISASSEMBLER)
	printf("\t\tcurrent -> ");
	disassinstr((uint8_t*) pr.mpc);
	printf("\t\tapplied -> ");
	disassinstr((uint8_t*) &(pr.mcode));
#endif
	return OS;
}


/**
 * patcher_function_list
 *
 * This is a list which maps patcher function pointers to the according
 * names of the patcher functions. It is only usefull for debugging
 * purposes.
 *
 */

#if !defined(NDEBUG)
typedef struct patcher_function_list_t {
	functionptr patcher;
	const char* name;
} patcher_function_list_t;

static patcher_function_list_t patcher_function_list[] = {
	{ PATCHER_initialize_class,              "initialize_class" },
#ifdef ENABLE_VERIFIER
	{ PATCHER_resolve_class,                 "resolve_class" },
#endif /* ENABLE_VERIFIER */
	{ PATCHER_resolve_native_function,       "resolve_native_function" },
	{ PATCHER_invokestatic_special,          "invokestatic_special" },
	{ PATCHER_invokevirtual,                 "invokevirtual" },
	{ PATCHER_invokeinterface,               "invokeinterface" },
	{ PATCHER_breakpoint,                    "breakpoint" },
	{ NULL,                                  "-UNKNOWN PATCHER FUNCTION-" }
};
#endif

const char* LegacyPatcher::get_name() const {
#if !defined(NDEBUG)
	// Lookup name in patcher function list.
	patcher_function_list_t* l;
	for (l = patcher_function_list; l->patcher != NULL; l++)
		if (l->patcher == pr.patcher)
			break;
	return l->name;
#else
	return "UnknownLegacyPatcher(NoDebugBuild)";
#endif
}

OStream& LegacyPatcher::print(OStream &OS) const {
	Patcher::print(OS);
	// Display information about patcher.
	OS << " datap:" << (void*) pr.datap;
	OS << " ref:0x" << (uintptr_t) pr.ref;
	return OS;
}

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

