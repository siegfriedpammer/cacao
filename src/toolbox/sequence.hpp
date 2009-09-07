/* src/toolbox/sequence.hpp - sequence builder class header

   Copyright (C) 2009
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
   Copyright (C) 2009 Theobroma Systems Ltd.

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


#ifndef _SEQUENCE_HPP
#define _SEQUENCE_HPP

#include "config.h"

#include <string>


/**
 * This class is a mutable and dynamically growing character sequence.
 * Its main purpose is to dynamically construct Strings or Symbols which
 * both are immutable.
 *
 * The buffer backing up this class is automatically (re)allocated as the
 * sequence grows, no additional length checks are required. Furthermore
 * the buffer memory is released once the builder is destructed and it's
 * content is destroyed. To preserve the content, it has to be exported
 * with one of the exporting functions.
 */
class SequenceBuilder {
private:
	std::string _str;

public:
	// Constructor.
	SequenceBuilder() {}

	// Concatenation operations.
	void cat(char ch)        { _str.push_back(ch); }
	void cat(char ch, int n) { _str.append(n, ch); }
	void cat(const char* s)  { _str.append(s); }

	// Exporting functions.
	//Object* export_string();
	//Symbol* export_symbol();

	// Be careful and see std::string::c_str() for details.
	const char* c_str() const { return _str.c_str(); }
};


#endif // _SEQUENCE_HPP


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
