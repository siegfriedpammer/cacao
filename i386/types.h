/* i386/types.h ****************************************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	machine specific definitions for the i386 processor
	
	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at
	         Michael Gschwind    EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 2002/09/25

*******************************************************************************/

#ifndef _CACAO_TYPES_H
#define _CACAO_TYPES_H

#define POINTERSIZE         4
#define WORDS_BIGENDIAN     0

#define SUPPORT_DIVISION    1
#define SUPPORT_LONG        1
#define SUPPORT_FLOAT       1
#define SUPPORT_DOUBLE      1

#define SUPPORT_LONG_ADD    1
#define SUPPORT_LONG_CMP    1
#define SUPPORT_LONG_LOG    1
#define SUPPORT_LONG_SHIFT  1
#define SUPPORT_LONG_MULDIV 0
#define SUPPORT_LONG_ICVT   1
#define SUPPORT_LONG_FCVT   1

#define CONDITIONAL_LOADCONST

#define U8_AVAILABLE        1


typedef signed char             s1;
typedef unsigned char           u1;
 
typedef signed short int        s2;
typedef unsigned short int      u2;

typedef signed int              s4;
typedef unsigned int            u4;

#if U8_AVAILABLE
typedef signed long long int    s8;
typedef unsigned long long int  u8;
#else
typedef struct {u4 low, high;}  u8;
#define s8 u8
#endif

#endif

