/* class: java/lang/Float */

#include "native-math.h"

/*
 * Class:     java/lang/Float
 * Method:    floatToIntBits
 * Signature: (F)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_Float_floatToIntBits ( JNIEnv *env ,  float par1)
{
    union {
        s4 i;
        float f;
    } d;

    d.f = par1;

    return d.i;
}

/*
 * Class:     java/lang/Float
 * Method:    intBitsToFloat
 * Signature: (I)F
 */
JNIEXPORT float JNICALL Java_java_lang_Float_intBitsToFloat ( JNIEnv *env ,  s4 par1)
{
    union {
        s4 i;
        float f;
    } d;
    
    d.i = par1;

    if (isnan(d.f)) {
        d.f = FLT_NAN;
    }

    return d.f;
}
