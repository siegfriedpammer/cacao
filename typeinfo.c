/* typeinfo.c - type system used by the type checker

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   Authors: Edwin Steiner

   $Id: typeinfo.c 723 2003-12-08 19:51:32Z edwin $

*/


#include <stdlib.h>
#include <string.h>
#include "typeinfo.h"
#include "tables.h"
#include "loader.h"
#include "toolbox/loging.h"
#include "toolbox/memory.h"


#define CLASS_IMPLEMENTS_INTERFACE(cls,index)                   \
    ( ((index) < (cls)->vftbl->interfacetablelength)            \
      && (VFTBLINTERFACETABLE((cls)->vftbl,(index)) != NULL) )

/**********************************************************************/
/* READ-ONLY FUNCTIONS                                                */
/* The following functions don't change typeinfo data.                */
/**********************************************************************/

bool
typeinfo_is_array(typeinfo *info)
{
    return TYPEINFO_IS_ARRAY(*info);
}

bool
typeinfo_is_primitive_array(typeinfo *info,int arraytype)
{
    return TYPEINFO_IS_PRIMITIVE_ARRAY(*info,arraytype);
}

bool
typeinfo_is_array_of_refs(typeinfo *info)
{
    return TYPEINFO_IS_ARRAY_OF_REFS(*info);
}

static
bool
interface_extends_interface(classinfo *cls,classinfo *interf)
{
    int i;
    
    /* first check direct superinterfaces */
    for (i=0; i<cls->interfacescount; ++i) {
        if (cls->interfaces[i] == interf)
            return true;
    }
    
    /* check indirect superinterfaces */
    for (i=0; i<cls->interfacescount; ++i) {
        if (interface_extends_interface(cls->interfaces[i],interf))
            return true;
    }
    
    return false;
}

/* XXX If really a performance issue, this could become a macro. */
static
bool
classinfo_implements_interface(classinfo *cls,classinfo *interf)
{
    if (cls->flags & ACC_INTERFACE) {
        /* cls is an interface */
        if (cls == interf)
            return true;

        /* check superinterfaces */
        return interface_extends_interface(cls,interf);
    }

    return CLASS_IMPLEMENTS_INTERFACE(cls,interf->index);
}

bool mergedlist_implements_interface(typeinfo_mergedlist *merged,
                                     classinfo *interf)
{
    int i;
    classinfo **mlist;
    
    /* Check if there is an non-empty mergedlist. */
    if (!merged)
        return false;

    /* If all classinfos in the (non-empty) merged array implement the
     * interface return true, otherwise false.
     */
    mlist = merged->list;
    i = merged->count;
    while (i--) {
        if (!classinfo_implements_interface(*mlist++,interf))
            return false;
    }
    return true;
}

bool
merged_implements_interface(classinfo *typeclass,typeinfo_mergedlist *merged,
                            classinfo *interf)
{
    /* primitive types don't support interfaces. */
    if (!typeclass)
        return false;

    /* the null type can be cast to any interface type. */
    if (typeclass == pseudo_class_Null)
        return true;

    /* check if typeclass implements the interface. */
    if (classinfo_implements_interface(typeclass,interf))
        return true;

    /* check the mergedlist */
    return (merged && mergedlist_implements_interface(merged,interf));
}

bool
typeinfo_is_assignable(typeinfo *value,typeinfo *dest)
{
    classinfo *cls;

    cls = value->typeclass;

    /* DEBUG CHECK: dest must not have a merged list. */
#ifdef TYPEINFO_DEBUG
    if (dest->merged)
        panic("Internal error: typeinfo_is_assignable on merged destination.");
#endif
    
    /* assignments of primitive values are not checked here. */
    if (!cls && !dest->typeclass)
        return true;

    /* primitive and reference types are not assignment compatible. */
    if (!cls || !dest->typeclass)
        return false;

    /* the null type can be assigned to any type */
    if (TYPEINFO_IS_NULLTYPE(*value))
        return true;

    /* uninitialized objects are not assignable */
    if (TYPEINFO_IS_NEWOBJECT(*value))
        return false;

    if (dest->typeclass->flags & ACC_INTERFACE) {
        /* We are assigning to an interface type. */
        return merged_implements_interface(cls,value->merged,
                                           dest->typeclass);
    }

    if (TYPEINFO_IS_ARRAY(*dest)) {
        /* We are assigning to an array type. */
        if (!TYPEINFO_IS_ARRAY(*value))
            return false;

        /* {Both value and dest are array types.} */

        /* value must have at least the dimension of dest. */
        if (value->dimension < dest->dimension)
            return false;

        if (value->dimension > dest->dimension) {
            /* value has higher dimension so we need to check
             * if its component array can be assigned to the
             * element type of dest */
            
            if (dest->elementclass->flags & ACC_INTERFACE) {
                /* We are assigning to an interface type. */
                return classinfo_implements_interface(pseudo_class_Arraystub,
                                                      dest->elementclass);
            }

            /* We are assigning to a class type. */
            return class_issubclass(pseudo_class_Arraystub,dest->elementclass);
        }

        /* {value and dest have the same dimension} */

        if (value->elementtype != dest->elementtype)
            return false;

        if (value->elementclass) {
            /* We are assigning an array of objects so we have to
             * check if the elements are assignable.
             */

            if (dest->elementclass->flags & ACC_INTERFACE) {
                /* We are assigning to an interface type. */

                return merged_implements_interface(value->elementclass,
                                                   value->merged,
                                                   dest->elementclass);
            }
            
            /* We are assigning to a class type. */
            return class_issubclass(value->elementclass,dest->elementclass);
        }

        return true;
    }

    /* {dest is not an array} */
        
    /* We are assigning to a class type */
    if (cls->flags & ACC_INTERFACE)
        cls = class_java_lang_Object;
    
    return class_issubclass(cls,dest->typeclass);
}

