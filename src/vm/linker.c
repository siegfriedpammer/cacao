/* src/vm/linker.c - class linker functions

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

   Authors: Reinhard Grafl

   Changes: Andreas Krall
            Roman Obermaiser
            Mark Probst
            Edwin Steiner
            Christian Thalinger

   $Id: linker.c 2169 2005-03-31 15:50:57Z twisti $

*/


#include "mm/memory.h"
#include "native/native.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/jit/codegen.inc.h"


/* global variables ***********************************************************/

static s4 interfaceindex;       /* sequential numbering of interfaces         */
static s4 classvalue;


/* primitivetype_table *********************************************************

   Structure for primitive classes: contains the class for wrapping
   the primitive type, the primitive class, the name of the class for
   wrapping, the one character type signature and the name of the
   primitive class.
 
   CAUTION: Don't change the order of the types. This table is indexed
   by the ARRAYTYPE_ constants (expcept ARRAYTYPE_OBJECT).

*******************************************************************************/

primitivetypeinfo primitivetype_table[PRIMITIVETYPE_COUNT] = { 
	{ NULL, NULL, "java/lang/Integer",   'I', "int"     , "[I", NULL, NULL },
	{ NULL, NULL, "java/lang/Long",      'J', "long"    , "[J", NULL, NULL },
	{ NULL, NULL, "java/lang/Float",     'F', "float"   , "[F", NULL, NULL },
	{ NULL, NULL, "java/lang/Double",    'D', "double"  , "[D", NULL, NULL },
	{ NULL, NULL, "java/lang/Byte",	     'B', "byte"    , "[B", NULL, NULL },
	{ NULL, NULL, "java/lang/Character", 'C', "char"    , "[C", NULL, NULL },
	{ NULL, NULL, "java/lang/Short",     'S', "short"   , "[S", NULL, NULL },
	{ NULL, NULL, "java/lang/Boolean",   'Z', "boolean" , "[Z", NULL, NULL },
	{ NULL, NULL, "java/lang/Void",	     'V', "void"    , NULL, NULL, NULL }
};


/* private functions **********************************************************/

static bool link_primitivetype_table(void);
static classinfo *link_class_intern(classinfo *c);
static arraydescriptor *link_array(classinfo *c);
static void linker_compute_class_values(classinfo *c);
static void linker_compute_subclasses(classinfo *c);
static void linker_addinterface(classinfo *c, classinfo *ic);
static s4 class_highestinterface(classinfo *c);


/* linker_init *****************************************************************

   Initializes the linker subsystem.

*******************************************************************************/

bool linker_init(void)
{
	/* reset interface index */

	interfaceindex = 0;

	/* link important system classes */

	if (!link_class(class_java_lang_Object))
		return false;

	if (!link_class(class_java_lang_String))
		return false;

	if (!link_class(class_java_lang_Cloneable))
		return false;

	if (!link_class(class_java_io_Serializable))
		return false;


	/* link classes for wrapping primitive types */

	if (!link_class(class_java_lang_Void))
		return false;

	if (!link_class(class_java_lang_Boolean))
		return false;

	if (!link_class(class_java_lang_Byte))
		return false;

	if (!link_class(class_java_lang_Character))
		return false;

	if (!link_class(class_java_lang_Short))
		return false;

	if (!link_class(class_java_lang_Integer))
		return false;

	if (!link_class(class_java_lang_Long))
		return false;

	if (!link_class(class_java_lang_Float))
		return false;

	if (!link_class(class_java_lang_Double))
		return false;


	/* create pseudo classes used by the typechecker */

    /* pseudo class for Arraystubs (extends java.lang.Object) */
    
	pseudo_class_Arraystub->loaded = true;
    pseudo_class_Arraystub->super = class_java_lang_Object;
    pseudo_class_Arraystub->interfacescount = 2;
    pseudo_class_Arraystub->interfaces = MNEW(classinfo*, 2);
    pseudo_class_Arraystub->interfaces[0] = class_java_lang_Cloneable;
    pseudo_class_Arraystub->interfaces[1] = class_java_io_Serializable;

    if (!link_class(pseudo_class_Arraystub))
		return false;

    /* pseudo class representing the null type */
    
	pseudo_class_Null->loaded = true;
    pseudo_class_Null->super = class_java_lang_Object;

	if (!link_class(pseudo_class_Null))
		return false;

    /* pseudo class representing new uninitialized objects */
    
	pseudo_class_New->loaded = true;
	pseudo_class_New->linked = true;
	pseudo_class_New->super = class_java_lang_Object;


	/* create classes representing primitive types */

	if (!link_primitivetype_table())
		return false;


	/* Correct vftbl-entries (retarded loading and linking of class           */
	/* java/lang/String).                                                     */

	stringtable_update();

	return true;
}


