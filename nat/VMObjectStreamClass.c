/* class: java/lang/ObjectStreamClass */


#include "jni.h"
#include "types.h"
#include "loader.h"
#include "toolbox/loging.h"
#include "java_lang_Class.h"


/*
 * Class:     java_io_VMObjectStreamClass
 * Method:    hasClassInitializer
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_io_VMObjectStreamClass_hasClassInitializer(JNIEnv *env, jclass clazz, struct java_lang_Class* par1)
{
	log_text("Java_java_io_VMOBjectStreamClass_hasClassInitializer");

	return (class_findmethodIndex(par1, clinit_name(), clinit_desc()) != -1);
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
