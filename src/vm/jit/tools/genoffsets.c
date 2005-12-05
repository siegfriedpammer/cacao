/* src/vm/jit/tools/genoffsets.c - generate asmpart offsets of structures

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

   Authors: Christian Thalinger

   Changes:

   $Id: genoffsets.c 3890 2005-12-05 22:10:54Z twisti $

*/


#include <stdio.h>

#include "vm/types.h"

#include "vm/global.h"
#include "mm/memory.h"
#include "vm/linker.h"
#include "vm/method.h"
#include "vm/class.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"


int main(int argc, char **argv)
{
	printf("/* This file is machine generated, don't edit it! */\n\n");

    printf("/* define some sizeof()'s */\n\n");

	printf("#define sizejniblock               %3d\n", (s4) sizeof(jni_callblock));
	printf("#define sizestackframeinfo         %3d\n", (s4) sizeof(stackframeinfo));

    printf("\n\n/* define some offsets */\n\n");

	printf("#define offobjvftbl                %3d\n", (s4) OFFSET(java_objectheader, vftbl));
	printf("\n\n");

	printf("/* vftbl_t */\n\n");
	printf("#define offbaseval                 %3d\n", (s4) OFFSET(vftbl_t, baseval));
	printf("#define offdiffval                 %3d\n", (s4) OFFSET(vftbl_t, diffval));
	printf("\n\n");

	printf("/* classinfo */\n\n");
	printf("#define offclassvftbl              %3d\n", (s4) OFFSET(classinfo, vftbl));
	printf("\n\n");

	printf("#define offjniitemtype             %3d\n", (s4) OFFSET(jni_callblock, itemtype));
	printf("#define offjniitem                 %3d\n", (s4) OFFSET(jni_callblock, item));
	printf("\n\n");

	printf("#define offcast_super_baseval      %3d\n", (s4) OFFSET(castinfo, super_baseval));
	printf("#define offcast_super_diffval      %3d\n", (s4) OFFSET(castinfo, super_diffval));
	printf("#define offcast_sub_baseval        %3d\n", (s4) OFFSET(castinfo, sub_baseval));

	/* everything is ok */

	return 0;
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

  
