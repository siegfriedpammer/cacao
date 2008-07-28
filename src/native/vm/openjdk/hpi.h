/* src/native/vm/openjdk/hpi.h - HotSpot HPI interface functions

   Copyright (C) 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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


#ifndef _HPI_H
#define _HPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

/* HPI headers *****************************************************************

   We include hpi_md.h before hpi.h as the latter includes the former.

   These includes define:

   #define _JAVASOFT_HPI_MD_H_
   #define _JAVASOFT_HPI_H_

*******************************************************************************/

#include INCLUDE_HPI_MD_H
#include INCLUDE_HPI_H


/* HPI interfaces *************************************************************/

extern HPI_FileInterface    *hpi_file;
extern HPI_SocketInterface  *hpi_socket;
extern HPI_LibraryInterface *hpi_library;
extern HPI_SystemInterface  *hpi_system;


/* functions ******************************************************************/

void hpi_initialize(void);
int  hpi_initialize_socket_library(void);

#ifdef __cplusplus
}
#endif

#endif /* _HPI_H */


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
