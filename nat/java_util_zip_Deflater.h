/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_UTIL_ZIP_DEFLATER_H
#define _JAVA_UTIL_ZIP_DEFLATER_H

/* Structure information for class: java/util/zip/Deflater */

typedef struct java_util_zip_Deflater {
   java_objectheader header;
   s4 level;
   s4 noHeader;
   s4 strategy;
   s4 state;
   s4 totalOut;
   struct java_util_zip_DeflaterPending* pending;
   struct java_util_zip_DeflaterEngine* engine;
} java_util_zip_Deflater;

#endif

