/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/io/ObjectInputStream */

typedef struct java_io_ObjectInputStream {
   java_objectheader header;
   struct java_io_InputStream* in;
   s4 count;
   s4 blockDataMode;
   java_bytearray* buffer;
   struct java_io_DataInputStream* dis;
   struct java_io_IOException* abortIOException;
   struct java_lang_ClassNotFoundException* abortClassNotFoundException;
   struct java_lang_Object* currentObject;
   struct java_io_ObjectStreamClass* currentClassDesc;
   struct java_lang_Class* currentClass;
   struct java_lang_Object* currentGetFields;
   java_bytearray* data;
   java_objectarray* classdesc;
   java_objectarray* classes;
   s4 spClass;
   struct java_util_ArrayList* wireHandle2Object;
   s4 nextWireOffset;
   struct java_util_ArrayList* callbacks;
   s4 recursionDepth;
   s4 currCode;
   s4 enableResolve;
   s4 enableSubclassImplementation;
   java_objectarray* readObjectArglist;
} java_io_ObjectInputStream;

/*
 * Class:     java/io/ObjectInputStream
 * Method:    allocateNewArray
 * Signature: (Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_io_ObjectInputStream_allocateNewArray (JNIEnv *env , struct java_lang_Class* par1, s4 par2);
/*
 * Class:     java/io/ObjectInputStream
 * Method:    allocateNewObject
 * Signature: (Ljava/lang/Class;Ljava/lang/Class;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_io_ObjectInputStream_allocateNewObject (JNIEnv *env , struct java_lang_Class* par1, struct java_lang_Class* par2);
/*
 * Class:     java/io/ObjectInputStream
 * Method:    loadClass0
 * Signature: (Ljava/lang/Class;Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_io_ObjectInputStream_loadClass0 (JNIEnv *env ,  struct java_io_ObjectInputStream* this , struct java_lang_Class* par1, struct java_lang_String* par2);
