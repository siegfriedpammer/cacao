#include "global.h"
#include "jni.h"
#include "java_lang_Throwable.h"
#include "java_lang_VMThrowable.h"
#include "asmpart.h"
#include "loader.h"
#include "builtin.h"
#include "tables.h"
#include "native.h"


/*
 * Class:     java/lang/VMThrowable
 * Method:    fillInStackTrace
 * Signature: (Ljava/lang/Throwable;)Ljava/lang/VMThrowable;
 */
JNIEXPORT struct java_lang_VMThrowable* JNICALL Java_java_lang_VMThrowable_fillInStackTrace(JNIEnv *env,
	 jclass clazz, struct java_lang_Throwable* par1) {

	classinfo *class_java_lang_VMThrowable=0;
	java_lang_VMThrowable *vmthrow;

/*	log_text("java_lang_VMThrowable");*/

        if (!class_java_lang_VMThrowable)
                class_java_lang_VMThrowable = class_new(utf_new_char ("java/lang/VMThrowable"));

	if (class_java_lang_VMThrowable==0)
		panic("Needed class java.lang.VMThrowable missing");

	vmthrow=(java_lang_VMThrowable*)native_new_and_init(class_java_lang_VMThrowable);

	if (vmthrow==0)
		panic("Needed instance of class  java.lang.VMThrowable could not be created");
	

	(void)asm_get_stackTrace(&(vmthrow->vmData));

	return vmthrow;
}



java_objectarray* generateStackTraceArray(JNIEnv *env,stacktraceelement *source,long size) {
	long resultPos;
	methodinfo *constructor;
	classinfo *class_stacktraceelement;
	java_objectarray *array_stacktraceelement;
	class_stacktraceelement = (classinfo *) loader_load(utf_new_char ("java/lang/StackTraceElement"));

	if (!class_stacktraceelement)
		return 0;


	constructor=class_findmethod(class_stacktraceelement,utf_new_char("<init>"),
		utf_new_char("(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Z)V"));
	if (!constructor)
		panic("java.lang.StackTraceElement misses needed constructor");	

	array_stacktraceelement = builtin_anewarray(size, class_stacktraceelement);

	if (!array_stacktraceelement)
                return 0;

/*	printf("Should return an array with %ld element(s)\n",size);*/
	size--;
	
	
	for(resultPos=0;size>=0;resultPos++,size--) {
		java_objectheader *element=builtin_new(class_stacktraceelement);
		if (!element) {
			panic("Memory for stack trace element could not be allocated");
		}
#ifdef __GNUC__
#warning call constructor once jni is fixed to allow more than three parameters
#endif
#if 0
		(*env)->CallVoidMethod(env,element,constructor,
			javastring_new(source[size].method->class->sourcefile),
			source[size].linenumber,
			javastring_new(source[size].method->class->name),
			javastring_new(source[size].method->name),
			source[size].method->flags & ACC_NATIVE);
#else
		if (!(source[size].method->flags & ACC_NATIVE))setfield_critical(class_stacktraceelement,element,"fileName",          
		"Ljava/lang/String;",  jobject, 
		(jobject) javastring_new(source[size].method->class->sourcefile));
		setfield_critical(class_stacktraceelement,element,"className",          "Ljava/lang/String;",  jobject, 
		(jobject) javastring_new(source[size].method->class->name));
		setfield_critical(class_stacktraceelement,element,"methodName",          "Ljava/lang/String;",  jobject, 
		(jobject) javastring_new(source[size].method->name));
		setfield_critical(class_stacktraceelement,element,"lineNumber",          "I",  jint, 
		(jint) ((source[size].method->flags & ACC_NATIVE) ? -1:(source[size].linenumber)));
		setfield_critical(class_stacktraceelement,element,"isNative",          "Z",  jboolean, 
		(jboolean) ((source[size].method->flags & ACC_NATIVE) ? 1:0));


#endif			

		array_stacktraceelement->data[resultPos]=element;
	}

	return array_stacktraceelement;

}



/*
 * Class:     java/lang/VMThrowable
 * Method:    getStackTrace
 * Signature: (Ljava/lang/Throwable;)[Ljava/lang/StackTraceElement;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMThrowable_getStackTrace(JNIEnv *env, 
	struct java_lang_VMThrowable* this, struct java_lang_Throwable* par1) {

	long  pos;
	long  maxpos;
	utf*  classname=par1->header.vftbl->class->name;
	utf*  init=utf_new_char("<init>");
	utf*  throwable=utf_new_char("java/lang/Throwable");
	stacktraceelement *el=(stacktraceelement*)this->vmData;

/*	log_text("Java_java_lang_VMThrowable_getStackTrace");
	utf_display(par1->header.vftbl->class->name);
	printf("\n----------------------------------------------\n");*/
	
	for (pos=0;el[pos].method!=0;pos++);
	if (pos==0) {
		panic("Stacktrace cannot have zero length");
	}
	pos--;
	pos--;
	maxpos=pos;
	if ( (el[pos].method->class->name==throwable)  && (el[pos].method->name==init)  ) {
		for (; (pos>=0) && (el[pos].method->name==init) && (el[pos].method->class->name!=classname);pos--);
		pos --;
		if (pos<0) {
			panic("Invalid stack trace for Throwable.getStackTrace()");
		}
	}
	
	/* build the result array*/
	pos++; /*arraysize*/
	return generateStackTraceArray(env,el,pos);	

}





