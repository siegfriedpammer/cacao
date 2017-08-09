/* src/vm/jit/compiler2/SSAPrinterPass.hpp - SSAPrinterPass

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

#ifndef _JIT_COMPILER2_SSAPRINTERPASS
#define _JIT_COMPILER2_SSAPRINTERPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "toolbox/Option.hpp"

MM_MAKE_NAME(SSAPrinterPass)

namespace cacao {
namespace jit {
namespace compiler2 {


/**
 * SSAPrinterPass
 * TODO: more info
 */
class SSAPrinterPass : public Pass, public memory::ManagerMixin<SSAPrinterPass> {
public:
	static Option<bool> enabled;
	SSAPrinterPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PA) const;

    virtual bool is_enabled() const {
        return SSAPrinterPass::enabled;
    }
};


/**
 * BasicBlockPrinterPass
 * TODO: more info
 */
class BasicBlockPrinterPass : public Pass, public memory::ManagerMixin<BasicBlockPrinterPass> {
public:
	static Option<bool> enabled;
	BasicBlockPrinterPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PA) const;
    
    virtual bool is_enabled() const {
        return BasicBlockPrinterPass::enabled;
    }

    virtual bool force_scheduling() const {
        return true;
    }
};

extern Option<bool> schedule_printer_enabled;

/**
 * GlobalSchedulePrinterPass
 * TODO: more info
 */
template <class _T>
class GlobalSchedulePrinterPass : public Pass, public memory::ManagerMixin<GlobalSchedulePrinterPass<_T> > {
private:
	static const char* name;
public:
	GlobalSchedulePrinterPass() : Pass() {}
	virtual bool run(JITData &JD);
	virtual PassUsage& get_PassUsage(PassUsage &PA) const;

    virtual bool is_enabled() const {
        return schedule_printer_enabled;
    }

    virtual bool force_scheduling() const {
        return true;
    }
};

class ScheduleEarlyPass;
class ScheduleLatePass;
class ScheduleClickPass;

#if defined(ENABLE_MEMORY_MANAGER_STATISTICS)
namespace memory {
template<>
inline const char* get_class_name<GlobalSchedulePrinterPass<ScheduleEarlyPass> >() {
	return "GlobalSchedulePrinterPass(early)";
}
template<>
inline const char* get_class_name<GlobalSchedulePrinterPass<ScheduleLatePass> >() {
	return "GlobalSchedulePrinterPass(late)";
}
template<>
inline const char* get_class_name<GlobalSchedulePrinterPass<ScheduleClickPass> >() {
	return "GlobalSchedulePrinterPass(click)";
}
} // end namespace memory
#endif /* ENABLE_MEMORY_MANAGER_STATISTICS */

} // end namespace cacao
} // end namespace jit
} // end namespace compiler2

#endif /* _JIT_COMPILER2_SSAPRINTERPASS */


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
