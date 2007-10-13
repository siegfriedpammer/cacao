/* src/vm/jit/methodheader.h - method header data segment offsets

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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


#ifndef _METHODHEADER_H
#define _METHODHEADER_H

#include "config.h"


/* data segment offsets *******************************************************/

#if SIZEOF_VOID_P == 8

#define CodeinfoPointer         -8
#define FrameSize               -12
#define IsLeaf                  -16
#define IntSave                 -20
#define FltSave                 -24
#define LineNumberTableSize     -32
#define LineNumberTableStart    -40
/* 4-byte alignment padding */
#define ExTableSize             -48
#define ExTableStart            -48
       
#else /* SIZEOF_VOID_P == 8 */

#define CodeinfoPointer         -4
#define FrameSize               -8
#define IsLeaf                  -12
#define IntSave                 -16
#define FltSave                 -20
#define LineNumberTableSize     -24
#define LineNumberTableStart    -28
#define ExTableSize             -32
#define ExTableStart            -32

#endif /* SIZEOF_VOID_P == 8 */

#endif /* _METHODHEADER_H */


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
