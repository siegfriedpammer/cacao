/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_NIO_CHARBUFFERIMPL_H
#define _JAVA_NIO_CHARBUFFERIMPL_H

/* Structure information for class: java/nio/CharBufferImpl */

typedef struct java_nio_CharBufferImpl {
   java_objectheader header;
   s4 cap;
   s4 limit;
   s4 pos;
   s4 mark;
   s4 array_offset;
   java_chararray* backing_buffer;
   s4 readOnly;
} java_nio_CharBufferImpl;

#endif

