/* src/vm/resolve.c - resolving classes/interfaces/fields/methods

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

   Changes: Christan Thalinger

   $Id: resolve.c 3948 2005-12-20 12:59:22Z edwin $

*/


#include <assert.h>

#include "mm/memory.h"
#include "vm/resolve.h"
#include "vm/access.h"
#include "vm/classcache.h"
#include "vm/descriptor.h"
#include "vm/exceptions.h"
#include "vm/linker.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"
#include "vm/jit/jit.h"
#include "vm/jit/verify/typeinfo.h"


/******************************************************************************/
/* DEBUG HELPERS                                                              */
/******************************************************************************/

/*#define RESOLVE_VERBOSE*/

/******************************************************************************/
/* CLASS RESOLUTION                                                           */
/******************************************************************************/

/* resolve_class_from_name *****************************************************
 
   Resolve a symbolic class reference
  
   IN:
       referer..........the class containing the reference
       refmethod........the method from which resolution was triggered
                        (may be NULL if not applicable)
       classname........class name to resolve
       mode.............mode of resolution:
                            resolveLazy...only resolve if it does not
                                          require loading classes
                            resolveEager..load classes if necessary
	   checkaccess......if true, access rights to the class are checked
	   link.............if true, guarantee that the returned class, if any,
	                    has been linked
  
   OUT:
       *result..........set to result of resolution, or to NULL if
                        the reference has not been resolved
                        In the case of an exception, *result is
                        guaranteed to be set to NULL.
  
   RETURN VALUE:
       true.............everything ok 
                        (*result may still be NULL for resolveLazy)
       false............an exception has been thrown

   NOTE:
       The returned class is *not* guaranteed to be linked!
	   (It is guaranteed to be loaded, though.)
   
*******************************************************************************/

bool resolve_class_from_name(classinfo *referer,
							 methodinfo *refmethod,
							 utf *classname,
							 resolve_mode_t mode,
							 bool checkaccess,
							 bool link,
							 classinfo **result)
{
	classinfo *cls = NULL;
	char *utf_ptr;
	int len;
	
	assert(result);
	assert(referer);
	assert(classname);
	assert(mode == resolveLazy || mode == resolveEager);
	
	*result = NULL;

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"resolve_class_from_name(");
	utf_fprint(stderr,referer->name);
	fprintf(stderr,",%p,",referer->classloader);
	utf_fprint(stderr,classname);
	fprintf(stderr,",%d,%d)\n",(int)checkaccess,(int)link);
#endif

	/* lookup if this class has already been loaded */

	cls = classcache_lookup(referer->classloader, classname);

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    lookup result: %p\n",(void*)cls);
#endif

	if (!cls) {
		/* resolve array types */

		if (classname->text[0] == '[') {
			utf_ptr = classname->text + 1;
			len = classname->blength - 1;

			/* classname is an array type name */

			switch (*utf_ptr) {
				case 'L':
					utf_ptr++;
					len -= 2;
					/* FALLTHROUGH */
				case '[':
					/* the component type is a reference type */
					/* resolve the component type */
					if (!resolve_class_from_name(referer,refmethod,
									   utf_new(utf_ptr,len),
									   mode,checkaccess,link,&cls))
						return false; /* exception */
					if (!cls) {
						assert(mode == resolveLazy);
						return true; /* be lazy */
					}
					/* create the array class */
					cls = class_array_of(cls,false);
					if (!cls)
						return false; /* exception */
			}
		}
		else {
			/* the class has not been loaded, yet */
			if (mode == resolveLazy)
				return true; /* be lazy */
		}

#ifdef RESOLVE_VERBOSE
		fprintf(stderr,"    loading...\n");
#endif

		/* load the class */
		if (!cls) {
			if (!(cls = load_class_from_classloader(classname,
													referer->classloader)))
				return false; /* exception */
		}
	}

	/* the class is now loaded */
	assert(cls);
	assert(cls->state & CLASS_LOADED);

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    checking access rights...\n");
#endif
	
	/* check access rights of referer to refered class */
	if (checkaccess && !access_is_accessible_class(referer,cls)) {
		int msglen;
		char *message;

		msglen = utf_strlen(cls->name) + utf_strlen(referer->name) + 100;
		message = MNEW(char,msglen);
		strcpy(message,"class is not accessible (");
		utf_sprint_classname(message+strlen(message),cls->name);
		strcat(message," from ");
		utf_sprint_classname(message+strlen(message),referer->name);
		strcat(message,")");
		*exceptionptr = new_exception_message(string_java_lang_IllegalAccessException,message);
		MFREE(message,char,msglen);
		return false; /* exception */
	}

	/* link the class if necessary */
	if (link) {
		if (!(cls->state & CLASS_LINKED))
			if (!link_class(cls))
				return false; /* exception */

		assert(cls->state & CLASS_LINKED);
	}

	/* resolution succeeds */
#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    success.\n");
#endif
	*result = cls;
	return true;
}

/* resolve_classref ************************************************************
 
   Resolve a symbolic class reference
  
   IN:
       refmethod........the method from which resolution was triggered
                        (may be NULL if not applicable)
       ref..............class reference
       mode.............mode of resolution:
                            resolveLazy...only resolve if it does not
                                          require loading classes
                            resolveEager..load classes if necessary
	   checkaccess......if true, access rights to the class are checked
	   link.............if true, guarantee that the returned class, if any,
	                    has been linked
  
   OUT:
       *result..........set to result of resolution, or to NULL if
                        the reference has not been resolved
                        In the case of an exception, *result is
                        guaranteed to be set to NULL.
  
   RETURN VALUE:
       true.............everything ok 
                        (*result may still be NULL for resolveLazy)
       false............an exception has been thrown
   
*******************************************************************************/

bool resolve_classref(methodinfo *refmethod,
					  constant_classref *ref,
					  resolve_mode_t mode,
					  bool checkaccess,
					  bool link,
					  classinfo **result)
{
	return resolve_classref_or_classinfo(refmethod,CLASSREF_OR_CLASSINFO(ref),mode,checkaccess,link,result);
}

/* resolve_classref_or_classinfo ***********************************************
 
   Resolve a symbolic class reference if necessary
  
   IN:
       refmethod........the method from which resolution was triggered
                        (may be NULL if not applicable)
       cls..............class reference or classinfo
       mode.............mode of resolution:
                            resolveLazy...only resolve if it does not
                                          require loading classes
                            resolveEager..load classes if necessary
	   checkaccess......if true, access rights to the class are checked
	   link.............if true, guarantee that the returned class, if any,
	                    has been linked
  
   OUT:
       *result..........set to result of resolution, or to NULL if
                        the reference has not been resolved
                        In the case of an exception, *result is
                        guaranteed to be set to NULL.
  
   RETURN VALUE:
       true.............everything ok 
                        (*result may still be NULL for resolveLazy)
       false............an exception has been thrown
   
*******************************************************************************/

bool resolve_classref_or_classinfo(methodinfo *refmethod,
								   classref_or_classinfo cls,
								   resolve_mode_t mode,
								   bool checkaccess,
								   bool link,
								   classinfo **result)
{
	classinfo         *c;
	
	assert(cls.any);
	assert(mode == resolveEager || mode == resolveLazy);
	assert(result);

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"resolve_classref_or_classinfo(");
	utf_fprint(stderr,(IS_CLASSREF(cls)) ? cls.ref->name : cls.cls->name);
	fprintf(stderr,",%i,%i,%i)\n",mode,(int)checkaccess,(int)link);
#endif

	*result = NULL;

	if (IS_CLASSREF(cls)) {
		/* we must resolve this reference */

		if (!resolve_class_from_name(cls.ref->referer, refmethod, cls.ref->name,
									 mode, checkaccess, link, &c))
			goto return_exception;

	} else {
		/* cls has already been resolved */
		c = cls.cls;
		assert(c->state & CLASS_LOADED);
	}
	assert(c || (mode == resolveLazy));

	if (!c)
		return true; /* be lazy */
	
	assert(c);
	assert(c->state & CLASS_LOADED);

	if (link) {
		if (!(c->state & CLASS_LINKED))
			if (!link_class(c))
				goto return_exception;

		assert(c->state & CLASS_LINKED);
	}

	/* succeeded */
	*result = c;
	return true;

 return_exception:
	*result = NULL;
	return false;
}


