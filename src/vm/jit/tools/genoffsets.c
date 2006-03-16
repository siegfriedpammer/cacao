/* src/vm/jit/tools/genoffsets.c - generate asmpart offsets of structures

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

   Authors: Christian Thalinger

   Changes: Edwin Steiner

   $Id: genoffsets.c 4643 2006-03-16 18:38:42Z edwin $

*/


#include "config.h"

#include <stdio.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "vm/class.h"
#include "vm/global.h"
#include "vm/linker.h"
#include "vm/method.h"
#include "vm/vm.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"
#include "vm/jit/replace.h"


int main(int argc, char **argv)
{
	printf("/* This file is machine generated, don't edit it! */\n\n");

    printf("/* define some sizeof()'s */\n\n");

	printf("#define sizevmarg                  %3d\n", (s4) sizeof(vm_arg));
	printf("#define sizestackframeinfo         %3d\n", (s4) sizeof(stackframeinfo));
	printf("#define sizeexecutionstate         %3d\n", (s4) sizeof(executionstate));

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

	printf("#define offvmargtype               %3d\n", (s4) OFFSET(vm_arg, type));
	printf("#define offvmargdata               %3d\n", (s4) OFFSET(vm_arg, data));
	printf("\n\n");

	printf("#define offcast_super_baseval      %3d\n", (s4) OFFSET(castinfo, super_baseval));
	printf("#define offcast_super_diffval      %3d\n", (s4) OFFSET(castinfo, super_diffval));
	printf("#define offcast_sub_baseval        %3d\n", (s4) OFFSET(castinfo, sub_baseval));

	printf("#define offes_pc                   %3d\n", (s4) OFFSET(executionstate, pc));
	printf("#define offes_sp                   %3d\n", (s4) OFFSET(executionstate, sp));
	printf("#define offes_pv                   %3d\n", (s4) OFFSET(executionstate, pv));
	printf("#define offes_intregs              %3d\n", (s4) OFFSET(executionstate, intregs));
	printf("#define offes_fltregs              %3d\n", (s4) OFFSET(executionstate, fltregs));

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
 * vim:noexpandtab:sw=4:ts=4:
 */

  
