/* jit/parseXTA.h - XTA parser header

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Carolyn Oates

   $Id: parseXTA.h 4357 2006-01-22 23:33:38Z twisti $

*/

#ifndef _PARSEXTA_H
#define _PARSEXTA_H

/* forward typedefs ***********************************************************/

typedef struct xtainfo xtainfo;
typedef	struct xtafldinfo xtafldinfo;


#include "toolbox/list.h"
#include "vm/global.h"
#include "vm/jit/inline/sets.h"


#define LAZYLOADING(class) { \
        if (!(class->state & CLASS_LINKED)) \
            if (!link_class(class)) \
                return 0; }



#define LAZYLOADING1(class) { \
        if (!(class->state & CLASS_LINKED)) \
            if (!link_class(class)) \
                return; }


/* methodinfo static info *****************************************************/

struct xtainfo {
	s4          XTAmethodUsed;     /* XTA if used in callgraph - not used /used */
	classSet    *XTAclassSet;      /* method class type set                 */ 
	/*classSet 	*PartClassSet */   /* method class type set                 */ 

	classSetNode    *paramClassSet; /* cone set of methods parameters       */
	
	/* Needed for interative checking */
	methSet  	*calls;            /* Edges - methods this method calls   	        */ 
	methSet  	*calledBy;         /* Edges - methods that call this method         */ 
	methSet         *markedBy;  
	fldSet          *fldsUsed;         /* fields used by this method             */ 
	/*methSetNode  *interfaceCalls*/   /* methods this method calls as interface */ 
	bool           chgdSinceLastParse; /* Changed since last parse ?          */
}; 


/* field, method and class structures *****************************************/

struct xtafldinfo {
	bool       fieldChecked; 		
	classinfo *fldClassType;
	classSet  *XTAclassSet;          /* field class type set                  */
};


extern FILE *xtaMissed;  /* Methods missed during XTA parse of Main  */

typedef struct {
        listnode linkage;
        methodinfo *method;
        } xtaNode ;


extern int XTA_jit_parse(methodinfo *m);

#endif /* _PARSEXTA_H */

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

