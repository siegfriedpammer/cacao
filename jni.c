/********************************** jni.c *****************************************

	implementation of the Java Native Interface functions				  
	which are used in the JNI function table					 

***********************************************************************************/


#define JNI_VERSION       0x00010002


/********************* accessing instance-fields **********************************/

#define setField(obj,typ,var,val) *((typ*) ((long int) obj + (long int) var->offset))=val;  
#define getField(obj,typ,var)     *((typ*) ((long int) obj + (long int) var->offset))
#define setfield_critical(clazz,obj,name,sig,jdatatype,val) setField(obj,jdatatype,getFieldID_critical(env,clazz,name,sig),val); 

/*************************** function: jclass_findfield ****************************
	
	searches for field with specified name and type in a 'classinfo'-structur
	if no such field is found NULL is returned 

************************************************************************************/

fieldinfo *jclass_findfield (classinfo *c, utf *name, utf *desc)
{
	s4 i;
	for (i = 0; i < c->fieldscount; i++) {
		if ((c->fields[i].name == name) && (c->fields[i].descriptor == desc))
			return &(c->fields[i]);
		}

	return NULL;
}

/********************* returns version of native method interface *****************/

jint GetVersion (JNIEnv* env)
{
	return JNI_VERSION;
}

/****************** loads a class from a buffer of raw class data *****************/

jclass DefineClass(JNIEnv* env, const char *name, jobject loader, const jbyte *buf, jsize len) 
{
        jclass clazz; 

	/* change suck-mode, so subsequent class_load will read from memory-buffer */
	classload_buffer( (u1*) buf,len);

        clazz = loader_load(utf_new_char ((char *) name));

	/* restore old suck-mode */
	classload_buffer(NULL,0);

	return clazz;
}


/*************** loads locally defined class with the specified name **************/

jclass FindClass (JNIEnv* env, const char *name) 
{
	classinfo *c;  
  
	c = loader_load(utf_new_char ((char *) name));

	if (!c) exceptionptr = native_new_and_init(class_java_lang_ClassFormatError);

  	return c;
}
  

/*********************************************************************************** 

	converts java.lang.reflect.Method or 
 	java.lang.reflect.Constructor object to a method ID  
  
 **********************************************************************************/   
  
jmethodID FromReflectedMethod (JNIEnv* env, jobject method)
{
	log_text("JNI-Call: FromReflectedMethod");
}


/*************** return superclass of the class represented by sub ****************/
 
jclass GetSuperclass (JNIEnv* env, jclass sub) 
{
	classinfo *c;

	c = ((classinfo*) sub) -> super;

	if (!c) return NULL; 
	use_class_as_object (c);
	return c;		
}
  
 
/*********************** check whether sub can be cast to sup  ********************/
  
jboolean IsAssignableForm (JNIEnv* env, jclass sub, jclass sup)
{
        return builtin_isanysubclass(sub,sup);
}


/***** converts a field ID derived from cls to a java.lang.reflect.Field object ***/

jobject ToReflectedField (JNIEnv* env, jclass cls, jfieldID fieldID, jboolean isStatic)
{
	log_text("JNI-Call: ToReflectedField");
}


/***************** throw java.lang.Throwable object  ******************************/

jint Throw (JNIEnv* env, jthrowable obj)
{
	exceptionptr = (java_objectheader*) obj;
	return 0;
}


/*********************************************************************************** 

	create exception object from the class clazz with the 
	specified message and cause it to be thrown

 **********************************************************************************/   


jint ThrowNew (JNIEnv* env, jclass clazz, const char *msg) 
{
	java_lang_Throwable *o;

  	/* instantiate exception object */
	o = (java_lang_Throwable *) native_new_and_init ((classinfo*) clazz);

	if (!o) return (-1);

  	o->detailMessage = (java_lang_String*) javastring_new_char((char *) msg);

	exceptionptr = (java_objectheader*) o;	
	return 0;
}

/************************* check if exception occured *****************************/

jthrowable ExceptionOccurred (JNIEnv* env) 
{
	return (jthrowable) exceptionptr;
}

/********** print exception and a backtrace of the stack (for debugging) **********/

void ExceptionDescribe (JNIEnv* env) 
{
	utf_display(exceptionptr->vftbl->class->name);
	printf ("\n");
	fflush (stdout);	
}


/******************* clear any exception currently being thrown *******************/

void ExceptionClear (JNIEnv* env) 
{
	exceptionptr = NULL;	
}


/********** raises a fatal error and does not expect the VM to recover ************/

void FatalError (JNIEnv* env, const char *msg)
{
	panic((char *) msg);	
}

/******************* creates a new local reference frame **************************/ 

jint PushLocalFrame (JNIEnv* env, jint capacity)
{
	/* empty */
}

/**************** Pops off the current local reference frame **********************/

jobject PopLocalFrame (JNIEnv* env, jobject result)
{
	/* empty */
}
    

/** Creates a new global reference to the object referred to by the obj argument **/
    
jobject NewGlobalRef (JNIEnv* env, jobject lobj)
{
	heap_addreference ( (void**) &lobj);
	return lobj;
}

/*************  Deletes the global reference pointed to by globalRef **************/

void DeleteGlobalRef (JNIEnv* env, jobject gref)
{
	/* empty */
}


/*************** Deletes the local reference pointed to by localRef ***************/

void DeleteLocalRef (JNIEnv* env, jobject localRef)
{
	/* empty */
}

/********** Tests whether two references refer to the same Java object ************/

