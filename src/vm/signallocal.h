/* src/vm/signallocal.h - machine independent signal functions

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

   $Id: signallocal.h 6172 2006-12-11 19:43:41Z twisti $

*/


#ifndef _CACAO_SIGNAL_H
#define _CACAO_SIGNAL_H

#include "config.h"

#include <signal.h>


/* function prototypes ********************************************************/

void signal_init(void);

/* machine dependent signal handler */

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p);

#if SUPPORT_HARDWARE_DIVIDE_BY_ZERO
void md_signal_handler_sigfpe(int sig, siginfo_t *siginfo, void *_p);
#endif

void md_signal_handler_sigusr2(int sig, siginfo_t *siginfo, void *_p);

#endif /* _CACAO_SIGNAL_H */


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
