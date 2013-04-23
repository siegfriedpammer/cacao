/* tests/regression/native/testarguments.c - tests argument passing

   Copyright (C) 1996-2012
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include INCLUDE_JNI_MD_H
#include INCLUDE_JNI_H


JNIEXPORT jobject JNICALL Java_testarguments_adr(JNIEnv *env, jclass clazz, jint i)
{
  intptr_t p;

  p = 0x11111111 * ((intptr_t) i);

#if defined(ENABLE_HANDLES)
  return (*env)->NewLocalRef(env, &p);
#else
  return (jobject) p;
#endif
}


JNIEXPORT void JNICALL Java_testarguments_np(JNIEnv *env, jclass clazz, jobject o)
{
#if defined(ENABLE_HANDLES)
    intptr_t p = *((intptr_t *) o);
#else
    intptr_t p = (intptr_t) o;
#endif

#if SIZEOF_VOID_P == 8
    printf(" 0x%lx", (long) p);
#else
    printf(" 0x%x", (int) p);
#endif

    fflush(stdout);
}


JNIEXPORT void JNICALL Java_testarguments_nisub(JNIEnv *env, jclass clazz, jint a, jint b, jint c, jint d, jint e, jint f, jint g, jint h, jint i, jint j, jint k, jint l, jint m, jint n, jint o)
{
    jmethodID mid;

    printf("java-native: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
    fflush(stdout);

    mid = (*env)->GetStaticMethodID(env, clazz, "jisub", "(IIIIIIIIIIIIIII)V");

    if (mid == 0) {
        printf("native: couldn't find jisub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
}


JNIEXPORT void JNICALL Java_testarguments_nlsub(JNIEnv *env, jclass clazz, jlong a, jlong b, jlong c, jlong d, jlong e, jlong f, jlong g, jlong h, jlong i, jlong j, jlong k, jlong l, jlong m, jlong n, jlong o)
{
    jmethodID mid;

    printf("java-native: 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx\n", a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
    fflush(stdout);

    mid = (*env)->GetStaticMethodID(env, clazz, "jlsub", "(JJJJJJJJJJJJJJJ)V");

    if (mid == 0) {
        printf("native: couldn't find jlsub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
}


JNIEXPORT void JNICALL Java_testarguments_nfsub(JNIEnv *env, jclass clazz, jfloat a, jfloat b, jfloat c, jfloat d, jfloat e, jfloat f, jfloat g, jfloat h, jfloat i, jfloat j, jfloat k, jfloat l, jfloat m, jfloat n, jfloat o)
{
    jmethodID mid;
    union {
      jint i;
      jfloat f;
    } x;

    printf("java-native:");

    x.f = a; printf(" 0x%x", x.i);
    x.f = b; printf(" 0x%x", x.i);
    x.f = c; printf(" 0x%x", x.i);
    x.f = d; printf(" 0x%x", x.i);
    x.f = e; printf(" 0x%x", x.i);
    x.f = f; printf(" 0x%x", x.i);
    x.f = g; printf(" 0x%x", x.i);
    x.f = h; printf(" 0x%x", x.i);
    x.f = i; printf(" 0x%x", x.i);
    x.f = j; printf(" 0x%x", x.i);
    x.f = k; printf(" 0x%x", x.i);
    x.f = l; printf(" 0x%x", x.i);
    x.f = m; printf(" 0x%x", x.i);
    x.f = n; printf(" 0x%x", x.i);
    x.f = o; printf(" 0x%x", x.i);

    printf("\n");
    fflush(stdout);

    mid = (*env)->GetStaticMethodID(env, clazz, "jfsub", "(FFFFFFFFFFFFFFF)V");

    if (mid == 0) {
        printf("native: couldn't find jfsub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
}


JNIEXPORT void JNICALL Java_testarguments_ndsub(JNIEnv *env, jclass clazz, jdouble a, jdouble b, jdouble c, jdouble d, jdouble e, jdouble f, jdouble g, jdouble h, jdouble i, jdouble j, jdouble k, jdouble l, jdouble m, jdouble n, jdouble o)
{
    jmethodID mid;
    union {
      jlong l;
      jdouble d;
    } x;

    printf("java-native:");

    x.d = a; printf(" 0x%llx", x.l);
    x.d = b; printf(" 0x%llx", x.l);
    x.d = c; printf(" 0x%llx", x.l);
    x.d = d; printf(" 0x%llx", x.l);
    x.d = e; printf(" 0x%llx", x.l);
    x.d = f; printf(" 0x%llx", x.l);
    x.d = g; printf(" 0x%llx", x.l);
    x.d = h; printf(" 0x%llx", x.l);
    x.d = i; printf(" 0x%llx", x.l);
    x.d = j; printf(" 0x%llx", x.l);
    x.d = k; printf(" 0x%llx", x.l);
    x.d = l; printf(" 0x%llx", x.l);
    x.d = m; printf(" 0x%llx", x.l);
    x.d = n; printf(" 0x%llx", x.l);
    x.d = o; printf(" 0x%llx", x.l);

    printf("\n");
    fflush(stdout);

    mid = (*env)->GetStaticMethodID(env, clazz, "jdsub", "(DDDDDDDDDDDDDDD)V");

    if (mid == 0) {
        printf("native: couldn't find jdsub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
}


JNIEXPORT void JNICALL Java_testarguments_nasub(JNIEnv *env, jclass clazz, jobject a, jobject b, jobject c, jobject d, jobject e, jobject f, jobject g, jobject h, jobject i, jobject j, jobject k, jobject l, jobject m, jobject n, jobject o)
{
    jmethodID mid;

#if defined(ENABLE_HANDLES)

# if SIZEOF_VOID_P == 8
    printf("java-native: 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n", *((long*) a), *((long*) b), *((long*) c), *((long*) d), *((long*) e), *((long*) f), *((long*) g), *((long*) h), *((long*) i), *((long*) j), *((long*) k), *((long*) l), *((long*) m), *((long*) n), *((long*) o));
# else
    printf("java-native: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", *((int*) a), *((int*) b), *((int*) c), *((int*) d), *((int*) e), *((int*) f), *((int*) g), *((int*) h), *((int*) i), *((int*) j), *((int*) k), *((int*) l), *((int*) m), *((int*) n), *((int*) o));
# endif

#else

# if SIZEOF_VOID_P == 8
    printf("java-native: 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n", (long) a, (long) b, (long) c, (long) d, (long) e, (long) f, (long) g, (long) h, (long) i, (long) j, (long) k, (long) l, (long) m, (long) n, (long) o);
# else
    printf("java-native: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", (int) a, (int) b, (int) c, (int) d, (int) e, (int) f, (int) g, (int) h, (int) i, (int) j, (int) k, (int) l, (int) m, (int) n, (int) o);
# endif

#endif

    fflush(stdout);

    mid = (*env)->GetStaticMethodID(env, clazz, "jasub", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");

    if (mid == 0) {
        printf("native: couldn't find jasub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
}


JNIEXPORT void JNICALL Java_testarguments_nmsub(JNIEnv *env, jclass clazz, jint a, jlong b, jfloat c, jdouble d, jint e, jlong f, jfloat g, jdouble h, jint i, jlong j, jfloat k, jdouble l, jint m, jlong n, jfloat o)
{
    jmethodID mid;
    union {
      jint i;
      jlong l;
      jfloat f;
      jdouble d;
    } x;

    printf("java-native:");

    printf(" 0x%x", a);
    printf(" 0x%llx", b);
    x.f = c; printf(" 0x%x", x.i);
    x.d = d; printf(" 0x%llx", x.l);
    printf(" 0x%x", e);
    printf(" 0x%llx", f);
    x.f = g; printf(" 0x%x", x.i);
    x.d = h; printf(" 0x%llx", x.l);
    printf(" 0x%x", i);
    printf(" 0x%llx", j);
    x.f = k; printf(" 0x%x", x.i);
    x.d = l; printf(" 0x%llx", x.l);
    printf(" 0x%x", m);
    printf(" 0x%llx", n);
    x.f = o; printf(" 0x%x", x.i);

    printf("\n");
    fflush(stdout);

    mid = (*env)->GetStaticMethodID(env, clazz, "jmsub", "(IJFDIJFDIJFDIJF)V");

    if (mid == 0) {
        printf("native: couldn't find jmsub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
}

JNIEXPORT void JNICALL Java_testarguments_nmfsub(JNIEnv *env, jclass clazz, jdouble a, jfloat b, jdouble c, jfloat d, jdouble e, jfloat f, jdouble x_, jfloat y, jfloat g, jdouble h, jint i, jfloat j, jdouble k, jint l, jfloat m, jdouble n, jint o, jint p, jlong q, jfloat r, jdouble s, jint t, jlong u, jfloat v, jdouble w)
{
    jmethodID mid;
    union {
      jint i;
      jlong l;
      jfloat f;
      jdouble d;
    } x;

    printf("java-native:");

    x.d = a; printf(" 0x%llx", x.l);
    x.f = b; printf(" 0x%x", x.i);
    x.d = c; printf(" 0x%llx", x.l);
    x.f = d; printf(" 0x%x", x.i);
    x.d = e; printf(" 0x%llx", x.l);
    x.f = f; printf(" 0x%x", x.i);
    x.d = x_; printf(" 0x%llx", x.l);
    x.f = y; printf(" 0x%x", x.i);

    x.f = g; printf(" 0x%x", x.i);
    x.d = h; printf(" 0x%llx", x.l);
    printf(" 0x%x", i);

    x.f = j; printf(" 0x%x", x.i);
    x.d = k; printf(" 0x%llx", x.l);
    printf(" 0x%x", l);

    x.f = m; printf(" 0x%x", x.i);
    x.d = n; printf(" 0x%llx", x.l);
    printf(" 0x%x", o);

    printf(" 0x%x", p);
    printf(" 0x%llx", q);
    x.f = r; printf(" 0x%x", x.i);
    x.d = s; printf(" 0x%llx", x.l);

    printf(" 0x%x", t);
    printf(" 0x%llx", u);
    x.f = v; printf(" 0x%x", x.i);
    x.d = w; printf(" 0x%llx", x.l);

    printf("\n");
    fflush(stdout);

    mid = (*env)->GetStaticMethodID(env, clazz, "jmfsub", "(DFDFDFDFFDIFDIFDIIJFDIJFD)V");

    if (mid == 0) {
        printf("native: couldn't find jmfsub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, x_, y, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w);
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
 * vim:noexpandtab:sw=4:ts=4:
 */
