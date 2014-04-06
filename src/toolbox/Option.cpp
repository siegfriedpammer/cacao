/* src/toolbox/Option.cpp - Command line option parsing library

   Copyright (C) 1996-2014
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

#include "toolbox/Option.hpp"
#include "toolbox/Debug.hpp"
#include "toolbox/OStream.hpp"

namespace cacao {

namespace option {

OptionPrefix& root(){
	static OptionPrefix prefix("-");
	return prefix;
}

OptionPrefix& xx_root(){
	static OptionPrefix prefix("-XX:");
	return prefix;
}

} // end namespace option

OptionPrefix::OptionPrefix(const char* name) : name(name) {
	err() << "OptionPrefix: " << name << nl;
}
void OptionPrefix::insert(OptionEntry* oe) {
	err() << "OptionPrefix::insert: " << oe->get_name() << nl;
	children.insert(oe);
}

void OptionParser::print_usage(OptionPrefix& root, FILE *fp) {
	static const char* blank29 = "                             "; // 29 spaces
	static const char* blank25 = blank29 + 4;                     // 25 spaces
	OStream OS(fp);
	const char* root_name = root.get_name();
	std::size_t root_name_len = std::strlen(root_name);
	std::set<OptionEntry*> sorted(root.begin(),root.end());
	for(std::set<OptionEntry*>::iterator i = sorted.begin(), e = sorted.end();
			i != e; ++i) {
		OptionEntry& oe = **i;
		const char* name = oe.get_name();
		const char* description = oe.get_desc();
		std::size_t name_len = std::strlen(name) + root_name_len;

		OS << "    " << root.get_name() << name;
		if (name_len < (25-1)) {
			OS << (blank25 + name_len);
		} else {
			OS << nl << blank29;
		}
		// TODO description line break
		OS << description << nl;
	}

}

bool OptionParser::parse_option(OptionPrefix& root, const char* name, const char* value) {
	assert(std::strncmp(root.get_name(), name, std::strlen(root.get_name())) == 0);
		//"root name: " << root.get_name() << " name: " << name );
	name += std::strlen(root.get_name());
	for(OptionPrefix::iterator i = root.begin(), e = root.end();
			i != e; ++i) {
		OptionEntry& oe = **i;
		err() << "OptionEntry: " << oe.get_name() << nl;
		err() << "name: " << name << nl;
		if (std::strncmp(oe.get_name(), name, std::strlen(oe.get_name())) == 0) {
			if (oe.parse(value))
				return true;
			return false;
		}
	}
	return false;
}

template<>
bool Option<const char*>::parse(const char* value) {
	err() << "set_value " << get_name() << " := " << value << nl;
	set_value(value);
	err() << "DEBUG: " << DEBUG_COND_WITH_NAME("properties") << nl;
	return true;
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
