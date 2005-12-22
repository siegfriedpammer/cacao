/* This file is machine generated, don't edit it! */

#ifndef _JAVA_UTIL_PROPERTIES_H
#define _JAVA_UTIL_PROPERTIES_H

/* Structure information for class: java/util/Properties */

typedef struct java_util_Properties {
   java_objectheader header;
   s4 threshold;
   float loadFactor;
   java_objectarray* buckets;
   s4 modCount;
   s4 size;
   struct java_util_Set* keys;
   struct java_util_Collection* values;
   struct java_util_Set* entries;
   struct java_util_Properties* defaults;
} java_util_Properties;

#endif

