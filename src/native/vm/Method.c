#include "helpers.h"
/* class: java/lang/reflect/Method */

/*
 * Class:     java_lang_reflect_Method
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Method_getModifiers (JNIEnv *env ,  struct java_lang_reflect_Method* this )
{
	/*log_text("Java_java_lang_reflect_Method_getModifiers called");*/
	classinfo *c=(classinfo*)(this->declaringClass);
	if ((this->slot<0) || (this->slot>=c->methodscount))
		panic("error illegal slot for method in class (getReturnType)");
	return (c->methods[this->slot]).flags &
                  (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_ABSTRACT | ACC_STATIC | ACC_FINAL |
                   ACC_SYNCHRONIZED | ACC_NATIVE | ACC_STRICT);

}

/*
 * Class:     java_lang_reflect_Method
 * Method:    getReturnType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_reflect_Method_getReturnType (JNIEnv *env ,  struct java_lang_reflect_Method* this )
{
/*	log_text("Java_java_lang_reflect_Method_getReturnType called");*/
	classinfo *c=(classinfo*)(this->declaringClass);
	if ((this->slot<0) || (this->slot>=c->methodscount))
		panic("error illegal slot for method in class (getReturnType)");
	return get_returntype(&(c->methods[this->slot]));

}

/*
 * Class:     java_lang_reflect_Method
 * Method:    getParameterTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_reflect_Method_getParameterTypes (JNIEnv *env ,  struct java_lang_reflect_Method* this )
{
/*	log_text("Java_java_lang_reflect_Method_getParameterTypes called");*/
	classinfo *c=(classinfo*)(this->declaringClass);
	if ((this->slot<0) || (this->slot>=c->methodscount))
		panic("error illegal slot for method in class(getParameterTypes)");
	return get_parametertypes(&(c->methods[this->slot]));
}

/*
 * Class:     java_lang_reflect_Method
 * Method:    getExceptionTypes
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_reflect_Method_getExceptionTypes (JNIEnv *env ,  struct java_lang_reflect_Method* this ) {
	java_objectarray *exceptiontypes;
/*	log_text("Java_java_lang_reflect_Method_getExceptionTypes called");	*/
            /* array of exceptions declared to be thrown, information not available !! */
            exceptiontypes = builtin_anewarray (0, class_java_lang_Class);
	return exceptiontypes;

}
/*
 * Class:     java_lang_reflect_Method
 * Method:    invokeNative
 * Signature: (Ljava/lang/Object;[Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_Method_invokeNative (JNIEnv *env ,  struct java_lang_reflect_Method* this , struct java_lang_Object* par1, java_objectarray* par2, struct java_lang_Class* par3, s4 par4)
{
	log_text("Java_java_lang_reflect_Method_invokeNative called");
	return 0;

}