/* link_primitivetype_table ****************************************************

   Create classes representing primitive types.

*******************************************************************************/

static bool link_primitivetype_table(void)
{  
	classinfo *c;
	s4 i;

	for (i = 0; i < PRIMITIVETYPE_COUNT; i++) {
		/* create primitive class */
		c = class_new_intern(utf_new_char(primitivetype_table[i].name));
		c->classUsed = NOTUSED; /* not used initially CO-RT */
		c->impldBy = NULL;
		
		/* prevent loader from loading primitive class */
		c->loaded = true;
		if (!link_class(c))
			return false;

		primitivetype_table[i].class_primitive = c;

		/* create class for wrapping the primitive type */
		c = class_new_intern(utf_new_char(primitivetype_table[i].wrapname));
		primitivetype_table[i].class_wrap = c;
		primitivetype_table[i].class_wrap->classUsed = NOTUSED; /* not used initially CO-RT */
		primitivetype_table[i].class_wrap->impldBy = NULL;

		/* create the primitive array class */
		if (primitivetype_table[i].arrayname) {
			c = class_new_intern(utf_new_char(primitivetype_table[i].arrayname));
			primitivetype_table[i].arrayclass = c;
			c->loaded = true;
			if (!c->linked)
				if (!link_class(c))
					return false;
			primitivetype_table[i].arrayvftbl = c->vftbl;
		}
	}

	return true;
}


/* link_class ******************************************************************

   Wrapper function for link_class_intern to ease monitor enter/exit
   and exception handling.

*******************************************************************************/

classinfo *link_class(classinfo *c)
{
	classinfo *r;

#if defined(USE_THREADS)
	/* enter a monitor on the class */

	builtin_monitorenter((java_objectheader *) c);
#endif

	/* maybe the class is already linked */
	if (c->linked) {
#if defined(USE_THREADS)
		builtin_monitorexit((java_objectheader *) c);
#endif

		return c;
	}

#if defined(STATISTICS)
	/* measure time */

	if (getcompilingtime)
		compilingtime_stop();

	if (getloadingtime)
		loadingtime_start();
#endif

	/* call the internal function */
	r = link_class_intern(c);

	/* if return value is NULL, we had a problem and the class is not linked */
	if (!r)
		c->linked = false;

#if defined(STATISTICS)
	/* measure time */

	if (getloadingtime)
		loadingtime_stop();

	if (getcompilingtime)
		compilingtime_start();
#endif

#if defined(USE_THREADS)
	/* leave the monitor */

	builtin_monitorexit((java_objectheader *) c);
#endif

	return r;
}


/* link_class_intern ***********************************************************

   Tries to link a class. The function calculates the length in bytes
   that an instance of this class requires as well as the VTBL for
   methods and interface methods.
	
*******************************************************************************/

