/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_NIO_DOUBLEBUFFERIMPL_H
#define _JAVA_NIO_DOUBLEBUFFERIMPL_H

/* Structure information for class: java/nio/DoubleBufferImpl */

typedef struct java_nio_DoubleBufferImpl {
   java_objectheader header;
   s4 cap;
   s4 limit;
   s4 pos;
   s4 mark;
   s4 array_offset;
   java_doublearray* backing_buffer;
   s4 readOnly;
} java_nio_DoubleBufferImpl;

#endif

