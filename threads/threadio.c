/*
 * threadCalls.c
 * Support for threaded ops which may block (read, write, connect, etc.).
 *
 * Copyright (c) 1996 T. J. Wilkinson & Associates, London, UK.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Written by Tim Wilkinson <tim@tjwassoc.demon.co.uk>, 1996.
 */

#define	DBG(s)                    

#include "sysdep/defines.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include "thread.h"

/*
 * We only need this stuff is we are using the internal thread system.
 */
#if defined(USE_INTERNAL_THREADS)

#define	TH_READ		0
#define	TH_WRITE	1
#define	TH_ACCEPT	TH_READ
#define	TH_CONNECT	TH_WRITE

static int maxFd;
static fd_set readsPending;
static fd_set writesPending;
static thread* readQ[FD_SETSIZE];
static thread* writeQ[FD_SETSIZE];
static struct timeval tm = { 0, 0 };

void blockOnFile(int, int);
void waitOnEvents(void);

extern thread* currentThread;

/* These are undefined because we do not yet support async I/O */
#undef	F_SETOWN
#undef	FIOSETOWN
#undef	O_ASYNC
#undef	FIOASYNC

/*
 * Create a threaded file descriptor.
 */
int
threadedFileDescriptor(int fd)
{
#if !defined(BLOCKING_CALLS)
    int r;
    int on = 1;
    int pid;

    /* Make non-blocking */
#if defined(HAVE_FCNTL) && defined(O_NONBLOCK)
    r = fcntl(fd, F_GETFL, 0);
    r = fcntl(fd, F_SETFL, r|O_NONBLOCK);
#elif defined(HAVE_IOCTL) && defined(FIONBIO)
    r = ioctl(fd, FIONBIO, &on);
#else
    r = 0;
#endif
    if (r < 0)
    {
	return (r);
    }

    /* Allow socket to signal this process when new data is available */
    pid = getpid();
#if defined(HAVE_FCNTL) && defined(F_SETOWN)
    r = fcntl(fd, F_SETOWN, pid);
#elif defined(HAVE_IOCTL) && defined(FIOSETOWN)
    r = ioctl(fd, FIOSETOWN, &pid);
#else
    r = 0;
#endif
    if (r < 0)
    {
	return (r);
    }

#if defined(HAVE_FCNTL) && defined(O_ASYNC)
    r = fcntl(fd, F_GETFL, 0);
    r = fcntl(fd, F_SETFL, r|O_ASYNC);
#elif defined(HAVE_IOCTL) && defined(FIOASYNC)
    r = ioctl(fd, FIOASYNC, &on);
#else
    r = 0;
#endif
    if (r < 0)
    {
	return (r);
    }
#endif
    return (fd);
}

void clear_thread_flags(void)
{
#if !defined(BLOCKING_CALLS)
	int fl, fd;

#if defined(HAVE_FCNTL) && defined(O_NONBLOCK)
	fd = fileno(stdin);
    fl = fcntl(fd, F_GETFL, 0);
    fl = fcntl(fd, F_SETFL, fl & (~O_NONBLOCK));

	fd = fileno(stdout);
    fl = fcntl(fd, F_GETFL, 0);
    fl = fcntl(fd, F_SETFL, fl & (~O_NONBLOCK));

	fd = fileno(stderr);
    fl = fcntl(fd, F_GETFL, 0);
    fl = fcntl(fd, F_SETFL, fl & (~O_NONBLOCK));
#elif defined(HAVE_IOCTL) && defined(FIONBIO)
	fl = 0;
	fd = fileno(stdin);
    (void) ioctl(fd, FIONBIO, &fl);

	fd = fileno(stdout);
    (void) ioctl(fd, FIONBIO, &fl);

	fd = fileno(stderr);
    (void) ioctl(fd, FIONBIO, &fl);
#endif

#endif
fflush (stdout);
fflush (stderr);
}


/*
 * Threaded create socket.
 */
int
threadedSocket(int af, int type, int proto)
{
    int fd;
    int r;
    int on = 1;
    int pid;

    fd = socket(af, type, proto);
    return (threadedFileDescriptor(fd));
}

/*
 * Threaded file open.
 */
int
threadedOpen(char* path, int flags, int mode)
{
    int fd;
    int r;
    int on = 1;
    int pid;

    fd = open(path, flags, mode);
    return (threadedFileDescriptor(fd));
}

/*
 * Threaded socket connect.
 */
int
threadedConnect(int fd, struct sockaddr* addr, int len)
{
    int r;

    r = connect(fd, addr, len);
#if !defined(BLOCKING_CALLS)
    if ((r < 0)
	&& (errno == EINPROGRESS || errno == EALREADY
	    || errno == EWOULDBLOCK)) {
	blockOnFile(fd, TH_CONNECT);
	r = 0; /* Assume it's okay when we get released */
    }
#endif

    return (r);
}