static classinfo *link_class_intern(classinfo *c)
{
	classinfo *super;             /* super class                              */
	classinfo *tc;                /* temporary class variable                 */
	s4 supervftbllength;          /* vftbllegnth of super class               */
	s4 vftbllength;               /* vftbllength of current class             */
	s4 interfacetablelength;      /* interface table length                   */
	vftbl_t *v;                   /* vftbl of current class                   */
	s4 i;                         /* interface/method/field counter           */
	arraydescriptor *arraydesc;   /* descriptor for array classes             */

	/* maybe the class is already linked */
	if (c->linked)
		return c;

	/* the class must be loaded */
	if (!c->loaded)
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Trying to link unloaded class");

	if (linkverbose)
		log_message_class("Linking class: ", c);

	/* ok, this class is somewhat linked */
	c->linked = true;

	arraydesc = NULL;

	/* check interfaces */

	for (i = 0; i < c->interfacescount; i++) {
		tc = c->interfaces[i];

		/* detect circularity */

		if (tc == c) {
			*exceptionptr =
				new_exception_utfmessage(string_java_lang_ClassCircularityError,
										 c->name);
			return NULL;
		}

		if (!tc->loaded)
			if (!load_class_from_classloader(tc, c->classloader))
				return NULL;

		if (!(tc->flags & ACC_INTERFACE)) {
			*exceptionptr =
				new_exception_message(string_java_lang_IncompatibleClassChangeError,
									  "Implementing class");
			return NULL;
		}

		if (!tc->linked)
			if (!link_class(tc))
				return NULL;
	}
	
	/* check super class */

	super = c->super;

	if (super == NULL) {          /* class java.lang.Object */
		c->index = 0;
        c->classUsed = USED;     /* Object class is always used CO-RT*/
		c->impldBy = NULL;
		c->instancesize = sizeof(java_objectheader);
		
		vftbllength = supervftbllength = 0;

		c->finalizer = NULL;

	} else {
		/* detect circularity */
		if (super == c) {
			*exceptionptr =
				new_exception_utfmessage(string_java_lang_ClassCircularityError,
										 c->name);
			return NULL;
		}

		if (!super->loaded)
			if (!load_class_from_classloader(super, c->classloader))
				return NULL;

		if (super->flags & ACC_INTERFACE) {
			/* java.lang.IncompatibleClassChangeError: class a has interface java.lang.Cloneable as super class */
			panic("Interface specified as super class");
		}

		/* Don't allow extending final classes */
		if (super->flags & ACC_FINAL) {
			*exceptionptr =
				new_exception_message(string_java_lang_VerifyError,
									  "Cannot inherit from final class");
			return NULL;
		}
		
		if (!super->linked)
			if (!link_class(super))
				return NULL;

		/* handle array classes */
		if (c->name->text[0] == '[')
			if (!(arraydesc = link_array(c)))
  				return NULL;

		if (c->flags & ACC_INTERFACE)
			c->index = interfaceindex++;
		else
			c->index = super->index + 1;
		
		c->instancesize = super->instancesize;
		
		vftbllength = supervftbllength = super->vftbl->vftbllength;
		
		c->finalizer = super->finalizer;
	}


	/* compute vftbl length */

	for (i = 0; i < c->methodscount; i++) {
		methodinfo *m = &(c->methods[i]);

		if (!(m->flags & ACC_STATIC)) { /* is instance method */
			tc = super;

			while (tc) {
				s4 j;

				for (j = 0; j < tc->methodscount; j++) {
					if (method_canoverwrite(m, &(tc->methods[j]))) {
						if (tc->methods[j].flags & ACC_PRIVATE)
							goto notfoundvftblindex;

						if (tc->methods[j].flags & ACC_FINAL) {
							/* class a overrides final method . */
							*exceptionptr =
								new_exception(string_java_lang_VerifyError);
							return NULL;
						}

						m->vftblindex = tc->methods[j].vftblindex;
						goto foundvftblindex;
					}
				}

				tc = tc->super;
			}

		notfoundvftblindex:
			m->vftblindex = (vftbllength++);
		foundvftblindex:
			;
		}
	}	


	/* check interfaces of ABSTRACT class for unimplemented methods */

	if (c->flags & ACC_ABSTRACT) {
		classinfo  *ic;
		methodinfo *im;
		s4 abstractmethodscount;
		s4 j;
		s4 k;

		abstractmethodscount = 0;

		for (i = 0; i < c->interfacescount; i++) {
			ic = c->interfaces[i];

			for (j = 0; j < ic->methodscount; j++) {
				im = &(ic->methods[j]);

				/* skip `<clinit>' and `<init>' */

				if (im->name == utf_clinit || im->name == utf_init)
					continue;

				tc = c;

				while (tc) {
					for (k = 0; k < tc->methodscount; k++) {
						if (method_canoverwrite(im, &(tc->methods[k])))
							goto noabstractmethod;
					}

					tc = tc->super;
				}

				abstractmethodscount++;

			noabstractmethod:
				;
			}
		}

		if (abstractmethodscount > 0) {
			methodinfo *am;

			/* reallocate methods memory */

			c->methods = MREALLOC(c->methods, methodinfo, c->methodscount,
								  c->methodscount + abstractmethodscount);

			for (i = 0; i < c->interfacescount; i++) {
				ic = c->interfaces[i];

				for (j = 0; j < ic->methodscount; j++) {
					im = &(ic->methods[j]);

					/* skip `<clinit>' and `<init>' */

					if (im->name == utf_clinit || im->name == utf_init)
						continue;

					tc = c;

					while (tc) {
						for (k = 0; k < tc->methodscount; k++) {
							if (method_canoverwrite(im, &(tc->methods[k])))
								goto noabstractmethod2;
						}

						tc = tc->super;
					}

					am = &(c->methods[c->methodscount]);
					c->methodscount++;

					MCOPY(am, im, methodinfo, 1);

					am->vftblindex = (vftbllength++);
					am->class = c;

				noabstractmethod2:
					;
				}
			}
		}
	}


#if defined(STATISTICS)
	if (opt_stat)
		count_vftbl_len +=
			sizeof(vftbl_t) + (sizeof(methodptr) * (vftbllength - 1));
#endif

	/* compute interfacetable length */

	interfacetablelength = 0;
	tc = c;
	while (tc) {
		for (i = 0; i < tc->interfacescount; i++) {
			s4 h = class_highestinterface(tc->interfaces[i]) + 1;
			if (h > interfacetablelength)
				interfacetablelength = h;
		}
		tc = tc->super;
	}

	/* allocate virtual function table */

	v = (vftbl_t *) mem_alloc(sizeof(vftbl_t) +
							  sizeof(methodptr) * (vftbllength - 1) +
							  sizeof(methodptr*) * (interfacetablelength - (interfacetablelength > 0)));
	v = (vftbl_t *) (((methodptr *) v) +
					 (interfacetablelength - 1) * (interfacetablelength > 1));
	c->header.vftbl = c->vftbl = v;
	v->class = c;
	v->vftbllength = vftbllength;
	v->interfacetablelength = interfacetablelength;
	v->arraydesc = arraydesc;

	/* store interface index in vftbl */

	if (c->flags & ACC_INTERFACE)
		v->baseval = -(c->index);

	/* copy virtual function table of super class */

	for (i = 0; i < supervftbllength; i++) 
		v->table[i] = super->vftbl->table[i];
	
	/* add method stubs into virtual function table */

	for (i = 0; i < c->methodscount; i++) {
		methodinfo *m = &(c->methods[i]);

		/* Methods in ABSTRACT classes from interfaces maybe already have a   */
		/* stubroutine.                                                       */

		if (!m->stubroutine) {
			if (!(m->flags & ACC_NATIVE)) {
				m->stubroutine = createcompilerstub(m);

			} else {
				functionptr f = native_findfunction(c->name,
										m->name,
										m->descriptor,
										(m->flags & ACC_STATIC));
#if defined(STATIC_CLASSPATH)
				if (f)
#endif
					m->stubroutine = createnativestub(f, m);
			}
		}

		if (!(m->flags & ACC_STATIC))
			v->table[m->vftblindex] = m->stubroutine;
	}

	/* compute instance size and offset of each field */
	
	for (i = 0; i < c->fieldscount; i++) {
		s4 dsize;
		fieldinfo *f = &(c->fields[i]);
		
		if (!(f->flags & ACC_STATIC)) {
			dsize = desc_typesize(f->descriptor);
			c->instancesize = ALIGN(c->instancesize, dsize);
			f->offset = c->instancesize;
			c->instancesize += dsize;
		}
	}

	/* initialize interfacetable and interfacevftbllength */
	
	v->interfacevftbllength = MNEW(s4, interfacetablelength);

#if defined(STATISTICS)
	if (opt_stat)
		count_vftbl_len += (4 + sizeof(s4)) * v->interfacetablelength;
#endif

	for (i = 0; i < interfacetablelength; i++) {
		v->interfacevftbllength[i] = 0;
		v->interfacetable[-i] = NULL;
	}
	
	/* add interfaces */
	
	for (tc = c; tc != NULL; tc = tc->super)
		for (i = 0; i < tc->interfacescount; i++)
			linker_addinterface(c, tc->interfaces[i]);

	/* add finalizer method (not for java.lang.Object) */

	if (super) {
		methodinfo *fi;

		fi = class_findmethod(c, utf_finalize, utf_void__void);

		if (fi)
			if (!(fi->flags & ACC_STATIC))
				c->finalizer = fi;
	}

	/* final tasks */

	linker_compute_subclasses(c);

	if (linkverbose)
		log_message_class("Linking done class: ", c);

	/* just return c to show that we didn't had a problem */

	return c;
}


