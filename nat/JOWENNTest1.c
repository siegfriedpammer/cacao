#include <stdio.h>
#include "jni.h"
#include "types.h"
#include "java_lang_JOWENNTest1.h"


/*
 * Class:     java/lang/JOWENNTest1
 * Method:    f1
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_JOWENNTest1_f1 (JNIEnv *env , jclass clazz , s4 par1) {
	jmethodID mid;

	printf("JOWENNTest1_nat:%ld\n",par1);

        mid = (*env)->GetMethodID(env, clazz, "f1a", "(I)V");

        (*env)->CallStaticVoidMethod(env, clazz, mid,par1);

}

/*
 * Class:     java/lang/JOWENNTest1
 * Method:    f2
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_JOWENNTest1_f2 (JNIEnv *env , jclass clazz , s8 par1) {
	jmethodID mid;

	printf("JOWENNTest1_nat:%lld\n",par1);
        mid = (*env)->GetMethodID(env, clazz, "f2a", "(J)V");

        (*env)->CallStaticVoidMethod(env, clazz, mid,par1);

}

/*
 * Class:     java/lang/JOWENNTest1
 * Method:    f3
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_java_lang_JOWENNTest1_f3 (JNIEnv *env , jclass clazz , float par1) {
	jmethodID mid;

	printf("JOWENNTest1_nat:%g\n",par1);
        mid = (*env)->GetMethodID(env, clazz, "f3a", "(F)V");

	if (mid==0) {printf("ERROR: f3a: (F)V not found\n"); exit(1);}


        (*env)->CallStaticVoidMethod(env, clazz, mid,par1);

}

/*
 * Class:     java/lang/JOWENNTest1
 * Method:    f4
 * Signature: (D)V
 */
JNIEXPORT void JNICALL Java_java_lang_JOWENNTest1_f4 (JNIEnv *env , jclass clazz , double par1) {
	jmethodID mid;

	printf("JOWENNTest1_nat:%g\n",par1);
        mid = (*env)->GetMethodID(env, clazz, "f4a", "(D)V");

        (*env)->CallStaticVoidMethod(env, clazz, mid,par1);

}


/*
 * Class:     java/lang/JOWENNTest1
 * Method:    f5
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_JOWENNTest1_f5 (JNIEnv *env , jclass clazz , s4 par1) {
	jmethodID mid;

	printf("JOWENNTest1_nat_f5:%d\n",par1);
        mid = (*env)->GetMethodID(env, clazz, "f5a", "(I)V");

        (*env)->CallStaticVoidMethod(env, clazz, mid,par1);

}


/*
 * Class:     java/lang/JOWENNTest1
 * Method:    f6
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_JOWENNTest1_f6 (JNIEnv *env ,  struct java_lang_JOWENNTest1* this , s4 par1) {
	jmethodID mid;

	printf("JOWENNTest1_nat_f6:%d\n",par1);
//	class_showmethods(this->header.vftbl->class);
        mid = (*env)->GetMethodID(env, this->header.vftbl->class, "f6a", "(I)V");

        (*env)->CallVoidMethod(env, this, mid,par1);

	if (par1==5) {
	        mid = (*env)->GetMethodID(env, this->header.vftbl->class->super, "f6a", "(I)V");

        	(*env)->CallVoidMethod(env, this, mid,par1);
        	(*env)->CallNonvirtualVoidMethod(env, this, this->header.vftbl->class->super,mid,par1);
	}

}

/*
 * Class:     java/lang/JOWENNTest1
 * Method:    f7
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_JOWENNTest1_f7 (JNIEnv *env ,  struct java_lang_JOWENNTest1* this) {

	jmethodID mid;

	printf("JOWENNTest1_nat_f7\n");
//	class_showmethods(this->header.vftbl->class);
        mid = (*env)->GetMethodID(env, this->header.vftbl->class, "f7a", "(III)V");

        (*env)->CallVoidMethod(env, this, mid,1,2,3);

        mid = (*env)->GetMethodID(env, this->header.vftbl->class, "f7b", "(III)V");

        (*env)->CallStaticVoidMethod(env, this, mid,1,2,3);


}


/*
 * Class:     java/lang/JOWENNTest1
 * Method:    f8
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_JOWENNTest1_f8 (JNIEnv *env ,  struct java_lang_JOWENNTest1* this) {

	jmethodID mid;

	printf("JOWENNTest1_nat_f8\n");
//	class_showmethods(this->header.vftbl->class);
        mid = (*env)->GetMethodID(env, this->header.vftbl->class, "f7a", "(III)V");

        (*env)->CallStaticVoidMethod(env, this, mid,1,2,3);
	if ((*env)->ExceptionOccurred(env)) {
		printf("good, there has been an Throwable/Exception\n");
	} else fprintf(stderr,"There should have been an exception\n");

}

/*
 * Class:     java/lang/JOWENNTest1
 * Method:    f9
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_JOWENNTest1_f9 (JNIEnv *env ,  struct java_lang_JOWENNTest1* this) {

	jmethodID mid;

	printf("JOWENNTest1_nat_f9\n");
//	class_showmethods(this->header.vftbl->class);
        mid = (*env)->GetMethodID(env, this->header.vftbl->class, "f7b", "(III)V");

        (*env)->CallVoidMethod(env, this, mid,1,2,3);
	if ((*env)->ExceptionOccurred(env)) {
		printf("good, there has been an exception/a throwable, clearing to avoid app termination\n");
		(*env)->ExceptionClear(env);
	} else fprintf(stderr,"There should have been an exception\n");
}

