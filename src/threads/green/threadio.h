/* -*- c -*- */

#ifndef __threadio_h_
#define __threadio_h_

#include <sys/types.h>
#include <sys/socket.h>

#include "../global.h"

#ifdef USE_THREADS
int threadedFileDescriptor(int fd);
int threadedSocket(int af, int type, int proto);
int threadedOpen(char* path, int flags, int mode);
int threadedConnect(int fd, struct sockaddr* addr, int len);
int threadedAccept(int fd, struct sockaddr* addr, int* len);
int threadedRead(int fd, char* buf, int len);
int threadedWrite(int fd, char* buf, int len);
#else
#define threadedFileDescriptor(fd)
#define threadedRead(fd,buf,len)          read(fd,buf,len)
#define threadedWrite(fd,buf,len)         write(fd,buf,len)
#endif


#endif
