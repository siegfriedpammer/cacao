/* class: java/net/PlainSocketImpl */

#include <sys/socket.h>
#include <netinet/tcp.h>
#include <ioctls.h>		/* a little linux-centric maybe?? (should be in autoconf) */

/*
 * Class:     java/net/PlainSocketImpl
 * Method:    initProto
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_initProto (JNIEnv *env )
{
    if (runverbose)
	log_text("Java_java_net_PlainSocketImpl_initProto called");
}

/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketAccept
 * Signature: (Ljava/net/SocketImpl;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketAccept (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , struct java_net_SocketImpl* sock)
{
	int r;
	int alen;
	struct sockaddr_in addr;

	if (runverbose)
	    log_text("Java_java_net_PlainSocketImpl_socketAccept called");

	alen = sizeof(addr);
#if defined(BSD44)
	addr.sin_len = sizeof(addr);
#endif
	addr.sin_family = AF_INET;
	addr.sin_port = htons(sock->localport);
	addr.sin_addr.s_addr = htonl(sock->address->address);

	r = threadedAccept(this->fd->fd, (struct sockaddr*)&addr, &alen);
	if (r < 0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
	}
	sock->fd->fd = r;

	/* Enter information into socket object */
	alen = sizeof(addr);
	r = getpeername(r, (struct sockaddr*)&addr, &alen);
	if (r < 0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
	}

	sock->address->address = ntohl(addr.sin_addr.s_addr);
	sock->port = ntohs(addr.sin_port);
}

/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketAvailable
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL
Java_java_net_PlainSocketImpl_socketAvailable (JNIEnv *env ,  struct java_net_PlainSocketImpl* this )
{
	int r;
	int len;

	if (runverbose)
	    log_text("Java_java_net_PlainSocketImpl_socketAvailable called");

	r = ioctl(this->fd->fd, FIONREAD, &len);
	if (r < 0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return 0;
	}
	return (s4)len;
}