/**********************************************************************/
/* INITIALIZATION FUNCTIONS                                           */
/* The following functions fill in uninitialized typeinfo structures. */
/**********************************************************************/

void
typeinfo_init_from_descriptor(typeinfo *info,char *utf_ptr,char *end_ptr)
{
    classinfo *cls;
    char *end;

    /* XXX simplify */
    cls = class_from_descriptor(utf_ptr,end_ptr,&end,
								CLASSLOAD_NULLPRIMITIVE
								| CLASSLOAD_NEW
								| CLASSLOAD_NOVOID
								| CLASSLOAD_CHECKEND);

	if (cls)
		/* a class, interface or array descriptor */
		TYPEINFO_INIT_CLASSINFO(*info,cls);
	else
		/* a primitive type */
		TYPEINFO_INIT_PRIMITIVE(*info);
}

int
typeinfo_count_method_args(utf *d,bool twoword)
{
    int args = 0;
    char *utf_ptr = d->text;
    char *end_pos = utf_end(d);
    char c;

    /* method descriptor must start with parenthesis */
    if (*utf_ptr++ != '(') panic ("Missing '(' in method descriptor");

	
    /* check arguments */
    while ((c = *utf_ptr) != ')') {
		class_from_descriptor(utf_ptr,end_pos,&utf_ptr,
							  CLASSLOAD_SKIP | CLASSLOAD_NOVOID
							  | CLASSLOAD_NULLPRIMITIVE);
		args++;
		if (twoword && (c == 'J' || c == 'D'))
			/* primitive two-word type */
			args++;
    }

    return args;
}

void
typeinfo_init_from_method_args(utf *desc,u1 *typebuf,typeinfo *infobuf,
                               int buflen,bool twoword,
                               int *returntype,typeinfo *returntypeinfo)
{
    char *utf_ptr = desc->text;     /* current position in utf text   */
    char *end_pos = utf_end(desc);  /* points behind utf string       */
    int args = 0;
    classinfo *cls;

    /* method descriptor must start with parenthesis */
    if (utf_ptr == end_pos || *utf_ptr++ != '(') panic ("Missing '(' in method descriptor");

    /* check arguments */
    while (utf_ptr != end_pos && *utf_ptr != ')') {
		if (++args > buflen)
			panic("Buffer too small for method arguments.");
		
        *typebuf++ = type_from_descriptor(&cls,utf_ptr,end_pos,&utf_ptr,
										  CLASSLOAD_NEW
										  | CLASSLOAD_NULLPRIMITIVE
										  | CLASSLOAD_NOVOID);
		
		if (cls)
			TYPEINFO_INIT_CLASSINFO(*infobuf,cls);
		else {
			TYPEINFO_INIT_PRIMITIVE(*infobuf);
		}
		infobuf++;

		if (twoword && (typebuf[-1] == TYPE_LONG || typebuf[-1] == TYPE_DOUBLE)) {
			if (++args > buflen)
				panic("Buffer too small for method arguments.");
			*typebuf++ = TYPE_VOID;
			TYPEINFO_INIT_PRIMITIVE(*infobuf);
			infobuf++;
		}
    }
    utf_ptr++; /* skip ')' */

    /* check returntype */
    if (returntype) {
		*returntype = type_from_descriptor(&cls,utf_ptr,end_pos,&utf_ptr,
										   CLASSLOAD_NULLPRIMITIVE
										   | CLASSLOAD_NEW
										   | CLASSLOAD_CHECKEND);
		if (returntypeinfo) {
			if (cls)
				TYPEINFO_INIT_CLASSINFO(*returntypeinfo,cls);
			else
				TYPEINFO_INIT_PRIMITIVE(*returntypeinfo);
		}
	}
}

