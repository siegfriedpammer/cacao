/* class: java/lang/Math */

/*
 * Class:     java/lang/Math
 * Method:    IEEEremainder
 * Signature: (DD)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_IEEEremainder ( JNIEnv *env ,  double a, double b)
{
    return remainder(a, b);
}

/*
 * Class:     java/lang/Math
 * Method:    acos
 * Signature: (D)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_acos ( JNIEnv *env ,  double par1)
{
	return acos(par1);
}

/*
 * Class:     java/lang/Math
 * Method:    asin
 * Signature: (D)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_asin ( JNIEnv *env ,  double par1)
{
	return asin(par1);
}

/*
 * Class:     java/lang/Math
 * Method:    atan
 * Signature: (D)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_atan ( JNIEnv *env ,  double par1)
{
	return atan(par1);
}

/*
 * Class:     java/lang/Math
 * Method:    atan2
 * Signature: (DD)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_atan2 ( JNIEnv *env ,  double par1, double par2)
{
	return atan2(par1,par2);
}

/*
 * Class:     java/lang/Math
 * Method:    ceil
 * Signature: (D)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_ceil ( JNIEnv *env ,  double par1)
{
	return ceil(par1);
}

/*
 * Class:     java/lang/Math
 * Method:    cos
 * Signature: (D)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_cos ( JNIEnv *env ,  double par1)
{
	return cos(par1);
}

/*
 * Class:     java/lang/Math
 * Method:    exp
 * Signature: (D)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_exp ( JNIEnv *env ,  double par1)
{
	return exp(par1);
}

/*
 * Class:     java/lang/Math
 * Method:    floor
 * Signature: (D)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_floor ( JNIEnv *env ,  double par1)
{
	return floor(par1);
}

/*
 * Class:     java/lang/Math
 * Method:    log
 * Signature: (D)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_log ( JNIEnv *env ,  double par1)
{
	if (par1<0.0) {
		exceptionptr = proto_java_lang_ArithmeticException;
		return 0.0;
		}
	return log(par1);
}

/*
 * Class:     java/lang/Math
 * Method:    pow
 * Signature: (DD)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_pow ( JNIEnv *env ,  double par1, double par2)
{
	return pow(par1,par2);
}

/*
 * Class:     java/lang/Math
 * Method:    rint
 * Signature: (D)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_rint ( JNIEnv *env ,  double par1)
{
	return rint(par1); /* phil, 1998/12/12 */
}

/*
 * Class:     java/lang/Math
 * Method:    sin
 * Signature: (D)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_sin ( JNIEnv *env ,  double par1)
{
	return sin(par1);
}

/*
 * Class:     java/lang/Math
 * Method:    sqrt
 * Signature: (D)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_sqrt ( JNIEnv *env ,  double par1)
{
	if (par1<0.0) {
		exceptionptr = proto_java_lang_ArithmeticException;
		return 0.0;
		}
	return sqrt(par1);
}

/*
 * Class:     java/lang/Math
 * Method:    tan
 * Signature: (D)D
 */
JNIEXPORT double JNICALL Java_java_lang_Math_tan ( JNIEnv *env ,  double par1)
{
	return tan(par1);
}
