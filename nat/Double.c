/* class: java/lang/Double */

/*
 * Class:     java/lang/Double
 * Method:    doubleToLongBits
 * Signature: (D)J
 */
JNIEXPORT s8 JNICALL Java_java_lang_Double_doubleToLongBits ( JNIEnv *env ,  double par1)
{
	s8 l;
	double d = par1;
	memcpy ((u1*) &l, (u1*) &d, 8);
	return l;
}

/*
 * Class:     java/lang/Double
 * Method:    longBitsToDouble
 * Signature: (J)D
 */
JNIEXPORT double JNICALL Java_java_lang_Double_longBitsToDouble ( JNIEnv *env ,  s8 par1)
{
	s8 l = par1;
	double d;
	memcpy ((u1*) &d, (u1*) &l, 8);
	return d;
}
