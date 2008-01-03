/* src/vmcore/system.h - system (OS) functions

   Copyright (C) 2007
   CACAOVM - Verein zu Foerderung der freien virtuellen Machine CACAO

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


#ifndef _VMCORE_SYSTEM_H
#define _VMCORE_SYSTEM_H

#include "config.h"

/* NOTE: In this file we check for all system headers, because we wrap
   all system calls into inline functions for better portability. */

#if defined(HAVE_FCNTL_H)
# include <fcntl.h>
#endif

#if defined(HAVE_STDINT_H)
# include <stdint.h>
#endif

#if defined(HAVE_STDLIB_H)
# include <stdlib.h>
#endif

#if defined(HAVE_STRING_H)
# include <string.h>
#endif

#if defined(HAVE_UNISTD_H)
# include <unistd.h>
#endif

#if defined(HAVE_SYS_MMAN_H)
# include <sys/mman.h>
#endif


/* inline functions ***********************************************************/

inline static void *system_calloc(size_t nmemb, size_t size)
{
#if defined(HAVE_CALLOC)
	return calloc(nmemb, size);
#else
# error calloc not available
#endif
}

inline static int system_close(int fd)
{
#if defined(HAVE_CLOSE)
	return close(fd);
#else
# error close not available
#endif
}

inline static void system_free(void *ptr)
{
#if defined(HAVE_FREE)
	free(ptr);
#else
# error free not available
#endif
}

inline static int system_getpagesize(void)
{
#if defined(HAVE_GETPAGESIZE)
	return getpagesize();
#else
# error getpagesize not available
#endif
}

inline static void *system_malloc(size_t size)
{
#if defined(HAVE_MALLOC)
	return malloc(size);
#else
# error malloc not available
#endif
}

inline static void *system_memcpy(void *dest, const void *src, size_t n)
{
#if defined(HAVE_MEMCPY)
	return memcpy(dest, src, n);
#else
# error memcpy not available
#endif
}

inline static void *system_memset(void *s, int c, size_t n)
{
#if defined(HAVE_MEMSET)
	return memset(s, c, n);
#else
# error memset not available
#endif
}

inline static int system_mprotect(void *addr, size_t len, int prot)
{
#if defined(HAVE_MPROTECT)
	return mprotect(addr, len, prot);
#else
# error mprotect not available
#endif
}

inline static int system_open(const char *pathname, int flags, mode_t mode)
{
#if defined(HAVE_OPEN)
	return open(pathname, flags, mode);
#else
# error open not available
#endif
}

inline static ssize_t system_read(int fd, void *buf, size_t count)
{
#if defined(HAVE_READ)
	return read(fd, buf, count);
#else
# error read not available
#endif
}

inline static void *system_realloc(void *ptr, size_t size)
{
#if defined(HAVE_REALLOC)
	return realloc(ptr, size);
#else
# error realloc not available
#endif
}

inline static ssize_t system_write(int fd, const void *buf, size_t count)
{
#if defined(HAVE_WRITE)
	return write(fd, buf, count);
#else
# error write not available
#endif
}


/* function prototypes ********************************************************/

void *system_mmap_anonymous(void *addr, size_t len, int prot, int flags);
int   system_processors_online(void);

#endif /* _VMCORE_SYSTEM_H */


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
