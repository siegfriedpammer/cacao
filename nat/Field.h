/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/lang/reflect/Field */

typedef struct java_lang_reflect_Field {
   java_objectheader header;
   s4 override;
   struct java_lang_Class* clazz;
   s4 slot;
   struct java_lang_String* name;
   struct java_lang_Class* type;
   s4 modifiers;
} java_lang_reflect_Field;

/*
 * Class:     java/lang/reflect/Field
 * Method:    get
 * Signature: (Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_Field_get (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1);
/*
 * Class:     java/lang/reflect/Field
 * Method:    getBoolean
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getBoolean (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1);
/*
 * Class:     java/lang/reflect/Field
 * Method:    getByte
 * Signature: (Ljava/lang/Object;)B
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getByte (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1);
/*
 * Class:     java/lang/reflect/Field
 * Method:    getChar
 * Signature: (Ljava/lang/Object;)C
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getChar (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1);
/*
 * Class:     java/lang/reflect/Field
 * Method:    getDouble
 * Signature: (Ljava/lang/Object;)D
 */
JNIEXPORT double JNICALL Java_java_lang_reflect_Field_getDouble (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1);
/*
 * Class:     java/lang/reflect/Field
 * Method:    getFloat
 * Signature: (Ljava/lang/Object;)F
 */
JNIEXPORT float JNICALL Java_java_lang_reflect_Field_getFloat (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1);
/*
 * Class:     java/lang/reflect/Field
 * Method:    getInt
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getInt (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1);
/*
 * Class:     java/lang/reflect/Field
 * Method:    getLong
 * Signature: (Ljava/lang/Object;)J
 */
JNIEXPORT s8 JNICALL Java_java_lang_reflect_Field_getLong (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1);
/*
 * Class:     java/lang/reflect/Field
 * Method:    getShort
 * Signature: (Ljava/lang/Object;)S
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getShort (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1);
/*
 * Class:     java/lang/reflect/Field
 * Method:    set
 * Signature: (Ljava/lang/Object;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_set (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1, struct java_lang_Object* par2);
/*
 * Class:     java/lang/reflect/Field
 * Method:    setBoolean
 * Signature: (Ljava/lang/Object;Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1, s4 par2);
/*
 * Class:     java/lang/reflect/Field
 * Method:    setByte
 * Signature: (Ljava/lang/Object;B)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1, s4 par2);
/*
 * Class:     java/lang/reflect/Field
 * Method:    setChar
 * Signature: (Ljava/lang/Object;C)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1, s4 par2);
/*
 * Class:     java/lang/reflect/Field
 * Method:    setDouble
 * Signature: (Ljava/lang/Object;D)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1, double par2);
/*
 * Class:     java/lang/reflect/Field
 * Method:    setFloat
 * Signature: (Ljava/lang/Object;F)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1, float par2);
/*
 * Class:     java/lang/reflect/Field
 * Method:    setInt
 * Signature: (Ljava/lang/Object;I)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1, s4 par2);
/*
 * Class:     java/lang/reflect/Field
 * Method:    setLong
 * Signature: (Ljava/lang/Object;J)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1, s8 par2);
/*
 * Class:     java/lang/reflect/Field
 * Method:    setShort
 * Signature: (Ljava/lang/Object;S)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort (JNIEnv *env ,  struct java_lang_reflect_Field* this , struct java_lang_Object* par1, s4 par2);
