/* src/vm/jit/i386/asmoffsets.h - data segment offsets for i386

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
            Christian Thalinger

   Changes:

   $Id: asmoffsets.h 2946 2005-07-09 12:17:00Z twisti $

*/


#ifndef _ASMOFFSETS_H
#define _ASMOFFSETS_H

/* data segment offsets *******************************************************/

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

#define ExEntrySize     -16
#define ExStartPC       -4
#define ExEndPC         -8
#define ExHandlerPC     -12
#define ExCatchType     -16

#define LineEntrySize   -8
#define LinePC          0
#define LineLine        -4

#endif /* _ASMOFFSETS_H */


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

