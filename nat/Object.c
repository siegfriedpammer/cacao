/* class: java/lang/Object */

/*
 * Class:     java/lang/Object
 * Method:    clone
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_Object_clone ( JNIEnv *env ,  struct java_lang_Object* this)
{
	classinfo *c;
	java_lang_Object *new;

	if (((java_objectheader*)this)->vftbl->class == class_array)
	  {
	    static u4 multiplicator[10];
	    static int is_initialized = 0;

	    java_arrayheader *array = (java_arrayheader*)this;
	    u4 size;

	    if (!is_initialized)
	      {
		multiplicator[ARRAYTYPE_INT] = sizeof(s4);
		multiplicator[ARRAYTYPE_LONG] = sizeof(s8);
		multiplicator[ARRAYTYPE_FLOAT] = sizeof(float);
		multiplicator[ARRAYTYPE_DOUBLE] = sizeof(double);
		multiplicator[ARRAYTYPE_BYTE] = sizeof(s1);
		multiplicator[ARRAYTYPE_CHAR] = sizeof(u2);
		multiplicator[ARRAYTYPE_SHORT] = sizeof(s2);
		multiplicator[ARRAYTYPE_BOOLEAN] = sizeof(u1);
		multiplicator[ARRAYTYPE_OBJECT] = sizeof(void*);
		multiplicator[ARRAYTYPE_ARRAY] = sizeof(void*);
		is_initialized = 1;
	      }

	    size = sizeof(java_arrayheader)
	      + array->size * multiplicator[array->arraytype];
	    if (array->arraytype == ARRAYTYPE_OBJECT /* elementtype */
		|| array->arraytype == ARRAYTYPE_ARRAY)	/* elementdescriptor */
		size += sizeof(void*);

	    if (array->arraytype==ARRAYTYPE_OBJECT || array->arraytype==ARRAYTYPE_OBJECT)
		size+=sizeof(void*);

	    new = (java_lang_Object*)heap_allocate(size, false, NULL);
	    memcpy(new, this, size);

	    return new;
	  }
	else
	  {
	    if (! builtin_instanceof ((java_objectheader*) this, class_java_lang_Cloneable) ) {
		exceptionptr = native_new_and_init (class_java_lang_CloneNotSupportedException);
		return NULL;
		}
	
	c = this -> header.vftbl -> class;
	new = (java_lang_Object*) builtin_new (c);
	if (!new) {
		exceptionptr = proto_java_lang_OutOfMemoryError;
		return NULL;
		}

	    memcpy (new, this, c->instancesize);
	    return new;
	  }
}

/*
 * Class:     java/lang/Object
 * Method:    getClass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_Object_getClass ( JNIEnv *env ,  struct java_lang_Object* this)
{
        classinfo *c = this->header.vftbl -> class;
	use_class_as_object (c);
	return (java_lang_Class*) c;
}

/*
 * Class:     java/lang/Object
 * Method:    hashCode
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_Object_hashCode ( JNIEnv *env ,  struct java_lang_Object* this)
{
	return ((char*) this) - ((char*) 0);	
}

/*
 * Class:     java/lang/Object
 * Method:    notify
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Object_notify ( JNIEnv *env ,  struct java_lang_Object* this)
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
JNIEXPORT void JNICALL Java_java_lang_Object_notifyAll ( JNIEnv *env ,  struct java_lang_Object* this)
{
	if (runverbose)
		log_text ("java_lang_Object_notifyAll called");

	#ifdef USE_THREADS
		broadcast_cond_for_object(&this->header);
	#endif
}

/*
 * Class:     java/lang/Object
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Object_registerNatives ( JNIEnv *env  )
{
    /* empty */
}

/*
 * Class:     java/lang/Object
 * Method:    wait
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_Object_wait ( JNIEnv *env ,  struct java_lang_Object* this, s8 time)
{
	if (runverbose)
		log_text ("java_lang_Object_wait called");

	#ifdef USE_THREADS
		wait_cond_for_object(&this->header, time);
	#endif
}


