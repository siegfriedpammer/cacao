/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_LANG_THROWABLE_H
#define _JAVA_LANG_THROWABLE_H

/* Structure information for class: java/lang/Throwable */

typedef struct java_lang_Throwable {
   java_objectheader header;
   struct java_lang_String* detailMessage;
   struct java_lang_Throwable* cause;
   java_objectarray* stackTrace;
   struct java_lang_VMThrowable* vmState;
} java_lang_Throwable;

#endif

