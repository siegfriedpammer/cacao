/* src/vm/jit/verify/typeinfo.c - type system used by the type checker

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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

   $Id: typeinfo.c 2182 2005-04-01 20:56:33Z edwin $

*/

#include <assert.h>
#include <string.h>

#include "mm/memory.h"
#include "toolbox/logging.h"
#include "vm/class.h"
#include "vm/loader.h"
#include "vm/tables.h"
#include "vm/jit/jit.h"
#include "vm/jit/verify/typeinfo.h"
#include "vm/descriptor.h"
#include "vm/resolve.h"


/* check if a linked class is an array class. Only use for linked classes! */
#define CLASSINFO_IS_ARRAY(clsinfo)  ((clsinfo)->vftbl->arraydesc != NULL)

#define CLASSINFO_IMPLEMENTS_INTERFACE(cls,index)                   \
    ( ((index) < (cls)->vftbl->interfacetablelength)            \
      && (VFTBLINTERFACETABLE((cls)->vftbl,(index)) != NULL) )

/******************************************************************************/
/* DEBUG HELPERS                                                              */
/******************************************************************************/

#ifdef TYPEINFO_DEBUG
#define TYPEINFO_ASSERT(cond)  assert(cond)
#else
#define TYPEINFO_ASSERT(cond)
#endif

/**********************************************************************/
/* TYPEVECTOR FUNCTIONS                                               */
/**********************************************************************/

typevector *
typevectorset_copy(typevector *src,int k,int size)
{
	typevector *dst = DNEW_TYPEVECTOR(size);
	
	memcpy(dst,src,TYPEVECTOR_SIZE(size));
	dst->k = k;
	if (src->alt)
		dst->alt = typevectorset_copy(src->alt,k+1,size);
	return dst;
}

bool
typevectorset_checktype(typevector *vec,int index,int type)
{
	do {
		if (vec->td[index].type != type)
			return false;
	} while ((vec = vec->alt) != NULL);
	return true;
}

bool
typevectorset_checkreference(typevector *vec,int index)
{
	do {
		if (!TYPEDESC_IS_REFERENCE(vec->td[index]))
			return false;
	} while ((vec = vec->alt) != NULL);
	return true;
}

bool
typevectorset_checkretaddr(typevector *vec,int index)
{
	do {
		if (!TYPEDESC_IS_RETURNADDRESS(vec->td[index]))
			return false;
	} while ((vec = vec->alt) != NULL);
	return true;
}

int
typevectorset_copymergedtype(typevector *vec,int index,typeinfo *dst)
{
	int type;
	typedescriptor *td;

	td = vec->td + index;
	type = td->type;
	TYPEINFO_COPY(td->info,*dst);
	
	if (vec->alt) {
		int primitive;
		
		primitive = TYPEINFO_IS_PRIMITIVE(*dst) ? 1 : 0;
		
		while ((vec = vec->alt) != NULL) {
			td = vec->td + index;
			if (type != td->type)
				return TYPE_VOID;

			if (type == TYPE_ADDRESS) {
				if ((TYPEINFO_IS_PRIMITIVE(td->info) ? 1 : 0) != primitive)
					return TYPE_VOID;
				typeinfo_merge(dst,&(td->info));
			}
		}
	}
	return type;
}

int
typevectorset_mergedtype(typevector *vec,int index,typeinfo *temp,typeinfo **result)
{
	if (vec->alt) {
		*result = temp;
		return typevectorset_copymergedtype(vec,index,temp);
	}

	*result = &(vec->td[index].info);
	return vec->td[index].type;
}

typeinfo *
typevectorset_mergedtypeinfo(typevector *vec,int index,typeinfo *temp)
{
	typeinfo *result;
	int type = typevectorset_mergedtype(vec,index,temp,&result);
	return (type == TYPE_ADDRESS) ? result : NULL;
}

void
typevectorset_store(typevector *vec,int index,int type,typeinfo *info)
{
	do {
		vec->td[index].type = type;
		if (info)
			TYPEINFO_COPY(*info,vec->td[index].info);
		if (index > 0 && IS_2_WORD_TYPE(vec->td[index-1].type))
			vec->td[index-1].type = TYPE_VOID;
	} while ((vec = vec->alt) != NULL);
}

void
typevectorset_store_retaddr(typevector *vec,int index,typeinfo *info)
{
	typeinfo_retaddr_set *adr;
	
	adr = (typeinfo_retaddr_set*) TYPEINFO_RETURNADDRESS(*info);
	do {
		vec->td[index].type = TYPE_ADDRESS;
		TYPEINFO_INIT_RETURNADDRESS(vec->td[index].info,adr->addr);
		if (index > 0 && IS_2_WORD_TYPE(vec->td[index-1].type))
			vec->td[index-1].type = TYPE_VOID;
		adr = adr->alt;
	} while ((vec = vec->alt) != NULL);
}

void
typevectorset_store_twoword(typevector *vec,int index,int type)
{
	do {
		vec->td[index].type = type;
		vec->td[index+1].type = TYPE_VOID;
		if (index > 0 && IS_2_WORD_TYPE(vec->td[index-1].type))
			vec->td[index-1].type = TYPE_VOID;
	} while ((vec = vec->alt) != NULL);
}

void
typevectorset_init_object(typevector *set,void *ins,
						  classref_or_classinfo initclass,
						  int size)
{
	int i;
	
	for (;set; set=set->alt) {
		for (i=0; i<size; ++i) {
			if (set->td[i].type == TYPE_ADR
				&& TYPEINFO_IS_NEWOBJECT(set->td[i].info)
				&& TYPEINFO_NEWOBJECT_INSTRUCTION(set->td[i].info) == ins)
			{
				TYPEINFO_INIT_CLASSREF_OR_CLASSINFO(set->td[i].info,initclass);
			}
		}
	}
}

