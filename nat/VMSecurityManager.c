/* class: java/lang/SecurityManager */


#include "jni.h"
#include "builtin.h"
#include "native.h"
#include "tables.h"
#include "toolbox/loging.h"


/*
 * Class:     java/lang/SecurityManager
 * Method:    currentClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT struct java_lang_ClassLoader* JNICALL Java_java_lang_VMSecurityManager_currentClassLoader ( JNIEnv *env, jclass clazz)
{
  init_systemclassloader();
  return SystemClassLoader;
}


/*
 * Class:     java/lang/SecurityManager
 * Method:    getClassContext
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMSecurityManager_getClassContext ( JNIEnv *env ,jclass clazz)
{
  log_text("Java_java_lang_VMSecurityManager_getClassContext  called");
#warning return something more usefull here
  /* XXX should use vftbl directly */
  return builtin_newarray(0,class_array_of(class_java_lang_Class)->vftbl);
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