/* resolve_class_from_typedesc *************************************************
 
   Return a classinfo * for the given type descriptor
  
   IN:
       d................type descriptor
	   checkaccess......if true, access rights to the class are checked
	   link.............if true, guarantee that the returned class, if any,
	                    has been linked
   OUT:
       *result..........set to result of resolution, or to NULL if
                        the reference has not been resolved
                        In the case of an exception, *result is
                        guaranteed to be set to NULL.
  
   RETURN VALUE:
       true.............everything ok 
       false............an exception has been thrown

   NOTE:
       This function always resolved eagerly.
   
*******************************************************************************/

bool resolve_class_from_typedesc(typedesc *d, bool checkaccess, bool link, classinfo **result)
{
	classinfo *cls;
	
	assert(d);
	assert(result);

	*result = NULL;

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"resolve_class_from_typedesc(");
	descriptor_debug_print_typedesc(stderr,d);
	fprintf(stderr,",%i,%i)\n",(int)checkaccess,(int)link);
#endif

	if (d->classref) {
		/* a reference type */
		if (!resolve_classref_or_classinfo(NULL,CLASSREF_OR_CLASSINFO(d->classref),
										   resolveEager,checkaccess,link,&cls))
			return false; /* exception */
	}
	else {
		/* a primitive type */
		cls = primitivetype_table[d->decltype].class_primitive;
		assert(cls->state & CLASS_LOADED);
		if (!(cls->state & CLASS_LINKED))
			if (!link_class(cls))
				return false; /* exception */
	}
	assert(cls);
	assert(cls->state & CLASS_LOADED);
	assert(!link || (cls->state & CLASS_LINKED));

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    result = ");utf_fprint(stderr,cls->name);fprintf(stderr,"\n");
#endif

	*result = cls;
	return true;
}

/******************************************************************************/
/* SUBTYPE SET CHECKS                                                         */
/******************************************************************************/

#ifdef ENABLE_VERIFIER

/* resolve_and_check_subtype_set ***********************************************
 
   Resolve the references in the given set and test subtype relationships
  
   IN:
       referer..........the class containing the references
       refmethod........the method triggering the resolution
       ref..............a set of class/interface references
                        (may be empty)
       type.............the type to test against the set
       reversed.........if true, test if type is a subtype of
                        the set members, instead of the other
                        way round
       mode.............mode of resolution:
                            resolveLazy...only resolve if it does not
                                          require loading classes
                            resolveEager..load classes if necessary
       error............which type of exception to throw if
                        the test fails. May be:
                            resolveLinkageError, or
                            resolveIllegalAccessError
						IMPORTANT: If error==resolveIllegalAccessError,
						then array types in the set are skipped.

   OUT:
       *checked.........set to true if all checks were performed,
	                    otherwise set to false
	                    (This is guaranteed to be true if mode was
						resolveEager and no exception occured.)
						If checked == NULL, this parameter is not used.
  
   RETURN VALUE:
       true.............the check succeeded
       false............the check failed. An exception has been
                        thrown.
   
   NOTE:
       The references in the set are resolved first, so any
       exception which may occurr during resolution may
       be thrown by this function.
   
*******************************************************************************/

bool resolve_and_check_subtype_set(classinfo *referer,methodinfo *refmethod,
								   unresolved_subtype_set *ref,
								   classref_or_classinfo typeref,
								   bool reversed,
								   resolve_mode_t mode,
								   resolve_err_t error,
								   bool *checked)
{
	classref_or_classinfo *setp;
	classinfo *result;
	classinfo *type;
	typeinfo resultti;
	typeinfo typeti;
	char *message;
	int msglen;
	typecheck_result r;

	assert(referer);
	assert(ref);
	assert(typeref.any);
	assert(mode == resolveLazy || mode == resolveEager);
	assert(error == resolveLinkageError || error == resolveIllegalAccessError);

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"resolve_and_check_subtype_set\n");
	unresolved_subtype_set_debug_dump(ref,stderr);
	if (IS_CLASSREF(typeref)) {
		fprintf(stderr,"    ref: ");utf_fprint(stderr,typeref.ref->name);
	}
	else {
		fprintf(stderr,"    cls: ");utf_fprint(stderr,typeref.cls->name);
	}
	fprintf(stderr,"\n");
#endif

	setp = ref->subtyperefs;

	/* an empty set of tests always succeeds */
	if (!setp || !setp->any) {
		if (checked)
			*checked = true;
		return true;
	}

	if (checked)
		*checked = false;

	/* first resolve the type if necessary */
	if (!resolve_classref_or_classinfo(refmethod,typeref,mode,false,true,&type))
		return false; /* exception */
	if (!type)
		return true; /* be lazy */

	assert(type);
	assert(type->state & CLASS_LOADED);
	assert(type->state & CLASS_LINKED);
	typeinfo_init_classinfo(&typeti,type);

	for (; setp->any; ++setp) {
		/* first resolve the set member if necessary */
		if (!resolve_classref_or_classinfo(refmethod,*setp,mode,false,true,&result)) {
			/* the type could not be resolved. therefore we are sure that  */
			/* no instances of this type will ever exist -> skip this test */
			/* XXX this assumes that class loading has invariant results (as in JVM spec) */
			*exceptionptr = NULL;
			continue;
		}
		if (!result)
			return true; /* be lazy */

		assert(result);
		assert(result->state & CLASS_LOADED);
		assert(result->state & CLASS_LINKED);


		/* do not check access to protected members of arrays */
		if (error == resolveIllegalAccessError && result->name->text[0] == '[') {
			continue;
		}

#ifdef RESOLVE_VERBOSE
		fprintf(stderr,"performing subclass test:\n");
		fprintf(stderr,"    ");utf_fprint(stderr,result->name);fputc('\n',stderr);
		fprintf(stderr,"  must be a %s of\n",(reversed) ? "superclass" : "subclass");
		fprintf(stderr,"    ");utf_fprint(stderr,type->name);fputc('\n',stderr);
#endif

		/* now check the subtype relationship */
		typeinfo_init_classinfo(&resultti,result);
		if (reversed) {
			/* we must test against `true` because `MAYBE` is also != 0 */
			r = typeinfo_is_assignable_to_class(&typeti,CLASSREF_OR_CLASSINFO(result));
			if (r == typecheck_FAIL)
				return false;
			if (r != typecheck_TRUE) {
#ifdef RESOLVE_VERBOSE
				fprintf(stderr,"reversed subclass test failed\n");
#endif
				goto throw_error;
			}
		}
		else {
			/* we must test against `true` because `MAYBE` is also != 0 */
			r = typeinfo_is_assignable_to_class(&resultti,CLASSREF_OR_CLASSINFO(type));
			if (r == typecheck_FAIL)
				return false;
			if (r != typecheck_TRUE) {
#ifdef RESOLVE_VERBOSE
				fprintf(stderr,"subclass test failed\n");
#endif
				goto throw_error;
			}
		}
	}
	
	/* check succeeds */
	if (checked)
		*checked = true;
	return true;

throw_error:
	msglen = utf_strlen(result->name) + utf_strlen(type->name) + 200;
	message = MNEW(char,msglen);
	strcpy(message,(error == resolveIllegalAccessError) ?
			"illegal access to protected member ("
			: "subtype constraint violated (");
	utf_sprint_classname(message+strlen(message),result->name);
	strcat(message," is not a subclass of ");
	utf_sprint_classname(message+strlen(message),type->name);
	strcat(message,")");
	if (error == resolveIllegalAccessError)
		*exceptionptr = new_exception_message(string_java_lang_IllegalAccessException,message);
	else
		*exceptionptr = exceptions_new_linkageerror(message,NULL);
	MFREE(message,char,msglen);
	return false; /* exception */
}

