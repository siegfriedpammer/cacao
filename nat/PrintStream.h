/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/io/PrintStream */

typedef struct java_io_PrintStream {
   java_objectheader header;
   struct java_io_OutputStream* out;
   s4 autoFlush;
   s4 trouble;
   struct java_io_BufferedWriter* textOut;
   struct java_io_OutputStreamWriter* charOut;
   s4 closing;
} java_io_PrintStream;

