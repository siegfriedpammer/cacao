/* class: java/lang/Object */


#include <stdlib.h>
#include <string.h>
#include "jni.h"
#include "builtin.h"
#include "native.h"
#include "mm/boehm.h"
#include "threads/locks.h"
#include "toolbox/loging.h"
#include "toolbox/memory.h"
#include "java_lang_Cloneable.h"
#include "java_lang_Object.h"


/*
 * Class:     java/lang/Object
 * Method:    clone
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_VMObject_clone ( JNIEnv *env ,  jclass clazz, struct java_lang_Cloneable* this)
{
/*	log_text("Java_java_lang_VMObject_clone called");
	log_utf(((java_objectheader*)this)->vftbl->class->name);
	log_text("starting cloning");     */
    classinfo *c;
    java_lang_Object *new;
    arraydescriptor *desc;
    
    if ((desc = this->header.vftbl->arraydesc) != NULL) {
        /* We are cloning an array */
        
        u4 size = desc->dataoffset + desc->componentsize * ((java_arrayheader*)this)->size;
        
        new = (java_lang_Object*)heap_allocate(size, (desc->arraytype == ARRAYTYPE_OBJECT), NULL);
        memcpy(new, this, size);
        
        return new;
    }
    
    /* We are cloning a non-array */
    if (! builtin_instanceof ((java_objectheader*) this, class_java_lang_Cloneable) ) {
        exceptionptr = native_new_and_init (class_java_lang_CloneNotSupportedException);
        return NULL;
    }

    /* XXX should use vftbl */
    c = this -> header.vftbl -> class;
    new = (java_lang_Object*) builtin_new (c);
    if (!new) {
        exceptionptr = proto_java_lang_OutOfMemoryError;
        return NULL;
    }
    memcpy (new, this, c->instancesize);
    return new;
}


/*
 * Class:     java/lang/Object
 * Method:    notify
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_notify ( JNIEnv *env ,  jclass clazz, struct java_lang_Object* this)
{
	if (runverbose)
		log_text ("java_lang_Object_notify called");

        #ifdef USE_THREADS
                signal_cond_for_object(&this->header);
        #endif
}

/*
 * Class:     java/lang/Object
 * Method:    notifyAll
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_notifyAll ( JNIEnv *env ,  jclass clazz, struct java_lang_Object* this)
{
	if (runverbose)
		log_text ("java_lang_Object_notifyAll called");

	#ifdef USE_THREADS
		broadcast_cond_for_object(&this->header);
	#endif
}


/*
 * Class:     java/lang/Object
 * Method:    wait
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMObject_wait ( JNIEnv *env , jclass clazz, struct java_lang_Object* this, s8 time,s4 par3)
{
	if (runverbose)
		log_text ("java_lang_Object_wait called");

	#ifdef USE_THREADS
		wait_cond_for_object(&this->header, time);
	#endif
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