jboolean IsSameObject (JNIEnv* env, jobject obj1, jobject obj2)
{
 	return (obj1==obj2);
}

/***** Creates a new local reference that refers to the same object as ref  *******/

jobject NewLocalRef (JNIEnv* env, jobject ref)
{
	return ref;
}

/*********************************************************************************** 

	Ensures that at least a given number of local references can 
	be created in the current thread

 **********************************************************************************/   

jint EnsureLocalCapacity (JNIEnv* env, jint capacity)
{
	return 0; /* return 0 on success */
}


/********* Allocates a new Java object without invoking a constructor *************/

jobject AllocObject (JNIEnv* env, jclass clazz)
{
        java_objectheader *o = builtin_new(clazz);	
	return o;
}


/*********************************************************************************** 

	Constructs a new Java object
	arguments that are to be passed to the constructor are placed after methodID

***********************************************************************************/

jobject NewObject (JNIEnv* env, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: NewObject");
}

/*********************************************************************************** 

       Constructs a new Java object
       arguments that are to be passed to the constructor are placed in va_list args 

***********************************************************************************/

jobject NewObjectV (JNIEnv* env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: NewObjectV");
}

/*********************************************************************************** 

	Constructs a new Java object
	arguments that are to be passed to the constructor are placed in 
	args array of jvalues 

***********************************************************************************/

jobject NewObjectA (JNIEnv* env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: NewObjectA");
}


/************************ returns the class of an object **************************/ 

jclass GetObjectClass (JNIEnv* env, jobject obj)
{
 	classinfo *c = obj->vftbl -> class;
	use_class_as_object (c);
	return c;
}

/************* tests whether an object is an instance of a class ******************/

jboolean IsInstanceOf (JNIEnv* env, jobject obj, jclass clazz)
{
        return builtin_instanceof(obj,clazz);
}


/***************** converts a java.lang.reflect.Field to a field ID ***************/
 
jfieldID FromReflectedField (JNIEnv* env, jobject field)
{
	log_text("JNI-Call: FromReflectedField");
}

/**********************************************************************************

	converts a method ID to a java.lang.reflect.Method or 
	java.lang.reflect.Constructor object

**********************************************************************************/

jobject ToReflectedMethod (JNIEnv* env, jclass cls, jmethodID methodID, jboolean isStatic)
{
	log_text("JNI-Call: ToReflectedMethod");
}

/**************** returns the method ID for an instance method ********************/

jmethodID GetMethodID (JNIEnv* env, jclass clazz, const char *name, const char *sig)
{
	jmethodID m;

 	m = class_resolvemethod (
		clazz, 
		utf_new_char ((char*) name), 
		utf_new_char ((char*) sig)
    	);

	if (!m) exceptionptr = native_new_and_init(class_java_lang_NoSuchMethodError);  	

	return m;
}

/******************** JNI-functions for calling instance methods ******************/

jobject CallObjectMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallObjectMethod");

	return NULL;
}


jobject CallObjectMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallObjectMethodV");

	return NULL;
}


jobject CallObjectMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallObjectMethodA");

	return NULL;
}


jboolean CallBooleanMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallBooleanMethod");

	return 0;
}

jboolean CallBooleanMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallBooleanMethodV");

	return 0;
}

jboolean CallBooleanMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallBooleanMethodA");

	return 0;
}

jbyte CallByteMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallByteMethod");

	return 0;
}

jbyte CallByteMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallByteMethodV");

	return 0;
}


jbyte CallByteMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallByteMethodA");

	return 0;
}


jchar CallCharMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallCharMethod");

	return 0;
}

jchar CallCharMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallCharMethodV");

	return 0;
}


jchar CallCharMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallCharMethodA");

	return 0;
}


jshort CallShortMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallShortMethod");

	return 0;
}


jshort CallShortMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallShortMethodV");

	return 0;
}


jshort CallShortMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallShortMethodA");

	return 0;
}



jint CallIntMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallIntMethod");

	return 0;
}


jint CallIntMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallIntMethodV");

	return 0;
}


jint CallIntMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallIntMethodA");

	return 0;
}



jlong CallLongMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallLongMethod");

	return 0;
}


jlong CallLongMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallLongMethodV");

	return 0;
}


jlong CallLongMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallLongMethodA");

	return 0;
}



jfloat CallFloatMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallFloatMethod");

	return 0;
}


jfloat CallFloatMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallFloatMethodV");

	return 0;
}


jfloat CallFloatMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallFloatMethodA");

	return 0;
}



jdouble CallDoubleMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallDoubleMethod");

	return 0;
}


jdouble CallDoubleMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallDoubleMethodV");

	return 0;
}


jdouble CallDoubleMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallDoubleMethodA");

	return 0;
}



void CallVoidMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallVoidMethod");

}


void CallVoidMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallVoidMethodV");

}


void CallVoidMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallVoidMethodA");

}



jobject CallNonvirtualObjectMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualObjectMethod");

	return NULL;
}


jobject CallNonvirtualObjectMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualObjectMethodV");

	return NULL;
}


jobject CallNonvirtualObjectMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallNonvirtualObjectMethodA");

	return NULL;
}



jboolean CallNonvirtualBooleanMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualBooleanMethod");

	return 0;
}


jboolean CallNonvirtualBooleanMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualBooleanMethodV");

	return 0;
}


jboolean CallNonvirtualBooleanMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallNonvirtualBooleanMethodA");

	return 0;
}



jbyte CallNonvirtualByteMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualByteMethod");

	return 0;
}


