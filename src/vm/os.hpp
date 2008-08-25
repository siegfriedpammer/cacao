/* src/vm/os.hpp - system (OS) functions

   Copyright (C) 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
   Copyright (C) 2008 Theobroma Systems Ltd.

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


#ifndef _OS_HPP
#define _OS_HPP

#include "config.h"

/* NOTE: In this file we check for all system headers, because we wrap
   all system calls into inline functions for better portability. */

#if defined(HAVE_DIRENT_H)
# include <dirent.h>
#endif

#if defined(HAVE_DLFCN_H)
# include <dlfcn.h>
#endif

#if defined(HAVE_ERRNO_H)
# include <errno.h>
#endif

#if defined(HAVE_EXECINFO_H)
# include <execinfo.h>
#endif

#if defined(HAVE_FCNTL_H)
# include <fcntl.h>
#endif

#if defined(ENABLE_JRE_LAYOUT)
# if defined(HAVE_LIBGEN_H)
#  include <libgen.h>
# endif
#endif

#if defined(HAVE_SIGNAL_H)
# include <signal.h>
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


#ifdef __cplusplus

// Class wrapping system (OS) functions.
class os {
public:
	// Inline functions.
	static inline void   abort();
	static inline int    accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
	static inline int    access(const char *pathname, int mode);
	static inline int    atoi(const char* nptr);
	static inline int    backtrace(void** array, int size);
	static inline char** backtrace_symbols(void* const* array, int size) throw ();
	static inline void*  calloc(size_t nmemb, size_t size);
	static inline int    close(int fd);
	static inline int    connect(int sockfd, const struct sockaddr* serv_addr, socklen_t addrlen);
#if defined(ENABLE_JRE_LAYOUT)
	static inline char*  dirname(char* path);
#endif
	static inline int    dlclose(void* handle);
	static inline char*  dlerror(void);
	static inline void*  dlopen(const char* filename, int flag);
	static inline void*  dlsym(void* handle, const char* symbol);
	static inline int    fclose(FILE* fp);
	static inline FILE*  fopen(const char* path, const char* mode);
	static inline size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
	static inline void   free(void* ptr);
	static inline char*  getenv(const char* name);
	static inline int    gethostname(char* name, size_t len);
	static inline int    getpagesize(void);
	static inline int    getsockname(int s, struct sockaddr* name, socklen_t* namelen);
	static inline int    getsockopt(int s, int level, int optname, void* optval, socklen_t* optlen);
	static inline int    listen(int sockfd, int backlog);
	static inline void*  malloc(size_t size);
	static inline void*  memcpy(void* dest, const void* src, size_t n);
	static inline void*  memset(void* s, int c, size_t n);
	static inline int    mprotect(void* addr, size_t len, int prot);
	static inline int    scandir(const char* dir, struct dirent*** namelist, int(*filter)(const struct dirent*), int(*compar)(const void*, const void*));
	static inline int    setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen);
	static inline int    shutdown(int s, int how);
	static inline int    socket(int domain, int type, int protocol);
	static inline int    stat(const char* path, struct stat* buf);
#if defined(__SOLARIS__)
	static inline int    str2sig(const char* str, int* signum);
#endif
	static inline char*  strcat(char* dest, const char* src);
	static inline int    strcmp(const char* s1, const char* s2);
	static inline char*  strcpy(char* dest, const char* src);
	static inline char*  strdup(const char* s);
	static inline size_t strlen(const char* s);
	static inline char*  strerror(int errnum);

	// Convenience functions.
	static void* mmap_anonymous(void *addr, size_t len, int prot, int flags);
	static void  print_backtrace();
	static int   processors_online();
};


// Includes.
#include "toolbox/logging.h"


inline void os::abort(void)
{
#if defined(HAVE_ABORT)
	::abort();
#else
# error abort not available
#endif
}

inline int os::accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
#if defined(HAVE_ACCEPT)
	return ::accept(sockfd, addr, addrlen);
#else
# error accept not available
#endif
}

inline int os::access(const char* pathname, int mode)
{
#if defined(HAVE_ACCESS)
	return ::access(pathname, mode);
#else
# error access not available
#endif
}

inline int os::atoi(const char* nptr)
{
#if defined(HAVE_ATOI)
	return ::atoi(nptr);
#else
# error atoi not available
#endif
}