void
typeinfo_init_component(typeinfo *srcarray,typeinfo *dst)
{
    vftbl *comp = NULL;

    if (TYPEINFO_IS_NULLTYPE(*srcarray)) {
        TYPEINFO_INIT_NULLTYPE(*dst);
        return;
    }
    
    /* XXX find component class */
    if (!TYPEINFO_IS_ARRAY(*srcarray))
        panic("Trying to access component of non-array");

    comp = srcarray->typeclass->vftbl->arraydesc->componentvftbl;
    if (comp) {
        TYPEINFO_INIT_CLASSINFO(*dst,comp->class);
    }
    else {
        TYPEINFO_INIT_PRIMITIVE(*dst);
    }

    /* XXX assign directly ? */
#if 0
    if ((dst->dimension = srcarray->dimension - 1) == 0) {
        dst->typeclass = srcarray->elementclass;
        dst->elementtype = 0;
        dst->elementclass = NULL;
    }
    else {
        dst->typeclass = srcarray->typeclass;
        dst->elementtype = srcarray->elementtype;
        dst->elementclass = srcarray->elementclass;
    }
#endif
    
    dst->merged = srcarray->merged;
}

void
typeinfo_clone(typeinfo *src,typeinfo *dest)
{
    int count;
    classinfo **srclist,**destlist;

    if (src == dest)
        return;
    
    *dest = *src;

    if (src->merged) {
        count = src->merged->count;
        TYPEINFO_ALLOCMERGED(dest->merged,count);
        dest->merged->count = count;

        /* XXX use memcpy? */
        srclist = src->merged->list;
        destlist = dest->merged->list;
        while (count--)
            *destlist++ = *srclist++;
    }
}

/**********************************************************************/
/* MISCELLANEOUS FUNCTIONS                                            */
/**********************************************************************/

void
typeinfo_free(typeinfo *info)
{
    TYPEINFO_FREEMERGED_IF_ANY(info->merged);
    info->merged = NULL;
}

/**********************************************************************/
/* MERGING FUNCTIONS                                                  */
/* The following functions are used to merge the types represented by */
/* two typeinfo structures into one typeinfo structure.               */
/**********************************************************************/

static
void
typeinfo_merge_error(char *str,typeinfo *x,typeinfo *y) {
#ifdef TYPEINFO_DEBUG
    fprintf(stderr,"Error in typeinfo_merge: %s\n",str);
    fprintf(stderr,"Typeinfo x:\n");
    typeinfo_print(stderr,x,1);
    fprintf(stderr,"Typeinfo y:\n");
    typeinfo_print(stderr,y,1);
#endif
    panic(str);
}

/* Condition: clsx != clsy. */
/* Returns: true if dest was changed (always true). */
static
bool
typeinfo_merge_two(typeinfo *dest,classinfo *clsx,classinfo *clsy)
{
    TYPEINFO_FREEMERGED_IF_ANY(dest->merged);
    TYPEINFO_ALLOCMERGED(dest->merged,2);
    dest->merged->count = 2;

#ifdef TYPEINFO_DEBUG
    if (clsx == clsy)
        panic("Internal error: typeinfo_merge_two called with clsx==clsy.");
#endif

    if (clsx < clsy) {
        dest->merged->list[0] = clsx;
        dest->merged->list[1] = clsy;
    }
    else {
        dest->merged->list[0] = clsy;
        dest->merged->list[1] = clsx;
    }

    return true;
}

/* Returns: true if dest was changed. */
static
bool
typeinfo_merge_add(typeinfo *dest,typeinfo_mergedlist *m,classinfo *cls)
{
    int count;
    typeinfo_mergedlist *newmerged;
    classinfo **mlist,**newlist;

    count = m->count;
    mlist = m->list;

    /* Check if cls is already in the mergedlist m. */
    while (count--) {
        if (*mlist++ == cls) {
            /* cls is in the list, so m is the resulting mergedlist */
            if (dest->merged == m)
                return false;

            /* We have to copy the mergedlist */
            TYPEINFO_FREEMERGED_IF_ANY(dest->merged);
            count = m->count;
            TYPEINFO_ALLOCMERGED(dest->merged,count);
            dest->merged->count = count;
            newlist = dest->merged->list;
            mlist = m->list;
            while (count--) {
                *newlist++ = *mlist++;
            }
            return true;
        }
    }

    /* Add cls to the mergedlist. */
    count = m->count;
    TYPEINFO_ALLOCMERGED(newmerged,count+1);
    newmerged->count = count+1;
    newlist = newmerged->list;    
    mlist = m->list;
    while (count) {
        if (*mlist > cls)
            break;
        *newlist++ = *mlist++;
        count--;
    }
    *newlist++ = cls;
    while (count--) {
        *newlist++ = *mlist++;
    }

    /* Put the new mergedlist into dest. */
    TYPEINFO_FREEMERGED_IF_ANY(dest->merged);
    dest->merged = newmerged;
    
    return true;
}

