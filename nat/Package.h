/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/Package */

typedef struct java_lang_Package {
   java_objectheader header;
   struct java_lang_String* pkgName;
   struct java_lang_String* specTitle;
   struct java_lang_String* specVersion;
   struct java_lang_String* specVendor;
   struct java_lang_String* implTitle;
   struct java_lang_String* implVersion;
   struct java_lang_String* implVendor;
   struct java_net_URL* sealBase;
} java_lang_Package;

/*
 * Class:     java/lang/Package
 * Method:    getSystemPackage0
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_Package_getSystemPackage0 (JNIEnv *env , struct java_lang_String* par1);
/*
 * Class:     java/lang/Package
 * Method:    getSystemPackages0
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Package_getSystemPackages0 (JNIEnv *env );
