#include "native/jni.h"
#include "testarguments.h"


JNIEXPORT void JNICALL Java_testarguments_nisub(JNIEnv *env, jclass clazz, int a, int b, int c, int d, int e, int f, int g, int h, int i, int j)
{
    jmethodID mid;

    printf("native: int: %d %d %d %d %d %d %d %d %d %d\n", a, b, c, d, e, f, g, h, i, j);

    mid = (*env)->GetStaticMethodID(env, clazz, "jisub", "(IIIIIIIIII)V");

    if (mid == 0) {
        printf("native: couldn't find jisub\n");
        return;
    }

/*      (*env)->CallVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j); */
    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j);
}


JNIEXPORT void JNICALL Java_testarguments_nlsub(JNIEnv *env, jclass clazz, long a, long b, long c, long d, long e, long f, long g, long h, long i, long j)
{
    jmethodID mid;

    printf("native: long: %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n", a, b, c, d, e, f, g, h, i, j);

    mid = (*env)->GetStaticMethodID(env, clazz, "jlsub", "(JJJJJJJJJJ)V");

    if (mid == 0) {
        printf("native: couldn't find jlsub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j);
}


JNIEXPORT void JNICALL Java_testarguments_nfsub(JNIEnv *env, jclass clazz, float a, float b, float c, float d, float e, float f, float g, float h, float i, float j)
{
    jmethodID mid;

    printf("native: float: %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f\n", a, b, c, d, e, f, g, h, i, j);

    mid = (*env)->GetStaticMethodID(env, clazz, "jfsub", "(FFFFFFFFFF)V");

    if (mid == 0) {
        printf("native: couldn't find jfsub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j);
}


JNIEXPORT void JNICALL Java_testarguments_ndsub(JNIEnv *env, jclass clazz, double a, double b, double c, double d, double e, double f, double g, double h, double i, double j)
{
    jmethodID mid;

    printf("native: double: %.16g %.16g %.16g %.16g %.16g %.16g %.16g %.16g %.16g %.16g\n", a, b, c, d, e, f, g, h, i, j);

    mid = (*env)->GetStaticMethodID(env, clazz, "jdsub", "(DDDDDDDDDD)V");

    if (mid == 0) {
        printf("native: couldn't find jfsub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j);
}
