/********************************* typeinfo.h **********************************

	Copyright (c) 2003 ? XXX

	See file COPYRIGHT for information on usage and disclaimer of warranties

	defininitions for the compiler's type system
	
	Authors: Edwin Steiner
                  
	Last Change:

*******************************************************************************/

#ifndef __typeinfo_h_
#define __typeinfo_h_

/* #define DEBUG_TYPES */ /* XXX */

#include "global.h"

/* resolve typedef cycles *****************************************************/

typedef struct typeinfo typeinfo;
typedef struct typeinfo_mergedlist typeinfo_mergedlist;

/* global variables ***********************************************************/

/* XXX move this documentation to global.h */
/* The following classinfo pointers are used internally by the type system.
 * Please do not use them directly, use the TYPEINFO_ macros instead.
 */

/*
 * pseudo_class_Arraystub
 *     (extends Object implements Cloneable, java.io.Serializable)
 *
 *     If two arrays of incompatible component types are merged,
 *     the resulting reference has no accessible components.
 *     The result does, however, implement the interfaces Cloneable
 *     and java.io.Serializable. This pseudo class is used internally
 *     to represent such results. (They are *not* considered arrays!)
 *
 * pseudo_class_Array                          XXX delete?
 *     (extends pseudo_class_Arraystub)
 *
 *     This pseudo class is used internally as the class of all arrays
 *     to distinguish them from arbitrary Objects.
 *
 * pseudo_class_Null
 *
 *     This pseudo class is used internally to represent the
 *     null type.
 */

/* data structures for the type system ****************************************/

/* The typeinfo structure stores detailed information on reference types.
 * (stack elements, variables, etc. with type == TYPE_ADR.)
 * XXX: exclude ReturnAddresses?
 *
 * For primitive types either there is no typeinfo allocated or the
 * typeclass pointer in the typeinfo struct is NULL.
 *
 * CAUTION: The typeinfo structure should be considered opaque outside of
 *          typeinfo.[ch]. Please use the macros and functions defined here to
 *          access typeinfo structures!
 */

/* At all times *exactly one* of the following conditions is true for
 * a particular typeinfo struct:
 *
 * A) typeclass == NULL
 *
 *        In this case the other fields of the structure
 *        are INVALID.
 *
 * B) typeclass == pseudo_class_Null
 *
 *        XXX
 *
 * C) typeclass is an array class
 *
 *        XXX
 *
 * D) typeclass == pseudo_class_Arraystub
 *
 *        XXX
 *
 * E) typeclass is an interface
 *
 *        XXX
 *
 * F) typeclass is a (non-pseudo-)class != java.lang.Object
 *
 *        XXX
 *        All classinfos in u.merged.list (if any) are
 *        subclasses of typeclass.
 *
 * G) typeclass is java.lang.Object
 *
 *        XXX
 *        In this case u.merged.count and u.merged.list
 *        are valid and may be non-zero.
 *        The classinfos in u.merged.list (if any) can be
 *        classes, interfaces or pseudo classes.
 */

/* The following algorithm is used to determine if the type described
 * by this typeinfo struct supports the interface X:
 *
 *     1) If typeclass is X or a subinterface of X the answer is "yes".
 *     2) If typeclass is a (pseudo) class implementing X the answer is "yes".
 *     3) XXX If typeclass is not an array and u.merged.count>0
 *        and all classes/interfaces in u.merged.list implement X
 *        the answer is "yes".
 *     4) If none of the above is true the answer is "no".
 */

/*
 * CAUTION: The typeinfo structure should be considered opaque outside of
 *          typeinfo.[ch]. Please use the macros and functions defined here to
 *          access typeinfo structures!
 */
struct typeinfo {
        classinfo           *typeclass;
        classinfo           *elementclass; /* valid if dimension>0 */
        typeinfo_mergedlist *merged;
        u1                   dimension;
        u1                   elementtype;  /* valid if dimension>0 */
};

struct typeinfo_mergedlist {
        s4         count;
        classinfo *list[1]; /* variable length! */
};

/****************************************************************************/
/* MACROS                                                                   */
/****************************************************************************/

/* NOTE: These macros take typeinfo *structs* not pointers as arguments.
 *       You have to dereference any pointers.
 */

/* internally used macros ***************************************************/

/* internal, don't use this explicitly! */
/* XXX change to GC? */
#define TYPEINFO_ALLOCMERGED(mergedlist,count)                  \
            {(mergedlist) = (typeinfo_mergedlist*)dump_alloc(   \
                sizeof(typeinfo_mergedlist)                     \
                + ((count)-1)*sizeof(classinfo*));}

