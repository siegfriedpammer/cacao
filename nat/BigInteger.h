/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/math/BigInteger */

typedef struct java_math_BigInteger {
   java_objectheader header;
   s4 signum;
   java_bytearray* magnitude;
   s4 bitCount;
   s4 bitLength;
   s4 lowestSetBit;
   s4 firstNonzeroByteNum;
} java_math_BigInteger;

/*
 * Class:     java/math/BigInteger
 * Method:    plumbAdd
 * Signature: ([B[B)[B
 */
JNIEXPORT java_bytearray* JNICALL Java_java_math_BigInteger_plumbAdd (JNIEnv *env , java_bytearray* par1, java_bytearray* par2);
/*
 * Class:     java/math/BigInteger
 * Method:    plumbDivide
 * Signature: ([B[B)[B
 */
JNIEXPORT java_bytearray* JNICALL Java_java_math_BigInteger_plumbDivide (JNIEnv *env , java_bytearray* par1, java_bytearray* par2);
/*
 * Class:     java/math/BigInteger
 * Method:    plumbDivideAndRemainder
 * Signature: ([B[B)[[B
 */
JNIEXPORT java_arrayarray* JNICALL Java_java_math_BigInteger_plumbDivideAndRemainder (JNIEnv *env , java_bytearray* par1, java_bytearray* par2);
/*
 * Class:     java/math/BigInteger
 * Method:    plumbGcd
 * Signature: ([B[B)[B
 */
JNIEXPORT java_bytearray* JNICALL Java_java_math_BigInteger_plumbGcd (JNIEnv *env , java_bytearray* par1, java_bytearray* par2);
/*
 * Class:     java/math/BigInteger
 * Method:    plumbGeneratePrime
 * Signature: ([B)[B
 */
JNIEXPORT java_bytearray* JNICALL Java_java_math_BigInteger_plumbGeneratePrime (JNIEnv *env , java_bytearray* par1);
/*
 * Class:     java/math/BigInteger
 * Method:    plumbInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_math_BigInteger_plumbInit (JNIEnv *env );
/*
 * Class:     java/math/BigInteger
 * Method:    plumbModInverse
 * Signature: ([B[B)[B
 */
JNIEXPORT java_bytearray* JNICALL Java_java_math_BigInteger_plumbModInverse (JNIEnv *env , java_bytearray* par1, java_bytearray* par2);
/*
 * Class:     java/math/BigInteger
 * Method:    plumbModPow
 * Signature: ([B[B[B)[B
 */
JNIEXPORT java_bytearray* JNICALL Java_java_math_BigInteger_plumbModPow (JNIEnv *env , java_bytearray* par1, java_bytearray* par2, java_bytearray* par3);
/*
 * Class:     java/math/BigInteger
 * Method:    plumbMultiply
 * Signature: ([B[B)[B
 */
JNIEXPORT java_bytearray* JNICALL Java_java_math_BigInteger_plumbMultiply (JNIEnv *env , java_bytearray* par1, java_bytearray* par2);
/*
 * Class:     java/math/BigInteger
 * Method:    plumbRemainder
 * Signature: ([B[B)[B
 */
JNIEXPORT java_bytearray* JNICALL Java_java_math_BigInteger_plumbRemainder (JNIEnv *env , java_bytearray* par1, java_bytearray* par2);
/*
 * Class:     java/math/BigInteger
 * Method:    plumbSquare
 * Signature: ([B)[B
 */
JNIEXPORT java_bytearray* JNICALL Java_java_math_BigInteger_plumbSquare (JNIEnv *env , java_bytearray* par1);
/*
 * Class:     java/math/BigInteger
 * Method:    plumbSubtract
 * Signature: ([B[B)Ljava/math/BigInteger;
 */
JNIEXPORT struct java_math_BigInteger* JNICALL Java_java_math_BigInteger_plumbSubtract (JNIEnv *env , java_bytearray* par1, java_bytearray* par2);
