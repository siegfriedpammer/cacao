/* i386/methodtable.h **********************************************************

    Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

    See file COPYRIGHT for information on usage and disclaimer of warranties

    Contains the codegenerator for an i386 processor.
    This module generates i386 machine code for a sequence of
    pseudo commands (ICMDs).

    Authors: Christian Thalinger EMAIL: cacao@complang.tuwien.ac.at

    Last Change: $Id: methodtable.h 385 2003-07-10 10:45:57Z twisti $

*******************************************************************************/

#ifndef _METHODTABLE_H
#define _METHODTABLE_H

typedef struct _mtentry mtentry;

struct _mtentry {
    u1 *start;
    u1 *end;
    mtentry *next;
};

void addmethod(u1 *start, u1 *end);
u1 *findmethod(u1 *pos);

#endif
