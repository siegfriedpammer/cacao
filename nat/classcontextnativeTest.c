#include <stdio.h>
#include "jni.h"
#include "types.h"
#include "classcontextnativeTest.h"

/*
 * Class:     classcontextnativeTest
 * Method:    a
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_classcontextnativeTest_a(JNIEnv *env, jclass clazz){
	jmethodID mid;
        printf("classcontextnativeTest_nat: a()\n");
        mid = (*env)->GetMethodID(env, clazz, "b", "()V");
        (*env)->CallStaticVoidMethod(env, clazz, mid);


}


/*
 * Class:     classcontextnativeTest
 * Method:    k
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_classcontextnativeTest_k(JNIEnv *env, jclass clazz){
	jmethodID mid;
        printf("classcontextnativeTest_nat: k()\n");
        mid = (*env)->GetMethodID(env, clazz, "l", "()V");
        (*env)->CallStaticVoidMethod(env, clazz, mid);
}

/*
 * Class:     classcontextnativeTest
 * Method:    y
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_classcontextnativeTest_y(JNIEnv *env, jclass clazz) {
	jmethodID mid;
        printf("classcontextnativeTest_nat: y()\n");
        mid = (*env)->GetMethodID(env, clazz, "z", "()V");
        (*env)->CallStaticVoidMethod(env, clazz, mid);
}