jbyte CallNonvirtualByteMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualByteMethodV");

	return 0;
}


jbyte CallNonvirtualByteMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualByteMethodA");

	return 0;
}



jchar CallNonvirtualCharMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualCharMethod");

	return 0;
}


jchar CallNonvirtualCharMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualCharMethodV");

	return 0;
}


jchar CallNonvirtualCharMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualCharMethodA");

	return 0;
}



jshort CallNonvirtualShortMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualShortMethod");

	return 0;
}


jshort CallNonvirtualShortMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualShortMethodV");

	return 0;
}


jshort CallNonvirtualShortMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualShortMethodA");

	return 0;
}



jint CallNonvirtualIntMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualIntMethod");

	return 0;
}


jint CallNonvirtualIntMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualIntMethodV");

	return 0;
}


jint CallNonvirtualIntMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualIntMethodA");

	return 0;
}



jlong CallNonvirtualLongMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualLongMethod");

	return 0;
}


jlong CallNonvirtualLongMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualLongMethodV");

	return 0;
}


jlong CallNonvirtualLongMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualLongMethodA");

	return 0;
}



jfloat CallNonvirtualFloatMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualFloatMethod");

	return 0;
}


jfloat CallNonvirtualFloatMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualFloatMethodV");

	return 0;
}


jfloat CallNonvirtualFloatMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualFloatMethodA");

	return 0;
}



jdouble CallNonvirtualDoubleMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualDoubleMethod");

	return 0;
}


jdouble CallNonvirtualDoubleMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualDoubleMethodV");

	return 0;
}


jdouble CallNonvirtualDoubleMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualDoubleMethodA");

	return 0;
}



void CallNonvirtualVoidMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualVoidMethod");
}


void CallNonvirtualVoidMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualVoidMethodV");
}


void CallNonvirtualVoidMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallNonvirtualVoidMethodA");
}

/************************* JNI-functions for accessing fields ************************/

jfieldID GetFieldID (JNIEnv *env, jclass clazz, const char *name, const char *sig) 
{
	jfieldID f;

	f = jclass_findfield(clazz,
			    utf_new_char ((char*) name), 
			    utf_new_char ((char*) sig)
		  	    ); 
	
	if (!f) exceptionptr =	native_new_and_init(class_java_lang_NoSuchFieldError);  

	return f;
}

/*************************** retrieve fieldid, abort on error ************************/

jfieldID getFieldID_critical(JNIEnv *env,jclass clazz,const char *name,const char *sig)
{
    jfieldID id = env->GetFieldID(env,clazz,name,sig);     
    if (!id) panic("setfield_critical failed"); 
    return id;
}

jobject GetObjectField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jobject,fieldID);
}

jboolean GetBooleanField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jboolean,fieldID);
}


jbyte GetByteField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
       	return getField(obj,jbyte,fieldID);
}


jchar GetCharField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jchar,fieldID);
}


jshort GetShortField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jshort,fieldID);
}


jint GetIntField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jint,fieldID);
}


jlong GetLongField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jlong,fieldID);
}


jfloat GetFloatField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jfloat,fieldID);
}


jdouble GetDoubleField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jdouble,fieldID);
}

void SetObjectField (JNIEnv *env, jobject obj, jfieldID fieldID, jobject val)
{
        setField(obj,jobject,fieldID,val);
}


void SetBooleanField (JNIEnv *env, jobject obj, jfieldID fieldID, jboolean val)
{
        setField(obj,jboolean,fieldID,val);
}


void SetByteField (JNIEnv *env, jobject obj, jfieldID fieldID, jbyte val)
{
        setField(obj,jbyte,fieldID,val);
}


void SetCharField (JNIEnv *env, jobject obj, jfieldID fieldID, jchar val)
{
        setField(obj,jchar,fieldID,val);
}


void SetShortField (JNIEnv *env, jobject obj, jfieldID fieldID, jshort val)
{
        setField(obj,jshort,fieldID,val);
}


void SetIntField (JNIEnv *env, jobject obj, jfieldID fieldID, jint val)
{
        setField(obj,jint,fieldID,val);
}


void SetLongField (JNIEnv *env, jobject obj, jfieldID fieldID, jlong val)
{
        setField(obj,jlong,fieldID,val);
}


void SetFloatField (JNIEnv *env, jobject obj, jfieldID fieldID, jfloat val)
{
        setField(obj,jfloat,fieldID,val);
}


void SetDoubleField (JNIEnv *env, jobject obj, jfieldID fieldID, jdouble val)
{
        setField(obj,jdouble,fieldID,val);
}

/**************** JNI-functions for calling static methods **********************/ 

jmethodID GetStaticMethodID (JNIEnv *env, jclass clazz, const char *name, const char *sig)
{
	jmethodID m;

 	m = class_resolvemethod (
		clazz, 
		utf_new_char ((char*) name), 
		utf_new_char ((char*) sig)
    	);

	if (!m) exceptionptr =	native_new_and_init(class_java_lang_NoSuchMethodError);  

	return m;
}

jobject CallStaticObjectMethod (JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallStaticObjectMethod");

	return NULL;
}


jobject CallStaticObjectMethodV (JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticObjectMethodV");

	return NULL;
}


jobject CallStaticObjectMethodA (JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticObjectMethodA");

	return NULL;
}


jboolean CallStaticBooleanMethod (JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallStaticBooleanMethod");

	return 0;
}


