/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_IO_OBJECTINPUTSTREAM_H
#define _JAVA_IO_OBJECTINPUTSTREAM_H

/* Structure information for class: java/io/ObjectInputStream */

typedef struct java_io_ObjectInputStream {
   java_objectheader header;
   struct java_io_DataInputStream* realInputStream;
   struct java_io_DataInputStream* dataInputStream;
   struct java_io_DataInputStream* blockDataInput;
   s4 blockDataPosition;
   s4 blockDataBytes;
   java_bytearray* blockData;
   s4 useSubclassMethod;
   s4 nextOID;
   s4 resolveEnabled;
   struct java_util_Hashtable* objectLookupTable;
   struct java_lang_Object* currentObject;
   struct java_io_ObjectStreamClass* currentObjectStreamClass;
   s4 readDataFromBlock;
   s4 isDeserializing;
   s4 fieldsAlreadyRead;
   struct java_util_Vector* validators;
   struct java_util_Hashtable* classLookupTable;
   struct java_io_ObjectInputStream_GetField* prereadFields;
} java_io_ObjectInputStream;


/*
 * Class:     java/io/ObjectInputStream
 * Method:    currentClassLoader
 * Signature: (Ljava/lang/SecurityManager;)Ljava/lang/ClassLoader;
 */
JNIEXPORT struct java_lang_ClassLoader* JNICALL Java_java_io_ObjectInputStream_currentClassLoader(JNIEnv *env, jclass clazz, struct java_lang_SecurityManager* par1);


/*
 * Class:     java/io/ObjectInputStream
 * Method:    allocateObject
 * Signature: (Ljava/lang/Class;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_io_ObjectInputStream_allocateObject(JNIEnv *env, struct java_io_ObjectInputStream* this, struct java_lang_Class* par1);


/*
 * Class:     java/io/ObjectInputStream
 * Method:    callConstructor
 * Signature: (Ljava/lang/Class;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_io_ObjectInputStream_callConstructor(JNIEnv *env, struct java_io_ObjectInputStream* this, struct java_lang_Class* par1, struct java_lang_Object* par2);

#endif

