/* tests/compiler2/catch/framework/CacaoVM.hpp - CacaoVM

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

#ifndef _TESTS_COMPILER2_CATCH_FRAMEWORK_CACAOVM
#define _TESTS_COMPILER2_CATCH_FRAMEWORK_CACAOVM

/**
 * The code is copied from the main cacao.cpp and is used
 * to initialize a basic CACAO VM so we can utilize the class loader.
 * This VM is NOT used to execute anything, so one instance for all
 * of the tests is enough!
 */
class CacaoVM {
public:
	static void initialize();

private:
	static bool initialized;
};

#endif /* _TESTS_COMPILER2_CATCH_FRAMEWORK_CACAOVM */

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
