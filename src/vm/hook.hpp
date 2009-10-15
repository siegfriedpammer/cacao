/* src/vm/hook.hpp - hook points inside the VM

   Copyright (C) 2009
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


#ifndef _HOOK_HPP
#define _HOOK_HPP

#include "config.h"


/**
 * Hook points are inline functions acting as probes scattered throughout
 * several VM subsystems. They can be used to implement event generation
 * or statistics gathering without polluting the source code. Hence all
 * compiler macro and runtime checks should be done in this file. One
 * example of where hooks are useful is JVMTI event firing.
 */
namespace Hook {
	void breakpoint     (Breakpoint *bp);
	void class_linked   (classinfo *c);
	void class_loaded   (classinfo *c);
	void method_enter   (methodinfo *m);
	void method_exit    (methodinfo *m);
	void method_unwind  (methodinfo *m);
	void native_resolved(methodinfo *m, void *symbol, void **symbolptr);
	void thread_start   (threadobject *t);
	void thread_end     (threadobject *t);
	void vm_init        ();
}


inline void Hook::breakpoint(Breakpoint *bp)
{
#if defined(ENABLE_JVMTI)
	methodinfo* m = bp->method;
	int32_t     l = bp->location;

	log_message_method("JVMTI: Reached breakpoint in method ", m);
	log_println("JVMTI: Reached breakpoint at location %d", l);
#endif
}

inline void Hook::class_linked(classinfo *c)
{
	/* nop */
}

inline void Hook::class_loaded(classinfo *c)
{
	/* nop */
}

inline void Hook::method_enter(methodinfo *m)
{
	/* nop */
}

inline void Hook::method_exit(methodinfo *m)
{
	/* nop */
}

inline void Hook::method_unwind(methodinfo *m)
{
	/* nop */
}

inline void Hook::native_resolved(methodinfo *m, void *symbol, void **symbolptr)
{
	/* nop */
}

inline void Hook::thread_start(threadobject *t)
{
	/* nop */
}

inline void Hook::thread_end(threadobject *t)
{
	/* nop */
}

inline void Hook::vm_init()
{
	/* nop */
}


#endif /* _HOOK_HPP */


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
