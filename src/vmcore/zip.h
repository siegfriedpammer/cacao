/* src/vmcore/zip.c - ZIP file handling for bootstrap classloader

   Copyright (C) 1996-2005, 2006, 2007, 2008
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


#ifndef _ZIP_H
#define _ZIP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "vm/types.h"

#include "toolbox/hashtable.h"

#include "vm/global.h"

#include "vmcore/class.h"
#include "vmcore/loader.h"
#include "vmcore/suck.h"
#include "vmcore/utf8.h"


/* Local file header ***********************************************************

   local file header signature     4 bytes  (0x04034b50)
   version needed to extract       2 bytes
   general purpose bit flag        2 bytes
   compression method              2 bytes
   last mod file time              2 bytes
   last mod file date              2 bytes
   crc-32                          4 bytes
   compressed size                 4 bytes
   uncompressed size               4 bytes
   file name length                2 bytes
   extra field length              2 bytes

   file name (variable size)
   extra field (variable size)

*******************************************************************************/

#define LFH_HEADER_SIZE              30

#define LFH_SIGNATURE                0x04034b50
#define LFH_FILE_NAME_LENGTH         26
#define LFH_EXTRA_FIELD_LENGTH       28

typedef struct lfh lfh;

struct lfh {
	u2 compressionmethod;
	u4 compressedsize;
	u4 uncompressedsize;
	u2 filenamelength;
	u2 extrafieldlength;
};

/* hashtable_zipfile_entry ****************************************************/

typedef struct hashtable_zipfile_entry hashtable_zipfile_entry;

struct hashtable_zipfile_entry {
	utf                     *filename;
	u2                       compressionmethod;
	u4                       compressedsize;
	u4                       uncompressedsize;
	u1                      *data;
	hashtable_zipfile_entry *hashlink;
};


/* function prototypes ********************************************************/

hashtable *zip_open(char *path);
hashtable_zipfile_entry *zip_find(list_classpath_entry *lce, utf *u);
classbuffer *zip_get(list_classpath_entry *lce, classinfo *c);

#ifdef __cplusplus
}
#endif

#endif /* _ZIP_H */


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
