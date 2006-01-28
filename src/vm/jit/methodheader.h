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

   Contact: cacao@cacaojvm.org

   Authors: Joseph Wenninger

   Changes: Christian Thalinger

   $Id: methodheader.h 4384 2006-01-28 14:42:58Z twisti $

*/


#ifndef _METHODHEADER_H
#define _METHODHEADER_H

#include "config.h"


/* data segment offsets *******************************************************/

#if SIZEOF_VOID_P == 8

#define MethodPointer           -8
#define FrameSize               -12
#define IsSync                  -16
#define IsLeaf                  -20
#define IntSave                 -24
#define FltSave                 -28
/* 4-byte alignment padding */
#define LineNumberTableSize     -40
#define LineNumberTableStart    -48
/* 4-byte alignment padding */
#define ExTableSize             -56
#define ExTableStart            -56
       
#define ExEntrySize             -32
#define ExStartPC               -8
#define ExEndPC                 -16
#define ExHandlerPC             -24
#define ExCatchType             -32

#else /* SIZEOF_VOID_P == 8 */

#define MethodPointer           -4
#define FrameSize               -8
#define IsSync                  -12
#define IsLeaf                  -16
#define IntSave                 -20
#define FltSave                 -24
#define LineNumberTableSize     -28
#define LineNumberTableStart    -32
#define ExTableSize             -36
#define ExTableStart            -36

#define ExEntrySize             -16
#define ExStartPC               -4
#define ExEndPC                 -8
#define ExHandlerPC             -12
#define ExCatchType             -16

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
