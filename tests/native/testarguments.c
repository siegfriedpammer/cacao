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

   $Id: testarguments.c 1756 2004-12-13 10:09:35Z twisti $

*/


#include <stdio.h>

#include "config.h"
#include "native/jni.h"


JNIEXPORT void JNICALL Java_testarguments_nisub(JNIEnv *env, jclass clazz, jint a, jint b, jint c, jint d, jint e, jint f, jint g, jint h, jint i, jint j)
{
    jmethodID mid;

    printf("java-native: %d %d %d %d %d %d %d %d %d %d\n", a, b, c, d, e, f, g, h, i, j);
    fflush(stdout);

    mid = (*env)->GetStaticMethodID(env, clazz, "jisub", "(IIIIIIIIII)V");

    if (mid == 0) {
        printf("native: couldn't find jisub\n");
        return;
    }

/*      (*env)->CallVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j); */
    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j);
}


JNIEXPORT void JNICALL Java_testarguments_nlsub(JNIEnv *env, jclass clazz, jlong a, jlong b, jlong c, jlong d, jlong e, jlong f, jlong g, jlong h, jlong i, jlong j)
{
    jmethodID mid;

#if defined(__I386__) || defined(__POWERPC__)
    printf("java-native: %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld\n", a, b, c, d, e, f, g, h, i, j);
#else
    printf("java-native: %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n", a, b, c, d, e, f, g, h, i, j);
#endif
    fflush(stdout);

    mid = (*env)->GetStaticMethodID(env, clazz, "jlsub", "(JJJJJJJJJJ)V");

    if (mid == 0) {
        printf("native: couldn't find jlsub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j);
}


JNIEXPORT void JNICALL Java_testarguments_nfsub(JNIEnv *env, jclass clazz, jfloat a, jfloat b, jfloat c, jfloat d, jfloat e, jfloat f, jfloat g, jfloat h, jfloat i, jfloat j)
{
    jmethodID mid;

    printf("java-native: %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f %.8f\n", a, b, c, d, e, f, g, h, i, j);
    fflush(stdout);

    mid = (*env)->GetStaticMethodID(env, clazz, "jfsub", "(FFFFFFFFFF)V");

    if (mid == 0) {
        printf("native: couldn't find jfsub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j);
}


JNIEXPORT void JNICALL Java_testarguments_ndsub(JNIEnv *env, jclass clazz, jdouble a, jdouble b, jdouble c, jdouble d, jdouble e, jdouble f, jdouble g, jdouble h, jdouble i, jdouble j)
{
    jmethodID mid;

    printf("java-native: %.16g %.16g %.16g %.16g %.16g %.16g %.16g %.16g %.16g %.16g\n", a, b, c, d, e, f, g, h, i, j);
    fflush(stdout);

    mid = (*env)->GetStaticMethodID(env, clazz, "jdsub", "(DDDDDDDDDD)V");

    if (mid == 0) {
        printf("native: couldn't find jfsub\n");
        return;
    }

    (*env)->CallStaticVoidMethod(env, clazz, mid, a, b, c, d, e, f, g, h, i, j);
}
