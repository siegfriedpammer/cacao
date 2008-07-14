/* src/vmcore/system.h - system (OS) functions

   Copyright (C) 2007, 2008
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


#ifndef _VMCORE_SYSTEM_H
#define _VMCORE_SYSTEM_H

#include "config.h"

/* NOTE: In this file we check for all system headers, because we wrap
   all system calls into inline functions for better portability. */

#if defined(HAVE_DIRENT_H)
# include <dirent.h>
#endif

#if defined(HAVE_DLFCN_H)
# include <dlfcn.h>
#endif

#if defined(HAVE_FCNTL_H)
# include <fcntl.h>
#endif

#if defined(ENABLE_JRE_LAYOUT)
# if defined(HAVE_LIBGEN_H)
#  include <libgen.h>
# endif
#endif

#if defined(HAVE_STDINT_H)
# include <stdint.h>
#endif

#if defined(HAVE_STDIO_H)
# include <stdio.h>
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

#if defined(HAVE_SYS_SOCKET_H)
# include <sys/socket.h>
#endif

#if defined(HAVE_SYS_STAT_H)
# include <sys/stat.h>
#endif

#if defined(HAVE_SYS_TYPES_H)
# include <sys/types.h>
#endif


/* inline functions ***********************************************************/

inline static void system_abort(void)
{
#if defined(HAVE_ABORT)
	abort();
#else
# error abort not available
#endif
}

inline static int system_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
#if defined(HAVE_ACCEPT)
	return accept(sockfd, addr, addrlen);
#else
# error accept not available
#endif
}

inline static int system_access(const char *pathname, int mode)
{
#if defined(HAVE_ACCESS)
	return access(pathname, mode);
#else
# error access not available
#endif
}

inline static int system_atoi(const char *nptr)
{
#if defined(HAVE_ATOI)
	return atoi(nptr);
#else
# error atoi not available
#endif
}

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

inline static int system_connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen)
{
#if defined(HAVE_CONNECT)
	return connect(sockfd, serv_addr, addrlen);
#else
# error connect not available
#endif
}

#if defined(ENABLE_JRE_LAYOUT)
inline static char *system_dirname(char *path)
{
#if defined(HAVE_DIRNAME)
	return dirname(path);
#else
# error dirname not available
#endif
}
#endif

inline static int system_dlclose(void* handle)
{
#if defined(HAVE_DLCLOSE)
	return dlclose(handle);
#else
# error dlclose not available
#endif
}

inline static char* system_dlerror(void)
{
#if defined(HAVE_DLERROR)
	return dlerror();
#else
# error dlerror not available
#endif
}

inline static void* system_dlopen(const char* filename, int flag)
{
#if defined(HAVE_DLOPEN)
	return dlopen(filename, flag);
#else
# error dlopen not available
#endif
}

inline static void* system_dlsym(void* handle, const char* symbol)
{
#if defined(HAVE_DLSYM)
	return dlsym(handle, symbol);
#else
# error dlsym not available
#endif
}

inline static FILE *system_fopen(const char *path, const char *mode)
{
#if defined(HAVE_FOPEN)
	return fopen(path, mode);
#else
# error fopen not available
#endif
}

inline static int system_fclose(FILE *fp)
{
#if defined(HAVE_FCLOSE)
	return fclose(fp);
#else
# error fclose not available
#endif
}

inline static size_t system_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
#if defined(HAVE_FREAD)
	return fread(ptr, size, nmemb, stream);
#else
# error fread not available
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

inline static int system_fsync(int fd)
{
#if defined(HAVE_FSYNC)
	return fsync(fd);
#else
# error fsync not available
#endif
}

inline static int system_ftruncate(int fd, off_t length)
{
#if defined(HAVE_FTRUNCATE)
	return ftruncate(fd, length);
#else
# error ftruncate not available
#endif
}

inline static int system_gethostname(char *name, size_t len)
{
#if defined(HAVE_GETHOSTNAME)
	return gethostname(name, len);
#else
# error gethostname not available
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

inline static int system_getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
#if defined(HAVE_GETSOCKNAME)
	return getsockname(s, name, namelen);
#else
# error getsockname not available
#endif
}

inline static int system_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
#if defined(HAVE_GETSOCKOPT)
	return getsockopt(s, level, optname, optval, optlen);
#else
# error getsockopt not available
#endif
}

inline static int system_listen(int sockfd, int backlog)
{
#if defined(HAVE_LISTEN)
	return listen(sockfd, backlog);
#else
# error listen not available
#endif
}

inline static off_t system_lseek(int fildes, off_t offset, int whence)
{
#if defined(HAVE_LSEEK)
	return lseek(fildes, offset, whence);
#else
# error lseek not available
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

#if defined(__LINUX__)
inline static int system_scandir(const char *dir, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const void *, const void *))
#elif defined(__SOLARIS__)
inline static int system_scandir(const char *dir, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const struct dirent **, const struct dirent **))
#elif defined(__IRIX__)
inline static int system_scandir(const char *dir, struct dirent ***namelist, int(*filter)(dirent_t *), int(*compar)(dirent_t **, dirent_t **))
#else
inline static int system_scandir(const char *dir, struct dirent ***namelist, int(*filter)(struct dirent *), int(*compar)(const void *, const void *))
#endif
{
#if defined(HAVE_SCANDIR)
	return scandir(dir, namelist, filter, compar);
#else
# error scandir not available
#endif
}

inline static int system_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
#if defined(HAVE_SETSOCKOPT)
	return setsockopt(s, level, optname, optval, optlen);
#else
# error setsockopt not available
#endif
}

inline static int system_shutdown(int s, int how)
{
#if defined(HAVE_SHUTDOWN)
	return shutdown(s, how);
#else
# error shutdown not available
#endif
}

inline static int system_socket(int domain, int type, int protocol)
{
#if defined(HAVE_SOCKET)
	return socket(domain, type, protocol);
#else
# error socket not available
#endif
}

inline static int system_stat(const char *path, struct stat *buf)
{
#if defined(HAVE_STAT)
	return stat(path, buf);
#else
# error stat not available
#endif
}

inline static char *system_strcat(char *dest, const char *src)
{
#if defined(HAVE_STRCAT)
	return strcat(dest, src);
#else
# error strcat not available
#endif
}

inline static char *system_strcpy(char *dest, const char *src)
{
#if defined(HAVE_STRCPY)
	return strcpy(dest, src);
#else
# error strcpy not available
#endif
}

inline static char *system_strdup(const char *s)
{
#if defined(HAVE_STRDUP)
	return strdup(s);
#else
# error strdup not available
#endif
}

inline static char *system_strerror(int errnum)
{
#if defined(HAVE_STRERROR)
	return strerror(errnum);
#else
# error strerror not available
#endif
}

inline static size_t system_strlen(const char *s)
{
#if defined(HAVE_STRLEN)
	return strlen(s);
#else
# error strlen not available
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
