/* acconfig.h - config.h defines

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

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

   Authors:

   $Id: acconfig.h 664 2003-11-21 18:24:01Z jowenn $

*/


/* Define if mman.h defines MAP_FAILED  */
#undef HAVE_MAP_FAILED

/* Define if mman.h defines MAP_ANONYMOUS  */
#undef HAVE_MAP_ANONYMOUS

#undef TRACE_ARGS_NUM

/* Define to include thread support */
#undef USE_THREADS
#undef EXTERNAL_OVERFLOW
#undef DONT_FREE_FIRST

/* Make automake happy */
#undef PACKAGE
#undef VERSION

/* Architecture directory */
#undef ARCH_DIR

#undef USE_CODEMMAP

#undef USE_GTK

#undef USE_ZLIB


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
