/* class: java/lang/Double */

#include "native-math.h"

/*
 * Class:     java/lang/Double
 * Method:    doubleToLongBits
 * Signature: (D)J
 */
JNIEXPORT s8 JNICALL Java_java_lang_Double_doubleToLongBits(JNIEnv *env, double par1)
{
    union {
        s8 l;
        double d;
    } d;

    d.d = par1;

    return d.l;
}

/*
 * Class:     java/lang/Double
 * Method:    longBitsToDouble
 * Signature: (J)D
 */
JNIEXPORT double JNICALL Java_java_lang_Double_longBitsToDouble(JNIEnv *env, s8 par1)
{
    union {
        s8 l;
        double d;
    } d;

    d.l = par1;

    if (isnan(d.d)) {
        d.d = DBL_NAN;
    }
    
    return d.d;
}
