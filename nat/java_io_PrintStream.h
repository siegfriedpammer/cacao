/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_IO_PRINTSTREAM_H
#define _JAVA_IO_PRINTSTREAM_H

/* Structure information for class: java/io/PrintStream */

typedef struct java_io_PrintStream {
   java_objectheader header;
   struct java_io_OutputStream* out;
   s4 error_occurred;
   s4 auto_flush;
   struct java_io_PrintWriter* pw;
   s4 closed;
} java_io_PrintStream;

#endif