#endif /* ENABLE_VERIFIER */

/******************************************************************************/
/* CLASS RESOLUTION                                                           */
/******************************************************************************/

/* resolve_class ***************************************************************
 
   Resolve an unresolved class reference. The class is also linked.
  
   IN:
       ref..............struct containing the reference
       mode.............mode of resolution:
                            resolveLazy...only resolve if it does not
                                          require loading classes
                            resolveEager..load classes if necessary
	   checkaccess......if true, access rights to the class are checked
   
   OUT:
       *result..........set to the result of resolution, or to NULL if
                        the reference has not been resolved
                        In the case of an exception, *result is
                        guaranteed to be set to NULL.
  
   RETURN VALUE:
       true.............everything ok 
                        (*result may still be NULL for resolveLazy)
       false............an exception has been thrown
   
*******************************************************************************/

#ifdef ENABLE_VERIFIER
bool resolve_class(unresolved_class *ref,
				   resolve_mode_t mode,
				   bool checkaccess,
				   classinfo **result)
{
	classinfo *cls;
	bool checked;
	
	assert(ref);
	assert(result);
	assert(mode == resolveLazy || mode == resolveEager);

	*result = NULL;

#ifdef RESOLVE_VERBOSE
	unresolved_class_debug_dump(ref,stderr);
#endif

	/* first we must resolve the class */
	if (!resolve_classref(ref->referermethod,
					      ref->classref,mode,checkaccess,true,&cls))
	{
		/* the class reference could not be resolved */
		return false; /* exception */
	}
	if (!cls)
		return true; /* be lazy */

	assert(cls);
	assert((cls->state & CLASS_LOADED) && (cls->state & CLASS_LINKED));

	/* now we check the subtype constraints */
	if (!resolve_and_check_subtype_set(ref->classref->referer,ref->referermethod,
									   &(ref->subtypeconstraints),
									   CLASSREF_OR_CLASSINFO(cls),
									   false,
									   mode,
									   resolveLinkageError,&checked))
	{
		return false; /* exception */
	}
	if (!checked)
		return true; /* be lazy */

	/* succeed */
	*result = cls;
	return true;
}
#endif /* ENABLE_VERIFIER */

/* resolve_classref_eager ******************************************************
 
   Resolve an unresolved class reference eagerly. The class is also linked and
   access rights to the class are checked.
  
   IN:
       ref..............constant_classref to the class
   
   RETURN VALUE:
       classinfo * to the class, or
	   NULL if an exception has been thrown
   
*******************************************************************************/

classinfo * resolve_classref_eager(constant_classref *ref)
{
	classinfo *c;

	if (!resolve_classref(NULL,ref,resolveEager,true,true,&c))
		return NULL;

	return c;
}

/* resolve_classref_eager_nonabstract ******************************************
 
   Resolve an unresolved class reference eagerly. The class is also linked and
   access rights to the class are checked. A check is performed that the class
   is not abstract.
  
   IN:
       ref..............constant_classref to the class
   
   RETURN VALUE:
       classinfo * to the class, or
	   NULL if an exception has been thrown
   
*******************************************************************************/

classinfo * resolve_classref_eager_nonabstract(constant_classref *ref)
{
	classinfo *c;

	if (!resolve_classref(NULL,ref,resolveEager,true,true,&c))
		return NULL;

	/* ensure that the class is not abstract */

	if (c->flags & ACC_ABSTRACT) {
		*exceptionptr = new_verifyerror(NULL,"creating instance of abstract class");
		return NULL;
	}

	return c;
}

/* resolve_class_eager *********************************************************
 
   Resolve an unresolved class reference eagerly. The class is also linked and
   access rights to the class are checked.
  
   IN:
       ref..............struct containing the reference
   
   RETURN VALUE:
       classinfo * to the class, or
	   NULL if an exception has been thrown
   
*******************************************************************************/

#ifdef ENABLE_VERIFIER
classinfo * resolve_class_eager(unresolved_class *ref)
{
	classinfo *c;

	if (!resolve_class(ref,resolveEager,true,&c))
		return NULL;

	return c;
}
#endif /* ENABLE_VERIFIER */

/******************************************************************************/
/* FIELD RESOLUTION                                                           */
/******************************************************************************/

/* resolve_field ***************************************************************
 
   Resolve an unresolved field reference
  
   IN:
       ref..............struct containing the reference
       mode.............mode of resolution:
                            resolveLazy...only resolve if it does not
                                          require loading classes
                            resolveEager..load classes if necessary
  
   OUT:
       *result..........set to the result of resolution, or to NULL if
                        the reference has not been resolved
                        In the case of an exception, *result is
                        guaranteed to be set to NULL.
  
   RETURN VALUE:
       true.............everything ok 
                        (*result may still be NULL for resolveLazy)
       false............an exception has been thrown
   
*******************************************************************************/

bool resolve_field(unresolved_field *ref,
				   resolve_mode_t mode,
				   fieldinfo **result)
{
	classinfo *referer;
	classinfo *container;
	classinfo *declarer;
	constant_classref *fieldtyperef;
	fieldinfo *fi;
	bool checked;
	
	assert(ref);
	assert(result);
	assert(mode == resolveLazy || mode == resolveEager);

	*result = NULL;

#ifdef RESOLVE_VERBOSE
	unresolved_field_debug_dump(ref,stderr);
#endif

	/* the class containing the reference */

	referer = ref->fieldref->classref->referer;
	assert(referer);

	/* first we must resolve the class containg the field */
	if (!resolve_class_from_name(referer,ref->referermethod,
					   ref->fieldref->classref->name,mode,true,true,&container))
	{
		/* the class reference could not be resolved */
		return false; /* exception */
	}
	if (!container)
		return true; /* be lazy */

	assert(container);
	assert(container->state & CLASS_LOADED);
	assert(container->state & CLASS_LINKED);

	/* now we must find the declaration of the field in `container`
	 * or one of its superclasses */

#ifdef RESOLVE_VERBOSE
		fprintf(stderr,"    resolving field in class...\n");
#endif

	fi = class_resolvefield(container,
							ref->fieldref->name,ref->fieldref->descriptor,
							referer,true);
	if (!fi) {
		if (mode == resolveLazy) {
			/* The field does not exist. But since we were called lazily, */
			/* this error must not be reported now. (It will be reported   */
			/* if eager resolving of this field is ever tried.)           */

			*exceptionptr = NULL;
			return true; /* be lazy */
		}
		
		return false; /* exception */
	}

#ifdef ENABLE_VERIFIER

	/* { the field reference has been resolved } */
	declarer = fi->class;
	assert(declarer);
	assert(declarer->state & CLASS_LOADED);
	assert(declarer->state & CLASS_LINKED);

#ifdef RESOLVE_VERBOSE
		fprintf(stderr,"    checking static...\n");
#endif

	/* check static */

	if (((fi->flags & ACC_STATIC) != 0) != ((ref->flags & RESOLVE_STATIC) != 0)) {
		/* a static field is accessed via an instance, or vice versa */
		*exceptionptr = new_exception_message(string_java_lang_IncompatibleClassChangeError,
				(fi->flags & ACC_STATIC) ? "static field accessed via instance"
				                         : "instance field accessed without instance");
		return false; /* exception */
	}

	/* for non-static accesses we have to check the constraints on the */
	/* instance type */

	if (!(ref->flags & RESOLVE_STATIC)) {
#ifdef RESOLVE_VERBOSE
		fprintf(stderr,"    checking instance types...\n");
#endif

		if (!resolve_and_check_subtype_set(referer,ref->referermethod,
										   &(ref->instancetypes),
										   CLASSREF_OR_CLASSINFO(container),
										   false, mode, resolveLinkageError,
										   &checked))
		{
			return false; /* exception */
		}

		if (!checked)
			return true; /* be lazy */
	}

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    checking instance types...done\n");
#endif

	fieldtyperef = ref->fieldref->parseddesc.fd->classref;

	/* for PUT* instructions we have to check the constraints on the value type */
	if (((ref->flags & RESOLVE_PUTFIELD) != 0) && fi->type == TYPE_ADR) {
#ifdef RESOLVE_VERBOSE
		fprintf(stderr,"    checking value constraints...\n");
#endif
		assert(fieldtyperef);
		if (!SUBTYPESET_IS_EMPTY(ref->valueconstraints)) {
			/* check subtype constraints */
			if (!resolve_and_check_subtype_set(referer, ref->referermethod,
											   &(ref->valueconstraints),
											   CLASSREF_OR_CLASSINFO(fieldtyperef),
											   false, mode, resolveLinkageError,
											   &checked))
			{
				return false; /* exception */
			}
			if (!checked)
				return true; /* be lazy */
		}
	}
									   
	/* check access rights */
#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    checking access rights...\n");
#endif
	if (!access_is_accessible_member(referer,declarer,fi->flags)) {
		int msglen;
		char *message;

		msglen = utf_strlen(declarer->name) + utf_strlen(fi->name) + utf_strlen(referer->name) + 100;
		message = MNEW(char,msglen);
		strcpy(message,"field is not accessible (");
		utf_sprint_classname(message+strlen(message),declarer->name);
		strcat(message,".");
		utf_sprint(message+strlen(message),fi->name);
		strcat(message," from ");
		utf_sprint_classname(message+strlen(message),referer->name);
		strcat(message,")");
		*exceptionptr = new_exception_message(string_java_lang_IllegalAccessException,message);
		MFREE(message,char,msglen);
		return false; /* exception */
	}
#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    checking access rights...done\n");
	fprintf(stderr,"        declarer = ");
	utf_fprint_classname(stderr,declarer->name); fputc('\n',stderr);
	fprintf(stderr,"        referer = ");
	utf_fprint_classname(stderr,referer->name); fputc('\n',stderr);
#endif

	/* check protected access */
	if (((fi->flags & ACC_PROTECTED) != 0) && !SAME_PACKAGE(declarer,referer)) {
#ifdef RESOLVE_VERBOSE
		fprintf(stderr,"    checking protected access...\n");
#endif
		if (!resolve_and_check_subtype_set(referer,ref->referermethod,
										   &(ref->instancetypes),
										   CLASSREF_OR_CLASSINFO(referer),
										   false, mode,
										   resolveIllegalAccessError, &checked))
		{
			return false; /* exception */
		}

		if (!checked)
			return true; /* be lazy */
	}

	/* impose loading constraint on field type */

	if (fi->type == TYPE_ADR) {
#ifdef RESOLVE_VERBOSE
		fprintf(stderr,"    adding constraint...\n");
#endif
		assert(fieldtyperef);
		if (!classcache_add_constraint(declarer->classloader,
									   referer->classloader,
									   fieldtyperef->name))
			return false;
	}
#endif /* ENABLE_VERIFIER */

	/* succeed */
#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    success.\n");
#endif
	*result = fi;

	return true;
}

