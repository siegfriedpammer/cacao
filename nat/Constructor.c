/* class: java/lang/reflect/Constructor */


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
        java_objectheader *o = builtin_new (clazz);         /*          create object */

        
/*	log_text("Java_java_lang_reflect_Constructor_constructNative called");
        utf_display(((struct classinfo*)clazz)->name);
        printf("\n");*/
        if (!o) return NULL;
        
/*	printf("o!=NULL\n");*/
        /* find initializer */

	if (!parameters) {
		if (this->parameterTypes->header.size!=0) {
			log_text("Parameter count mismatch in Java_java_lang_reflect_Constructor_constructNative");
#warning throw an exception here
			return 0;
		}
	} else
	if (this->parameterTypes->header.size!=parameters->header.size) {
		log_text("Parameter count mismatch in Java_java_lang_reflect_Constructor_constructNative");
#warning throw an exception here
		return 0;
	}

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
	return (this->flag);
}