/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketBind
 * Signature: (Ljava/net/InetAddress;I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketBind (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , struct java_net_InetAddress* laddr, s4 lport)
{
	int r;
	struct sockaddr_in addr;
	int fd;
	int on = 1;
	int alen;

	if (runverbose)
	    log_text("Java_java_net_PlainSocketImpl_socketBind called");

#if defined(BSD44)
	addr.sin_len = sizeof(addr);
#endif
	addr.sin_family = AF_INET;
	addr.sin_port = htons(lport);
	addr.sin_addr.s_addr = htonl(laddr->address);

	fd = this->fd->fd;

	/* Allow rebinding to socket - ignore errors */
	(void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
	r = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	if (r < 0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
	}

	/* Enter information into socket object */
	this->address = laddr;
	if (lport == 0) {
		alen = sizeof(addr);
		r = getsockname(fd, (struct sockaddr*)&addr, &alen);
		if (r < 0) {
			exceptionptr = native_new_and_init (class_java_io_IOException);
			return;
		}
		lport = ntohs(addr.sin_port);
	}
	this->localport = lport;
}

/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketClose (JNIEnv *env ,  struct java_net_PlainSocketImpl* this)
{
	int r;

	if (runverbose)
	    log_text("Java_java_net_PlainSocketImpl_socketClose called");

	if (this->fd->fd != -1) {
		r = close(this->fd->fd);
		this->fd->fd = -1;
		if (r < 0) {
			exceptionptr = native_new_and_init (class_java_io_IOException);
			return;
		}
	}
}

/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketConnect
 * Signature: (Ljava/net/InetAddress;I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketConnect (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , struct java_net_InetAddress* daddr, s4 dport)
{
	int fd;
	int r;
	struct sockaddr_in addr;
	int alen;

	if (runverbose)
	    log_text("Java_java_net_PlainSocketImpl_socketConnect called");

#if defined(BSD44)
	addr.sin_len = sizeof(addr);
#endif
	addr.sin_family = AF_INET;
	addr.sin_port = htons(dport);
	addr.sin_addr.s_addr = htonl(daddr->address);

	fd = this->fd->fd;
	r = threadedConnect(fd, (struct sockaddr*)&addr, sizeof(addr));
	if (r < 0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
	}

	/* Enter information into socket object */
	alen = sizeof(addr);
	r = getsockname(fd, (struct sockaddr*)&addr, &alen);
	if (r < 0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
	}

	this->address = daddr;
	this->port = dport;
	this->localport = ntohs(addr.sin_port);
}

/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketCreate
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketCreate (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , s4 /* bool */ stream)
{
	int fd;
	int type;

	if (runverbose)
	    log_text("Java_java_net_PlainSocketImpl_socketCreate called");

	if (stream == 0) {
		type = SOCK_DGRAM;
	}
	else {
		type = SOCK_STREAM;
	}

	fd = threadedSocket(AF_INET, type, 0);
	if (fd < 0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
	}
	this->fd->fd = fd;
}

/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketGetOption
 * Signature: (I)I
 */
JNIEXPORT s4 JNICALL
Java_java_net_PlainSocketImpl_socketGetOption (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , s4 opt)
{
    int level;
    int optname;
    void *optval;
    int optlen;

    struct linger linger;
    int intopt;

    if (runverbose)
	log_text("Java_java_net_PlainSocketImpl_socketGetOption called");

    switch (opt)
    {
	case /* SO_LINGER */ 0x0080 :
	    level = SOL_SOCKET;
	    optname = SO_LINGER;
	    optval = &linger;
	    optlen = sizeof(struct linger);
	    break;

	case /* TCP_NODELAY */ 0x0001 :
	    level = IPPROTO_TCP;
	    optname = TCP_NODELAY;
	    optval = &intopt;
	    optlen = sizeof(int);
	    break;

	case /* SO_SNDBUF */ 0x1001 :
	case /* SO_RCVBUF */ 0x1002 :
	    level = SOL_SOCKET;
	    optname = (opt == 0x1001) ? SO_SNDBUF : SO_RCVBUF;
	    optval = &intopt;
	    optlen = sizeof(int);
	    break;

	case /* SO_BINDADDR */ 0x000f :
	    return this->address->address;

	default :
	    assert(0);
    }

    if (getsockopt(this->fd->fd, level, optname, optval, &optlen) == -1)
	exceptionptr = native_new_and_init(class_java_net_SocketException);

    switch (optname)
    {
	case SO_LINGER :
	    if (linger.l_onoff)
		return linger.l_linger;
	    return -1;

	case TCP_NODELAY :
	    if (intopt)
		return 1;
	    return -1;		/* -1 == false? */

	case SO_SNDBUF :
	case SO_RCVBUF :
	    return intopt;
    }

    assert(0);

    return 0;
}

/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketListen
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketListen (JNIEnv *env ,  struct java_net_PlainSocketImpl* this , s4 count)
{
	int r;

	if (runverbose)
	    log_text("java_net_PlainSocketImpl_socketListen called");

	r = listen(this->fd->fd, count);
	if (r < 0) {
		exceptionptr = native_new_and_init (class_java_io_IOException);
		return;
	}
}

/*
 * Class:     java/net/PlainSocketImpl
 * Method:    socketSetOption
 * Signature: (IZLjava/lang/Object;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainSocketImpl_socketSetOption (JNIEnv *env,
					       struct java_net_PlainSocketImpl* this , s4 cmd, s4 /* bool */ on, struct java_lang_Object* value)
{
    int level;
    int optname;
    void *optval;
    int optlen;

    struct linger linger;
    int nodelay;
    int bufsize;

    if (runverbose)
	log_text("Java_java_net_PlainSocketImpl_socketSetOption called");

    switch (cmd)
    {
	case /* SO_LINGER */ 0x0080 :
	    if (on)
	    {
		linger.l_onoff = 1;
		linger.l_linger = ((java_lang_Integer*)value)->value;
	    }
	    else
		linger.l_onoff = 0;

	    level = SOL_SOCKET;
	    optname = SO_LINGER;
	    optval = &linger;
	    optlen = sizeof(struct linger);
	    break;

	case /* TCP_NODELAY */ 0x0001 :
	    nodelay = on ? 1 : 0;

	    level = IPPROTO_TCP;
	    optname = TCP_NODELAY;
	    optval = &nodelay;
	    optlen = sizeof(int);
	    break;

	case /* SO_SNDBUF */ 0x1001 :
	case /* SO_RCVBUF */ 0x1002 :
	    bufsize = ((java_lang_Integer*)value)->value;

	    level = SOL_SOCKET;
	    optname = (cmd == 0x1001) ? SO_SNDBUF : SO_RCVBUF;
	    optval = &bufsize;
	    optlen = sizeof(int);
	    break;

	default :
	    assert(0);
    }

    if (setsockopt(this->fd->fd, level, optname, optval, optlen) == -1)
	exceptionptr = native_new_and_init(class_java_net_SocketException);
}
