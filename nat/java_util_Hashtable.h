/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_UTIL_HASHTABLE_H
#define _JAVA_UTIL_HASHTABLE_H

/* Structure information for class: java/util/Hashtable */

typedef struct java_util_Hashtable {
   java_objectheader header;
   s4 threshold;
   float loadFactor;
   java_objectarray* buckets;
   s4 modCount;
   s4 size;
   struct java_util_Set* keys;
   struct java_util_Collection* values;
   struct java_util_Set* entries;
} java_util_Hashtable;

#endif

