/* This file is machine generated, don't edit it !*/

/*
 * Class:     java_io_VMObjectStreamClass
 * Method:    hasClassInitializer
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMObjectStreamClass_hasClassInitializer (JNIEnv *env , jclass clazz, struct java_lang_Class* par1) {
	log_text("Java_java_io_VMOBjectStreamClass_hasClassInitializer");

	return (class_findmethodIndex (par1, clinit_name(), clinit_desc())!=-1);
}
