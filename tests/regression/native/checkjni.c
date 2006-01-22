/* src/tests/native/checkjni.c - for testing JNI stuff

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

   Changes:

   $Id: checkjni.c 2170 2005-03-31 19:18:38Z twisti $

*/


#include <stdio.h>

#include "config.h"
#include "native/jni.h"


JNIEXPORT jboolean JNICALL Java_checkjni_IsAssignableFrom(JNIEnv *env, jclass clazz, jclass sub, jclass sup)
{
  return (*env)->IsAssignableFrom(env, sub, sup);
}

JNIEXPORT jboolean JNICALL Java_checkjni_IsInstanceOf(JNIEnv *env, jclass clazz, jobject obj, jclass c)
{
  return (*env)->IsInstanceOf(env, obj, c);
}