inline int os::backtrace(void** array, int size)
{
#if defined(HAVE_BACKTRACE)
	return ::backtrace(array, size);
#else
	log_println("os::backtrace: Not available.");
	return 0;
#endif
}

inline char** os::backtrace_symbols(void* const* array, int size) throw ()
{
#if defined(HAVE_BACKTRACE_SYMBOLS)
	return ::backtrace_symbols(array, size);
#else
	log_println("os::backtrace_symbols: Not available.");
	return NULL;
#endif
}

inline void* os::calloc(size_t nmemb, size_t size)
{
#if defined(HAVE_CALLOC)
	return ::calloc(nmemb, size);
#else
# error calloc not available
#endif
}

inline int os::close(int fd)
{
#if defined(HAVE_CLOSE)
	return ::close(fd);
#else
# error close not available
#endif
}

inline int os::connect(int sockfd, const struct sockaddr* serv_addr, socklen_t addrlen)
{
#if defined(HAVE_CONNECT)
	return ::connect(sockfd, serv_addr, addrlen);
#else
# error connect not available
#endif
}

#if defined(ENABLE_JRE_LAYOUT)
inline char* os::dirname(char* path)
{
#if defined(HAVE_DIRNAME)
	return ::dirname(path);
#else
# error dirname not available
#endif
}
#endif

inline int os::dlclose(void* handle)
{
#if defined(HAVE_DLCLOSE)
	return ::dlclose(handle);
#else
# error dlclose not available
#endif
}

inline char* os::dlerror(void)
{
#if defined(HAVE_DLERROR)
	return ::dlerror();
#else
# error dlerror not available
#endif
}

inline void* os::dlopen(const char* filename, int flag)
{
#if defined(HAVE_DLOPEN)
	return ::dlopen(filename, flag);
#else
# error dlopen not available
#endif
}

inline void* os::dlsym(void* handle, const char* symbol)
{
#if defined(HAVE_DLSYM)
	return ::dlsym(handle, symbol);
#else
# error dlsym not available
#endif
}

inline int os::fclose(FILE* fp)
{
#if defined(HAVE_FCLOSE)
	return ::fclose(fp);
#else
# error fclose not available
#endif
}

inline FILE* os::fopen(const char* path, const char* mode)
{
#if defined(HAVE_FOPEN)
	return ::fopen(path, mode);
#else
# error fopen not available
#endif
}

inline size_t os::fread(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
#if defined(HAVE_FREAD)
	return ::fread(ptr, size, nmemb, stream);
#else
# error fread not available
#endif
}