/* link_array ******************************************************************

   This function is called by link_class to create the arraydescriptor
   for an array class.

   This function returns NULL if the array cannot be linked because
   the component type has not been linked yet.

*******************************************************************************/

static arraydescriptor *link_array(classinfo *c)
{
	classinfo *comp = NULL;
	s4 namelen = c->name->blength;
	arraydescriptor *desc;
	vftbl_t *compvftbl;

	/* Check the component type */
	switch (c->name->text[1]) {
	case '[':
		/* c is an array of arrays. */
		comp = class_new(utf_new_intern(c->name->text + 1, namelen - 1));
		if (!comp)
			panic("Could not find component array class.");
		break;

	case 'L':
		/* c is an array of objects. */
		comp = class_new(utf_new_intern(c->name->text + 2, namelen - 3));
		if (!comp)
			panic("Could not find component class.");
		break;
	}

	/* If the component type has not been linked, link it now */
	if (comp && !comp->linked) {
		if (!comp->loaded)
			if (!load_class_from_classloader(comp, c->classloader))
				return NULL;

		if (!link_class(comp))
			return NULL;
	}

	/* Allocate the arraydescriptor */
	desc = NEW(arraydescriptor);

	if (comp) {
		/* c is an array of references */
		desc->arraytype = ARRAYTYPE_OBJECT;
		desc->componentsize = sizeof(void*);
		desc->dataoffset = OFFSET(java_objectarray, data);
		
		compvftbl = comp->vftbl;
		if (!compvftbl)
			panic("Component class has no vftbl");
		desc->componentvftbl = compvftbl;
		
		if (compvftbl->arraydesc) {
			desc->elementvftbl = compvftbl->arraydesc->elementvftbl;
			if (compvftbl->arraydesc->dimension >= 255)
				panic("Creating array of dimension >255");
			desc->dimension = compvftbl->arraydesc->dimension + 1;
			desc->elementtype = compvftbl->arraydesc->elementtype;

		} else {
			desc->elementvftbl = compvftbl;
			desc->dimension = 1;
			desc->elementtype = ARRAYTYPE_OBJECT;
		}

	} else {
		/* c is an array of a primitive type */
		switch (c->name->text[1]) {
		case 'Z':
			desc->arraytype = ARRAYTYPE_BOOLEAN;
			desc->dataoffset = OFFSET(java_booleanarray,data);
			desc->componentsize = sizeof(u1);
			break;

		case 'B':
			desc->arraytype = ARRAYTYPE_BYTE;
			desc->dataoffset = OFFSET(java_bytearray,data);
			desc->componentsize = sizeof(u1);
			break;

		case 'C':
			desc->arraytype = ARRAYTYPE_CHAR;
			desc->dataoffset = OFFSET(java_chararray,data);
			desc->componentsize = sizeof(u2);
			break;

		case 'D':
			desc->arraytype = ARRAYTYPE_DOUBLE;
			desc->dataoffset = OFFSET(java_doublearray,data);
			desc->componentsize = sizeof(double);
			break;

		case 'F':
			desc->arraytype = ARRAYTYPE_FLOAT;
			desc->dataoffset = OFFSET(java_floatarray,data);
			desc->componentsize = sizeof(float);
			break;

		case 'I':
			desc->arraytype = ARRAYTYPE_INT;
			desc->dataoffset = OFFSET(java_intarray,data);
			desc->componentsize = sizeof(s4);
			break;

		case 'J':
			desc->arraytype = ARRAYTYPE_LONG;
			desc->dataoffset = OFFSET(java_longarray,data);
			desc->componentsize = sizeof(s8);
			break;

		case 'S':
			desc->arraytype = ARRAYTYPE_SHORT;
			desc->dataoffset = OFFSET(java_shortarray,data);
			desc->componentsize = sizeof(s2);
			break;

		default:
			panic("Invalid array class name");
		}
		
		desc->componentvftbl = NULL;
		desc->elementvftbl = NULL;
		desc->dimension = 1;
		desc->elementtype = desc->arraytype;
	}

	return desc;
}


