/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_LANG_THREAD_H
#define _JAVA_LANG_THREAD_H

/* Structure information for class: java/lang/Thread */

typedef struct java_lang_Thread {
   java_objectheader header;
   struct java_lang_VMThread* vmThread;
   struct java_lang_ThreadGroup* group;
   struct java_lang_Runnable* runnable;
   struct java_lang_String* name;
   s4 daemon;
   s4 priority;
   s8 stacksize;
   struct java_lang_Throwable* stillborn;
   struct java_lang_ClassLoader* contextClassLoader;
} java_lang_Thread;

#endif

