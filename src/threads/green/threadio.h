/* -*- c -*- */

#ifndef __threadio_h_
#define __threadio_h_

#include <sys/types.h>
#include <sys/socket.h>

#include "global.h"

#ifdef USE_THREADS
int threadedFileDescriptor(int fd);
int threadedSocket(int af, int type, int proto);
int threadedOpen(char* path, int flags, int mode);
int threadedConnect(int fd, struct sockaddr* addr, int len);
int threadedAccept(int fd, struct sockaddr* addr, int* len);
int threadedRead(int fd, char* buf, int len);
int threadedWrite(int fd, char* buf, int len);
int threadedRecvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *addr, int *addrlen);
int threadedSendto(int fd, void *buf, size_t len, int flags, struct sockaddr *addr, int addrlen);
#else
#define threadedFileDescriptor(fd)
#define threadedRead(fd,buf,len)          read(fd,buf,len)
#define threadedWrite(fd,buf,len)         write(fd,buf,len)
#define threadedSocket(af,type,proto)     socket(af,type,proto)
#define threadedAccept(fd,addr,len)       accept(fd,addr,len)
#define threadedRecvfrom(fd,buf,len,flags,addr,addrlen) recvfrom(fd,buf,len,flags,addr,addrlen)
#define threadedSendto(fd,buf,len,flags,addr,addrlen) sendto(fd,buf,len,flags,addr,addrlen)
#define threadedConnect(fd,addr,len)      connect(fd,addr,len)
#endif


#endif