/* internal, don't use this explicitly! */
/* XXX change to GC? */
#if 0
#define TYPEINFO_FREEMERGED(mergedlist)                         \
            {mem_free((mergedlist),sizeof(typeinfo_mergedlist)  \
                + ((mergedlist)->count - 1)*sizeof(classinfo*));}
#endif
#define TYPEINFO_FREEMERGED(mergedlist)

/* internal, don't use this explicitly! */
#define TYPEINFO_FREEMERGED_IF_ANY(mergedlist)                  \
            {if (mergedlist) TYPEINFO_FREEMERGED(mergedlist);}

/* macros for type queries **************************************************/

#define TYPEINFO_IS_PRIMITIVE(info)                             \
            ((info).typeclass == NULL)

#define TYPEINFO_IS_REFERENCE(info)                             \
            ((info).typeclass != NULL)

#define TYPEINFO_IS_NULLTYPE(info)                              \
            ((info).typeclass == pseudo_class_Null)

/* macros for array type queries ********************************************/

#define TYPEINFO_IS_ARRAY(info)                                 \
            ( TYPEINFO_IS_REFERENCE(info)                       \
              && ((info).dimension != 0) )

#define TYPEINFO_IS_SIMPLE_ARRAY(info)                          \
            ( ((info).dimension == 1) )

#define TYPEINFO_IS_ARRAY_ARRAY(info)                           \
            ( ((info).dimension >= 2) )

#define TYPEINFO_IS_PRIMITIVE_ARRAY(info,arraytype)             \
            ( TYPEINFO_IS_SIMPLE_ARRAY(info)                    \
              && ((info).elementtype == (arraytype)) )

#define TYPEINFO_IS_OBJECT_ARRAY(info)                          \
            ( TYPEINFO_IS_SIMPLE_ARRAY(info)                    \
              && ((info).elementclass != NULL) )

/* assumes that info describes an array type */
#define TYPEINFO_IS_ARRAY_OF_REFS_NOCHECK(info)                 \
            ( ((info).elementclass != NULL)                     \
              || ((info).dimension >= 2) )

#define TYPEINFO_IS_ARRAY_OF_REFS(info)                         \
            ( TYPEINFO_IS_ARRAY(info)                           \
              && TYPEINFO_IS_ARRAY_OF_REFS_NOCHECK(info) )

/* macros for initializing typeinfo structures ******************************/

#define TYPEINFO_INIT_PRIMITIVE(info)                           \
            {(info).typeclass = NULL;                           \
             (info).elementclass = NULL;                        \
             (info).merged = NULL;                              \
             (info).dimension = 0;                              \
             (info).elementtype = 0;}

#define TYPEINFO_INIT_NON_ARRAY_CLASSINFO(info,cinfo)   \
            {(info).typeclass = (cinfo);                \
             (info).elementclass = NULL;                \
             (info).merged = NULL;                      \
             (info).dimension = 0;                      \
             (info).elementtype = 0;}

#define TYPEINFO_INIT_NULLTYPE(info)                            \
            TYPEINFO_INIT_CLASSINFO(info,pseudo_class_Null)

/* XXX delete */
#if 0
#define TYPEINFO_INIT_ARRAY(info,dim,elmtype,elmcinfo)        \
            {(info).typeclass = pseudo_class_Array;             \
             (info).elementclass = (elmcinfo);                  \
             (info).merged = NULL;                              \
             (info).dimension = (dim);                          \
             (info).elementtype = (elmtype);}
#endif

/* XXX delete? */
#define TYPEINFO_INIT_ARRAY(info,cls)                                   \
            {(info).typeclass = cls;                                    \
             if (cls->vftbl->arraydesc->elementvftbl)                   \
                 (info).elementclass = cls->vftbl->arraydesc->elementvftbl->class; \
             else                                                       \
                 (info).elementclass = NULL;                            \
             (info).merged = NULL;                                      \
             (info).dimension = cls->vftbl->arraydesc->dimension;       \
             (info).elementtype = cls->vftbl->arraydesc->elementtype;}

#define TYPEINFO_INIT_CLASSINFO(info,cls)                               \
        {if (((info).typeclass = cls)->vftbl->arraydesc) {              \
                if (cls->vftbl->arraydesc->elementvftbl)                \
                    (info).elementclass = cls->vftbl->arraydesc->elementvftbl->class; \
                else                                                    \
                    (info).elementclass = NULL;                         \
                (info).dimension = cls->vftbl->arraydesc->dimension;    \
                (info).elementtype = cls->vftbl->arraydesc->elementtype;\
            }                                                           \
            else {                                                      \
                (info).elementclass = NULL;                             \
                (info).dimension = 0;                                   \
                (info).elementtype = 0;                                 \
            }                                                           \
            (info).merged = NULL;}

/* XXX */
#define TYPEINFO_INC_DIMENSION(info)                            \
             (info).dimension++

