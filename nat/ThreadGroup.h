/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/ThreadGroup */

typedef struct java_lang_ThreadGroup {
   java_objectheader header;
   struct java_lang_ThreadGroup* parent;
   struct java_lang_String* name;
   s4 maxPriority;
   s4 destroyed;
   s4 daemon;
   s4 vmAllowSuspension;
   s4 nthreads;
   java_objectarray* threads;
   s4 ngroups;
   java_objectarray* groups;
} java_lang_ThreadGroup;