/* Returns: true if dest was changed. */
static
bool
typeinfo_merge_mergedlists(typeinfo *dest,typeinfo_mergedlist *x,
                           typeinfo_mergedlist *y)
{
    int count = 0;
    int countx,county;
    typeinfo_mergedlist *temp,*result;
    classinfo **clsx,**clsy,**newlist;

    /* count the elements that will be in the resulting list */
    /* (Both lists are sorted, equal elements are counted only once.) */
    clsx = x->list;
    clsy = y->list;
    countx = x->count;
    county = y->count;
    while (countx && county) {
        if (*clsx == *clsy) {
            clsx++;
            clsy++;
            countx--;
            county--;
        }
        else if (*clsx < *clsy) {
            clsx++;
            countx--;
        }
        else {
            clsy++;
            county--;
        }
        count++;
    }
    count += countx + county;

    /* {The new mergedlist will have count entries.} */

    if ((x->count != count) && (y->count == count)) {
        temp = x; x = y; y = temp;
    }
    /* {If one of x,y is already the result it is x.} */
    if (x->count == count) {
        /* x->merged is equal to the result */
        if (x == dest->merged)
            return false;

        if (!dest->merged || dest->merged->count != count) {
            TYPEINFO_FREEMERGED_IF_ANY(dest->merged);
            TYPEINFO_ALLOCMERGED(dest->merged,count);
            dest->merged->count = count;
        }

        newlist = dest->merged->list;
        clsx = x->list;
        while (count--) {
            *newlist++ = *clsx++;
        }
        return true;
    }

    /* {We have to merge two lists.} */

    /* allocate the result list */
    TYPEINFO_ALLOCMERGED(result,count);
    result->count = count;
    newlist = result->list;

    /* merge the sorted lists */
    clsx = x->list;
    clsy = y->list;
    countx = x->count;
    county = y->count;
    while (countx && county) {
        if (*clsx == *clsy) {
            *newlist++ = *clsx++;
            clsy++;
            countx--;
            county--;
        }
        else if (*clsx < *clsy) {
            *newlist++ = *clsx++;
            countx--;
        }
        else {
            *newlist++ = *clsy++;
            county--;
        }
    }
    while (countx--)
            *newlist++ = *clsx++;
    while (county--)
            *newlist++ = *clsy++;

    /* replace the list in dest with the result list */
    TYPEINFO_FREEMERGED_IF_ANY(dest->merged);
    dest->merged = result;

    return true;
}

