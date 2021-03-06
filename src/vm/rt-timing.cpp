/* src/vm/rt-timing.cpp - POSIX real-time timing utilities

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


#include "config.h"

#include "vm/rt-timing.hpp"

namespace cacao {

OStream& operator<<(OStream &ostr, DurationTy duration) {
	const char *unit;

	auto dur_sec = std::chrono::duration_cast<std::chrono::seconds>(duration);

	if (dur_sec.count() >= 10) {
		// display seconds if at least 10 sec
		ostr << dur_sec.count();
		unit = "sec";
	} else {
		auto dur_milli = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
		auto dur_micro = std::chrono::duration_cast<std::chrono::microseconds>(duration);

		if (dur_milli.count() >= 100) {
			// display milliseconds if at least 100ms
			ostr << dur_milli.count();
			unit = "msec";
		} else {
			// otherwise display microseconds
			ostr << dur_micro.count();
			unit = "usec";
		}
	}
	ostr << setw(5) << unit;
	return ostr;
}

}

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
