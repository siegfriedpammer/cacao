#include <jni.h>
#include <stdio.h>

static jobject ref = NULL;

JNIEXPORT void JNICALL Java_NativeGlobalRef_setReference(JNIEnv *env, jclass c, jobject o)
{
	printf("Native-World: setReference()\n");

	//ref = o;
	ref = (*env)->NewGlobalRef(env, o);

	return;
}

JNIEXPORT jobject JNICALL Java_NativeGlobalRef_getReference(JNIEnv *env, jclass c)
{
	printf("Native-World: getReference()\n");

	return ref;
}