/* resolve_field_eager *********************************************************
 
   Resolve an unresolved field reference eagerly.
  
   IN:
       ref..............struct containing the reference
   
   RETURN VALUE:
       fieldinfo * to the field, or
	   NULL if an exception has been thrown
   
*******************************************************************************/

fieldinfo * resolve_field_eager(unresolved_field *ref)
{
	fieldinfo *fi;

	if (!resolve_field(ref,resolveEager,&fi))
		return NULL;

	return fi;
}

/******************************************************************************/
/* METHOD RESOLUTION                                                          */
/******************************************************************************/

/* resolve_method **************************************************************
 
   Resolve an unresolved method reference
  
   IN:
       ref..............struct containing the reference
       mode.............mode of resolution:
                            resolveLazy...only resolve if it does not
                                          require loading classes
                            resolveEager..load classes if necessary
  
   OUT:
       *result..........set to the result of resolution, or to NULL if
                        the reference has not been resolved
                        In the case of an exception, *result is
                        guaranteed to be set to NULL.
  
   RETURN VALUE:
       true.............everything ok 
                        (*result may still be NULL for resolveLazy)
       false............an exception has been thrown
   
*******************************************************************************/

bool resolve_method(unresolved_method *ref, resolve_mode_t mode, methodinfo **result)
{
	classinfo *referer;
	classinfo *container;
	classinfo *declarer;
	methodinfo *mi;
	typedesc *paramtypes;
	int instancecount;
	int i;
	bool checked;
	
	assert(ref);
	assert(result);
	assert(mode == resolveLazy || mode == resolveEager);

#ifdef RESOLVE_VERBOSE
	unresolved_method_debug_dump(ref,stderr);
#endif

	*result = NULL;

	/* the class containing the reference */
	referer = ref->methodref->classref->referer;
	assert(referer);

	/* first we must resolve the class containg the method */
	if (!resolve_class_from_name(referer,ref->referermethod,
					   ref->methodref->classref->name,mode,true,true,&container))
	{
		/* the class reference could not be resolved */
		return false; /* exception */
	}
	if (!container)
		return true; /* be lazy */

	assert(container);
	assert(container->state & CLASS_LINKED);

	/* now we must find the declaration of the method in `container`
	 * or one of its superclasses */

	if (container->flags & ACC_INTERFACE) {
		mi = class_resolveinterfacemethod(container,
									      ref->methodref->name,
										  ref->methodref->descriptor,
									      referer, true);

	} else {
		mi = class_resolveclassmethod(container,
									  ref->methodref->name,
									  ref->methodref->descriptor,
									  referer, true);
	}

	if (!mi) {
		if (mode == resolveLazy) {
			/* The method does not exist. But since we were called lazily, */
			/* this error must not be reported now. (It will be reported   */
			/* if eager resolving of this method is ever tried.)           */

			*exceptionptr = NULL;
			return true; /* be lazy */
		}
		
		return false; /* exception */ /* XXX set exceptionptr? */
	}

#ifdef ENABLE_VERIFIER

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    flags: %02x\n",mi->flags);
#endif
	/* { the method reference has been resolved } */

	declarer = mi->class;
	assert(declarer);
	assert(referer->state & CLASS_LINKED);

	/* checks for INVOKESPECIAL:                                       */
	/* for <init> and methods of the current class we don't need any   */
	/* special checks. Otherwise we must verify that the called method */
	/* belongs to a super class of the current class                   */
	if (((ref->flags & RESOLVE_SPECIAL) != 0) 
		&& referer != declarer 
		&& mi->name != utf_init) 
	{
		/* check that declarer is a super class of the current class   */
		if (!class_issubclass(referer,declarer)) {
			*exceptionptr = new_verifyerror(ref->referermethod,
					"INVOKESPECIAL calling non-super class method");
			return false;
		}

		/* if the referer has ACC_SUPER set, we must do the special    */
		/* lookup starting with the direct super class of referer      */
		if ((referer->flags & ACC_SUPER) != 0) {
			mi = class_resolvemethod(referer->super.cls,
									 ref->methodref->name,
									 ref->methodref->descriptor);
			if (!mi) {
				/* the spec calls for an AbstractMethodError in this case */
				*exceptionptr = new_exception(string_java_lang_AbstractMethodError);
				return false;
			}
			declarer = mi->class;
		}
	}

	/* check static */

	if (((mi->flags & ACC_STATIC) != 0) != ((ref->flags & RESOLVE_STATIC) != 0)) {
		/* a static method is accessed via an instance, or vice versa */
		*exceptionptr =
			new_exception_message(string_java_lang_IncompatibleClassChangeError,
				(mi->flags & ACC_STATIC) ? "static method called via instance"
				                         : "instance method called without instance");
		return false;
	}

	/* have the method params already been parsed? no, do it. */

	if (!mi->parseddesc->params)
		if (!descriptor_params_from_paramtypes(mi->parseddesc, mi->flags))
			return false;
		
	/* for non-static methods we have to check the constraints on the         */
	/* instance type                                                          */

	if (!(ref->flags & RESOLVE_STATIC)) {
		if (!resolve_and_check_subtype_set(referer,ref->referermethod,
										   &(ref->instancetypes),
										   CLASSREF_OR_CLASSINFO(container),
										   false,
										   mode,
										   resolveLinkageError,&checked))
		{
			return false; /* exception */
		}
		if (!checked)
			return true; /* be lazy */
		instancecount = 1;
	}
	else {
		instancecount = 0;
	}

	/* check subtype constraints for TYPE_ADR parameters */

	assert(mi->parseddesc->paramcount == ref->methodref->parseddesc.md->paramcount);
	paramtypes = mi->parseddesc->paramtypes;
	
	for (i = 0; i < mi->parseddesc->paramcount-instancecount; i++) {
		if (paramtypes[i+instancecount].type == TYPE_ADR) {
			if (ref->paramconstraints) {
				if (!resolve_and_check_subtype_set(referer,ref->referermethod,
							ref->paramconstraints + i,
							CLASSREF_OR_CLASSINFO(paramtypes[i+instancecount].classref),
							false,
							mode,
							resolveLinkageError,&checked))
				{
					return false; /* exception */
				}
				if (!checked)
					return true; /* be lazy */
			}
		}
	}

	/* check access rights */

	if (!access_is_accessible_member(referer,declarer,mi->flags)) {
		int msglen;
		char *message;

		msglen = utf_strlen(declarer->name) + utf_strlen(mi->name) + 
			utf_strlen(mi->descriptor) + utf_strlen(referer->name) + 100;
		message = MNEW(char,msglen);
		strcpy(message,"method is not accessible (");
		utf_sprint_classname(message+strlen(message),declarer->name);
		strcat(message,".");
		utf_sprint(message+strlen(message),mi->name);
		utf_sprint(message+strlen(message),mi->descriptor);
		strcat(message," from ");
		utf_sprint_classname(message+strlen(message),referer->name);
		strcat(message,")");
		*exceptionptr = new_exception_message(string_java_lang_IllegalAccessException,message);
		MFREE(message,char,msglen);
		return false; /* exception */
	}

	/* check protected access */

	if (((mi->flags & ACC_PROTECTED) != 0) && !SAME_PACKAGE(declarer,referer))
	{
		if (!resolve_and_check_subtype_set(referer,ref->referermethod,
										   &(ref->instancetypes),
										   CLASSREF_OR_CLASSINFO(referer),
										   false,
										   mode,
										   resolveIllegalAccessError,&checked))
		{
			return false; /* exception */
		}
		if (!checked)
			return true; /* be lazy */
	}

	/* impose loading constraints on parameters (including instance) */

	paramtypes = mi->parseddesc->paramtypes;

	for (i = 0; i < mi->parseddesc->paramcount; i++) {
		if (i < instancecount || paramtypes[i].type == TYPE_ADR) {
			utf *name;
			
			if (i < instancecount) {
				/* The type of the 'this' pointer is the class containing */
				/* the method definition. Since container is the same as, */
				/* or a subclass of declarer, we also constrain declarer  */
				/* by transitivity of loading constraints.                */
				name = container->name;
			}
			else {
				name = paramtypes[i].classref->name;
			}
			
			/* The caller (referer) and the callee (container) must agree */
			/* on the types of the parameters.                            */
			if (!classcache_add_constraint(referer->classloader,
										   container->classloader, name))
				return false; /* exception */
		}
	}

	/* impose loading constraint onto return type */

	if (ref->methodref->parseddesc.md->returntype.type == TYPE_ADR) {
		/* The caller (referer) and the callee (container) must agree */
		/* on the return type.                                        */
		if (!classcache_add_constraint(referer->classloader,container->classloader,
				ref->methodref->parseddesc.md->returntype.classref->name))
			return false; /* exception */
	}

#endif /* ENABLE_VERIFIER */

	/* succeed */
	*result = mi;
	return true;
}

