/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/io/ObjectStreamClass */

typedef struct java_io_ObjectStreamClass {
   java_objectheader header;
   struct java_lang_String* name;
   struct java_io_ObjectStreamClass* superclass;
   s4 serializable;
   s4 externalizable;
   java_objectarray* fields;
   struct java_lang_Class* ofClass;
   s8 suid;
   s4 primBytes;
   s4 objFields;
   s4 hasWriteObjectMethod;
   s4 hasExternalizableBlockData;
   struct java_lang_reflect_Method* writeObjectMethod;
   struct java_lang_reflect_Method* readObjectMethod;
   struct java_lang_reflect_Method* readResolveMethod;
   struct java_lang_reflect_Method* writeReplaceMethod;
   struct java_io_ObjectStreamClass* localClassDesc;
   s4 disableInstanceDeserialization;
} java_io_ObjectStreamClass;

/*
 * Class:     java/io/ObjectStreamClass
 * Method:    hasStaticInitializer
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_ObjectStreamClass_hasStaticInitializer (JNIEnv *env , struct java_lang_Class* par1);
