/* vm/jit/mips/asmoffsets.h - data segment offsets for MIPS

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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

   Authors: Andreas Krall

   Changes:

   $Id: asmoffsets.h 1735 2004-12-07 14:33:27Z twisti $

*/


#ifndef _ASMOFFSETS_H
#define _ASMOFFSETS_H

/* data segment offsets *******************************************************/

#define MethodPointer           -8
#define FrameSize               -12
#define IsSync                  -16
#define IsLeaf                  -20
#define IntSave                 -24
#define FltSave                 -28
#define LineNumberTableSize     0    /* XXX dummy entries */
#define LineNumberTableStart    0    /* XXX dummy entries */
#define ExTableSize             -32
#define ExTableStart            -32

#define ExEntrySize             -32
#define ExStartPC               -8
#define ExEndPC                 -16
#define ExHandlerPC             -24
#define ExCatchType             -32

#endif /* _ASMOFFSETS_H */


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: asm
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
