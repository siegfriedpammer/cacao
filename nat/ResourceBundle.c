/* class: java/util/ResourceBundle */

/*
 * Class:     java/util/ResourceBundle
 * Method:    getClassContext
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_util_ResourceBundle_getClassContext ( JNIEnv *env  )
{
    /* not implemented */

    java_objectarray *array_class;
    array_class = builtin_anewarray(3, class_java_lang_Class);
    use_class_as_object(class_java_lang_Class);
    array_class->data[0] = (java_objectheader*) class_java_lang_Class;
    array_class->data[1] = (java_objectheader*) class_java_lang_Class;
    array_class->data[2] = (java_objectheader*) class_java_lang_Class;
    return array_class;
}