/* linker_compute_subclasses ***************************************************

   XXX

*******************************************************************************/

static void linker_compute_subclasses(classinfo *c)
{
#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	compiler_lock();
#else
	intsDisable();
#endif
#endif

	if (!(c->flags & ACC_INTERFACE)) {
		c->nextsub = 0;
		c->sub = 0;
	}

	if (!(c->flags & ACC_INTERFACE) && (c->super != NULL)) {
		c->nextsub = c->super->sub;
		c->super->sub = c;
	}

	classvalue = 0;

	/* this is the java.lang.Object special case */

	if (!class_java_lang_Object) {
		linker_compute_class_values(c);

	} else {
		linker_compute_class_values(class_java_lang_Object);
	}

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	compiler_unlock();
#else
	intsRestore();
#endif
#endif
}


/* linker_compute_class_values *************************************************

   XXX

*******************************************************************************/

static void linker_compute_class_values(classinfo *c)
{
	classinfo *subs;

	c->vftbl->baseval = ++classvalue;

	subs = c->sub;

	while (subs) {
		linker_compute_class_values(subs);

		subs = subs->nextsub;
	}

	c->vftbl->diffval = classvalue - c->vftbl->baseval;
}


/* linker_addinterface *********************************************************

   Is needed by link_class for adding a VTBL to a class. All
   interfaces implemented by ic are added as well.

*******************************************************************************/

