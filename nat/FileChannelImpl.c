/* nat/FileChannelImpl.c - java/nio/channels/FileChannelImpl

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   Authors: Roman Obermaiser

   Changes: Joseph Wenninger

   $Id: FileChannelImpl.c 873 2004-01-11 20:59:29Z twisti $

*/


#include <stdlib.h>
#include "jni.h"
#include "types.h"
#include "toolbox/loging.h"
#include "gnu_classpath_RawData.h"
#include "java_nio_channels_FileChannelImpl.h"


/*
 * Class:     java_nio_channels_FileChannelImpl
 * Method:    implPosition
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_nio_channels_FileChannelImpl_implPosition(JNIEnv *env, java_nio_channels_FileChannelImpl *this) {
	log_text("Java_java_nio_channels_FileChannelImpl_implPosition ");

	return 0;
}


/*
 * Class:     java_nio_channels_FileChannelImpl
 * Method:    nio_mmap_file
 * Signature: (JJI)Lgnu/classpath/RawData;
 */
JNIEXPORT gnu_classpath_RawData* JNICALL Java_java_nio_channels_FileChannelImpl_nio_mmap_file(JNIEnv *env, java_nio_channels_FileChannelImpl *this, s8 par1, s8 par2, s4 par3)
{
  log_text("Java_java_nio_channels_FileChannelImpl_nio_mmap_file");

  return NULL;
}


/*
 * Class:     java_nio_channels_FileChannelImpl
 * Method:    nio_unmmap_file
 * Signature: (Lgnu/classpath/RawData;I)V
 */
JNIEXPORT void JNICALL Java_java_nio_channels_FileChannelImpl_nio_unmmap_file(JNIEnv *env, struct java_nio_channels_FileChannelImpl* this, struct gnu_classpath_RawData* par1, s4 par2)
{
	log_text("Java_java_nio_channels_FileChannelImpl_nio_unmmap_file");

	return;
}


/*
 * Class:     java_nio_channels_FileChannelImpl
 * Method:    nio_msync
 * Signature: (Lgnu/classpath/RawData;I)V
 */
JNIEXPORT void JNICALL Java_java_nio_channels_FileChannelImpl_nio_msync(JNIEnv *env, struct java_nio_channels_FileChannelImpl* this, struct gnu_classpath_RawData* par1, s4 par2)
{
	log_text("Java_java_nio_channels_FileChannelImpl_nio_msync");

	return;
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
 */