bool
typevector_merge(typevector *dst,typevector *y,int size)
{
	bool changed = false;
	
	typedescriptor *a = dst->td;
	typedescriptor *b = y->td;
	while (size--) {
		if (a->type != TYPE_VOID && a->type != b->type) {
			a->type = TYPE_VOID;
			changed = true;
		}
		else if (a->type == TYPE_ADDRESS) {
			if (TYPEINFO_IS_PRIMITIVE(a->info)) {
				/* 'a' is a returnAddress */
				if (!TYPEINFO_IS_PRIMITIVE(b->info)
					|| (TYPEINFO_RETURNADDRESS(a->info)
						!= TYPEINFO_RETURNADDRESS(b->info)))
				{
					a->type = TYPE_VOID;
					changed = true;
				}
			}
			else {
				/* 'a' is a reference */
				if (TYPEINFO_IS_PRIMITIVE(b->info)) {
					a->type = TYPE_VOID;
					changed = true;
				}
				else {
					changed |= typeinfo_merge(&(a->info),&(b->info));
				}
			}
		}
		a++;
		b++;
	}
	return changed;
}

bool 
typevector_separable_from(typevector *a,typevector *b,int size)
{
	typedescriptor *tda = a->td;
	typedescriptor *tdb = b->td;
	for (;size--; tda++,tdb++) {
		if (TYPEDESC_IS_RETURNADDRESS(*tda)
			&& TYPEDESC_IS_RETURNADDRESS(*tdb)
			&& TYPEINFO_RETURNADDRESS(tda->info)
			   != TYPEINFO_RETURNADDRESS(tdb->info))
			return true;
	}
	return false;
}

void
typevectorset_add(typevector *dst,typevector *v,int size)
{
	while (dst->alt)
		dst = dst->alt;
	dst->alt = DNEW_TYPEVECTOR(size);
	memcpy(dst->alt,v,TYPEVECTOR_SIZE(size));
	dst->alt->alt = NULL;
	dst->alt->k = dst->k + 1;
}

typevector *
typevectorset_select(typevector **set,int retindex,void *retaddr)
{
	typevector *selected;

	if (!*set) return NULL;
	
	if (TYPEINFO_RETURNADDRESS((*set)->td[retindex].info) == retaddr) {
		selected = *set;
		*set = selected->alt;
		selected->alt = typevectorset_select(set,retindex,retaddr);
	}
	else {
		selected = typevectorset_select(&((*set)->alt),retindex,retaddr);
	}
	return selected;
}

bool
typevectorset_separable_with(typevector *set,typevector *add,int size)
{
	int i;
	typevector *v;
	void *addr;
	bool separable;

	for (i=0; i<size; ++i) {
		if (!TYPEDESC_IS_RETURNADDRESS(add->td[i]))
			continue;
		addr = TYPEINFO_RETURNADDRESS(add->td[i].info);
		
		v = set;
		separable = false;
		do {
			if (!TYPEDESC_IS_RETURNADDRESS(v->td[i]))
				goto next_index;
			if (TYPEINFO_RETURNADDRESS(v->td[i].info) != addr)
				separable = true;
			v = v->alt;
			if (!v && separable) return true;
		} while (v);
	next_index:
		;
	}
	return false;
}

bool
typevectorset_collapse(typevector *dst,int size)
{
	bool changed = false;
	
	while (dst->alt) {
		typevector_merge(dst,dst->alt,size);
		dst->alt = dst->alt->alt;
		changed = true;
	}
	return changed;
}

/**********************************************************************/
/* READ-ONLY FUNCTIONS                                                */
/* The following functions don't change typeinfo data.                */
/**********************************************************************/

bool
typeinfo_is_array(typeinfo *info)
{
	TYPEINFO_ASSERT(info);
    return TYPEINFO_IS_ARRAY(*info);
}

bool
typeinfo_is_primitive_array(typeinfo *info,int arraytype)
{
	TYPEINFO_ASSERT(info);
    return TYPEINFO_IS_PRIMITIVE_ARRAY(*info,arraytype);
}

bool
typeinfo_is_array_of_refs(typeinfo *info)
{
	TYPEINFO_ASSERT(info);
    return TYPEINFO_IS_ARRAY_OF_REFS(*info);
}

