/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/security/AccessController */

typedef struct java_security_AccessController {
   java_objectheader header;
} java_security_AccessController;

/*
 * Class:     java/security/AccessController
 * Method:    doPrivileged
 * Signature: (Ljava/security/PrivilegedAction;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_security_AccessController_doPrivileged (JNIEnv *env , struct java_security_PrivilegedAction* par1);
/*
 * Class:     java/security/AccessController
 * Method:    doPrivileged
 * Signature: (Ljava/security/PrivilegedAction;Ljava/security/AccessControlContext;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_security_AccessController0_doPrivileged (JNIEnv *env , struct java_security_PrivilegedAction* par1, struct java_security_AccessControlContext* par2);
/*
 * Class:     java/security/AccessController
 * Method:    doPrivileged
 * Signature: (Ljava/security/PrivilegedExceptionAction;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_security_AccessController1_doPrivileged (JNIEnv *env , struct java_security_PrivilegedExceptionAction* par1);
/*
 * Class:     java/security/AccessController
 * Method:    doPrivileged
 * Signature: (Ljava/security/PrivilegedExceptionAction;Ljava/security/AccessControlContext;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_security_AccessController2_doPrivileged (JNIEnv *env , struct java_security_PrivilegedExceptionAction* par1, struct java_security_AccessControlContext* par2);
/*
 * Class:     java/security/AccessController
 * Method:    getInheritedAccessControlContext
 * Signature: ()Ljava/security/AccessControlContext;
 */
JNIEXPORT struct java_security_AccessControlContext* JNICALL Java_java_security_AccessController_getInheritedAccessControlContext (JNIEnv *env );
/*
 * Class:     java/security/AccessController
 * Method:    getStackAccessControlContext
 * Signature: ()Ljava/security/AccessControlContext;
 */
JNIEXPORT struct java_security_AccessControlContext* JNICALL Java_java_security_AccessController_getStackAccessControlContext (JNIEnv *env );