static
bool
typeinfo_merge_nonarrays(typeinfo *dest,
                         classinfo **result,
                         classinfo *clsx,classinfo *clsy,
                         typeinfo_mergedlist *mergedx,
                         typeinfo_mergedlist *mergedy)
{
    classinfo *tcls,*common;
    typeinfo_mergedlist *tmerged;
    bool changed;

    /* XXX remove */
    /*
#ifdef TYPEINFO_DEBUG
    typeinfo dbgx,dbgy;
    printf("typeinfo_merge_nonarrays:\n");
    TYPEINFO_INIT_CLASSINFO(dbgx,clsx);
    dbgx.merged = mergedx;
    TYPEINFO_INIT_CLASSINFO(dbgy,clsy);
    dbgy.merged = mergedy;
    typeinfo_print(stdout,&dbgx,4);
    printf("  with:\n");
    typeinfo_print(stdout,&dbgy,4);
#endif
    */ 

    /* Common case: clsx == clsy */
    /* (This case is very simple unless *both* x and y really represent
     *  merges of subclasses of clsx==clsy.)
     */
    /* XXX count this case for statistics */
    if ((clsx == clsy) && (!mergedx || !mergedy)) {
  return_simple_x:
        /* XXX remove */ /* log_text("return simple x"); */
        changed = (dest->merged != NULL);
        TYPEINFO_FREEMERGED_IF_ANY(dest->merged);
        dest->merged = NULL;
        *result = clsx;
        /* XXX remove */ /* log_text("returning"); */
        return changed;
    }
    
    /* If clsy is an interface, swap x and y. */
    if (clsy->flags & ACC_INTERFACE) {
        tcls = clsx; clsx = clsy; clsy = tcls;
        tmerged = mergedx; mergedx = mergedy; mergedy = tmerged;
    }
    /* {We know: If only one of x,y is an interface it is x.} */

    /* Handle merging of interfaces: */
    if (clsx->flags & ACC_INTERFACE) {
        /* {clsx is an interface and mergedx == NULL.} */
        
        if (clsy->flags & ACC_INTERFACE) {
            /* We are merging two interfaces. */
            /* {mergedy == NULL} */
            /* XXX: should we optimize direct superinterfaces? */

            /* {We know that clsx!=clsy (see common case at beginning.)} */
            *result = class_java_lang_Object;
            return typeinfo_merge_two(dest,clsx,clsy);
        }

        /* {We know: x is an interface, clsy is a class.} */

        /* Check if we are merging an interface with java.lang.Object */
        if (clsy == class_java_lang_Object && !mergedy) {
            clsx = clsy;
            goto return_simple_x;
        }
            

        /* If the type y implements clsx then the result of the merge
         * is clsx regardless of mergedy.
         */
        
        if (CLASS_IMPLEMENTS_INTERFACE(clsy,clsx->index)
            || mergedlist_implements_interface(mergedy,clsx))
        {
            /* y implements x, so the result of the merge is x. */
            goto return_simple_x;
        }
        
        /* {We know: x is an interface, the type y a class or a merge
         * of subclasses and does not implement x.} */

        /* There may still be superinterfaces of x which are implemented
         * by y, too, so we have to add clsx to the mergedlist.
         */

        /* XXX if x has no superinterfaces we could return a simple java.lang.Object */
        
        common = class_java_lang_Object;
        goto merge_with_simple_x;
    }

    /* {We know: x and y are classes (not interfaces).} */
    
    /* If *x is deeper in the inheritance hierarchy swap x and y. */
    if (clsx->index > clsy->index) {
        tcls = clsx; clsx = clsy; clsy = tcls;
        tmerged = mergedx; mergedx = mergedy; mergedy = tmerged;
    }

    /* {We know: y is at least as deep in the hierarchy as x.} */

    /* Find nearest common anchestor for the classes. */
    common = clsx;
    tcls = clsy;
    while (tcls->index > common->index)
        tcls = tcls->super;
    while (common != tcls) {
        common = common->super;
        tcls = tcls->super;
    }

    /* {common == nearest common anchestor of clsx and clsy.} */

    /* If clsx==common and x is a whole class (not a merge of subclasses)
     * then the result of the merge is clsx.
     */
    if (clsx == common && !mergedx) {
        goto return_simple_x;
    }
   
    if (mergedx) {
        *result = common;
        if (mergedy)
            return typeinfo_merge_mergedlists(dest,mergedx,mergedy);
        else
            return typeinfo_merge_add(dest,mergedx,clsy);
    }

 merge_with_simple_x:
    *result = common;
    if (mergedy)
        return typeinfo_merge_add(dest,mergedy,clsx);
    else
        return typeinfo_merge_two(dest,clsx,clsy);
}