static bool
interface_extends_interface(classinfo *cls,classinfo *interf)
{
    int i;
    
	TYPEINFO_ASSERT(cls);
	TYPEINFO_ASSERT(interf);
	TYPEINFO_ASSERT((interf->flags & ACC_INTERFACE) != 0);
	TYPEINFO_ASSERT((cls->flags & ACC_INTERFACE) != 0);

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

static bool
classinfo_implements_interface(classinfo *cls,classinfo *interf)
{
	TYPEINFO_ASSERT(cls);
	TYPEINFO_ASSERT(interf);
	TYPEINFO_ASSERT((interf->flags & ACC_INTERFACE) != 0);

	if (!cls->loaded)
		if (!load_class_bootstrap(cls)) /* XXX */
			return false;

	if (!cls->linked)
		if (!link_class(cls)) /* XXX */
			return false;

    if (cls->flags & ACC_INTERFACE) {
        /* cls is an interface */
        if (cls == interf)
            return true;

        /* check superinterfaces */
        return interface_extends_interface(cls,interf);
    }

    return CLASSINFO_IMPLEMENTS_INTERFACE(cls,interf->index);
}

tristate_t 
mergedlist_implements_interface(typeinfo_mergedlist *merged,
                                classinfo *interf)
{
    int i;
    classref_or_classinfo *mlist;
    
	TYPEINFO_ASSERT(interf);
	TYPEINFO_ASSERT((interf->flags & ACC_INTERFACE) != 0);

    /* Check if there is an non-empty mergedlist. */
    if (!merged)
        return false;

    /* If all classinfos in the (non-empty) merged array implement the
     * interface return true, otherwise false.
     */
    mlist = merged->list;
    i = merged->count;
    while (i--) {
		if (IS_CLASSREF(*mlist)) {
			return MAYBE;
		}
        if (!classinfo_implements_interface((mlist++)->cls,interf))
            return false;
    }
    return true;
}

tristate_t
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

tristate_t
typeinfo_is_assignable(typeinfo *value,typeinfo *dest)
{
	TYPEINFO_ASSERT(value);
	TYPEINFO_ASSERT(dest);
	TYPEINFO_ASSERT(dest->merged == NULL);

	return typeinfo_is_assignable_to_class(value,dest->typeclass);
}

tristate_t
typeinfo_is_assignable_to_class(typeinfo *value,classref_or_classinfo dest)
{
	classref_or_classinfo c;
    classinfo *cls;
	utf *classname;

	TYPEINFO_ASSERT(value);

    c = value->typeclass;

    /* assignments of primitive values are not checked here. */
    if (!c.any && !dest.any)
        return true;

    /* primitive and reference types are not assignment compatible. */
    if (!c.any || !dest.any)
        return false;

    /* the null type can be assigned to any type */
    if (TYPEINFO_IS_NULLTYPE(*value))
        return true;

    /* uninitialized objects are not assignable */
    if (TYPEINFO_IS_NEWOBJECT(*value))
        return false;

	if (IS_CLASSREF(c)) {
		/* The value type is an unresolved class reference. */
		classname = c.ref->name;
	}
	else {
		classname = c.cls->name;
	}

	if (IS_CLASSREF(dest)) {
		/* the destination type is an unresolved class reference */
		/* In this case we cannot tell a lot about assignability. */

		/* the common case of value and dest type having the same classname */
		if (dest.ref->name == classname && !value->merged)
			return true;

		/* we cannot tell if value is assignable to dest, so we */
		/* leave it up to the resolving code to check this      */
		return MAYBE;
	}

	/* { we know that dest is a loaded class } */

	if (IS_CLASSREF(c)) {
		/* the value type is an unresolved class reference */
		
		/* the common case of value and dest type having the same classname */
		if (dest.cls->name == classname)
			return true;

		/* we cannot tell if value is assignable to dest, so we */
		/* leave it up to the resolving code to check this      */
		return MAYBE;
	}

	/* { we know that both c and dest are loaded classes } */

	cls = c.cls;
	
	if (!cls->loaded)
		load_class_bootstrap(cls); /* XXX */
	if (!dest.cls->loaded)
		load_class_bootstrap(dest.cls); /* XXX */

	TYPEINFO_ASSERT(cls->loaded);
	TYPEINFO_ASSERT(dest.cls->loaded);

	/* maybe we need to link the classes */
	if (!cls->linked)
		link_class(cls); /* XXX */
	if (!dest.cls->linked)
		link_class(dest.cls); /* XXX */

	/* { we know that both c and dest are linked classes } */
	TYPEINFO_ASSERT(cls->linked);
	TYPEINFO_ASSERT(dest.cls->linked);

    if (dest.cls->flags & ACC_INTERFACE) {
        /* We are assigning to an interface type. */
        return merged_implements_interface(cls,value->merged,dest.cls);
    }

    if (CLASSINFO_IS_ARRAY(dest.cls)) {
		arraydescriptor *arraydesc = dest.cls->vftbl->arraydesc;
		int dimension = arraydesc->dimension;
		classinfo *elementclass = (arraydesc->elementvftbl)
			? arraydesc->elementvftbl->class : NULL;
			
        /* We are assigning to an array type. */
        if (!TYPEINFO_IS_ARRAY(*value))
            return false;

        /* {Both value and dest.cls are array types.} */

        /* value must have at least the dimension of dest.cls. */
        if (value->dimension < dimension)
            return false;

        if (value->dimension > dimension) {
            /* value has higher dimension so we need to check
             * if its component array can be assigned to the
             * element type of dest.cls */

			if (!elementclass) return false;
            
            if (elementclass->flags & ACC_INTERFACE) {
                /* We are assigning to an interface type. */
                return classinfo_implements_interface(pseudo_class_Arraystub,
                                                      elementclass);
            }

            /* We are assigning to a class type. */
            return class_issubclass(pseudo_class_Arraystub,elementclass);
        }

        /* {value and dest.cls have the same dimension} */

        if (value->elementtype != arraydesc->elementtype)
            return false;

        if (value->elementclass.any) {
            /* We are assigning an array of objects so we have to
             * check if the elements are assignable.
             */

            if (elementclass->flags & ACC_INTERFACE) {
                /* We are assigning to an interface type. */

                return merged_implements_interface(value->elementclass.cls,
                                                   value->merged,
                                                   elementclass);
            }
            
            /* We are assigning to a class type. */
            return class_issubclass(value->elementclass.cls,elementclass);
        }

        return true;
    }

    /* {dest.cls is not an array} */
    /* {dest.cls is a loaded class} */

	/* If there are any unresolved references in the merged list, we cannot */
	/* tell if the assignment will be ok.                                   */
	/* This can only happen when cls is java.lang.Object                    */
	if (cls == class_java_lang_Object && value->merged) {
		classref_or_classinfo *mlist = value->merged->list;
		int i = value->merged->count;
		while (i--)
			if (IS_CLASSREF(*mlist++))
				return MAYBE;
	}
        
    /* We are assigning to a class type */
    if (cls->flags & ACC_INTERFACE)
        cls = class_java_lang_Object;
    
    return class_issubclass(cls,dest.cls);
}

/**********************************************************************/
/* INITIALIZATION FUNCTIONS                                           */
/* The following functions fill in uninitialized typeinfo structures. */
/**********************************************************************/

bool
typeinfo_init_class(typeinfo *info,classref_or_classinfo c)
{
	char *utf_ptr;
	int len;
	classinfo *cls;
		
	TYPEINFO_ASSERT(c.any);
	TYPEINFO_ASSERT(info);

	/* if necessary, try to resolve lazily */
	if (!resolve_classref_or_classinfo(NULL /* XXX should now method */,
				c,resolveLazy,true,&cls))
	{
		panic("XXX could not resolve class reference"); /* XXX */
		return false;
	}
	
	if (cls) {
		TYPEINFO_INIT_CLASSINFO(*info,cls);
		return true;
	}

	/* {the type could no be resolved lazily} */

	info->typeclass.ref = c.ref;
	info->elementclass.any = NULL;
	info->dimension = 0;
	info->merged = NULL;

	/* handle array type references */
	utf_ptr = c.ref->name->text;
	len = c.ref->name->blength;
	if (*utf_ptr == '[') {
		/* count dimensions */
		while (*utf_ptr == '[') {
			utf_ptr++;
			info->dimension++;
			len--;
		}
		if (*utf_ptr == 'L') {
			utf_ptr++;
			len -= 2;
			info->elementtype = ARRAYTYPE_OBJECT;
			info->elementclass.ref = class_get_classref(c.ref->referer,utf_new(utf_ptr,len));
		}
		else {
			/* an array with primitive element type */
			/* should have been resolved above */
			TYPEINFO_ASSERT(false);
		}
	}
	return true;
}

void
typeinfo_init_from_typedesc(typedesc *desc,u1 *type,typeinfo *info)
{
	TYPEINFO_ASSERT(desc);

#ifdef TYPEINFO_VERBOSE
	fprintf(stderr,"typeinfo_init_from_typedesc(");
	descriptor_debug_print_typedesc(stderr,desc);
	fprintf(stderr,")\n");
#endif

	if (type)
		*type = desc->type;

	if (info) {
		if (desc->type == TYPE_ADR) {
			TYPEINFO_ASSERT(desc->classref);
			TYPEINFO_INIT_CLASSREF(*info,desc->classref);
		}
		else {
			TYPEINFO_INIT_PRIMITIVE(*info);
		}
	}
}

void
typeinfo_init_from_methoddesc(methoddesc *desc,u1 *typebuf,typeinfo *infobuf,
                              int buflen,bool twoword,
                              u1 *returntype,typeinfo *returntypeinfo)
{
	int i;
    int args = 0;

	TYPEINFO_ASSERT(desc);
	TYPEINFO_ASSERT(typebuf);
	TYPEINFO_ASSERT(infobuf);

#ifdef TYPEINFO_VERBOSE
	fprintf(stderr,"typeinfo_init_from_methoddesc(");
	descriptor_debug_print_methoddesc(stderr,desc);
	fprintf(stderr,")\n");
#endif

    /* check arguments */
    for (i=0; i<desc->paramcount; ++i) {
		if (++args > buflen)
			panic("Buffer too small for method arguments."); /* XXX */

		typeinfo_init_from_typedesc(desc->paramtypes + i,typebuf++,infobuf++);
		
		if (twoword && (typebuf[-1] == TYPE_LONG || typebuf[-1] == TYPE_DOUBLE)) {
			if (++args > buflen)
				panic("Buffer too small for method arguments."); /* XXX */
			*typebuf++ = TYPE_VOID;
			TYPEINFO_INIT_PRIMITIVE(*infobuf);
			infobuf++;
		}
    }

    /* check returntype */
    if (returntype) {
		typeinfo_init_from_typedesc(&(desc->returntype),returntype,returntypeinfo);
	}
}

void
typedescriptor_init_from_typedesc(typedescriptor *td,
								  typedesc *desc)
{
	td->type = desc->type;
	if (td->type == TYPE_ADR) {
		TYPEINFO_INIT_CLASSREF(td->info,desc->classref);
	}
	else {
		TYPEINFO_INIT_PRIMITIVE(td->info);
	}
}

int
typedescriptors_init_from_methoddesc(typedescriptor *td,
									 methoddesc *desc,
									 int buflen,bool twoword,
									 typedescriptor *returntype)
{
	int i;
    int args = 0;

    /* check arguments */
    for (i=0; i<desc->paramcount; ++i) {
		if (++args > buflen)
			panic("Buffer too small for method arguments."); /* XXX */

		typedescriptor_init_from_typedesc(td,desc->paramtypes + i);
		td++;

		if (twoword && (td[-1].type == TYPE_LONG || td[-1].type == TYPE_DOUBLE)) {
			if (++args > buflen)
				panic("Buffer too small for method arguments."); /* XXX */
			td->type = TYPE_VOID;
			TYPEINFO_INIT_PRIMITIVE(td->info);
			td++;
		}
    }

    /* check returntype */
    if (returntype) {
		typedescriptor_init_from_typedesc(returntype,&(desc->returntype));
	}

	return args;
}

void
typeinfo_init_component(typeinfo *srcarray,typeinfo *dst)
{
    if (TYPEINFO_IS_NULLTYPE(*srcarray)) {
        TYPEINFO_INIT_NULLTYPE(*dst);
        return;
    }
    
    if (!TYPEINFO_IS_ARRAY(*srcarray))
        panic("Trying to access component of non-array"); /* XXX throw exception */

	if (IS_CLASSREF(srcarray->typeclass)) {
		constant_classref *comp;
		comp = class_get_classref_component_of(srcarray->typeclass.ref);

		if (comp)
			TYPEINFO_INIT_CLASSREF(*dst,comp);
		else
			TYPEINFO_INIT_PRIMITIVE(*dst);
	}
	else {
		vftbl_t *comp;

		TYPEINFO_ASSERT(srcarray->typeclass.cls->vftbl);
		TYPEINFO_ASSERT(srcarray->typeclass.cls->vftbl->arraydesc);

		comp = srcarray->typeclass.cls->vftbl->arraydesc->componentvftbl;
		if (comp)
			TYPEINFO_INIT_CLASSINFO(*dst,comp->class);
		else
			TYPEINFO_INIT_PRIMITIVE(*dst);
	}
    
    dst->merged = srcarray->merged; /* XXX should we do a deep copy? */
}

void
typeinfo_clone(typeinfo *src,typeinfo *dest)
{
    int count;
    classref_or_classinfo *srclist,*destlist;

    if (src == dest)
        return;
    
    *dest = *src;

    if (src->merged) {
        count = src->merged->count;
        TYPEINFO_ALLOCMERGED(dest->merged,count);
        dest->merged->count = count;

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
#ifdef TYPEINFO_VERBOSE
    fprintf(stderr,"Error in typeinfo_merge: %s\n",str);
    fprintf(stderr,"Typeinfo x:\n");
    typeinfo_print(stderr,x,1);
    fprintf(stderr,"Typeinfo y:\n");
    typeinfo_print(stderr,y,1);
#endif
    panic(str); /* XXX throw an exception */
}

/* Condition: clsx != clsy. */
/* Returns: true if dest was changed (currently always true). */
static
bool
typeinfo_merge_two(typeinfo *dest,classref_or_classinfo clsx,classref_or_classinfo clsy)
{
    TYPEINFO_FREEMERGED_IF_ANY(dest->merged);
    TYPEINFO_ALLOCMERGED(dest->merged,2);
    dest->merged->count = 2;

	TYPEINFO_ASSERT(clsx.any != clsy.any);

    if (clsx.any < clsy.any) {
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
typeinfo_merge_add(typeinfo *dest,typeinfo_mergedlist *m,classref_or_classinfo cls)
{
    int count;
    typeinfo_mergedlist *newmerged;
    classref_or_classinfo *mlist,*newlist;

    count = m->count;
    mlist = m->list;

    /* Check if cls is already in the mergedlist m. */
    while (count--) {
        if ((mlist++)->any == cls.any) { /* XXX check equal classrefs? */
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
        if (mlist->any > cls.any)
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
    classref_or_classinfo *clsx,*clsy,*newlist;

    /* count the elements that will be in the resulting list */
    /* (Both lists are sorted, equal elements are counted only once.) */
    clsx = x->list;
    clsy = y->list;
    countx = x->count;
    county = y->count;
    while (countx && county) {
        if (clsx->any == clsy->any) {
            clsx++;
            clsy++;
            countx--;
            county--;
        }
        else if (clsx->any < clsy->any) {
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
        if (clsx->any == clsy->any) {
            *newlist++ = *clsx++;
            clsy++;
            countx--;
            county--;
        }
        else if (clsx->any < clsy->any) {
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
                         classref_or_classinfo *result,
                         classref_or_classinfo x,classref_or_classinfo y,
                         typeinfo_mergedlist *mergedx,
                         typeinfo_mergedlist *mergedy)
{
	classref_or_classinfo t;
    classinfo *tcls,*common;
    typeinfo_mergedlist *tmerged;
    bool changed;
	utf *xname;
	utf *yname;

	TYPEINFO_ASSERT(dest && result && x.any && y.any);
	TYPEINFO_ASSERT(x.cls != pseudo_class_Null);
	TYPEINFO_ASSERT(y.cls != pseudo_class_Null);
	TYPEINFO_ASSERT(x.cls != pseudo_class_New);
	TYPEINFO_ASSERT(y.cls != pseudo_class_New);

#ifdef XXX
	/* check clsx */
	if (!clsx->loaded)
		if (!load_class_bootstrap(clsx))
			return false;

	if (!clsx->linked)
		if (!link_class(clsx))
			return false;

	/* check clsy */
	if (!clsy->loaded)
		if (!load_class_bootstrap(clsy))
			return false;

	if (!clsy->linked)
		if (!link_class(clsy))
			return false;
#endif

	/*--------------------------------------------------*/
	/* common cases                                     */
	/*--------------------------------------------------*/

    /* Common case 1: x and y are the same class or class reference */
    /* (This case is very simple unless *both* x and y really represent
     *  merges of subclasses of clsx==clsy.)
     */
    if ( (x.any == y.any) && (!mergedx || !mergedy) ) {
  return_simple_x:
        /* DEBUG */ /* log_text("return simple x"); */
        changed = (dest->merged != NULL);
        TYPEINFO_FREEMERGED_IF_ANY(dest->merged);
        dest->merged = NULL;
        *result = x;
        /* DEBUG */ /* log_text("returning"); */
        return changed;
    }

	xname = (IS_CLASSREF(x)) ? x.ref->name : x.cls->name;
	yname = (IS_CLASSREF(y)) ? y.ref->name : y.cls->name;

	/* Common case 2: xname == yname, at least one unresolved */
    if ((IS_CLASSREF(x) || IS_CLASSREF(y)) && (xname == yname))
	{
		/* use the loaded one if any */
		if (!IS_CLASSREF(y))
			x = y;
		goto return_simple_x;
    }

	/*--------------------------------------------------*/
	/* non-trivial cases                                */
	/*--------------------------------------------------*/

#ifdef TYPEINFO_VERBOSE
	{
		typeinfo dbgx,dbgy;
		fprintf(stderr,"merge_nonarrays:\n");
		TYPEINFO_INIT_CLASSREF_OR_CLASSINFO(dbgx,x);
		dbgx.merged = mergedx;
		TYPEINFO_INIT_CLASSREF_OR_CLASSINFO(dbgy,y);
		dbgy.merged = mergedy;
		typeinfo_print(stderr,&dbgx,4);
		fprintf(stderr,"  with:\n");
		typeinfo_print(stderr,&dbgy,4);
	}
#endif

    /* If y is unresolved or an interface, swap x and y. */
    if (IS_CLASSREF(y) || (!IS_CLASSREF(x) && y.cls->flags & ACC_INTERFACE))
	{
        t = x; x = y; y = t;
        tmerged = mergedx; mergedx = mergedy; mergedy = tmerged;
    }
	
    /* {We know: If only one of x,y is unresolved it is x,} */
    /* {         If both x,y are resolved and only one of x,y is an interface it is x.} */

	if (IS_CLASSREF(x)) {
		/* {We know: x and y have different class names} */
		
        /* Check if we are merging an unresolved type with java.lang.Object */
        if (y.cls == class_java_lang_Object && !mergedy) {
            x = y;
            goto return_simple_x;
        }
            
		common = class_java_lang_Object;
		goto merge_with_simple_x;
	}

	/* {We know: both x and y are resolved} */
    /* {We know: If only one of x,y is an interface it is x.} */

    /* Handle merging of interfaces: */
    if (x.cls->flags & ACC_INTERFACE) {
        /* {x.cls is an interface and mergedx == NULL.} */
        
        if (y.cls->flags & ACC_INTERFACE) {
            /* We are merging two interfaces. */
            /* {mergedy == NULL} */

            /* {We know that x.cls!=y.cls (see common case at beginning.)} */
            result->cls = class_java_lang_Object;
            return typeinfo_merge_two(dest,x,y);
        }

        /* {We know: x is an interface, y is a class.} */

        /* Check if we are merging an interface with java.lang.Object */
        if (y.cls == class_java_lang_Object && !mergedy) {
            x = y;
            goto return_simple_x;
        }
            

        /* If the type y implements x then the result of the merge
         * is x regardless of mergedy.
         */
        
        if (CLASSINFO_IMPLEMENTS_INTERFACE(y.cls,x.cls->index)
            || mergedlist_implements_interface(mergedy,x.cls))
        {
            /* y implements x, so the result of the merge is x. */
            goto return_simple_x;
        }
        
        /* {We know: x is an interface, the type y a class or a merge
         * of subclasses and does not implement x.} */

        /* There may still be superinterfaces of x which are implemented
         * by y, too, so we have to add x.cls to the mergedlist.
         */

        /* if x has no superinterfaces we could return a simple java.lang.Object */
        
        common = class_java_lang_Object;
        goto merge_with_simple_x;
    }

    /* {We know: x and y are classes (not interfaces).} */
    
    /* If *x is deeper in the inheritance hierarchy swap x and y. */
    if (x.cls->index > y.cls->index) {
        t = x; x = y; y = t;
        tmerged = mergedx; mergedx = mergedy; mergedy = tmerged;
    }

    /* {We know: y is at least as deep in the hierarchy as x.} */

    /* Find nearest common anchestor for the classes. */
    common = x.cls;
    tcls = y.cls;
    while (tcls->index > common->index)
        tcls = tcls->super;
    while (common != tcls) {
        common = common->super;
        tcls = tcls->super;
    }

    /* {common == nearest common anchestor of x and y.} */

    /* If x.cls==common and x is a whole class (not a merge of subclasses)
     * then the result of the merge is x.
     */
    if (x.cls == common && !mergedx) {
        goto return_simple_x;
    }
   
    if (mergedx) {
        result->cls = common;
        if (mergedy)
            return typeinfo_merge_mergedlists(dest,mergedx,mergedy);
        else
            return typeinfo_merge_add(dest,mergedx,y);
    }

merge_with_simple_x:
    result->cls = common;
    if (mergedy)
        return typeinfo_merge_add(dest,mergedy,x);
    else
        return typeinfo_merge_two(dest,x,y);
}

/* Condition: *dest must be a valid initialized typeinfo. */
/* Condition: dest != y. */
/* Returns: true if dest was changed. */
bool
typeinfo_merge(typeinfo *dest,typeinfo* y)
{
    typeinfo *x;
    typeinfo *tmp;
    classref_or_classinfo common;
    classref_or_classinfo elementclass;
    int dimension;
    int elementtype;
    bool changed;

	/*--------------------------------------------------*/
	/* fast checks                                      */
	/*--------------------------------------------------*/

    /* Merging something with itself is a nop */
    if (dest == y)
        return false;

    /* Merging two returnAddress types is ok. */
	/* Merging two different returnAddresses never happens, as the verifier */
	/* keeps them separate in order to check all the possible return paths  */
	/* from JSR subroutines.                                                */
    if (!dest->typeclass.any && !y->typeclass.any) {
		TYPEINFO_ASSERT(TYPEINFO_RETURNADDRESS(*dest) ==  TYPEINFO_RETURNADDRESS(*y));
        return false;
	}
    
    /* Primitive types cannot be merged with reference types */
	/* XXX only check this in debug mode? */
    if (!dest->typeclass.any || !y->typeclass.any)
        typeinfo_merge_error("Trying to merge primitive types.",dest,y);

    /* handle uninitialized object types */
    if (TYPEINFO_IS_NEWOBJECT(*dest) || TYPEINFO_IS_NEWOBJECT(*y)) {
        if (!TYPEINFO_IS_NEWOBJECT(*dest) || !TYPEINFO_IS_NEWOBJECT(*y))
            typeinfo_merge_error("Trying to merge uninitialized object type.",dest,y);
        if (TYPEINFO_NEWOBJECT_INSTRUCTION(*dest)
            != TYPEINFO_NEWOBJECT_INSTRUCTION(*y))
            typeinfo_merge_error("Trying to merge different uninitialized objects.",dest,y);
        return false;
    }
    
	/*--------------------------------------------------*/
	/* common cases                                     */
	/*--------------------------------------------------*/

    /* Common case: dest and y are the same class or class reference */
    /* (This case is very simple unless *both* dest and y really represent
     *  merges of subclasses of class dest==class y.)
     */
    if ((dest->typeclass.any == y->typeclass.any) && (!dest->merged || !y->merged)) {
return_simple:
        changed = (dest->merged != NULL);
        TYPEINFO_FREEMERGED_IF_ANY(dest->merged);
        dest->merged = NULL;
        return changed;
    }
    
    /* Handle null types: */
    if (TYPEINFO_IS_NULLTYPE(*y)) {
        return false;
    }
    if (TYPEINFO_IS_NULLTYPE(*dest)) {
        TYPEINFO_FREEMERGED_IF_ANY(dest->merged);
        TYPEINFO_CLONE(*y,*dest);
        return true;
    }

	/* Common case: two types with the same name, at least one unresolved */
	if (IS_CLASSREF(dest->typeclass)) {
		if (IS_CLASSREF(y->typeclass)) {
			if (dest->typeclass.ref->name == y->typeclass.ref->name)
				goto return_simple;
		}
		else {
			/* XXX should we take y instead of dest here? */
			if (dest->typeclass.ref->name == y->typeclass.cls->name)
				goto return_simple;
		}
	}
	else {
		if (IS_CLASSREF(y->typeclass) 
		    && (dest->typeclass.cls->name == y->typeclass.ref->name))
		{
			goto return_simple;
		}
	}

	/*--------------------------------------------------*/
	/* non-trivial cases                                */
	/*--------------------------------------------------*/

#ifdef TYPEINFO_VERBOSE
	fprintf(stderr,"merge:\n");
    typeinfo_print(stderr,dest,4);
    typeinfo_print(stderr,y,4);
#endif

    /* This function uses x internally, so x and y can be swapped
     * without changing dest. */
    x = dest;
    changed = false;
    
    /* Handle merging of arrays: */
    if (TYPEINFO_IS_ARRAY(*x) && TYPEINFO_IS_ARRAY(*y)) {
        
        /* Make x the one with lesser dimension */
        if (x->dimension > y->dimension) {
            tmp = x; x = y; y = tmp;
        }

        /* If one array (y) has higher dimension than the other,
         * interpret it as an array (same dim. as x) of Arraystubs. */
        if (x->dimension < y->dimension) {
            dimension = x->dimension;
            elementtype = ARRAYTYPE_OBJECT;
            elementclass.cls = pseudo_class_Arraystub;
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
                common.cls = pseudo_class_Arraystub;
                elementtype = 0;
                elementclass.any = NULL;
            }
            else {
                common.cls = class_multiarray_of(dimension,pseudo_class_Arraystub);
                elementtype = ARRAYTYPE_OBJECT;
                elementclass.cls = pseudo_class_Arraystub;
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

                /* DEBUG */ /* log_text("finding resulting array class: "); */
				if (IS_CLASSREF(elementclass))
					common.ref = class_get_classref_multiarray_of(dimension,elementclass.ref);
				else
					common.cls = class_multiarray_of(dimension,elementclass.cls);
                /* DEBUG */ /* utf_display(common->name); printf("\n"); */
            }
			else {
				common.any = y->typeclass.any;
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
        elementclass.any = NULL;
    }

    /* Put the new values into dest if neccessary. */

    if (dest->typeclass.any != common.any) {
        dest->typeclass.any = common.any;
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
    if (dest->elementclass.any != elementclass.any) {
        dest->elementclass.any = elementclass.any;
        changed = true;
    }

    return changed;
}


/**********************************************************************/
/* DEBUGGING HELPERS                                                  */
/**********************************************************************/

#ifdef TYPEINFO_DEBUG

#if 0
static int
typeinfo_test_compare(classref_or_classinfo *a,classref_or_classinfo *b)
{
    if (a->any == b->any) return 0;
    if (a->any < b->any) return -1;
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
            info->merged->list[i].any = infobuf[i].typeclass.any;
        }
        qsort(info->merged->list,num,sizeof(classref_or_classinfo),
              (int(*)(const void *,const void *))&typeinfo_test_compare);
    }
    else {
        typeinfo_init_from_method_args(desc,NULL,NULL,0,false,
                                       &returntype,info);
    }
}
#endif

#define TYPEINFO_TEST_BUFLEN  4000

static bool
typeinfo_equal(typeinfo *x,typeinfo *y)
{
    int i;
    
    if (x->typeclass.any != y->typeclass.any) return false;
    if (x->dimension != y->dimension) return false;
    if (x->dimension) {
        if (x->elementclass.any != y->elementclass.any) return false;
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
            if (x->merged->list[i].any != y->merged->list[i].any)
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

#if 0
static void
typeinfo_inc_dimension(typeinfo *info)
{
    if (info->dimension++ == 0) {
        info->elementtype = ARRAYTYPE_OBJECT;
        info->elementclass = info->typeclass;
    }
    info->typeclass = class_array_of(info->typeclass);
}
#endif

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
	int res;
    
    if (!file)
        panic("could not open typeinfo test file");

    while (fgets(buf,TYPEINFO_TEST_BUFLEN,file)) {
        if (buf[0] == '#' || !strlen(buf))
            continue;
        
        res = sscanf(buf,"%s\t%s\t%s\n",bufa,bufb,bufc);
        if (res != 3 || !strlen(bufa) || !strlen(bufb) || !strlen(bufc))
            panic("Invalid line in typeinfo test file (none of empty, comment or test)");
#if 0
        typeinfo_test_parse(&a,bufa);
        typeinfo_test_parse(&b,bufb);
        typeinfo_test_parse(&c,bufc);
#endif
#if 0
        do {
#endif
            typeinfo_testmerge(&a,&b,&c,&failed); /* check result */
            typeinfo_testmerge(&b,&a,&c,&failed); /* check commutativity */

            if (TYPEINFO_IS_NULLTYPE(a)) break;
            if (TYPEINFO_IS_NULLTYPE(b)) break;
            if (TYPEINFO_IS_NULLTYPE(c)) break;
            
            maxdim = a.dimension;
            if (b.dimension > maxdim) maxdim = b.dimension;
            if (c.dimension > maxdim) maxdim = c.dimension;

#if 0
            if (maxdim < TYPEINFO_TEST_MAXDIM) {
                typeinfo_inc_dimension(&a);
                typeinfo_inc_dimension(&b);
                typeinfo_inc_dimension(&c);
            }
        } while (maxdim < TYPEINFO_TEST_MAXDIM);
#endif
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

#if 0
void
typeinfo_init_from_fielddescriptor(typeinfo *info,char *desc)
{
    typeinfo_init_from_descriptor(info,desc,desc+strlen(desc));
}
#endif

#define TYPEINFO_MAXINDENT  80

void
typeinfo_print_class(FILE *file,classref_or_classinfo c)
{
	/*fprintf(file,"<class %p>",c.any);*/

	if (!c.any) {
		fprintf(file,"<null>");
	}
	else {
		if (IS_CLASSREF(c)) {
			fprintf(file,"<ref>");
			utf_fprint(file,c.ref->name);
		}
		else {
			utf_fprint(file,c.cls->name);
		}
	}
}

void
typeinfo_print(FILE *file,typeinfo *info,int indent)
{
    int i;
    char ind[TYPEINFO_MAXINDENT + 1];
    instruction *ins;
	basicblock *bptr;

    if (indent > TYPEINFO_MAXINDENT) indent = TYPEINFO_MAXINDENT;
    
    for (i=0; i<indent; ++i)
        ind[i] = ' ';
    ind[i] = (char) 0;
    
    if (TYPEINFO_IS_PRIMITIVE(*info)) {
		bptr = (basicblock*) TYPEINFO_RETURNADDRESS(*info);
		if (bptr)
			fprintf(file,"%sreturnAddress (L%03d)\n",ind,bptr->debug_nr);
		else
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
            fprintf(file,"%sNEW(%p):",ind,(void*)ins);
			typeinfo_print_class(file,CLASSREF_OR_CLASSINFO(ins[-1].val.a));
            fprintf(file,"\n");
        }
        else {
            fprintf(file,"%sNEW(this)",ind);
        }
        return;
    }

    fprintf(file,"%sClass:      ",ind);
	typeinfo_print_class(file,info->typeclass);
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
			  typeinfo_print_class(file,info->elementclass);
              fprintf(file,"\n");
              break;
              
          default:
              fprintf(file,"INVALID ARRAYTYPE!\n");
        }
    }

    if (info->merged) {
        fprintf(file,"%sMerged:     ",ind);
        for (i=0; i<info->merged->count; ++i) {
            if (i) fprintf(file,", ");
			typeinfo_print_class(file,info->merged->list[i]);
        }
        fprintf(file,"\n");
    }
}

void
typeinfo_print_short(FILE *file,typeinfo *info)
{
    int i;
    instruction *ins;
	basicblock *bptr;

	/*fprintf(file,"<typeinfo %p>",info);*/

	if (!info) {
		fprintf(file,"(typeinfo*)NULL");
		return;
	}

    if (TYPEINFO_IS_PRIMITIVE(*info)) {
		bptr = (basicblock*) TYPEINFO_RETURNADDRESS(*info);
		if (bptr)
			fprintf(file,"ret(L%03d)",bptr->debug_nr);
		else
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
			/*fprintf(file,"<ins %p>",ins);*/
            fprintf(file,"NEW(%p):",(void*)ins);
			typeinfo_print_class(file,CLASSREF_OR_CLASSINFO(ins[-1].val.a));
        }
        else
            fprintf(file,"NEW(this)");
        return;
    }

    typeinfo_print_class(file,info->typeclass);

    if (info->merged) {
        fprintf(file,"{");
        for (i=0; i<info->merged->count; ++i) {
            if (i) fprintf(file,",");
			typeinfo_print_class(file,info->merged->list[i]);
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
		  typeinfo_print_short(file,info);
          break;
          
      default:
          fprintf(file,"!");
    }
}

void
typeinfo_print_stacktype(FILE *file,int type,typeinfo *info)
{
	TYPEINFO_ASSERT(file);
	TYPEINFO_ASSERT(type != TYPE_ADDRESS || info != NULL);
	if (type == TYPE_ADDRESS && TYPEINFO_IS_PRIMITIVE(*info)) {	
		typeinfo_retaddr_set *set = (typeinfo_retaddr_set*)
			TYPEINFO_RETURNADDRESS(*info);
		fprintf(file,"ret(L%03d",((basicblock*)(set->addr))->debug_nr);
		set = set->alt;
		while (set) {
			fprintf(file,"|L%03d",((basicblock*)(set->addr))->debug_nr);
			set = set->alt;
		}
		fprintf(file,")");
	}
	else
		typeinfo_print_type(file,type,info);
}

void
typedescriptor_print(FILE *file,typedescriptor *td)
{
	typeinfo_print_type(file,td->type,&(td->info));
}

void
typevector_print(FILE *file,typevector *vec,int size)
{
    int i;

	fprintf(file,"[%d]",vec->k);
    for (i=0; i<size; ++i) {
		fprintf(file," %d=",i);
        typedescriptor_print(file,vec->td + i);
    }
}

void
typevectorset_print(FILE *file,typevector *set,int size)
{
    int i;
	typevector *vec;

	fprintf(file,"[%d",set->k);
	vec = set->alt;
	while (vec) {
		fprintf(file,"|%d",vec->k);
		vec = vec->alt;
	}
	fprintf(file,"]");
	
    for (i=0; i<size; ++i) {
		fprintf(file," %d=",i);
        typedescriptor_print(file,set->td + i);
		vec = set->alt;
		while (vec) {
			fprintf(file,"|");
			typedescriptor_print(file,vec->td + i);
			vec = vec->alt;
		}
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