inline void os::free(void* ptr)
{
#if defined(HAVE_FREE)
	::free(ptr);
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

inline char* os::getenv(const char* name)
{
#if defined(HAVE_GETENV)
	return ::getenv(name);
#else
# error getenv not available
#endif
}

inline int os::gethostname(char* name, size_t len)
{
#if defined(HAVE_GETHOSTNAME)
	return ::gethostname(name, len);
#else
# error gethostname not available
#endif
}

inline int os::getpagesize(void)
{
#if defined(HAVE_GETPAGESIZE)
	return ::getpagesize();
#else
# error getpagesize not available
#endif
}

inline int os::getsockname(int s, struct sockaddr* name, socklen_t* namelen)
{
#if defined(HAVE_GETSOCKNAME)
	return ::getsockname(s, name, namelen);
#else
# error getsockname not available
#endif
}

inline int os::getsockopt(int s, int level, int optname, void* optval, socklen_t* optlen)
{
#if defined(HAVE_GETSOCKOPT)
	return ::getsockopt(s, level, optname, optval, optlen);
#else
# error getsockopt not available
#endif
}

inline int os::listen(int sockfd, int backlog)
{
#if defined(HAVE_LISTEN)
	return ::listen(sockfd, backlog);
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

inline void* os::malloc(size_t size)
{
#if defined(HAVE_MALLOC)
	return ::malloc(size);
#else
# error malloc not available
#endif
}

inline void* os::memcpy(void* dest, const void* src, size_t n)
{
#if defined(HAVE_MEMCPY)
	return ::memcpy(dest, src, n);
#else
# error memcpy not available
#endif
}

inline void* os::memset(void* s, int c, size_t n)
{
#if defined(HAVE_MEMSET)
	return ::memset(s, c, n);
#else
# error memset not available
#endif
}

inline int os::mprotect(void* addr, size_t len, int prot)
{
#if defined(HAVE_MPROTECT)
	return ::mprotect(addr, len, prot);
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

inline int os::scandir(const char *dir, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const void *, const void *))
/*
#elif defined(__SOLARIS__)
inline int os::scandir(const char *dir, struct dirent ***namelist, int(*filter)(const struct dirent *), int(*compar)(const struct dirent **, const struct dirent **))
#elif defined(__IRIX__)
inline int os::scandir(const char *dir, struct dirent ***namelist, int(*filter)(dirent_t *), int(*compar)(dirent_t **, dirent_t **))
#else
inline int os::scandir(const char *dir, struct dirent ***namelist, int(*filter)(struct dirent *), int(*compar)(const void *, const void *))
#endif
*/
{
#if defined(HAVE_SCANDIR)
# if defined(__LINUX__)
	return ::scandir(dir, namelist, filter, compar);
#elif defined(__SOLARIS__)
	return ::scandir(dir, namelist, filter, (int (*)(const dirent**, const dirent**)) compar);
# else
	return ::scandir(dir, namelist, (int (*)(struct dirent*)) filter, compar);
# endif
#else
# error scandir not available
#endif
}

inline int os::setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen)
{
#if defined(HAVE_SETSOCKOPT)
	return ::setsockopt(s, level, optname, optval, optlen);
#else
# error setsockopt not available
#endif
}

inline int os::shutdown(int s, int how)
{
#if defined(HAVE_SHUTDOWN)
	return ::shutdown(s, how);
#else
# error shutdown not available
#endif
}

inline int os::socket(int domain, int type, int protocol)
{
#if defined(HAVE_SOCKET)
	return ::socket(domain, type, protocol);
#else
# error socket not available
#endif
}

inline int os::stat(const char* path, struct stat* buf)
{
#if defined(HAVE_STAT)
	return ::stat(path, buf);
#else
# error stat not available
#endif
}

#if defined(__SOLARIS__)
inline int os::str2sig(const char* str, int* signum)
{
#if defined(HAVE_STR2SIG)
	return ::str2sig(str, signum);
#else
# error str2sig not available
#endif
}
#endif

inline char* os::strcat(char* dest, const char* src)
{
#if defined(HAVE_STRCAT)
	return ::strcat(dest, src);
#else
# error strcat not available
#endif
}

inline int os::strcmp(const char* s1, const char* s2)
{
#if defined(HAVE_STRCMP)
	return ::strcmp(s1, s2);
#else
# error strcmp not available
#endif
}

inline char* os::strcpy(char* dest, const char* src)
{
#if defined(HAVE_STRCPY)
	return ::strcpy(dest, src);
#else
# error strcpy not available
#endif
}

inline char* os::strdup(const char* s)
{
#if defined(HAVE_STRDUP)
	return ::strdup(s);
#else
# error strdup not available
#endif
}

inline char* os::strerror(int errnum)
{
#if defined(HAVE_STRERROR)
	return ::strerror(errnum);
#else
# error strerror not available
#endif
}

inline size_t os::strlen(const char* s)
{
#if defined(HAVE_STRLEN)
	return ::strlen(s);
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

#else

void*  os_mmap_anonymous(void *addr, size_t len, int prot, int flags);

void   os_abort(void);
int    os_access(const char* pathname, int mode);
int    os_atoi(const char* nptr);
void*  os_calloc(size_t nmemb, size_t size);
char*  os_dirname(char* path);
int    os_dlclose(void* handle);
char*  os_dlerror(void);
void*  os_dlopen(const char* filename, int flag);
void*  os_dlsym(void* handle, const char* symbol);
int    os_fclose(FILE* fp);
FILE*  os_fopen(const char* path, const char* mode);
size_t os_fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
void   os_free(void* ptr);
int    os_getpagesize(void);
void*  os_memcpy(void* dest, const void* src, size_t n);
void*  os_memset(void* s, int c, size_t n);
int    os_mprotect(void* addr, size_t len, int prot);
int    os_scandir(const char* dir, struct dirent*** namelist, int(*filter)(const struct dirent*), int(*compar)(const void*, const void*));
int    os_stat(const char* path, struct stat* buf);
char*  os_strcat(char* dest, const char* src);
char*  os_strcpy(char* dest, const char* src);
char*  os_strdup(const char* s);
int    os_strlen(const char* s);

#endif

#endif // _OS_HPP


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
