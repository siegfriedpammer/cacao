/* class: java/lang/Float */

/*
 * Class:     java/lang/Float
 * Method:    floatToIntBits
 * Signature: (F)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_Float_floatToIntBits ( JNIEnv *env ,  float par1)
{
	s4 i;
	float f = par1;
	memcpy ((u1*) &i, (u1*) &f, 4);
	return i;
}

/*
 * Class:     java/lang/Float
 * Method:    intBitsToFloat
 * Signature: (I)F
 */
JNIEXPORT float JNICALL Java_java_lang_Float_intBitsToFloat ( JNIEnv *env ,  s4 par1)
{
	s4 i = par1;
	float f;
	memcpy ((u1*) &f, (u1*) &i, 4);
	return f;
}