/* resolve_method_eager ********************************************************
 
   Resolve an unresolved method reference eagerly.
  
   IN:
       ref..............struct containing the reference
   
   RETURN VALUE:
       methodinfo * to the method, or
	   NULL if an exception has been thrown
   
*******************************************************************************/

methodinfo * resolve_method_eager(unresolved_method *ref)
{
	methodinfo *mi;

	if (!resolve_method(ref,resolveEager,&mi))
		return NULL;

	return mi;
}

/******************************************************************************/
/* CREATING THE DATA STRUCTURES                                               */
/******************************************************************************/

#ifdef ENABLE_VERIFIER
static bool unresolved_subtype_set_from_typeinfo(classinfo *referer,
												 methodinfo *refmethod,
												 unresolved_subtype_set *stset,
												 typeinfo *tinfo,
												 constant_classref *declaredtype)
{
	int count;
	int i;
	
	assert(stset);
	assert(tinfo);

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"unresolved_subtype_set_from_typeinfo\n");
#ifdef TYPEINFO_DEBUG
	typeinfo_print(stderr,tinfo,4);
#endif
	fprintf(stderr,"    declared type:");utf_fprint(stderr,declaredtype->name);
	fprintf(stderr,"\n");
#endif

	if (TYPEINFO_IS_PRIMITIVE(*tinfo)) {
		*exceptionptr = new_verifyerror(refmethod,
				"Invalid use of returnAddress");
		return false;
	}

	if (TYPEINFO_IS_NEWOBJECT(*tinfo)) {
		*exceptionptr = new_verifyerror(refmethod,
				"Invalid use of uninitialized object");
		return false;
	}

	/* the nulltype is always assignable (XXX for reversed?) */
	if (TYPEINFO_IS_NULLTYPE(*tinfo))
		goto empty_set;

	/* every type is assignable to (BOOTSTRAP)java.lang.Object */
	if (declaredtype->name == utf_java_lang_Object
			&& referer->classloader == NULL)
	{
		goto empty_set;
	}

	if (tinfo->merged) {
		count = tinfo->merged->count;
		stset->subtyperefs = MNEW(classref_or_classinfo,count + 1);
		for (i=0; i<count; ++i) {
			classref_or_classinfo c = tinfo->merged->list[i];
			if (tinfo->dimension > 0) {
				/* a merge of array types */
				/* the merged list contains the possible _element_ types, */
				/* so we have to create array types with these elements.  */
				if (IS_CLASSREF(c)) {
					c.ref = class_get_classref_multiarray_of(tinfo->dimension,c.ref);
				}
				else {
					c.cls = class_multiarray_of(tinfo->dimension,c.cls,false);
				}
			}
			stset->subtyperefs[i] = c;
		}
		stset->subtyperefs[count].any = NULL; /* terminate */
	}
	else {
		if ((IS_CLASSREF(tinfo->typeclass)
					? tinfo->typeclass.ref->name 
					: tinfo->typeclass.cls->name) == declaredtype->name)
		{
			/* the class names are the same */
		    /* equality is guaranteed by the loading constraints */
			goto empty_set;
		}
		else {
			stset->subtyperefs = MNEW(classref_or_classinfo,1 + 1);
			stset->subtyperefs[0] = tinfo->typeclass;
			stset->subtyperefs[1].any = NULL; /* terminate */
		}
	}

	return true;

empty_set:
	UNRESOLVED_SUBTYPE_SET_EMTPY(*stset);
	return true;
}
#endif /* ENABLE_VERIFIER */

/* create_unresolved_class *****************************************************
 
   Create an unresolved_class struct for the given class reference
  
   IN:
	   refmethod........the method triggering the resolution (if any)
	   classref.........the class reference
	   valuetype........value type to check against the resolved class
	   					may be NULL, if no typeinfo is available

   RETURN VALUE:
       a pointer to a new unresolved_class struct, or
	   NULL if an exception has been thrown

*******************************************************************************/

