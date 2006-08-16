/* src/toolbox/bitvector.c - bitvector implementation

   Copyright (C) 2005, 2006 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Christian Ullrich

   $Id: bitvector.c$

*/

#include "mm/memory.h"
#include "toolbox/bitvector.h"


/******************************************************************************

Bitvector Implementation


******************************************************************************/

#ifdef BV_DEBUG_CHECK
  /* Number of ints needed for size bits */
# define BV_NUM_INTS(size) (((((size) + 7)/ 8) + sizeof(int) - 1) / sizeof(int) + 1)
  /* Get index in bitvector */
# define BV_INT_INDEX(bit) ( ((bit) / 8) / sizeof(int) + 1)
  /* Get bit index inside int */
# define BV_BIT_INDEX(bit, index) ( (bit) - (index - 1) * sizeof(int) * 8 );
#else
  /* Number of ints needed for size bits */
# define BV_NUM_INTS(size) (((((size) + 7)/ 8) + sizeof(int) - 1) / sizeof(int))
  /* Get index in bitvector */
# define BV_INT_INDEX(bit) ( ((bit) / 8) / sizeof(int) )
  /* Get bit index inside int */
# define BV_BIT_INDEX(bit, index) ( (bit) - (index) * sizeof(int) * 8 );
#endif


char *bv_to_string(bitvector bv, char *string, int size) {
	int i;

	_BV_ASSERT(bv[0] == size);

	for(i = 0; i < size; i++) 
		if (bv_get_bit(bv, i))
			string[i]='1';
		else
			string[i]='0';

	string[i]=0;
	return string;
}

int *bv_new(int size) {
	int i,n;
	int *bv;

	/* Number of ints needed for size bits */
/* 	n = (((size+7)/8) + sizeof(int) - 1)/sizeof(int);  */
	n = BV_NUM_INTS(size);

	bv = DMNEW(int, n);

	for(i = 0; i < n; i++) bv[i] = 0;
   
#ifdef BV_DEBUG_CHECK
	bv[0] = size;
#endif

	return bv;
}

bool bv_get_bit(bitvector bv, int bit) {
	int i, n;

	_BV_ASSERT(bit >= 0);

	i = BV_INT_INDEX(bit);
	n = BV_BIT_INDEX(bit, i);

	_BV_ASSERT(i < (BV_NUM_INTS(bv[0])));
	return (bv[i] & (1<<n));
}

void bv_set_bit(bitvector bv, int bit) {
	int i, n;

	_BV_ASSERT(bit >= 0);

	i = BV_INT_INDEX(bit);
	n = BV_BIT_INDEX(bit, i);

	_BV_ASSERT(i < BV_NUM_INTS(bv[0]));

	bv[i] |= 1<<n;
}

void bv_reset_bit(bitvector bv, int bit) {
	int i, n;

	_BV_ASSERT(bit >= 0);

	i = BV_INT_INDEX(bit);
	n = BV_BIT_INDEX(bit, i);

	_BV_ASSERT(i < BV_NUM_INTS(bv[0]));

	bv[i] &= ~(1<<n);
}

void bv_reset(bitvector bv, int size) {
	int i,n;

	_BV_ASSERT(bv[0] == size);

	n = BV_NUM_INTS(size);

#ifdef BV_DEBUG_CHECK
	for(i = 1; i < n; i++) 
#else
	for(i = 0; i < n; i++) 
#endif
		bv[i] = 0;
}

bool bv_is_empty(bitvector bv, int size) {
	int i,n;
	bool empty;

	_BV_ASSERT(bv[0] == size);

	n = BV_NUM_INTS(size);

	empty = true;
#ifdef BV_DEBUG_CHECK
	for(i = 1; (i < n) && empty; i++) 
#else
	for(i = 0; (i < n) && empty; i++) 
#endif
		empty = empty && (bv[i] == 0);
	return empty;
}

void bv_copy(bitvector dst, bitvector src, int size) {
	int i,n;
	/* copy the whole bitvector    */
	_BV_ASSERT(dst[0] == size);
	_BV_ASSERT(src[0] == size);
	n = BV_NUM_INTS(size);

#ifdef BV_DEBUG_CHECK
	for(i = 1; i < n; i++) 
#else
	for(i = 0; i < n; i++) 
#endif
		dst[i] = src[i];
}

bool bv_equal(bitvector s1, bitvector s2, int size) {
	int i,n;
	int mask;
	bool equal = true;
	/* copy the whole bitvector    */
	_BV_ASSERT(s1[0] == size);
	_BV_ASSERT(s2[0] == size);

	if (size == 0)
		return true;

	n = BV_NUM_INTS(size);

#ifdef BV_DEBUG_CHECK
	for(i = 1; equal && (i < n-1); i++) 
#else
	for(i = 0; equal && (i < n-1); i++) 
#endif
		equal = (s1[i] == s2[i]);

	/* Last compare maybe has to be masked! */

	i = BV_INT_INDEX(size - 1);
	n = BV_BIT_INDEX(size - 1, i);

	_BV_ASSERT(i < BV_NUM_INTS(s1[0]));
	_BV_ASSERT(i < BV_NUM_INTS(s2[0]));

	if (n == (sizeof(int) * 8 - 1)) {
		/* full mask */
		mask = -1;
	} else {
		mask = (1<<(n+1)) - 1;
	}
	
	equal = equal && ( (s1[i]&mask) == (s2[i]&mask));

	return equal;
}

void bv_minus(bitvector d, bitvector s1, bitvector s2, int size) {
	int i,n;
    /* d = s1 - s2     */
	_BV_ASSERT(d[0] == size);
	_BV_ASSERT(s1[0] == size);
	_BV_ASSERT(s2[0] == size);
	n = BV_NUM_INTS(size);
#ifdef BV_DEBUG_CHECK
	for(i = 1; i < n; i++) 
#else
	for(i = 0; i < n; i++) 
#endif
		d[i] = s1[i] & (~s2[i]);
}

void bv_union(bitvector d, bitvector s1, bitvector s2, int size) {
	int i,n;
    /* d = s1 union s2 */
	_BV_ASSERT(d[0] == size);
	_BV_ASSERT(s1[0] == size);
	_BV_ASSERT(s2[0] == size);
	n = BV_NUM_INTS(size);
#ifdef BV_DEBUG_CHECK
	for(i = 1; i < n; i++) 
#else
	for(i = 0; i < n; i++) 
#endif
		d[i] = s1[i] | s2[i];
}

/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
