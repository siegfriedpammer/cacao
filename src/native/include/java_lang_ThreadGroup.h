/* This file is machine generated, don't edit it! */

#ifndef _JAVA_LANG_THREADGROUP_H
#define _JAVA_LANG_THREADGROUP_H

/* Structure information for class: java/lang/ThreadGroup */

typedef struct java_lang_ThreadGroup {
   java_objectheader header;
   struct java_lang_ThreadGroup* parent;
   struct java_lang_String* name;
   struct java_util_Vector* threads;
   struct java_util_Vector* groups;
   s4 daemon_flag;
   s4 maxpri;
} java_lang_ThreadGroup;

#endif