/*
 * Threaded socket accept.
 */
int
threadedAccept(int fd, struct sockaddr* addr, int* len)
{
    int r;
    int on = 1;

    for (;;)
    {
#if defined(BLOCKING_CALLS)
	blockOnFile(fd, TH_ACCEPT);
#endif
	r = accept(fd, addr, len);
	if (r >= 0
	    || !(errno == EINPROGRESS || errno == EALREADY
		 || errno == EWOULDBLOCK))
	{
	    break;
	}
#if !defined(BLOCKING_CALLS)
	blockOnFile(fd, TH_ACCEPT);
#endif
    }
    return (threadedFileDescriptor(r));
}

/*
 * Read but only if we can.
 */
int
threadedRead(int fd, char* buf, int len)
{
    int r;

    DBG(   printf("threadedRead\n");          )

#if defined(BLOCKING_CALLS)
    blockOnFile(fd, TH_READ);
#endif
    for (;;)
    {
	r = read(fd, buf, len);
	if (r < 0
	    && (errno == EAGAIN || errno == EWOULDBLOCK
		|| errno == EINTR))
	{
	    blockOnFile(fd, TH_READ);
	    continue;
	}
	return (r);
    }
}

/*
 * Write but only if we can.
 */
int
threadedWrite(int fd, char* buf, int len)
{
    int r;
    char* ptr;

    ptr = buf;
    r = 1;

    DBG(    printf("threadedWrite %dbytes\n",len);      )

    while (len > 0 && r > 0)
    {
#if defined(BLOCKING_CALLS)
	blockOnFile(fd, TH_WRITE);
#endif
	r = write(fd, ptr, len);
	if (r < 0
	    && (errno == EAGAIN || errno == EWOULDBLOCK
		|| errno == EINTR))
	{
#if !defined(BLOCKING_CALLS)
	    blockOnFile(fd, TH_WRITE);
#endif
	    r = 1;
	}
	else
	{
	    ptr += r;
	    len -= r;
	}
    }
    return (ptr - buf);
}

/*
 * An attempt to access a file would block, so suspend the thread until
 * it will happen.
 */
void
blockOnFile(int fd, int op)
{
DBG(	printf("blockOnFile()\n");					)

    intsDisable();

    if (fd > maxFd)
    {
	maxFd = fd;
    }
    if (op == TH_READ)
    {
	FD_SET(fd, &readsPending);
	suspendOnQThread(currentThread, &readQ[fd]);
	FD_CLR(fd, &readsPending);
    }
    else
    {
	FD_SET(fd, &writesPending);
	suspendOnQThread(currentThread, &writeQ[fd]);
	FD_CLR(fd, &writesPending);
    }

    intsRestore();
}

/*
 * Check if some file descriptor or other event to become ready.
 * Block if required (but make sure we can still take timer interrupts).
 */
void
checkEvents(bool block)
{
    int r;
    fd_set rd;
    fd_set wr;
    thread* tid;
    thread* ntid;
    int i;
    int b;

DBG(	printf("checkEvents block:%d\n", block);			)

#if defined(FD_COPY)
    FD_COPY(&readsPending, &rd);
    FD_COPY(&writesPending, &wr);
#else
    memcpy(&rd, &readsPending, sizeof(rd));
    memcpy(&wr, &writesPending, sizeof(wr));
#endif

    /* 
     * If select() is called with indefinite wait, we have to make sure
     * we can get interrupted by timer events. 
     */
    if (block == true)
    {
	b = blockInts;
	blockInts = 0;
	r = select(maxFd+1, &rd, &wr, 0, 0);
	blockInts = b;
    }
    else
    {
	r = select(maxFd+1, &rd, &wr, 0, &tm);
    }

    /* We must be holding off interrupts before we start playing with
     * the read and write queues.  This should be already done but a
     * quick check never hurt anyone.
     */
    assert(blockInts > 0);

DBG(	printf("Select returns %d\n", r);				)

    for (i = 0; r > 0 && i <= maxFd; i++)
    {
	if (readQ[i] != 0 && FD_ISSET(i, &rd))
	{
	    for (tid = readQ[i]; tid != 0; tid = ntid)
	    {
		ntid = tid->next;
		iresumeThread(tid);
	    }
	    readQ[i] = 0;
	    r--;
	}
	if (writeQ[i] != 0 && FD_ISSET(i, &wr))
	{
	    for (tid = writeQ[i]; tid != 0; tid = ntid)
	    {
		ntid = tid->next;
		iresumeThread(tid);
	    }
	    writeQ[i] = 0;
	    r--;
	}
    }
}
#endif
