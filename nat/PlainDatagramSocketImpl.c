/* class: java/net/PlainDatagramSocketImpl */

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    bind
 * Signature: (ILjava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_bind (JNIEnv *env, 
					    struct java_net_PlainDatagramSocketImpl* this,
					    s4 port, struct java_net_InetAddress* laddr)
{
    int r;
    struct sockaddr_in addr;

    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_bind called");

#if defined(BSD44)
    addr.sin_len = sizeof(addr);
#endif
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(laddr->address);

    r = bind(this->fd->fd, (struct sockaddr*)&addr, sizeof(addr));
    if (r < 0) {
	exceptionptr = native_new_and_init (class_java_net_SocketException);
	return;
    }

    this->localPort = addr.sin_port; /* is this correct? */
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    datagramSocketClose
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_datagramSocketClose (JNIEnv *env,  struct java_net_PlainDatagramSocketImpl* this)
{
    int r;

    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_datagramSocketClose called");

    if (this->fd->fd != -1) {
	r = close(this->fd->fd);
	this->fd->fd = -1;
	if (r < 0) {
	    exceptionptr = native_new_and_init (class_java_net_SocketException);
	    return;
	}
    }
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    datagramSocketCreate
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_datagramSocketCreate (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this)
{
    int fd;

    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_datagramSocketCreate called");

    fd = threadedSocket(AF_INET, SOCK_DGRAM, 0);
    this->fd->fd = fd;
    if (fd < 0) {
	exceptionptr = native_new_and_init (class_java_net_SocketException);
	return;
    }
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    getTTL
 * Signature: ()B
 */
JNIEXPORT s4 JNICALL
Java_java_net_PlainDatagramSocketImpl_getTTL (JNIEnv *env, struct java_net_PlainDatagramSocketImpl* this)
{
    return Java_java_net_PlainDatagramSocketImpl_getTimeToLive(env, this);
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    getTimeToLive
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL
Java_java_net_PlainDatagramSocketImpl_getTimeToLive (JNIEnv *env, struct java_net_PlainDatagramSocketImpl* this)
{
    int ttl;
    int size = sizeof(int);

    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_getTimeToLive called");

    if (getsockopt(this->fd->fd, IPPROTO_IP, IP_TTL, &ttl, &size) == -1)
    {
	exceptionptr = native_new_and_init(class_java_io_IOException);
	return 0;
    }

    return ttl;
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_init (JNIEnv *env )
{
    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_init called");
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    join
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_join (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , struct java_net_InetAddress* addr)
{
    struct ip_mreq mreq;

    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_join called");

    mreq.imr_multiaddr.s_addr = htonl(addr->address);
    mreq.imr_interface.s_addr = INADDR_ANY;

    if (setsockopt(this->fd->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq)) == -1)
	exceptionptr = native_new_and_init(class_java_io_IOException);
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    leave
 * Signature: (Ljava/net/InetAddress;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_leave (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , struct java_net_InetAddress* addr)
{
    struct ip_mreq mreq;

    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_leave called");

    mreq.imr_multiaddr.s_addr = htonl(addr->address);
    mreq.imr_interface.s_addr = INADDR_ANY;

    if (setsockopt(this->fd->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(struct ip_mreq)) == -1)
	exceptionptr = native_new_and_init(class_java_io_IOException);
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    peek
 * Signature: (Ljava/net/InetAddress;)I
 */
JNIEXPORT s4 JNICALL
Java_java_net_PlainDatagramSocketImpl_peek (JNIEnv *env ,  struct java_net_PlainDatagramSocketImpl* this , struct java_net_InetAddress* addr)
{
    int r;
    struct sockaddr_in saddr;
    int alen = sizeof(saddr);

    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_peek called");

    r = threadedRecvfrom(this->fd->fd, 0, 0, MSG_PEEK, (struct sockaddr*)&saddr, &alen);
    if (r < 0) {
	exceptionptr = native_new_and_init (class_java_net_SocketException);
	return;
    }

    addr->address = ntohl(saddr.sin_addr.s_addr);

    return (r);
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    receive
 * Signature: (Ljava/net/DatagramPacket;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_receive (JNIEnv *env, struct java_net_PlainDatagramSocketImpl* this, struct java_net_DatagramPacket* pkt)
{
    int r;
    struct sockaddr_in addr;
    int alen = sizeof(addr);

    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_receive called");

    r = threadedRecvfrom(this->fd->fd, pkt->buf->data, pkt->length, 0, (struct sockaddr*)&addr, &alen);
    if (r < 0) {
	exceptionptr = native_new_and_init (class_java_net_SocketException);
	return;
    }

    pkt->length = r;
    pkt->port = ntohs(addr.sin_port);
    if (pkt->address == 0)
	pkt->address = (java_net_InetAddress*)native_new_and_init(loader_load(utf_new_char("java/net/InetAddress")));
    pkt->address->address = ntohl(addr.sin_addr.s_addr);
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    send
 * Signature: (Ljava/net/DatagramPacket;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_send (JNIEnv *env, struct java_net_PlainDatagramSocketImpl* this, struct java_net_DatagramPacket* pkt)
{
    int r;
    struct sockaddr_in addr;

    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_send called");

#if defined(BSD44)
    addr.sin_len = sizeof(addr);
#endif
    addr.sin_family = AF_INET;
    addr.sin_port = htons(pkt->port);
    addr.sin_addr.s_addr = htonl(pkt->address->address);

    r = threadedSendto(this->fd->fd, pkt->buf->data, pkt->length, 0, (struct sockaddr*)&addr, sizeof(addr));
    if (r < 0) {
	exceptionptr = native_new_and_init (class_java_net_SocketException);
	return;
    }
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    setTTL
 * Signature: (B)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_setTTL (JNIEnv *env, struct java_net_PlainDatagramSocketImpl* this, s4 ttl)
{
    Java_java_net_PlainDatagramSocketImpl_setTimeToLive(env, this, ttl);
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    setTimeToLive
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_setTimeToLive (JNIEnv *env, struct java_net_PlainDatagramSocketImpl* this, s4 ttl)
{
    int optval = ttl;

    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_setTimeToLive called");

    if (setsockopt(this->fd->fd, IPPROTO_IP, IP_TTL, &optval, sizeof(int)) == -1)
	exceptionptr = native_new_and_init(class_java_io_IOException);
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    socketGetOption
 * Signature: (I)I
 */
JNIEXPORT s4 JNICALL
Java_java_net_PlainDatagramSocketImpl_socketGetOption (JNIEnv *env, 
						       struct java_net_PlainDatagramSocketImpl* this, s4 opt)
{
    int level;
    int optname;
    void *optval;
    int optlen;

    int intopt;
    struct in_addr in_addr;

    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_bind called");

    switch (opt)
    {
	case /* SO_SNDBUF */ 0x1001 :
	case /* SO_RCVBUF */ 0x1002 :
	    level = SOL_SOCKET;
	    optname = (opt == 0x1001) ? SO_SNDBUF : SO_RCVBUF;
	    optval = &intopt;
	    optlen = sizeof(int);
	    break;

	case /* IP_MULTICAST_IF */ 0x10 :
	    level = IPPROTO_IP;
	    optname = IP_MULTICAST_IF;
	    optval = &in_addr;
	    optlen = sizeof(struct in_addr);
	    break;
	    
	case /* SO_BINDADDR */ 0x000f :
	    {
		struct sockaddr_in sockaddr;
		int size = sizeof(struct sockaddr_in);

		if (getsockname(this->fd->fd, (struct sockaddr*) &sockaddr, &size) == -1)
		{
		    exceptionptr = native_new_and_init(class_java_net_SocketException);
		    return 0;
		}

		return ntohl(sockaddr.sin_addr.s_addr);
	    }
	    break;

	default :
	    assert(0);
    }

    if (getsockopt(this->fd->fd, level, optname, optval, &optlen) == -1)
	exceptionptr = native_new_and_init(class_java_net_SocketException);

    switch (optname)
    {
	case SO_SNDBUF :
	case SO_RCVBUF :
	    return intopt;

	case IP_MULTICAST_IF :
	    return in_addr.s_addr;
    }

    assert(0);

    return 0;
}

/*
 * Class:     java/net/PlainDatagramSocketImpl
 * Method:    socketSetOption
 * Signature: (ILjava/lang/Object;)V
 */
JNIEXPORT void JNICALL
Java_java_net_PlainDatagramSocketImpl_socketSetOption (JNIEnv *env, 
						       struct java_net_PlainDatagramSocketImpl* this, s4 opt, struct java_lang_Object* obj)
{
    int level;
    int optname;
    void *optval;
    int optlen;

    int flag;
    int bufsize;
    struct in_addr in_addr;

    if (runverbose)
	log_text("Java_java_net_PlainDatagramSocketImpl_socketSetOption called");

    switch (opt)
    {
	case /* SO_REUSEADDR */ 0x04 :
	    flag = ((java_lang_Integer*)obj)->value;

	    level = SOL_SOCKET;
	    optname = SO_REUSEADDR;
	    optval = &flag;
	    optlen = sizeof(int);
	    break;

	case /* SO_SNDBUF */ 0x1001 :
	case /* SO_RCVBUF */ 0x1002 :
	    bufsize = ((java_lang_Integer*)obj)->value;

	    level = SOL_SOCKET;
	    optname = (opt == 0x1001) ? SO_SNDBUF : SO_RCVBUF;
	    optval = &level;
	    optlen = sizeof(int);
	    break;

	case /* IP_MULTICAST_IF */ 0x10 :
	    in_addr.s_addr = ((java_net_InetAddress*)obj)->address;

	    level = IPPROTO_IP;
	    optname = IP_MULTICAST_IF;
	    optval = &in_addr;
	    optlen = sizeof(struct in_addr);
	    break;

	default :
	    assert(0);
    }

    if (setsockopt(this->fd->fd, level, optname, optval, optlen) == -1)
	exceptionptr = native_new_and_init(class_java_net_SocketException);
}
