/* src/toolbox/endianess.hpp - utilities for reading/writing to/from a 
							   buffer in little or big endian

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

#ifndef CACAO_ENDIANESS_
#define CACAO_ENDIANESS_ 1

#include <cassert>
#include "mm/memory.hpp"
#include "vm/types.hpp"

namespace cacao {
	// ***** READ UNSIGNED, LITTLE ENDIAN ************************

	/// read u1 from pointer little endian
	static inline u1 read_u1_le(const u1 *src);

	/// read u2 from pointer little endian
	static inline u2 read_u2_le(const u1 *src);

	/// read u4 from pointer little endian
	static inline u4 read_u4_le(const u1 *src);

	/// read u8 from pointer little endian
	static inline u8 read_u8_le(const u1 *src);

	// ***** READ SIGNED, LITTLE ENDIAN ************************

	/// read s1 from pointer little endian
	static inline s1 read_s1_le(const u1 *src) { return (s1) read_u1_le(src); }

	/// read s2 from pointer little endian
	static inline s2 read_s2_le(const u1 *src) { return (s2) read_u2_le(src); }

	/// read s4 from pointer little endian
	static inline s4 read_s4_le(const u1 *src) { return (s4) read_u4_le(src); }

	/// read s8 from pointer little endian
	static inline s8 read_s8_le(const u1 *src) { return (s8) read_u8_le(src); }

	// ***** READ UNSIGNED, BIG ENDIAN ************************

	/// read u1 from pointer big endian
	static inline u1 read_u1_be(const u1 *src);

	/// read u2 from pointer big endian
	static inline u2 read_u2_be(const u1 *src);

	/// read u4 from pointer big endian
	static inline u4 read_u4_be(const u1 *src);

	/// read u8 from pointer big endian
	static inline u8 read_u8_be(const u1 *src);

	// ***** READ SIGNED, BIG ENDIAN ************************

	/// read s1 from pointer big endian
	static inline s1 read_s1_be(const u1 *src) { return (s1) read_u1_be(src); }

	/// read s2 from pointer big endian
	static inline s2 read_s2_be(const u1 *src) { return (s2) read_u2_be(src); }

	/// read s4 from pointer big endian
	static inline s4 read_s4_be(const u1 *src) { return (s4) read_u4_be(src); }

	/// read s8 from pointer big endian
	static inline s8 read_s8_be(const u1 *src) { return (s8) read_u8_be(src); }

	// ***** READ FLOATING POINT, BIG ENDIAN ****************

	static inline float  read_float_be(const u1 *src);
	static inline double read_double_be(const u1 *src);
} // end namespace cacao

#if defined(__I386__) || defined(__X86_64__)

/* we can optimize the LE access on little endian machines without alignment */

static inline u1 cacao::read_u1_le(const u1 *src) { return *((u1*) src); }
static inline u2 cacao::read_u2_le(const u1 *src) { return *((u2*) src); }
static inline u4 cacao::read_u4_le(const u1 *src) { return *((u4*) src); }
static inline u8 cacao::read_u8_le(const u1 *src) { return *((u8*) src); }

#else /* defined(__I386__) || defined(__X86_64__) */

static inline u1 cacao::read_u1_le(const u1 *src) {
	return *src;
}

static inline u2 cacao::read_u2_le(const u1 *src) {
	return (((u2) src[1]) << 8) +
	       ((u2) src[0]);
}

static inline u4 cacao::read_u4_le(const u1 *src) {
	return (((u4) src[3]) << 24) +
	       (((u4) src[2]) << 16) +
	       (((u4) src[1]) <<  8) +
	       ((u4) src[0]);
}

static inline u8 cacao::read_u8_le(const u1 *src) {
	return (((u8) src[7]) << 56) +
	       (((u8) src[6]) << 48) +
	       (((u8) src[5]) << 40) +
	       (((u8) src[4]) << 32) +
	       (((u8) src[3]) << 24) +
	       (((u8) src[2]) << 16) +
	       (((u8) src[1]) <<  8) +
	       ((u8) src[0]);
}

#endif /* defined(__I386__) || defined(__X86_64__) */


/* BE macros (for Java class files ) ******************************************/

static inline u1 cacao::read_u1_be(const u1 *src) {
	return *src;
}

static inline u2 cacao::read_u2_be(const u1 *src) {
	return (((u2) src[0]) << 8) +
	       ((u2) src[1]);
}

static inline u4 cacao::read_u4_be(const u1 *src) {
	return (((u4) src[0]) << 24) +
	       (((u4) src[1]) << 16) +
	       (((u4) src[2]) << 8) +
	       ((u4) src[3]);
}

static inline u8 cacao::read_u8_be(const u1 *src) {
	return (((u8) src[0]) << 56) +
	       (((u8) src[1]) << 48) +
	       (((u8) src[2]) << 40) +
	       (((u8) src[3]) << 32) +
	       (((u8) src[4]) << 24) +
	       (((u8) src[5]) << 16) +
	       (((u8) src[6]) << 8) +
	       ((u8) src[7]);
}

static inline float cacao::read_float_be(const u1 *src) {
	assert(sizeof(float) == 4);
	
	float f;

#if WORDS_BIGENDIAN == 0
	u1 buffer[4];
	u2 i;

	for (i = 0; i < 4; i++)
		buffer[3 - i] = cacao::read_u1_be(src + i);

	MCOPY((u1 *) (&f), buffer, u1, 4);
#else
	MCOPY((u1 *) (&f), src,    u1, 4);
#endif
	
	return f;
}

static inline double cacao::read_double_be(const u1 *src) {
	assert(sizeof(double) == 8);

	double d;

#if WORDS_BIGENDIAN == 0
	u1 buffer[8];
	u2 i; 

# if defined(__ARM__) && defined(__ARMEL__) && !defined(__VFP_FP__)
	// On little endian ARM processors when using FPA, word order
	// of doubles is still big endian. So take that into account
	// here. When using VFP, word order of doubles follows byte
	// order. (michi 2005/07/24)

	for (i = 0; i < 4; i++)
		buffer[3 - i] = cacao::read_u1_be(src + i);
	for (i = 0; i < 4; i++)
		buffer[7 - i] = cacao::read_u1_be(src + i + 4);
# else
	for (i = 0; i < 8; i++)
		buffer[7 - i] = cacao::read_u1_be(src + i);
# endif

  MCOPY((u1 *) (&d), buffer, u1, 8);
#else 
  MCOPY((u1 *) (&d), src,    u1, 8);
#endif

	return d;
}

#endif // CACAO_ENDIANESS_

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