#define TYPEINFO_INIT_FROM_FIELDINFO(info,fi)                   \
            typeinfo_init_from_descriptor(&(info),              \
                (fi)->descriptor->text,utf_end((fi)->descriptor));

/* macros for freeing typeinfo structures ***********************************/

#define TYPEINFO_FREE(info)                                     \
            {TYPEINFO_FREEMERGED_IF_ANY((info).merged);         \
             (info).merged = NULL;}

/* macros for writing types (destination must have been initialized) ********/
/* XXX delete them? */

#define TYPEINFO_PUT_NULLTYPE(info)                             \
            {(info).typeclass = pseudo_class_Null;}

#define TYPEINFO_PUT_NON_ARRAY_CLASSINFO(info,cinfo)            \
            {(info).typeclass = (cinfo);}

/* XXX delete */
#if 0
#define TYPEINFO_PUT_ARRAY(info,dim,elmtype)                    \
            {(info).typeclass = pseudo_class_Array;             \
             (info).dimension = (dim);                          \
             (info).elementtype = (elmtype);}
#endif

/* XXX delete? */
#define TYPEINFO_PUT_ARRAY(info,cls)                                    \
            {(info).typeclass = cls;                                    \
             if (cls->vftbl->arraydesc->elementvftbl)                \
                 (info).elementclass = cls->vftbl->arraydesc->elementvftbl->class; \
             (info).dimension = cls->vftbl->arraydesc->dimension;       \
             (info).elementtype = cls->vftbl->arraydesc->elementtype;}

#define TYPEINFO_PUT_CLASSINFO(info,cls)                                \
        {if (((info).typeclass = cls)->vftbl->arraydesc) {              \
                if (cls->vftbl->arraydesc->elementvftbl)                \
                    (info).elementclass = cls->vftbl->arraydesc->elementvftbl->class; \
                (info).dimension = cls->vftbl->arraydesc->dimension;    \
                (info).elementtype = cls->vftbl->arraydesc->elementtype; \
            }}

/* XXX delete */
#if 0
#define TYPEINFO_PUT_OBJECT_ARRAY(info,dim,elmcinfo)            \
            {TYPEINFO_PUT_ARRAY(info,dim,ARRAYTYPE_OBJECT);     \
             (info).elementclass = (elmcinfo);}
#endif

/* srcarray must be an array (not checked) */
#define TYPEINFO_PUT_COMPONENT(srcarray,dst)                    \
            {typeinfo_put_component(&(srcarray),&(dst));}

/* macros for copying types (destinition is not checked or freed) ***********/

/* TYPEINFO_COPY makes a shallow copy, the merged pointer is simply copied. */
#define TYPEINFO_COPY(src,dst)                                  \
            {(dst) = (src);}

/* TYPEINFO_CLONE makes a deep copy, the merged list (if any) is duplicated
 * into a newly allocated array.
 */
#define TYPEINFO_CLONE(src,dst)                                 \
            {(dst) = (src);                                     \
             if ((dst).merged) typeinfo_clone(&(src),&(dst));}

/****************************************************************************/
/* FUNCTIONS                                                                */
/****************************************************************************/

/* inquiry functions (read-only) ********************************************/

bool typeinfo_is_array(typeinfo *info);
bool typeinfo_is_primitive_array(typeinfo *info,int arraytype);
bool typeinfo_is_array_of_refs(typeinfo *info);

bool typeinfo_implements_interface(typeinfo *info,classinfo *interf);
bool typeinfo_is_assignable(typeinfo *value,typeinfo *dest);

/* initialization functions *************************************************/

void typeinfo_init_from_descriptor(typeinfo *info,char *utf_ptr,char *end_ptr);
void typeinfo_init_component(typeinfo *srcarray,typeinfo *dst);

int  typeinfo_count_method_args(utf *d,bool twoword); /* this not included */
void typeinfo_init_from_method_args(utf *desc,u1 *typebuf,
                                    typeinfo *infobuf,
                                    int buflen,bool twoword,
                                    int *returntype,typeinfo *returntypeinfo);

void typeinfo_clone(typeinfo *src,typeinfo *dest);

/* functions for the type system ********************************************/

void typeinfo_free(typeinfo *info);

bool typeinfo_merge(typeinfo *dest,typeinfo* y);

/* debugging helpers ********************************************************/

#ifdef DEBUG_TYPES

void typeinfo_test();
void typeinfo_init_from_fielddescriptor(typeinfo *info,char *desc);
void typeinfo_print(FILE *file,typeinfo *info,int indent);
void typeinfo_print_short(FILE *file,typeinfo *info);
void typeinfo_print_type(FILE *file,int type,typeinfo *info);

#endif // DEBUG_TYPES

#endif