jboolean CallStaticBooleanMethodV (JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticBooleanMethodV");

	return 0;
}


jboolean CallStaticBooleanMethodA (JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticBooleanMethodA");

	return 0;
}


jbyte CallStaticByteMethod (JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallStaticByteMethod");

	return 0;
}


jbyte CallStaticByteMethodV (JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticByteMethodV");

	return 0;
}


jbyte CallStaticByteMethodA (JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticByteMethodA");

	return 0;
}

jchar CallStaticCharMethod (JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallStaticCharMethod");

	return 0;
}


jchar CallStaticCharMethodV (JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticCharMethodV");

	return 0;
}


jchar CallStaticCharMethodA (JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticCharMethodA");

	return 0;
}



jshort CallStaticShortMethod (JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallStaticShortMethod");

	return 0;
}


jshort CallStaticShortMethodV (JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticShortMethodV");

	return 0;
}


jshort CallStaticShortMethodA (JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticShortMethodA");

	return 0;
}



jint CallStaticIntMethod (JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallStaticIntMethod");

	return 0;
}


jint CallStaticIntMethodV (JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticIntMethodV");

	return 0;
}


jint CallStaticIntMethodA (JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticIntMethodA");

	return 0;
}



jlong CallStaticLongMethod (JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallStaticLongMethod");

	return 0;
}


jlong CallStaticLongMethodV (JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticLongMethodV");

	return 0;
}


jlong CallStaticLongMethodA (JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticLongMethodA");

	return 0;
}



jfloat CallStaticFloatMethod (JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallStaticFloatMethod");

	return 0;
}


jfloat CallStaticFloatMethodV (JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticFloatMethodV");

	return 0;
}


jfloat CallStaticFloatMethodA (JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticFloatMethodA");

	return 0;
}



jdouble CallStaticDoubleMethod (JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallStaticDoubleMethod");

	return 0;
}


jdouble CallStaticDoubleMethodV (JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticDoubleMethodV");

	return 0;
}


jdouble CallStaticDoubleMethodA (JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticDoubleMethodA");

	return 0;
}



void CallStaticVoidMethod (JNIEnv *env, jclass cls, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallStaticVoidMethod");

}


void CallStaticVoidMethodV (JNIEnv *env, jclass cls, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticVoidMethodV");

}


void CallStaticVoidMethodA (JNIEnv *env, jclass cls, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallStaticVoidMethodA");

}

/****************** JNI-functions for accessing static fields ********************/

jfieldID GetStaticFieldID (JNIEnv *env, jclass clazz, const char *name, const char *sig) 
{
	jfieldID f;

	f = jclass_findfield(clazz,
			    utf_new_char ((char*) name), 
			    utf_new_char ((char*) sig)
		 	    ); 
	
	if (!f) exceptionptr =	native_new_and_init(class_java_lang_NoSuchFieldError);  

	return f;
}


jobject GetStaticObjectField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.a;       
}


jboolean GetStaticBooleanField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jbyte GetStaticByteField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jchar GetStaticCharField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jshort GetStaticShortField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jint GetStaticIntField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jlong GetStaticLongField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.l;
}


jfloat GetStaticFloatField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
 	return fieldID->value.f;
}


jdouble GetStaticDoubleField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.d;
}



void SetStaticObjectField (JNIEnv *env, jclass clazz, jfieldID fieldID, jobject value)
{
	class_init(clazz);
	fieldID->value.a = value;
}


