/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/ref/Finalizer */

typedef struct java_lang_ref_Finalizer {
   java_objectheader header;
   struct java_lang_Object* referent;
   struct java_lang_ref_ReferenceQueue* queue;
   struct java_lang_ref_Reference* next;
   struct java_lang_ref_Finalizer* next0;
   struct java_lang_ref_Finalizer* prev;
} java_lang_ref_Finalizer;

/*
 * Class:     java/lang/ref/Finalizer
 * Method:    invokeFinalizeMethod
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_ref_Finalizer_invokeFinalizeMethod (JNIEnv *env , struct java_lang_Object* par1);
