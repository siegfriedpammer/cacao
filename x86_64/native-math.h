/* x86_64/native-math.h ********************************************************

    Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

    See file COPYRIGHT for information on usage and disclaimer of warranties

    Contains the machine-specific floating point definitions.

    Authors: Michael Gschwind    EMAIL: cacao@complang.tuwien.ac.at
             Andreas Krall       EMAIL: cacao@complang.tuwien.ac.at
             Christian Thalinger EMAIL: cacao@complang.tuwien.ac.at

    Last Change: $Id: native-math.h 465 2003-09-17 23:14:48Z twisti $

*******************************************************************************/

#ifndef _NATIVE_MATH_H
#define _NATIVE_MATH_H

/* include machine-specific math.h */

#include <math.h>

/* define infinity for floating point numbers */

static u4 flt_nan    = 0x7fc00000;
static u4 flt_posinf = 0x7f800000;
static u4 flt_neginf = 0xff800000;

#define FLT_NAN     (*((float*) (&flt_nan)))
#define FLT_POSINF  (*((float*) (&flt_posinf)))
#define FLT_NEGINF  (*((float*) (&flt_neginf)))

/* define infinity for double floating point numbers */

static u8 dbl_nan    = 0x7ff8000000000000L;
static u8 dbl_posinf = 0x7ff0000000000000L;
static u8 dbl_neginf = 0xfff0000000000000L;

#define DBL_NAN     (*((double*) (&dbl_nan)))
#define DBL_POSINF  (*((double*) (&dbl_posinf)))
#define DBL_NEGINF  (*((double*) (&dbl_neginf)))

#endif