void SetStaticBooleanField (JNIEnv *env, jclass clazz, jfieldID fieldID, jboolean value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticByteField (JNIEnv *env, jclass clazz, jfieldID fieldID, jbyte value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticCharField (JNIEnv *env, jclass clazz, jfieldID fieldID, jchar value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticShortField (JNIEnv *env, jclass clazz, jfieldID fieldID, jshort value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticIntField (JNIEnv *env, jclass clazz, jfieldID fieldID, jint value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticLongField (JNIEnv *env, jclass clazz, jfieldID fieldID, jlong value)
{
	class_init(clazz);
	fieldID->value.l = value;
}


void SetStaticFloatField (JNIEnv *env, jclass clazz, jfieldID fieldID, jfloat value)
{
	class_init(clazz);
	fieldID->value.f = value;
}


void SetStaticDoubleField (JNIEnv *env, jclass clazz, jfieldID fieldID, jdouble value)
{
	class_init(clazz);
	fieldID->value.d = value;
}


/*****  create new java.lang.String object from an array of Unicode characters ****/ 

jstring NewString (JNIEnv *env, const jchar *buf, jsize len)
{
	u4 i;
	java_lang_String *s;
	java_chararray *a;
	
	s = (java_lang_String*) builtin_new (class_java_lang_String);
	a = builtin_newarray_char (len);

	/* javastring or characterarray could not be created */
	if ( (!a) || (!s) ) return NULL;

	/* copy text */
	for (i=0; i<len; i++) a->data[i] = buf[i];
	s -> value = a;
	s -> offset = 0;
	s -> count = len;

	return (jstring) s;
}

/******************* returns the length of a Java string ***************************/

jsize GetStringLength (JNIEnv *env, jstring str)
{
	return ((java_lang_String*) str)->count;
}


/********************  convertes javastring to u2-array ****************************/
	
u2 *javastring_tou2 (jstring so) 
{
	java_lang_String *s = (java_lang_String*) so;
	java_chararray *a;
	u4 i;
	u2 *stringbuffer;
	
	if (!s) return NULL;

	a = s->value;
	if (!a) return NULL;

	/* allocate memory */
	stringbuffer = MNEW( u2 , s->count + 1 );

	/* copy text */
	for (i=0; i<s->count; i++) stringbuffer[i] = a->data[s->offset+i];
	
	/* terminate string */
	stringbuffer[i] = '\0';

	return stringbuffer;
}

/********* returns a pointer to an array of Unicode characters of the string *******/

const jchar *GetStringChars (JNIEnv *env, jstring str, jboolean *isCopy)
{	
	return javastring_tou2(str);
}

/**************** native code no longer needs access to chars **********************/

void ReleaseStringChars (JNIEnv *env, jstring str, const jchar *chars)
{
	MFREE(((jchar*) chars),jchar,((java_lang_String*) str)->count);
}

/************ create new java.lang.String object from utf8-characterarray **********/

jstring NewStringUTF (JNIEnv *env, const char *utf)
{
    log_text("NewStringUTF called");
}

/****************** returns the utf8 length in bytes of a string *******************/

jsize GetStringUTFLength (JNIEnv *env, jstring string)
{   
    java_lang_String *s = (java_lang_String*) string;

    return (jsize) u2_utflength(s->value->data, s->count); 
}

/************ converts a Javastring to an array of UTF-8 characters ****************/

const char* GetStringUTFChars (JNIEnv *env, jstring string, jboolean *isCopy)
{
    return javastring_toutf((java_lang_String*) string,false)->text;
}

/***************** native code no longer needs access to utf ***********************/

void ReleaseStringUTFChars (JNIEnv *env, jstring str, const char* chars)
{
    log_text("JNI-Call: ReleaseStringUTFChars");
}

/************************** array operations ***************************************/

jsize GetArrayLength (JNIEnv *env, jarray array)
{
    return array->size;
}

jobjectArray NewObjectArray (JNIEnv *env, jsize len, jclass clazz, jobject init)
{
    java_objectarray *j = builtin_anewarray (len, clazz);
    if (!j) exceptionptr = proto_java_lang_OutOfMemoryError;
    return j;
}

jobject GetObjectArrayElement (JNIEnv *env, jobjectArray array, jsize index)
{
    jobject j = NULL;

    if (index<array->header.size)	
	j = array->data[index];
    else
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    
    return j;
}

void SetObjectArrayElement (JNIEnv *env, jobjectArray array, jsize index, jobject val)
{
    if (index>=array->header.size)	
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else {

	/* check if the class of value is a subclass of the element class of the array */

	if (!builtin_instanceof(val, array->elementtype))
	    exceptionptr = proto_java_lang_ArrayStoreException;
	else
	    array->data[index] = val;
    }
}



jbooleanArray NewBooleanArray (JNIEnv *env, jsize len)
{
    java_booleanarray *j = builtin_newarray_boolean(len);
    if (!j) exceptionptr = proto_java_lang_OutOfMemoryError;
    return j;
}


jbyteArray NewByteArray (JNIEnv *env, jsize len)
{
    java_bytearray *j = builtin_newarray_byte(len);
    if (!j) exceptionptr = proto_java_lang_OutOfMemoryError;
    return j;
}


jcharArray NewCharArray (JNIEnv *env, jsize len)
{
    java_chararray *j = builtin_newarray_char(len);
    if (!j) exceptionptr = proto_java_lang_OutOfMemoryError;
    return j;
}


jshortArray NewShortArray (JNIEnv *env, jsize len)
{
    java_shortarray *j = builtin_newarray_short(len);   
    if (!j) exceptionptr = proto_java_lang_OutOfMemoryError;
    return j;
}


jintArray NewIntArray (JNIEnv *env, jsize len)
{
    java_intarray *j = builtin_newarray_int(len);
    if (!j) exceptionptr = proto_java_lang_OutOfMemoryError;
    return j;
}


jlongArray NewLongArray (JNIEnv *env, jsize len)
{
    java_longarray *j = builtin_newarray_long(len);
    if (!j) exceptionptr = proto_java_lang_OutOfMemoryError;
    return j;
}


jfloatArray NewFloatArray (JNIEnv *env, jsize len)
{
    java_floatarray *j = builtin_newarray_float(len);
    if (!j) exceptionptr = proto_java_lang_OutOfMemoryError;
    return j;
}


jdoubleArray NewDoubleArray (JNIEnv *env, jsize len)
{
    java_doublearray *j = builtin_newarray_double(len);
    if (!j) exceptionptr = proto_java_lang_OutOfMemoryError;
    return j;
}


jboolean * GetBooleanArrayElements (JNIEnv *env, jbooleanArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jbyte * GetByteArrayElements (JNIEnv *env, jbyteArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jchar * GetCharArrayElements (JNIEnv *env, jcharArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jshort * GetShortArrayElements (JNIEnv *env, jshortArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jint * GetIntArrayElements (JNIEnv *env, jintArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jlong * GetLongArrayElements (JNIEnv *env, jlongArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jfloat * GetFloatArrayElements (JNIEnv *env, jfloatArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jdouble * GetDoubleArrayElements (JNIEnv *env, jdoubleArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}



void ReleaseBooleanArrayElements (JNIEnv *env, jbooleanArray array, jboolean *elems, jint mode)
{
    /* empty */
}


void ReleaseByteArrayElements (JNIEnv *env, jbyteArray array, jbyte *elems, jint mode)
{
    /* empty */
}


void ReleaseCharArrayElements (JNIEnv *env, jcharArray array, jchar *elems, jint mode)
{
    /* empty */
}


void ReleaseShortArrayElements (JNIEnv *env, jshortArray array, jshort *elems, jint mode)
{
    /* empty */
}


void ReleaseIntArrayElements (JNIEnv *env, jintArray array, jint *elems, jint mode)
{
    /* empty */
}


void ReleaseLongArrayElements (JNIEnv *env, jlongArray array, jlong *elems, jint mode)
{
    /* empty */
}


void ReleaseFloatArrayElements (JNIEnv *env, jfloatArray array, jfloat *elems, jint mode)
{
    /* empty */
}


void ReleaseDoubleArrayElements (JNIEnv *env, jdoubleArray array, jdouble *elems, jint mode)
{
    /* empty */
}

void GetBooleanArrayRegion (JNIEnv* env, jbooleanArray array, jsize start, jsize len, jboolean *buf)
{
    if (start<0 || len<0 || start+len>array->header.size)
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(buf,&array->data[start],len*sizeof(array->data[0]));	
}


void GetByteArrayRegion (JNIEnv* env, jbyteArray array, jsize start, jsize len, jbyte *buf)
{
    if (start<0 || len<0 || start+len>array->header.size) 
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(buf,&array->data[start],len*sizeof(array->data[0]));
}


void GetCharArrayRegion (JNIEnv* env, jcharArray array, jsize start, jsize len, jchar *buf)
{
    if (start<0 || len<0 || start+len>array->header.size)
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(buf,&array->data[start],len*sizeof(array->data[0]));	
}


void GetShortArrayRegion (JNIEnv* env, jshortArray array, jsize start, jsize len, jshort *buf)
{
    if (start<0 || len<0 || start+len>array->header.size)
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else	
	memcpy(buf,&array->data[start],len*sizeof(array->data[0]));	
}


void GetIntArrayRegion (JNIEnv* env, jintArray array, jsize start, jsize len, jint *buf)
{
    if (start<0 || len<0 || start+len>array->header.size) 	
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(buf,&array->data[start],len*sizeof(array->data[0]));	
}


void GetLongArrayRegion (JNIEnv* env, jlongArray array, jsize start, jsize len, jlong *buf)
{
    if (start<0 || len<0 || start+len>array->header.size) 	
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(buf,&array->data[start],len*sizeof(array->data[0]));	
}


void GetFloatArrayRegion (JNIEnv* env, jfloatArray array, jsize start, jsize len, jfloat *buf)
{
    if (start<0 || len<0 || start+len>array->header.size) 	
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(buf,&array->data[start],len*sizeof(array->data[0]));	
}


void GetDoubleArrayRegion (JNIEnv* env, jdoubleArray array, jsize start, jsize len, jdouble *buf)
{
    if (start<0 || len<0 || start+len>array->header.size) 
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(buf,&array->data[start],len*sizeof(array->data[0]));	
}


void SetBooleanArrayRegion (JNIEnv* env, jbooleanArray array, jsize start, jsize len, jboolean *buf)
{
    if (start<0 || len<0 || start+len>array->header.size)
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(&array->data[start],buf,len*sizeof(array->data[0]));	
}


void SetByteArrayRegion (JNIEnv* env, jbyteArray array, jsize start, jsize len, jbyte *buf)
{
    if (start<0 || len<0 || start+len>array->header.size)
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(&array->data[start],buf,len*sizeof(array->data[0]));	
}


void SetCharArrayRegion (JNIEnv* env, jcharArray array, jsize start, jsize len, jchar *buf)
{
    if (start<0 || len<0 || start+len>array->header.size)
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(&array->data[start],buf,len*sizeof(array->data[0]));	

}


void SetShortArrayRegion (JNIEnv* env, jshortArray array, jsize start, jsize len, jshort *buf)
{
    if (start<0 || len<0 || start+len>array->header.size)
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(&array->data[start],buf,len*sizeof(array->data[0]));	
}


void SetIntArrayRegion (JNIEnv* env, jintArray array, jsize start, jsize len, jint *buf)
{
    if (start<0 || len<0 || start+len>array->header.size)
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(&array->data[start],buf,len*sizeof(array->data[0]));	

}

void SetLongArrayRegion (JNIEnv* env, jlongArray array, jsize start, jsize len, jlong *buf)
{
    if (start<0 || len<0 || start+len>array->header.size)
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(&array->data[start],buf,len*sizeof(array->data[0]));	

}

void SetFloatArrayRegion (JNIEnv* env, jfloatArray array, jsize start, jsize len, jfloat *buf)
{
    if (start<0 || len<0 || start+len>array->header.size)
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(&array->data[start],buf,len*sizeof(array->data[0]));	

}

void SetDoubleArrayRegion (JNIEnv* env, jdoubleArray array, jsize start, jsize len, jdouble *buf)
{
    if (start<0 || len<0 || start+len>array->header.size)
	exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException;
    else
	memcpy(&array->data[start],buf,len*sizeof(array->data[0]));	
}

jint RegisterNatives (JNIEnv* env, jclass clazz, const JNINativeMethod *methods, jint nMethods)
{
    log_text("JNI-Call: RegisterNatives");
    return 0;
}


jint UnregisterNatives (JNIEnv* env, jclass clazz)
{
    log_text("JNI-Call: UnregisterNatives");
    return 0;
}

/******************************* monitor operations ********************************/

jint MonitorEnter (JNIEnv* env, jobject obj)
{
    builtin_monitorenter(obj);
    return 0;
}


jint MonitorExit (JNIEnv* env, jobject obj)
{
    builtin_monitorexit(obj);
    return 0;
}


/************************************* JavaVM interface ****************************/

jint GetJavaVM (JNIEnv* env, JavaVM **vm)
{
    log_text("JNI-Call: GetJavaVM");
    return 0;
}

void GetStringRegion (JNIEnv* env, jstring str, jsize start, jsize len, jchar *buf)
{
    log_text("JNI-Call: GetStringRegion");

}

void GetStringUTFRegion (JNIEnv* env, jstring str, jsize start, jsize len, char *buf)
{
    log_text("JNI-Call: GetStringUTFRegion");

}

/****************** obtain direct pointer to array elements ***********************/

void * GetPrimitiveArrayCritical (JNIEnv* env, jarray array, jboolean *isCopy)
{
	java_arrayheader *s = (java_arrayheader*) array;

	switch (s->arraytype) {
		case ARRAYTYPE_BYTE:   	return (((java_bytearray*)    array)->data);
		case ARRAYTYPE_BOOLEAN:	return (((java_booleanarray*) array)->data);
		case ARRAYTYPE_CHAR:   	return (((java_chararray*)    array)->data);
		case ARRAYTYPE_SHORT:  	return (((java_shortarray*)   array)->data);
		case ARRAYTYPE_INT:	return (((java_intarray*)     array)->data);
		case ARRAYTYPE_LONG:	return (((java_longarray*)    array)->data);
		case ARRAYTYPE_FLOAT:	return (((java_floatarray*)   array)->data);
		case ARRAYTYPE_DOUBLE:	return (((java_doublearray*)  array)->data);
		case ARRAYTYPE_OBJECT:	return (((java_objectarray*)  array)->data); 
		case ARRAYTYPE_ARRAY:	return (((java_arrayarray*)   array)->data);
	}

	return NULL;
}


void ReleasePrimitiveArrayCritical (JNIEnv* env, jarray array, void *carray, jint mode)
{
	log_text("JNI-Call: ReleasePrimitiveArrayCritical");

	/* empty */
}

/********* returns a pointer to an array of Unicode characters of the string *******/

const jchar * GetStringCritical (JNIEnv* env, jstring string, jboolean *isCopy)
{
	log_text("JNI-Call: GetStringCritical");

	return GetStringChars(env,string,isCopy);
}

/**************** native code no longer needs access to chars **********************/

void ReleaseStringCritical (JNIEnv* env, jstring string, const jchar *cstring)
{
	log_text("JNI-Call: ReleaseStringCritical");

	ReleaseStringChars(env,string,cstring);
}


jweak NewWeakGlobalRef (JNIEnv* env, jobject obj)
{
	log_text("JNI-Call: NewWeakGlobalRef");

	return obj;
}


void DeleteWeakGlobalRef (JNIEnv* env, jweak ref)
{
	log_text("JNI-Call: DeleteWeakGlobalRef");

	/* empty */
}


/******************************* check for pending exception ***********************/


jboolean ExceptionCheck (JNIEnv* env)
{
	log_text("JNI-Call: ExceptionCheck");

	return exceptionptr ? JNI_TRUE : JNI_FALSE;
}
       
/********************************* JNI function table ******************************/

JNIEnv env =     
   {   
    NULL,
    NULL,
    NULL,
    NULL,    
    &GetVersion,
    &DefineClass,
    &FindClass,
    &FromReflectedMethod,
    &FromReflectedField,
    &ToReflectedMethod,
    &GetSuperclass,
    &IsAssignableForm,
    &ToReflectedField,
    &Throw,
    &ThrowNew,
    &ExceptionOccurred,
    &ExceptionDescribe,
    &ExceptionClear,
    &FatalError,
    &PushLocalFrame,
    &PopLocalFrame,
    &NewGlobalRef,
    &DeleteGlobalRef,
    &DeleteLocalRef,
    &IsSameObject,
    &NewLocalRef,
    &EnsureLocalCapacity,
    &AllocObject,
    &NewObject,
    &NewObjectV,
    &NewObjectA,
    &GetObjectClass,
    &IsInstanceOf,
    &GetMethodID,
    &CallObjectMethod,
    &CallObjectMethodV,
    &CallObjectMethodA,
    &CallBooleanMethod,
    &CallBooleanMethodV,
    &CallBooleanMethodA,
    &CallByteMethod,
    &CallByteMethodV,
    &CallByteMethodA,
    &CallCharMethod,
    &CallCharMethodV,
    &CallCharMethodA,
    &CallShortMethod,
    &CallShortMethodV,
    &CallShortMethodA,
    &CallIntMethod,
    &CallIntMethodV,
    &CallIntMethodA,
    &CallLongMethod,
    &CallLongMethodV,
    &CallLongMethodA,
    &CallFloatMethod,
    &CallFloatMethodV,
    &CallFloatMethodA,
    &CallDoubleMethod,
    &CallDoubleMethodV,
    &CallDoubleMethodA,
    &CallVoidMethod,
    &CallVoidMethodV,
    &CallVoidMethodA,
    &CallNonvirtualObjectMethod,
    &CallNonvirtualObjectMethodV,
    &CallNonvirtualObjectMethodA,
    &CallNonvirtualBooleanMethod,
    &CallNonvirtualBooleanMethodV,
    &CallNonvirtualBooleanMethodA,
    &CallNonvirtualByteMethod,
    &CallNonvirtualByteMethodV,
    &CallNonvirtualByteMethodA,
    &CallNonvirtualCharMethod,
    &CallNonvirtualCharMethodV,
    &CallNonvirtualCharMethodA,
    &CallNonvirtualShortMethod,
    &CallNonvirtualShortMethodV,
    &CallNonvirtualShortMethodA,
    &CallNonvirtualIntMethod,
    &CallNonvirtualIntMethodV,
    &CallNonvirtualIntMethodA,
    &CallNonvirtualLongMethod,
    &CallNonvirtualLongMethodV,
    &CallNonvirtualLongMethodA,
    &CallNonvirtualFloatMethod,
    &CallNonvirtualFloatMethodV,
    &CallNonvirtualFloatMethodA,
    &CallNonvirtualDoubleMethod,
    &CallNonvirtualDoubleMethodV,
    &CallNonvirtualDoubleMethodA,
    &CallNonvirtualVoidMethod,
    &CallNonvirtualVoidMethodV,
    &CallNonvirtualVoidMethodA,
    &GetFieldID,
    &GetObjectField,
    &GetBooleanField,
    &GetByteField,
    &GetCharField,
    &GetShortField,
    &GetIntField,
    &GetLongField,
    &GetFloatField,
    &GetDoubleField,
    &SetObjectField,
    &SetBooleanField,
    &SetByteField,
    &SetCharField,
    &SetShortField,
    &SetIntField,
    &SetLongField,
    &SetFloatField,
    &SetDoubleField,
    &GetStaticMethodID,
    &CallStaticObjectMethod,
    &CallStaticObjectMethodV,
    &CallStaticObjectMethodA,
    &CallStaticBooleanMethod,
    &CallStaticBooleanMethodV,
    &CallStaticBooleanMethodA,
    &CallStaticByteMethod,
    &CallStaticByteMethodV,
    &CallStaticByteMethodA,
    &CallStaticCharMethod,
    &CallStaticCharMethodV,
    &CallStaticCharMethodA,
    &CallStaticShortMethod,
    &CallStaticShortMethodV,
    &CallStaticShortMethodA,
    &CallStaticIntMethod,
    &CallStaticIntMethodV,
    &CallStaticIntMethodA,
    &CallStaticLongMethod,
    &CallStaticLongMethodV,
    &CallStaticLongMethodA,
    &CallStaticFloatMethod,
    &CallStaticFloatMethodV,
    &CallStaticFloatMethodA,
    &CallStaticDoubleMethod,
    &CallStaticDoubleMethodV,
    &CallStaticDoubleMethodA,
    &CallStaticVoidMethod,
    &CallStaticVoidMethodV,
    &CallStaticVoidMethodA,
    &GetStaticFieldID,
    &GetStaticObjectField,
    &GetStaticBooleanField,
    &GetStaticByteField,
    &GetStaticCharField,
    &GetStaticShortField,
    &GetStaticIntField,
    &GetStaticLongField,
    &GetStaticFloatField,
    &GetStaticDoubleField,
    &SetStaticObjectField,
    &SetStaticBooleanField,
    &SetStaticByteField,
    &SetStaticCharField,
    &SetStaticShortField,
    &SetStaticIntField,
    &SetStaticLongField,
    &SetStaticFloatField,
    &SetStaticDoubleField,
    &NewString,
    &GetStringLength,
    &GetStringChars,
    &ReleaseStringChars,
    &NewStringUTF,
    &GetStringUTFLength,
    &GetStringUTFChars,
    &ReleaseStringUTFChars,
    &GetArrayLength,
    &NewObjectArray,
    &GetObjectArrayElement,
    &SetObjectArrayElement,
    &NewBooleanArray,
    &NewByteArray,
    &NewCharArray,
    &NewShortArray,
    &NewIntArray,
    &NewLongArray,
    &NewFloatArray,
    &NewDoubleArray,
    &GetBooleanArrayElements,
    &GetByteArrayElements,
    &GetCharArrayElements,
    &GetShortArrayElements,
    &GetIntArrayElements,
    &GetLongArrayElements,
    &GetFloatArrayElements,
    &GetDoubleArrayElements,
    &ReleaseBooleanArrayElements,
    &ReleaseByteArrayElements,
    &ReleaseCharArrayElements,
    &ReleaseShortArrayElements,
    &ReleaseIntArrayElements,
    &ReleaseLongArrayElements,
    &ReleaseFloatArrayElements,
    &ReleaseDoubleArrayElements,
    &GetBooleanArrayRegion,
    &GetByteArrayRegion,
    &GetCharArrayRegion,
    &GetShortArrayRegion,
    &GetIntArrayRegion,
    &GetLongArrayRegion,
    &GetFloatArrayRegion,
    &GetDoubleArrayRegion,
    &SetBooleanArrayRegion,
    &SetByteArrayRegion,
    &SetCharArrayRegion,
    &SetShortArrayRegion,
    &SetIntArrayRegion,
    &SetLongArrayRegion,
    &SetFloatArrayRegion,
    &SetDoubleArrayRegion,
    &RegisterNatives,
    &UnregisterNatives,
    &MonitorEnter,
    &MonitorExit,
    &GetJavaVM,
    &GetStringRegion,
    &GetStringUTFRegion,
    &GetPrimitiveArrayCritical,
    &ReleasePrimitiveArrayCritical,
    &GetStringCritical,
    &ReleaseStringCritical,
    &NewWeakGlobalRef,
    &DeleteWeakGlobalRef,
    &ExceptionCheck
    };

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