#ifdef ENABLE_VERIFIER
unresolved_class * create_unresolved_class(methodinfo *refmethod,
										   constant_classref *classref,
										   typeinfo *valuetype)
{
	unresolved_class *ref;
	
#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"create_unresolved_class\n");
	fprintf(stderr,"    referer: ");utf_fprint(stderr,classref->referer->name);fputc('\n',stderr);
	if (refmethod) {
		fprintf(stderr,"    rmethod: ");utf_fprint(stderr,refmethod->name);fputc('\n',stderr);
		fprintf(stderr,"    rmdesc : ");utf_fprint(stderr,refmethod->descriptor);fputc('\n',stderr);
	}
	fprintf(stderr,"    name   : ");utf_fprint(stderr,classref->name);fputc('\n',stderr);
#endif

	ref = NEW(unresolved_class);
	ref->classref = classref;
	ref->referermethod = refmethod;

	if (valuetype) {
		if (!unresolved_subtype_set_from_typeinfo(classref->referer,refmethod,
					&(ref->subtypeconstraints),valuetype,classref))
			return NULL;
	}
	else {
		UNRESOLVED_SUBTYPE_SET_EMTPY(ref->subtypeconstraints);
	}

	return ref;
}
#endif /* ENABLE_VERIFIER */

/* create_unresolved_field *****************************************************
 
   Create an unresolved_field struct for the given field access instruction
  
   IN:
       referer..........the class containing the reference
	   refmethod........the method triggering the resolution (if any)
	   iptr.............the {GET,PUT}{FIELD,STATIC}{,CONST} instruction

   RETURN VALUE:
       a pointer to a new unresolved_field struct, or
	   NULL if an exception has been thrown

*******************************************************************************/

unresolved_field * create_unresolved_field(classinfo *referer, methodinfo *refmethod,
										   instruction *iptr)
{
	unresolved_field *ref;
	constant_FMIref *fieldref = NULL;

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"create_unresolved_field\n");
	fprintf(stderr,"    referer: ");utf_fprint(stderr,referer->name);fputc('\n',stderr);
	fprintf(stderr,"    rmethod: ");utf_fprint(stderr,refmethod->name);fputc('\n',stderr);
	fprintf(stderr,"    rmdesc : ");utf_fprint(stderr,refmethod->descriptor);fputc('\n',stderr);
#endif

	ref = NEW(unresolved_field);
	ref->flags = 0;
	ref->referermethod = refmethod;
	UNRESOLVED_SUBTYPE_SET_EMTPY(ref->valueconstraints);

	switch (iptr[0].opc) {
		case ICMD_PUTFIELD:
			ref->flags |= RESOLVE_PUTFIELD;
			fieldref = (constant_FMIref *) iptr[0].val.a;
			break;

		case ICMD_PUTFIELDCONST:
			ref->flags |= RESOLVE_PUTFIELD;
			fieldref = (constant_FMIref *) iptr[1].val.a;
			break;

		case ICMD_PUTSTATIC:
			ref->flags |= RESOLVE_PUTFIELD | RESOLVE_STATIC;
			fieldref = (constant_FMIref *) iptr[0].val.a;
			break;

		case ICMD_PUTSTATICCONST:
			ref->flags |= RESOLVE_PUTFIELD | RESOLVE_STATIC;
			fieldref = (constant_FMIref *) iptr[1].val.a;
			break;

		case ICMD_GETFIELD:
			fieldref = (constant_FMIref *) iptr[0].val.a;
			break;
			
		case ICMD_GETSTATIC:
			ref->flags |= RESOLVE_STATIC;
			fieldref = (constant_FMIref *) iptr[0].val.a;
			break;
	}
	
	assert(fieldref);

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    class  : ");utf_fprint(stderr,fieldref->classref->name);fputc('\n',stderr);
	fprintf(stderr,"    name   : ");utf_fprint(stderr,fieldref->name);fputc('\n',stderr);
	fprintf(stderr,"    desc   : ");utf_fprint(stderr,fieldref->descriptor);fputc('\n',stderr);
	fprintf(stderr,"    type   : ");descriptor_debug_print_typedesc(stderr,fieldref->parseddesc.fd);
	fputc('\n',stderr);
	/*fprintf(stderr,"    opcode : %d %s\n",iptr[0].opc,icmd_names[iptr[0].opc]);*/
#endif

	ref->fieldref = fieldref;

	return ref;
}

/* constrain_unresolved_field **************************************************
 
   Record subtype constraints for a field access.
  
   IN:
       ref..............the unresolved_field structure of the access
       referer..........the class containing the reference
	   refmethod........the method triggering the resolution (if any)
	   iptr.............the {GET,PUT}{FIELD,STATIC}{,CONST} instruction
	   stack............the input stack of the instruction

   RETURN VALUE:
       true.............everything ok
	   false............an exception has been thrown

*******************************************************************************/

#ifdef ENABLE_VERIFIER
bool constrain_unresolved_field(unresolved_field *ref,
							    classinfo *referer, methodinfo *refmethod,
							    instruction *iptr,
							    stackelement *stack)
{
	constant_FMIref *fieldref;
	stackelement *instanceslot = NULL;
	int type;
	typeinfo tinfo;
	typeinfo *tip = NULL;
	typedesc *fd;

	assert(ref);

	fieldref = ref->fieldref;
	assert(fieldref);

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"constrain_unresolved_field\n");
	fprintf(stderr,"    referer: ");utf_fprint(stderr,referer->name);fputc('\n',stderr);
	fprintf(stderr,"    rmethod: ");utf_fprint(stderr,refmethod->name);fputc('\n',stderr);
	fprintf(stderr,"    rmdesc : ");utf_fprint(stderr,refmethod->descriptor);fputc('\n',stderr);
	fprintf(stderr,"    class  : ");utf_fprint(stderr,fieldref->classref->name);fputc('\n',stderr);
	fprintf(stderr,"    name   : ");utf_fprint(stderr,fieldref->name);fputc('\n',stderr);
	fprintf(stderr,"    desc   : ");utf_fprint(stderr,fieldref->descriptor);fputc('\n',stderr);
	fprintf(stderr,"    type   : ");descriptor_debug_print_typedesc(stderr,fieldref->parseddesc.fd);
	fputc('\n',stderr);
	/*fprintf(stderr,"    opcode : %d %s\n",iptr[0].opc,icmd_names[iptr[0].opc]);*/
#endif

	switch (iptr[0].opc) {
		case ICMD_PUTFIELD:
			instanceslot = stack->prev;
			tip = &(stack->typeinfo);
			break;

		case ICMD_PUTFIELDCONST:
			instanceslot = stack;
			break;

		case ICMD_PUTSTATIC:
			tip = &(stack->typeinfo);
			break;

		case ICMD_GETFIELD:
			instanceslot = stack;
			break;
	}
	
	assert(instanceslot || ((ref->flags & RESOLVE_STATIC) != 0));
	fd = fieldref->parseddesc.fd;
	assert(fd);

	/* record subtype constraints for the instance type, if any */
	if (instanceslot) {
		typeinfo *insttip;

		/* The instanceslot must contain a reference to a non-array type */
		if (!TYPEINFO_IS_REFERENCE(instanceslot->typeinfo)) {
			*exceptionptr = new_verifyerror(refmethod, "illegal instruction: field access on non-reference");
			return false;
		}
		if (TYPEINFO_IS_ARRAY(instanceslot->typeinfo)) {
			*exceptionptr = new_verifyerror(refmethod, "illegal instruction: field access on array");
			return false;
		}
		
		if (((ref->flags & RESOLVE_PUTFIELD) != 0) && 
				TYPEINFO_IS_NEWOBJECT(instanceslot->typeinfo))
		{
			/* The instruction writes a field in an uninitialized object. */
			/* This is only allowed when a field of an uninitialized 'this' object is */
			/* written inside an initialization method                                */
			
			classinfo *initclass;
			instruction *ins = (instruction*)TYPEINFO_NEWOBJECT_INSTRUCTION(instanceslot->typeinfo);

			if (ins != NULL) {
				*exceptionptr = new_verifyerror(refmethod,"accessing field of uninitialized object");
				return false;
			}
			/* XXX check that class of field == refmethod->class */
			initclass = refmethod->class; /* XXX classrefs */
			assert(initclass->state & CLASS_LOADED);
			assert(initclass->state & CLASS_LINKED);

			typeinfo_init_classinfo(&tinfo,initclass);
			insttip = &tinfo;
		}
		else {
			insttip = &(instanceslot->typeinfo);
		}
		if (!unresolved_subtype_set_from_typeinfo(referer,refmethod,
					&(ref->instancetypes),insttip,fieldref->classref))
			return false;
	}
	else {
		UNRESOLVED_SUBTYPE_SET_EMTPY(ref->instancetypes);
	}
	
	/* record subtype constraints for the value type, if any */
	type = fd->type;
	if (type == TYPE_ADR && ((ref->flags & RESOLVE_PUTFIELD) != 0)) {
		if (!tip) {
			/* we have a PUTSTATICCONST or PUTFIELDCONST with TYPE_ADR */
			tip = &tinfo;
			if (INSTRUCTION_PUTCONST_VALUE_ADR(iptr)) {
				assert(class_java_lang_String);
				assert(class_java_lang_String->state & CLASS_LOADED);
				assert(class_java_lang_String->state & CLASS_LINKED);
				typeinfo_init_classinfo(&tinfo,class_java_lang_String);
			}
			else
				TYPEINFO_INIT_NULLTYPE(tinfo);
		}
		if (!unresolved_subtype_set_from_typeinfo(referer,refmethod,
					&(ref->valueconstraints),tip,fieldref->parseddesc.fd->classref))
			return false;
	}
	else {
		UNRESOLVED_SUBTYPE_SET_EMTPY(ref->valueconstraints);
	}

	return true;
}
#endif /* ENABLE_VERIFIER */

