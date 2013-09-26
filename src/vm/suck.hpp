/* src/vm/suck.hpp - functions to read LE ordered types from a buffer

   Copyright (C) 1996-2005, 2006, 2007, 2008
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


#ifndef SUCK_HPP_
#define SUCK_HPP_ 1

#include "config.h"
#include <list>
#include "vm/types.hpp"

struct classbuffer;
struct classinfo;
struct hashtable;
class Mutex;
class ZipFile;

/* list_classpath_entry *******************************************************/

enum {
	CLASSPATH_PATH,
	CLASSPATH_ARCHIVE
};

typedef struct list_classpath_entry {
	Mutex             *mutex;	        /* mutex locking on zip/jar files */
	s4                 type;
	char              *path;
	s4                 pathlen;
#if defined(ENABLE_ZLIB)
	ZipFile           *zip;
#endif
} list_classpath_entry;

/**
 * Classpath entries list.
 */
class SuckClasspath : protected std::list<list_classpath_entry*> {
public:
	void add(char *classpath);
	void add_from_property(const char *key);

	// make iterator of std::list visible
	using std::list<list_classpath_entry*>::iterator;

	// make functions of std::list visible
	using std::list<list_classpath_entry*>::begin;
	using std::list<list_classpath_entry*>::end;
};


/* signed suck defines ********************************************************/

#define suck_s1(a)    (s1) suck_u1((a))
#define suck_s2(a)    (s2) suck_u2((a))
#define suck_s4(a)    (s4) suck_u4((a))
#define suck_s8(a)    (s8) suck_u8((a))


/* function prototypes ********************************************************/

bool suck_check_classbuffer_size(classbuffer *cb, s4 len);

u1 suck_u1(classbuffer *cb);
u2 suck_u2(classbuffer *cb);
u4 suck_u4(classbuffer *cb);
u8 suck_u8(classbuffer *cb);

float suck_float(classbuffer *cb);
double suck_double(classbuffer *cb);

void suck_nbytes(u1 *buffer, classbuffer *cb, s4 len);
void suck_skip_nbytes(classbuffer *cb, s4 len);

classbuffer *suck_start(classinfo *c);

void suck_stop(classbuffer *cb);

#endif // SUCK_HPP_


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
