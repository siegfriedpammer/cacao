/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/ClassLoader */

typedef struct java_lang_ClassLoader {
   java_objectheader header;
   struct java_util_Map* loadedClasses;
   struct java_util_Map* definedPackages;
   struct java_lang_ClassLoader* parent;
   s4 initialized;
   s4 defaultAssertionStatus;
   struct java_util_Map* packageAssertionStatus;
   struct java_util_Map* classAssertionStatus;
} java_lang_ClassLoader;

