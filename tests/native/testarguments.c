#include "jni.h"
#include "testarguments.h"

JNIEXPORT void JNICALL Java_testarguments_nsub(JNIEnv *env, jclass clazz)
{
  (*env)->GetVersion(env);
}

