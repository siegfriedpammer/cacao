/* class: java/lang/reflect/Constructor */


#include "jni.h"
#include "builtin.h"
#include "loader.h"
#include "native.h"
#include "tables.h"
#include "asmpart.h"
#include "java_lang_Class.h"
#include "java_lang_reflect_Constructor.h"


/*
 * Class:     java/lang/reflect/Constructor
 * Method:    newInstance
 * Signature: ([Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_Constructor_constructNative( JNIEnv *env ,  struct java_lang_reflect_Constructor* this, 
	java_objectarray* parameters,struct java_lang_Class* clazz, s4 par3)
{

#warning fix me for parameters float/double and long long  parameters

	methodinfo *m;
        java_objectheader *o;

        
/*	log_text("Java_java_lang_reflect_Constructor_constructNative called");
        utf_display(((struct classinfo*)clazz)->name);*/
        printf("\n");

        /* find initializer */

	if (!parameters) {
		if (this->parameterTypes->header.size!=0) {
			log_text("Parameter count mismatch in Java_java_lang_reflect_Constructor_constructNative(1)");
#warning throw an exception here
			return 0;
		}
	} else
	if (this->parameterTypes->header.size!=parameters->header.size) {
		log_text("Parameter count mismatch in Java_java_lang_reflect_Constructor_constructNative(2)");
#warning throw an exception here
		return 0;
	}

	if (this->slot>=((classinfo*)clazz)->methodscount) {
		log_text("illegal index in methods table");
		return 0;
	}

	o = builtin_new (clazz);         /*          create object */
        if (!o) {
		log_text("Objet instance could not be created");
		return NULL;
	}
        
/*	log_text("o!=NULL\n");*/


        m = &((classinfo*)clazz)->methods[this->slot];
	if (!((m->name == utf_new_char("<init>")) && 
                  (m->descriptor == create_methodsig(this->parameterTypes,"V"))))
	{
                if (verbose) {
                        sprintf(logtext, "Warning: class has no instance-initializer of specified type: ");
                        utf_sprint(logtext + strlen(logtext), ((struct classinfo*)clazz)->name);
                        dolog();
			utf_display( create_methodsig(this->parameterTypes,"V"));
			printf("\n");
			class_showconstantpool(clazz);
                        }
#warning throw an exception here, although this should never happen
                return o;
                }

/*	log_text("calling initializer");*/
        /* call initializer */

	switch (this->parameterTypes->header.size) {
		case 0: exceptionptr=asm_calljavamethod (m, o, NULL, NULL, NULL);
			break;
		case 1: exceptionptr=asm_calljavamethod (m, o, parameters->data[0], NULL, NULL);
			break;
		case 2: exceptionptr=asm_calljavamethod (m, o, parameters->data[0], parameters->data[1], NULL);
			break;
		case 3: exceptionptr=asm_calljavamethod (m, o, parameters->data[0], parameters->data[1], 
				parameters->data[2]);
			break;
		default:
			log_text("Not supported number of arguments in Java_java_lang_reflect_Constructor");
	}
        return o;
}

/*
 * Class:     java_lang_reflect_Constructor
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Constructor_getModifiers (JNIEnv *env ,  struct java_lang_reflect_Constructor* this ) {
	log_text("Java_java_lang_reflect_Constructor_getModifiers called");
        classinfo *c=(classinfo*)(this->clazz);
        if ((this->slot<0) || (this->slot>=c->methodscount))
                panic("error illegal slot for method in class (getReturnType)");
        return (c->methods[this->slot]).flags & (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED);
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