/* Condition: *dest must be a valid initialized typeinfo. */
/* Condition: dest != y. */
/* Returns: true if dest was changed. */
bool
typeinfo_merge(typeinfo *dest,typeinfo* y)
{
    typeinfo *x;
    typeinfo *tmp;                      /* used for swapping */
    classinfo *common;
    classinfo *elementclass;
    int dimension;
    int elementtype;
    bool changed;

    /* XXX remove */
    /*
#ifdef TYPEINFO_DEBUG
    typeinfo_print(stdout,dest,4);
    typeinfo_print(stdout,y,4);
#endif
    */

    /* Merging something with itself is a nop */
    if (dest == y)
        return false;

    /* Merging two returnAddress types is ok. */
    if (!dest->typeclass && !y->typeclass)
        return false;
    
    /* Primitive types cannot be merged with reference types */
    if (!dest->typeclass || !y->typeclass)
        typeinfo_merge_error("Trying to merge primitive types.",dest,y);

#ifdef TYPEINFO_DEBUG
    /* check that no unlinked classes are merged. */
    if (!dest->typeclass->linked || !y->typeclass->linked)
        typeinfo_merge_error("Trying to merge unlinked class(es).",dest,y);
#endif

    /* handle uninitialized object types */
    /* XXX is there a way we could put this after the common case? */
    if (TYPEINFO_IS_NEWOBJECT(*dest) || TYPEINFO_IS_NEWOBJECT(*y)) {
        if (!TYPEINFO_IS_NEWOBJECT(*dest) || !TYPEINFO_IS_NEWOBJECT(*y))
            typeinfo_merge_error("Trying to merge uninitialized object type.",dest,y);
        if (TYPEINFO_NEWOBJECT_INSTRUCTION(*dest)
            != TYPEINFO_NEWOBJECT_INSTRUCTION(*y))
            typeinfo_merge_error("Trying to merge different uninitialized objects.",dest,y);
        return false;
    }
    
    /* XXX remove */ /* log_text("Testing common case"); */

    /* Common case: class dest == class y */
    /* (This case is very simple unless *both* dest and y really represent
     *  merges of subclasses of class dest==class y.)
     */
    /* XXX count this case for statistics */
    if ((dest->typeclass == y->typeclass) && (!dest->merged || !y->merged)) {
        changed = (dest->merged != NULL);
        TYPEINFO_FREEMERGED_IF_ANY(dest->merged); /* XXX unify if? */
        dest->merged = NULL;
        /* XXX remove */ /* log_text("common case handled"); */
        return changed;
    }
    
    /* XXX remove */ /* log_text("Handling null types"); */

    /* Handle null types: */
    if (TYPEINFO_IS_NULLTYPE(*y)) {
        return false;
    }
    if (TYPEINFO_IS_NULLTYPE(*dest)) {
        TYPEINFO_FREEMERGED_IF_ANY(dest->merged);
        TYPEINFO_CLONE(*y,*dest);
        return true;
    }

    /* This function uses x internally, so x and y can be swapped
     * without changing dest. */
    x = dest;
    changed = false;
    
    /* Handle merging of arrays: */
    if (TYPEINFO_IS_ARRAY(*x) && TYPEINFO_IS_ARRAY(*y)) {
        
        /* XXX remove */ /* log_text("Handling arrays"); */

        /* Make x the one with lesser dimension */
        if (x->dimension > y->dimension) {
            tmp = x; x = y; y = tmp;
        }

        /* If one array (y) has higher dimension than the other,
         * interpret it as an array (same dim. as x) of Arraystubs. */
        if (x->dimension < y->dimension) {
            dimension = x->dimension;
            elementtype = ARRAYTYPE_OBJECT;
            elementclass = pseudo_class_Arraystub;
        }
        else {
            dimension = y->dimension;
            elementtype = y->elementtype;
            elementclass = y->elementclass;
        }
        
        /* {The arrays are of the same dimension.} */
        
        if (x->elementtype != elementtype) {
            /* Different element types are merged, so the resulting array
             * type has one accessible dimension less. */
            if (--dimension == 0) {
                common = pseudo_class_Arraystub;
                elementtype = 0;
                elementclass = NULL;
            }
            else {
                common = class_multiarray_of(dimension,pseudo_class_Arraystub);
                elementtype = ARRAYTYPE_OBJECT;
                elementclass = pseudo_class_Arraystub;
            }
        }
        else {
            /* {The arrays have the same dimension and elementtype.} */

            if (elementtype == ARRAYTYPE_OBJECT) {
                /* The elements are references, so their respective
                 * types must be merged.
                 */
                changed |= typeinfo_merge_nonarrays(dest,
                                                    &elementclass,
                                                    x->elementclass,
                                                    elementclass,
                                                    x->merged,y->merged);

                /* XXX otimize this? */
                /* XXX remove */ /* log_text("finding resulting array class: "); */
                common = class_multiarray_of(dimension,elementclass);
                /* XXX remove */ /* utf_display(common->name); printf("\n"); */
            }
        }
    }
    else {
        /* {We know that at least one of x or y is no array, so the
         *  result cannot be an array.} */
        
        changed |= typeinfo_merge_nonarrays(dest,
                                            &common,
                                            x->typeclass,y->typeclass,
                                            x->merged,y->merged);

        dimension = 0;
        elementtype = 0;
        elementclass = NULL;
    }

    /* Put the new values into dest if neccessary. */

    if (dest->typeclass != common) {
        dest->typeclass = common;
        changed = true;
    }
    if (dest->dimension != dimension) {
        dest->dimension = dimension;
        changed = true;
    }
    if (dest->elementtype != elementtype) {
        dest->elementtype = elementtype;
        changed = true;
    }
    if (dest->elementclass != elementclass) {
        dest->elementclass = elementclass;
        changed = true;
    }

    /* XXX remove */ /* log_text("returning from merge"); */
    
    return changed;
}


/**********************************************************************/
/* DEBUGGING HELPERS                                                  */
/**********************************************************************/

#ifdef TYPEINFO_DEBUG

#include "tables.h"
#include "loader.h"
#include "jit/jit.h"

extern instruction *instr;

static int
typeinfo_test_compare(classinfo **a,classinfo **b)
{
    if (*a == *b) return 0;
    if (*a < *b) return -1;
    return +1;
}

static void
typeinfo_test_parse(typeinfo *info,char *str)
{
    int num;
    int i;
    typeinfo *infobuf;
    u1 *typebuf;
    int returntype;
    utf *desc = utf_new_char(str);
    
    num = typeinfo_count_method_args(desc,false);
    if (num) {
        typebuf = DMNEW(u1,num);
        infobuf = DMNEW(typeinfo,num);
        
        typeinfo_init_from_method_args(desc,typebuf,infobuf,num,false,
                                       &returntype,info);

        TYPEINFO_ALLOCMERGED(info->merged,num);
        info->merged->count = num;

        for (i=0; i<num; ++i) {
            if (typebuf[i] != TYPE_ADDRESS)
                panic("non-reference type in mergedlist");
            info->merged->list[i] = infobuf[i].typeclass;
        }
        qsort(info->merged->list,num,sizeof(classinfo*),
              (int(*)(const void *,const void *))&typeinfo_test_compare);
    }
    else {
        typeinfo_init_from_method_args(desc,NULL,NULL,0,false,
                                       &returntype,info);
    }
}

