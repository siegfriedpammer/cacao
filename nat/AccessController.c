/* class: java/security/AccessController */

int java_security_AccessController_calls(int classIndex, int callCnt)
{

//java/security/PrivilegedAction run ()Ljava/lang/Object;
//java/security/PrivilegedActionException <init> ()V

}

/* wrap exception in PrivilegedActionException */

void check_for_exception(JNIEnv *env)
{
	if (exceptionptr) {

	  java_security_PrivilegedActionException *wrapped_exception; 

	  /* create exception object */	  
	  wrapped_exception = (java_security_PrivilegedActionException *) 
	                      native_new_and_init(class_java_security_PrivilegedActionException);
	  
	  /* set field */
	  wrapped_exception->exception = (java_lang_Exception*) exceptionptr;

	  /* replace exception by wrapped one */
	  exceptionptr = (java_objectheader*) wrapped_exception;
	}
}


/*
 * Class:     java/security/AccessController
 * Method:    doPrivileged
 * Signature: (Ljava/security/PrivilegedAction;)Ljava/lang/Object;
 */

JNIEXPORT struct java_lang_Object* JNICALL Java_java_security_AccessController_doPrivileged (JNIEnv *env , struct java_security_PrivilegedAction* action)
{
	methodinfo *m;    
	java_lang_Object* o;

	/* find run-Method */
	m = class_resolvemethod (
		action->header.vftbl->class, 
		utf_new_char ("run"), 
		utf_new_char ("()Ljava/lang/Object;")
    	);

	/* illegal PrivilegedAction specified */
	if (!m) panic ("Can not find method 'run' of PrivliegedAction for doPrivileged");

	/* call run-Method */
	o = (java_lang_Object*) asm_calljavafunction (m, action , NULL,NULL,NULL);
	
	/* wrap exception */
	check_for_exception(env);
	return o;
}

/*
 * Class:     java/security/AccessController
 * Method:    doPrivileged
 * Signature: (Ljava/security/PrivilegedAction;Ljava/security/AccessControlContext;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_security_AccessController_doPrivileged0 (JNIEnv *env , struct java_security_PrivilegedAction* action, struct java_security_AccessControlContext* par2)
{
    return Java_java_security_AccessController_doPrivileged(env, action);
}

/*
 * Class:     java/security/AccessController
 * Method:    doPrivileged
 * Signature: (Ljava/security/PrivilegedExceptionAction;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_security_AccessController_doPrivileged1 (JNIEnv *env , struct java_security_PrivilegedExceptionAction* action)
{
	methodinfo *m;    
	java_lang_Object* o;

	/* find run-Method */
	m = class_resolvemethod (
		action->header.vftbl->class, 
		utf_new_char ("run"), 
		utf_new_char ("()Ljava/lang/Object;")
    	);

	/* illegal PrivilegedAction specified */
	if (!m) panic ("Can not find method 'run' of PrivliegedAction for doPrivileged");

    	/* call run-Method */
    	o = (java_lang_Object*) asm_calljavafunction (m, action ,NULL,NULL,NULL);
    	
    	/* wrap exception */
	check_for_exception(env);
	return o;
}

/*
 * Class:     java/security/AccessController
 * Method:    doPrivileged
 * Signature: (Ljava/security/PrivilegedExceptionAction;Ljava/security/AccessControlContext;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_security_AccessController_doPrivileged2 (JNIEnv *env , struct java_security_PrivilegedExceptionAction* action, struct java_security_AccessControlContext* par2)
{
    Java_java_security_AccessController_doPrivileged1(env,action);
}

/*
 * Class:     java/security/AccessController
 * Method:    getInheritedAccessControlContext
 * Signature: ()Ljava/security/AccessControlContext;
 */
JNIEXPORT struct java_security_AccessControlContext* JNICALL Java_java_security_AccessController_getInheritedAccessControlContext (JNIEnv *env )
{
  /* null if there was only privileged system code */
  return NULL;
}

/*
 * Class:     java/security/AccessController
 * Method:    getStackAccessControlContext
 * Signature: ()Ljava/security/AccessControlContext;
 */
JNIEXPORT struct java_security_AccessControlContext* JNICALL Java_java_security_AccessController_getStackAccessControlContext (JNIEnv *env )
{
  /* null if there was only privileged system code */
  return NULL;
}