static void linker_addinterface(classinfo *c, classinfo *ic)
{
	s4     j, m;
	s4     i   = ic->index;
	vftbl_t *v = c->vftbl;

	if (i >= v->interfacetablelength)
		panic ("Inernal error: interfacetable overflow");

	if (v->interfacetable[-i])
		return;

	if (ic->methodscount == 0) {  /* fake entry needed for subtype test */
		v->interfacevftbllength[i] = 1;
		v->interfacetable[-i] = MNEW(methodptr, 1);
		v->interfacetable[-i][0] = NULL;

	} else {
		v->interfacevftbllength[i] = ic->methodscount;
		v->interfacetable[-i] = MNEW(methodptr, ic->methodscount);

#if defined(STATISTICS)
		if (opt_stat)
			count_vftbl_len += sizeof(methodptr) *
				(ic->methodscount + (ic->methodscount == 0));
#endif

		for (j = 0; j < ic->methodscount; j++) {
			classinfo *sc = c;

			while (sc) {
				for (m = 0; m < sc->methodscount; m++) {
					methodinfo *mi = &(sc->methods[m]);

					if (method_canoverwrite(mi, &(ic->methods[j]))) {
						v->interfacetable[-i][j] = v->table[mi->vftblindex];
						goto foundmethod;
					}
				}
				sc = sc->super;
			}
		foundmethod:
			;
		}
	}

	for (j = 0; j < ic->interfacescount; j++) 
		linker_addinterface(c, ic->interfaces[j]);
}


/* class_highestinterface ******************************************************

   Used by the function link_class to determine the amount of memory
   needed for the interface table.

*******************************************************************************/

static s4 class_highestinterface(classinfo *c)
{
	s4 h;
	s4 h2;
	s4 i;
	
    /* check for ACC_INTERFACE bit already done in link_class_intern */

    h = c->index;

	for (i = 0; i < c->interfacescount; i++) {
		h2 = class_highestinterface(c->interfaces[i]);

		if (h2 > h)
			h = h2;
	}

	return h;
}


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
