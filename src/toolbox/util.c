/* src/toolbox/util.c - contains some utility functions

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

   $Id: util.c 2063 2005-03-23 11:19:11Z twisti $

*/


#include <errno.h>
#include <unistd.h>

#include "types.h"
#include "mm/memory.h"
#include "vm/exceptions.h"


/* getcwd **********************************************************************

   XXX

*******************************************************************************/

char *_Jv_getcwd(void)
{
	char *buf;
	s4    size;

	size = 1024;

	buf = MNEW(char, size);

	while (buf) {
		if (getcwd(buf, size) != NULL)
			return buf;

		MFREE(buf, char, size);

		/* too small buffer or a more serious problem */

		if (errno != ERANGE)
			throw_cacao_exception_exit(string_java_lang_InternalError,
									   strerror(errno));

		/* double the buffer size */

		size *= 2;

		buf = MNEW(char, size);
	}

	return NULL;
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
