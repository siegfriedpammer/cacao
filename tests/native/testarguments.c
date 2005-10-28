/* tests/native/testarguments.c - tests argument passing

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Christian Thalinger

   $Id: testarguments.c 3522 2005-10-28 18:07:00Z twisti $

*/


#include <stdio.h>

#include "config.h"
#include "native/jni.h"


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

#if SIZEOF_VOID_P == 4
    printf("java-native: 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx 0x%llx\n", a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
#else
    printf("java-native: 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n", a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
#endif
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

#if SIZEOF_VOID_P == 4
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
#else
    x.d = a; printf(" 0x%lx", x.l);
    x.d = b; printf(" 0x%lx", x.l);
    x.d = c; printf(" 0x%lx", x.l);
    x.d = d; printf(" 0x%lx", x.l);
    x.d = e; printf(" 0x%lx", x.l);
    x.d = f; printf(" 0x%lx", x.l);
    x.d = g; printf(" 0x%lx", x.l);
    x.d = h; printf(" 0x%lx", x.l);
    x.d = i; printf(" 0x%lx", x.l);
    x.d = j; printf(" 0x%lx", x.l);
    x.d = k; printf(" 0x%lx", x.l);
    x.d = l; printf(" 0x%lx", x.l);
    x.d = m; printf(" 0x%lx", x.l);
    x.d = n; printf(" 0x%lx", x.l);
    x.d = o; printf(" 0x%lx", x.l);
#endif

    printf("\n");
    fflush(stdout);

    mid = (*env)->GetStaticMethodID(env, clazz, "jdsub", "(DDDDDDDDDDDDDDD)V");

    if (mid == 0) {
        printf("native: couldn't find jfsub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
}