#define TYPEINFO_TEST_BUFLEN  4000

static bool
typeinfo_equal(typeinfo *x,typeinfo *y)
{
    int i;
    
    if (x->typeclass != y->typeclass) return false;
    if (x->dimension != y->dimension) return false;
    if (x->dimension) {
        if (x->elementclass != y->elementclass) return false;
        if (x->elementtype != y->elementtype) return false;
    }

    if (TYPEINFO_IS_NEWOBJECT(*x))
        if (TYPEINFO_NEWOBJECT_INSTRUCTION(*x)
            != TYPEINFO_NEWOBJECT_INSTRUCTION(*y))
            return false;

    if (x->merged || y->merged) {
        if (!(x->merged && y->merged)) return false;
        if (x->merged->count != y->merged->count) return false;
        for (i=0; i<x->merged->count; ++i)
            if (x->merged->list[i] != y->merged->list[i])
                return false;
    }
    return true;
}

static void
typeinfo_testmerge(typeinfo *a,typeinfo *b,typeinfo *result,int *failed)
{
    typeinfo dest;
    bool changed,changed_should_be;

    TYPEINFO_CLONE(*a,dest);
    
    printf("\n          ");
    typeinfo_print_short(stdout,&dest);
    printf("\n          ");
    typeinfo_print_short(stdout,b);
    printf("\n");

    changed = (typeinfo_merge(&dest,b)) ? 1 : 0;
    changed_should_be = (!typeinfo_equal(&dest,a)) ? 1 : 0;

    printf("          %s\n",(changed) ? "changed" : "=");

    if (typeinfo_equal(&dest,result)) {
        printf("OK        ");
        typeinfo_print_short(stdout,&dest);
        printf("\n");
        if (changed != changed_should_be) {
            printf("WRONG RETURN VALUE!\n");
            (*failed)++;
        }
    }
    else {
        printf("RESULT    ");
        typeinfo_print_short(stdout,&dest);
        printf("\n");
        printf("SHOULD BE ");
        typeinfo_print_short(stdout,result);
        printf("\n");
        (*failed)++;
    }
}

static void
typeinfo_inc_dimension(typeinfo *info)
{
    if (info->dimension++ == 0) {
        info->elementtype = ARRAYTYPE_OBJECT;
        info->elementclass = info->typeclass;
    }
    info->typeclass = class_array_of(info->typeclass);
}

#define TYPEINFO_TEST_MAXDIM  10

static void
typeinfo_testrun(char *filename)
{
    char buf[TYPEINFO_TEST_BUFLEN];
    char bufa[TYPEINFO_TEST_BUFLEN];
    char bufb[TYPEINFO_TEST_BUFLEN];
    char bufc[TYPEINFO_TEST_BUFLEN];
    typeinfo a,b,c;
    int maxdim;
    int failed = 0;
    FILE *file = fopen(filename,"rt");
    
    if (!file)
        panic("could not open typeinfo test file");

    while (fgets(buf,TYPEINFO_TEST_BUFLEN,file)) {
        if (buf[0] == '#' || !strlen(buf))
            continue;
        
        int res = sscanf(buf,"%s\t%s\t%s\n",bufa,bufb,bufc);
        if (res != 3 || !strlen(bufa) || !strlen(bufb) || !strlen(bufc))
            panic("Invalid line in typeinfo test file (none of empty, comment or test)");

        typeinfo_test_parse(&a,bufa);
        typeinfo_test_parse(&b,bufb);
        typeinfo_test_parse(&c,bufc);
        do {
            typeinfo_testmerge(&a,&b,&c,&failed); /* check result */
            typeinfo_testmerge(&b,&a,&c,&failed); /* check commutativity */

            if (TYPEINFO_IS_NULLTYPE(a)) break;
            if (TYPEINFO_IS_NULLTYPE(b)) break;
            if (TYPEINFO_IS_NULLTYPE(c)) break;
            
            maxdim = a.dimension;
            if (b.dimension > maxdim) maxdim = b.dimension;
            if (c.dimension > maxdim) maxdim = c.dimension;

            if (maxdim < TYPEINFO_TEST_MAXDIM) {
                typeinfo_inc_dimension(&a);
                typeinfo_inc_dimension(&b);
                typeinfo_inc_dimension(&c);
            }
        } while (maxdim < TYPEINFO_TEST_MAXDIM);
    }

    fclose(file);

    if (failed) {
        fprintf(stderr,"Failed typeinfo_merge tests: %d\n",failed);
        panic("Failed test");
    }
}

void
typeinfo_test()
{
    log_text("Running typeinfo test file...");
    typeinfo_testrun("typeinfo.tst");
    log_text("Finished typeinfo test file.");
}

