/* natcalls.h -

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   Institut f. Computersprachen, TU Wien
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser, M. Probst,
   S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich,
   J. Wenninger

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Carolyn Oates

   $Id: natcalls.h 662 2003-11-21 18:06:25Z jowenn $

*/


#ifndef _NATCALLS_H
#define _NATCALLS_H

#include "global.h"


/*---------- Define Constants ---------------------------*/
#define MAX 256
#define MAXCALLS 30 

/*---------- global variables ---------------------------*/
typedef struct classMeth classMeth;
typedef struct nativeCall   nativeCall;
typedef struct methodCall   methodCall;
typedef struct nativeMethod nativeMethod;

typedef struct nativeCompCall   nativeCompCall;
typedef struct methodCompCall   methodCompCall;
typedef struct nativeCompMethod nativeCompMethod;

int classCnt;

struct classMeth {
	int i_class;
	int j_method;
	int methCnt;
};

struct  methodCall{
	char *classname;
	char *methodname;
	char *descriptor;
};

struct  nativeMethod  {
	char *methodname;
	char *descriptor;
	struct methodCall methodCalls[MAXCALLS];
};


static struct nativeCall {
	char *classname;
	struct nativeMethod methods[MAXCALLS];
	int methCnt;
	int callCnt[MAXCALLS];
} nativeCalls[] =
{
#include "nativecalls.h"
};

#define NATIVECALLSSIZE  (sizeof(nativeCalls)/sizeof(struct nativeCall))


struct methodCompCall {
	utf *classname;
	utf *methodname;
	utf *descriptor;
};


struct nativeCompMethod {
	utf *methodname;
	utf *descriptor;
	struct methodCompCall methodCalls[MAXCALLS];
};


struct nativeCompCall {
	utf *classname;
	struct nativeCompMethod methods[MAXCALLS];
	int methCnt;
	int callCnt[MAXCALLS];
} nativeCompCalls[NATIVECALLSSIZE];


bool natcall2utf(bool);
void printNativeCall(nativeCall);
void markNativeMethodsRT(utf *, utf* , utf* ); 

#endif /* _NATCALLS_H */


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */

