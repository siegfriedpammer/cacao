/* class: java/lang/ClassLoader */


#include "jni.h"
#include "loader.h"
#include "native.h"
#include "builtin.h"
#include "toolbox/loging.h"
#include "java_lang_Class.h"
#include "java_lang_String.h"
#include "java_lang_ClassLoader.h"


/*
 * Class:     java/lang/ClassLoader
 * Method:    defineClass0
 * Signature: (Ljava/lang/String;[BII)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClassLoader_defineClass ( JNIEnv *env ,  jclass clazz, struct java_lang_ClassLoader* this, struct java_lang_String* name, java_bytearray* buf, s4 off, s4 len)
{
    classinfo *c;

    log_text("Java_java_lang_VMClassLoader_defineClass called");

    /* call JNI-function to load the class */
    c = (*env)->DefineClass(env, javastring_tochar((java_objectheader*) name), (jobject) this, (const jbyte *) &buf[off], len);
    use_class_as_object (c);    
    return (java_lang_Class*) c;
}

/*
 * Class:     java/lang/Class
 * Method:    getPrimitiveClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClassLoader_getPrimitiveClass ( JNIEnv *env ,  jclass clazz, struct java_lang_String* name)
{
    classinfo *c;
    utf *u = javastring_toutf(name, false);

    if (u) {    	
      /* get primitive class */
		c = loader_load_sysclass(NULL,u);
      use_class_as_object (c);
      return (java_lang_Class*) c;      
    }

    /* illegal primitive classname specified */
    *exceptionptr = native_new_and_init (class_java_lang_ClassNotFoundException);
    return NULL;
}

/*
 * Class:     java/lang/ClassLoader
 * Method:    resolveClass0
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClassLoader_resolveClass ( JNIEnv *env ,   jclass clazz, struct java_lang_Class* par1)
{
  /* class already linked, so return */
  return;
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