/* create_unresolved_method ****************************************************
 
   Create an unresolved_method struct for the given method invocation
  
   IN:
       referer..........the class containing the reference
	   refmethod........the method triggering the resolution (if any)
	   iptr.............the INVOKE* instruction

   RETURN VALUE:
       a pointer to a new unresolved_method struct, or
	   NULL if an exception has been thrown

*******************************************************************************/

unresolved_method * create_unresolved_method(classinfo *referer, methodinfo *refmethod,
											 instruction *iptr)
{
	unresolved_method *ref;
	constant_FMIref *methodref;
	bool staticmethod;

	methodref = (constant_FMIref *) iptr[0].val.a;
	assert(methodref);
	staticmethod = (iptr[0].opc == ICMD_INVOKESTATIC);

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"create_unresolved_method\n");
	fprintf(stderr,"    referer: ");utf_fprint(stderr,referer->name);fputc('\n',stderr);
	fprintf(stderr,"    rmethod: ");utf_fprint(stderr,refmethod->name);fputc('\n',stderr);
	fprintf(stderr,"    rmdesc : ");utf_fprint(stderr,refmethod->descriptor);fputc('\n',stderr);
	fprintf(stderr,"    class  : ");utf_fprint(stderr,methodref->classref->name);fputc('\n',stderr);
	fprintf(stderr,"    name   : ");utf_fprint(stderr,methodref->name);fputc('\n',stderr);
	fprintf(stderr,"    desc   : ");utf_fprint(stderr,methodref->descriptor);fputc('\n',stderr);
	/*fprintf(stderr,"    opcode : %d %s\n",iptr[0].opc,icmd_names[iptr[0].opc]);*/
#endif

	/* allocate params if necessary */
	if (!methodref->parseddesc.md->params)
		if (!descriptor_params_from_paramtypes(methodref->parseddesc.md,
					(staticmethod) ? ACC_STATIC : ACC_NONE))
			return NULL;

	/* create the data structure */
	ref = NEW(unresolved_method);
	ref->flags = ((staticmethod) ? RESOLVE_STATIC : 0)
			   | ((iptr[0].opc == ICMD_INVOKESPECIAL) ? RESOLVE_SPECIAL : 0);
	ref->referermethod = refmethod;
	ref->methodref = methodref;
	ref->paramconstraints = NULL;
	UNRESOLVED_SUBTYPE_SET_EMTPY(ref->instancetypes);

	return ref;
}

/* constrain_unresolved_method *************************************************
 
   Record subtype constraints for the arguments of a method call.
  
   IN:
       ref..............the unresolved_method structure of the call
       referer..........the class containing the reference
	   refmethod........the method triggering the resolution (if any)
	   iptr.............the INVOKE* instruction
	   stack............the input stack of the instruction

   RETURN VALUE:
       true.............everything ok
	   false............an exception has been thrown

*******************************************************************************/

#ifdef ENABLE_VERIFIER
bool constrain_unresolved_method(unresolved_method *ref,
								 classinfo *referer, methodinfo *refmethod,
								 instruction *iptr,
								 stackelement *stack)
{
	constant_FMIref *methodref;
	stackelement *instanceslot = NULL;
	stackelement *param;
	methoddesc *md;
	typeinfo tinfo;
	int i,j;
	int type;
	int instancecount;

	assert(ref);
	methodref = ref->methodref;
	assert(methodref);
	md = methodref->parseddesc.md;
	assert(md);
	assert(md->params != NULL);

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"constrain_unresolved_method\n");
	fprintf(stderr,"    referer: ");utf_fprint(stderr,referer->name);fputc('\n',stderr);
	fprintf(stderr,"    rmethod: ");utf_fprint(stderr,refmethod->name);fputc('\n',stderr);
	fprintf(stderr,"    rmdesc : ");utf_fprint(stderr,refmethod->descriptor);fputc('\n',stderr);
	fprintf(stderr,"    class  : ");utf_fprint(stderr,methodref->classref->name);fputc('\n',stderr);
	fprintf(stderr,"    name   : ");utf_fprint(stderr,methodref->name);fputc('\n',stderr);
	fprintf(stderr,"    desc   : ");utf_fprint(stderr,methodref->descriptor);fputc('\n',stderr);
	/*fprintf(stderr,"    opcode : %d %s\n",iptr[0].opc,icmd_names[iptr[0].opc]);*/
#endif

	if ((ref->flags & RESOLVE_STATIC) == 0) {
		/* find the instance slot under all the parameter slots on the stack */
		instanceslot = stack;
		for (i=1; i<md->paramcount; ++i)
			instanceslot = instanceslot->prev;
		instancecount = 1;
	}
	else {
		instancecount = 0;
	}
	
	assert((instanceslot && instancecount==1) || ((ref->flags & RESOLVE_STATIC) != 0));

	/* record subtype constraints for the instance type, if any */
	if (instanceslot) {
		typeinfo *tip;
		
		assert(instanceslot->type == TYPE_ADR);
		
		if (iptr[0].opc == ICMD_INVOKESPECIAL && 
				TYPEINFO_IS_NEWOBJECT(instanceslot->typeinfo))
		{   /* XXX clean up */
			instruction *ins = (instruction*)TYPEINFO_NEWOBJECT_INSTRUCTION(instanceslot->typeinfo);
			classref_or_classinfo initclass = (ins) ? CLASSREF_OR_CLASSINFO(ins[-1].target)
										 : CLASSREF_OR_CLASSINFO(refmethod->class);
			tip = &tinfo;
			if (!typeinfo_init_class(tip,initclass))
				return false;
		}
		else {
			tip = &(instanceslot->typeinfo);
		}
		if (!unresolved_subtype_set_from_typeinfo(referer,refmethod,
					&(ref->instancetypes),tip,methodref->classref))
			return false;
	}
	
	/* record subtype constraints for the parameter types, if any */
	param = stack;
	for (i=md->paramcount-1-instancecount; i>=0; --i, param=param->prev) {
		type = md->paramtypes[i+instancecount].type;

		assert(param);
		assert(type == param->type);

		if (type == TYPE_ADR) {
			if (!ref->paramconstraints) {
				ref->paramconstraints = MNEW(unresolved_subtype_set,md->paramcount);
				for (j=md->paramcount-1-instancecount; j>i; --j)
					UNRESOLVED_SUBTYPE_SET_EMTPY(ref->paramconstraints[j]);
			}
			assert(ref->paramconstraints);
			if (!unresolved_subtype_set_from_typeinfo(referer,refmethod,
						ref->paramconstraints + i,&(param->typeinfo),
						md->paramtypes[i+instancecount].classref))
				return false;
		}
		else {
			if (ref->paramconstraints)
				UNRESOLVED_SUBTYPE_SET_EMTPY(ref->paramconstraints[i]);
		}
	}

	return true;
}
#endif /* ENABLE_VERIFIER */