void
typeinfo_init_from_fielddescriptor(typeinfo *info,char *desc)
{
    typeinfo_init_from_descriptor(info,desc,desc+strlen(desc));
}

#define TYPEINFO_MAXINDENT  80

void
typeinfo_print(FILE *file,typeinfo *info,int indent)
{
    int i;
    char ind[TYPEINFO_MAXINDENT + 1];
    instruction *ins;

    if (indent > TYPEINFO_MAXINDENT) indent = TYPEINFO_MAXINDENT;
    
    for (i=0; i<indent; ++i)
        ind[i] = ' ';
    ind[i] = (char) 0;
    
    if (TYPEINFO_IS_PRIMITIVE(*info)) {
        fprintf(file,"%sprimitive\n",ind);
        return;
    }
    
    if (TYPEINFO_IS_NULLTYPE(*info)) {
        fprintf(file,"%snull\n",ind);
        return;
    }

    if (TYPEINFO_IS_NEWOBJECT(*info)) {
        ins = (instruction *)TYPEINFO_NEWOBJECT_INSTRUCTION(*info);
        if (ins) {
            fprintf(file,"%sNEW(%d):",ind,ins-instr);
            utf_fprint(file,((classinfo *)ins[-1].val.a)->name);
            fprintf(file,"\n");
        }
        else {
            fprintf(file,"%sNEW(this)",ind);
        }
        return;
    }

    fprintf(file,"%sClass:      ",ind);
    utf_fprint(file,info->typeclass->name);
    fprintf(file,"\n");

    if (TYPEINFO_IS_ARRAY(*info)) {
        fprintf(file,"%sDimension:    %d",ind,(int)info->dimension);
        fprintf(file,"\n%sElements:     ",ind);
        switch (info->elementtype) {
          case ARRAYTYPE_INT     : fprintf(file,"int\n"); break;
          case ARRAYTYPE_LONG    : fprintf(file,"long\n"); break;
          case ARRAYTYPE_FLOAT   : fprintf(file,"float\n"); break;
          case ARRAYTYPE_DOUBLE  : fprintf(file,"double\n"); break;
          case ARRAYTYPE_BYTE    : fprintf(file,"byte\n"); break;
          case ARRAYTYPE_CHAR    : fprintf(file,"char\n"); break;
          case ARRAYTYPE_SHORT   : fprintf(file,"short\n"); break;
          case ARRAYTYPE_BOOLEAN : fprintf(file,"boolean\n"); break;
              
          case ARRAYTYPE_OBJECT:
              fprintf(file,"reference: ");
              utf_fprint(file,info->elementclass->name);
              fprintf(file,"\n");
              break;
              
          default:
              fprintf(file,"INVALID ARRAYTYPE!\n");
        }
    }

    if (info->merged) {
        fprintf(file,"%sMerged: ",ind);
        for (i=0; i<info->merged->count; ++i) {
            if (i) fprintf(file,", ");
            utf_fprint(file,info->merged->list[i]->name);
        }
        fprintf(file,"\n");
    }
}

void
typeinfo_print_short(FILE *file,typeinfo *info)
{
    int i;
    instruction *ins;

    if (TYPEINFO_IS_PRIMITIVE(*info)) {
        fprintf(file,"primitive");
        return;
    }
    
    if (TYPEINFO_IS_NULLTYPE(*info)) {
        fprintf(file,"null");
        return;
    }
    
    if (TYPEINFO_IS_NEWOBJECT(*info)) {
        ins = (instruction *)TYPEINFO_NEWOBJECT_INSTRUCTION(*info);
        if (ins) {
            fprintf(file,"NEW(%d):",ins-instr);
            utf_fprint(file,((classinfo *)ins[-1].val.a)->name);
        }
        else
            fprintf(file,"NEW(this)");
        return;
    }

    utf_fprint(file,info->typeclass->name);

    if (info->merged) {
        fprintf(file,"{");
        for (i=0; i<info->merged->count; ++i) {
            if (i) fprintf(file,",");
            utf_fprint(file,info->merged->list[i]->name);
        }
        fprintf(file,"}");
    }
}

void
typeinfo_print_type(FILE *file,int type,typeinfo *info)
{
    switch (type) {
      case TYPE_VOID:   fprintf(file,"V"); break;
      case TYPE_INT:    fprintf(file,"I"); break;
      case TYPE_FLOAT:  fprintf(file,"F"); break;
      case TYPE_DOUBLE: fprintf(file,"D"); break;
      case TYPE_LONG:   fprintf(file,"J"); break;
      case TYPE_ADDRESS:
          if (TYPEINFO_IS_PRIMITIVE(*info))
              fprintf(file,"R"); /* returnAddress */
          else {
              typeinfo_print_short(file,info);
          }
          break;
          
      default:
          fprintf(file,"!");
    }
}

#endif /* TYPEINFO_DEBUG */


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
