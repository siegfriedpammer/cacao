/* This file is machine generated, don't edit it !*/


#include "jni.h"
#include "types.h"
#include "java_nio_channels_FileChannelImpl.h"


/*
 * Class:     java_nio_channels_FileChannelImpl
 * Method:    implPosition
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_nio_channels_FileChannelImpl_implPosition (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this ) {
	log_text("Java_java_nio_channels_FileChannelImpl_implPosition ");
	return 0;
}


/*
 * Class:     java_nio_channels_FileChannelImpl
 * Method:    nio_mmap_file
 * Signature: (JJI)Lgnu/classpath/RawData;
 */
JNIEXPORT struct gnu_classpath_RawData* JNICALL Java_java_nio_channels_FileChannelImpl_nio_mmap_file (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this , s8 par1, s8 par2, s4 par3) {
  log_text("Java_java_nio_channels_FileChannelImpl_nio_mmap_file");
  return NULL;
}


/*
 * Class:     java_nio_channels_FileChannelImpl
 * Method:    nio_unmmap_file
 * Signature: (Lgnu/classpath/RawData;I)V
 */
JNIEXPORT void JNICALL Java_java_nio_channels_FileChannelImpl_nio_unmmap_file (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this , struct gnu_classpath_RawData* par1, s4 par2){
	log_text("Java_java_nio_channels_FileChannelImpl_nio_unmmap_file");
	return;
}


/*
 * Class:     java_nio_channels_FileChannelImpl
 * Method:    nio_msync
 * Signature: (Lgnu/classpath/RawData;I)V
 */
JNIEXPORT void JNICALL Java_java_nio_channels_FileChannelImpl_nio_msync (JNIEnv *env ,  struct java_nio_channels_FileChannelImpl* this , struct gnu_classpath_RawData* par1, s4 par2){
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