/******************************************************************************/
/* FREEING MEMORY                                                             */
/******************************************************************************/

#ifdef ENABLE_VERIFIER
inline static void unresolved_subtype_set_free_list(classref_or_classinfo *list)
{
	if (list) {
		classref_or_classinfo *p = list;

		/* this is silly. we *only* need to count the elements for MFREE */
		while ((p++)->any)
			;
		MFREE(list,classref_or_classinfo,(p - list));
	}
}
#endif /* ENABLE_VERIFIER */

/* unresolved_class_free *******************************************************
 
   Free the memory used by an unresolved_class
  
   IN:
       ref..............the unresolved_class

*******************************************************************************/

void unresolved_class_free(unresolved_class *ref)
{
	assert(ref);

#ifdef ENABLE_VERIFIER
	unresolved_subtype_set_free_list(ref->subtypeconstraints.subtyperefs);
#endif
	FREE(ref,unresolved_class);
}

/* unresolved_field_free *******************************************************
 
   Free the memory used by an unresolved_field
  
   IN:
       ref..............the unresolved_field

*******************************************************************************/

void unresolved_field_free(unresolved_field *ref)
{
	assert(ref);

#ifdef ENABLE_VERIFIER
	unresolved_subtype_set_free_list(ref->instancetypes.subtyperefs);
	unresolved_subtype_set_free_list(ref->valueconstraints.subtyperefs);
#endif
	FREE(ref,unresolved_field);
}

/* unresolved_method_free ******************************************************
 
   Free the memory used by an unresolved_method
  
   IN:
       ref..............the unresolved_method

*******************************************************************************/

void unresolved_method_free(unresolved_method *ref)
{
	assert(ref);

#ifdef ENABLE_VERIFIER
	unresolved_subtype_set_free_list(ref->instancetypes.subtyperefs);
	if (ref->paramconstraints) {
		int i;
		int count = ref->methodref->parseddesc.md->paramcount;

		for (i=0; i<count; ++i)
			unresolved_subtype_set_free_list(ref->paramconstraints[i].subtyperefs);
		MFREE(ref->paramconstraints,unresolved_subtype_set,count);
	}
#endif
	FREE(ref,unresolved_method);
}

#ifndef NDEBUG
/******************************************************************************/
/* DEBUG DUMPS                                                                */
/******************************************************************************/

/* unresolved_subtype_set_debug_dump *******************************************
 
   Print debug info for unresolved_subtype_set to stream
  
   IN:
       stset............the unresolved_subtype_set
	   file.............the stream

*******************************************************************************/

void unresolved_subtype_set_debug_dump(unresolved_subtype_set *stset,FILE *file)
{
	classref_or_classinfo *p;
	
	if (SUBTYPESET_IS_EMPTY(*stset)) {
		fprintf(file,"        (empty)\n");
	}
	else {
		p = stset->subtyperefs;
		for (;p->any; ++p) {
			if (IS_CLASSREF(*p)) {
				fprintf(file,"        ref: ");
				utf_fprint(file,p->ref->name);
			}
			else {
				fprintf(file,"        cls: ");
				utf_fprint(file,p->cls->name);
			}
			fputc('\n',file);
		}
	}
}

/* unresolved_class_debug_dump *************************************************
 
   Print debug info for unresolved_class to stream
  
   IN:
       ref..............the unresolved_class
	   file.............the stream

*******************************************************************************/

void unresolved_class_debug_dump(unresolved_class *ref,FILE *file)
{
	fprintf(file,"unresolved_class(%p):\n",(void *)ref);
	if (ref) {
		fprintf(file,"    referer   : ");
		utf_fprint(file,ref->classref->referer->name); fputc('\n',file);
		fprintf(file,"    refmethod : ");
		utf_fprint(file,ref->referermethod->name); fputc('\n',file);
		fprintf(file,"    refmethodd: ");
		utf_fprint(file,ref->referermethod->descriptor); fputc('\n',file);
		fprintf(file,"    classname : ");
		utf_fprint(file,ref->classref->name); fputc('\n',file);
		fprintf(file,"    subtypeconstraints:\n");
		unresolved_subtype_set_debug_dump(&(ref->subtypeconstraints),file);
	}
}

/* unresolved_field_debug_dump *************************************************
 
   Print debug info for unresolved_field to stream
  
   IN:
       ref..............the unresolved_field
	   file.............the stream

*******************************************************************************/

void unresolved_field_debug_dump(unresolved_field *ref,FILE *file)
{
	fprintf(file,"unresolved_field(%p):\n",(void *)ref);
	if (ref) {
		fprintf(file,"    referer   : ");
		utf_fprint(file,ref->fieldref->classref->referer->name); fputc('\n',file);
		fprintf(file,"    refmethod : ");
		utf_fprint(file,ref->referermethod->name); fputc('\n',file);
		fprintf(file,"    refmethodd: ");
		utf_fprint(file,ref->referermethod->descriptor); fputc('\n',file);
		fprintf(file,"    classname : ");
		utf_fprint(file,ref->fieldref->classref->name); fputc('\n',file);
		fprintf(file,"    name      : ");
		utf_fprint(file,ref->fieldref->name); fputc('\n',file);
		fprintf(file,"    descriptor: ");
		utf_fprint(file,ref->fieldref->descriptor); fputc('\n',file);
		fprintf(file,"    parseddesc: ");
		descriptor_debug_print_typedesc(file,ref->fieldref->parseddesc.fd); fputc('\n',file);
		fprintf(file,"    flags     : %04x\n",ref->flags);
		fprintf(file,"    instancetypes:\n");
		unresolved_subtype_set_debug_dump(&(ref->instancetypes),file);
		fprintf(file,"    valueconstraints:\n");
		unresolved_subtype_set_debug_dump(&(ref->valueconstraints),file);
	}
}

/* unresolved_method_debug_dump ************************************************
 
   Print debug info for unresolved_method to stream
  
   IN:
       ref..............the unresolved_method
	   file.............the stream

*******************************************************************************/

void unresolved_method_debug_dump(unresolved_method *ref,FILE *file)
{
	int i;

	fprintf(file,"unresolved_method(%p):\n",(void *)ref);
	if (ref) {
		fprintf(file,"    referer   : ");
		utf_fprint(file,ref->methodref->classref->referer->name); fputc('\n',file);
		fprintf(file,"    refmethod : ");
		utf_fprint(file,ref->referermethod->name); fputc('\n',file);
		fprintf(file,"    refmethodd: ");
		utf_fprint(file,ref->referermethod->descriptor); fputc('\n',file);
		fprintf(file,"    classname : ");
		utf_fprint(file,ref->methodref->classref->name); fputc('\n',file);
		fprintf(file,"    name      : ");
		utf_fprint(file,ref->methodref->name); fputc('\n',file);
		fprintf(file,"    descriptor: ");
		utf_fprint(file,ref->methodref->descriptor); fputc('\n',file);
		fprintf(file,"    parseddesc: ");
		descriptor_debug_print_methoddesc(file,ref->methodref->parseddesc.md); fputc('\n',file);
		fprintf(file,"    flags     : %04x\n",ref->flags);
		fprintf(file,"    instancetypes:\n");
		unresolved_subtype_set_debug_dump(&(ref->instancetypes),file);
		fprintf(file,"    paramconstraints:\n");
		if (ref->paramconstraints) {
			for (i=0; i<ref->methodref->parseddesc.md->paramcount; ++i) {
				fprintf(file,"      param %d:\n",i);
				unresolved_subtype_set_debug_dump(ref->paramconstraints + i,file);
			}
		} 
		else {
			fprintf(file,"      (empty)\n");
		}
	}
}
#endif

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
 * vim:noexpandtab:sw=4:ts=4:
 */

