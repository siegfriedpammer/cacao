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

   $Id: resolve.c 3031 2005-07-13 11:52:50Z twisti $

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

#ifndef NDEBUG
#define RESOLVE_DEBUG
#endif

#ifdef RESOLVE_DEBUG
#define RESOLVE_ASSERT(cond)  assert(cond)
#else
#define RESOLVE_ASSERT(cond)
#endif

/******************************************************************************/
/* CLASS RESOLUTION                                                           */
/******************************************************************************/

/* resolve symbolic class reference -- see resolve.h */
bool
resolve_class_from_name(classinfo *referer,methodinfo *refmethod,
			  utf *classname,
			  resolve_mode_t mode,
			  bool checkaccess,
			  bool link,
			  classinfo **result)
{
	classinfo *cls = NULL;
	char *utf_ptr;
	int len;
	
	RESOLVE_ASSERT(result);
	RESOLVE_ASSERT(referer);
	RESOLVE_ASSERT(classname);
	RESOLVE_ASSERT(mode == resolveLazy || mode == resolveEager);
	
	*result = NULL;

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"resolve_class_from_name(");
	utf_fprint(stderr,referer->name);
	fprintf(stderr,",");
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
						RESOLVE_ASSERT(mode == resolveLazy);
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
	RESOLVE_ASSERT(cls);
	RESOLVE_ASSERT(cls->loaded);

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    checking access rights...\n");
#endif
	
	/* check access rights of referer to refered class */
	if (checkaccess && !is_accessible_class(referer,cls)) {
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
		if (!cls->linked)
			if (!link_class(cls))
				return false; /* exception */
		RESOLVE_ASSERT(cls->linked);
	}

	/* resolution succeeds */
#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    success.\n");
#endif
	*result = cls;
	return true;
}

bool
resolve_classref(methodinfo *refmethod,
				 constant_classref *ref,
				 resolve_mode_t mode,
				 bool checkaccess,
			     bool link,
				 classinfo **result)
{
	return resolve_classref_or_classinfo(refmethod,CLASSREF_OR_CLASSINFO(ref),mode,checkaccess,link,result);
}

bool
resolve_classref_or_classinfo(methodinfo *refmethod,
							  classref_or_classinfo cls,
							  resolve_mode_t mode,
							  bool checkaccess,
							  bool link,
							  classinfo **result)
{
	classinfo         *c;
	java_objectheader *e;
	java_objectheader *cause;
	
	RESOLVE_ASSERT(cls.any);
	RESOLVE_ASSERT(mode == resolveEager || mode == resolveLazy);
	RESOLVE_ASSERT(result);

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
		RESOLVE_ASSERT(c->loaded);
	}
	RESOLVE_ASSERT(c || (mode == resolveLazy));

	if (!c)
		return true; /* be lazy */
	
	RESOLVE_ASSERT(c);
	RESOLVE_ASSERT(c->loaded);

	if (link) {
		if (!c->linked)
			if (!link_class(c))
				goto return_exception;

		RESOLVE_ASSERT(c->linked);
	}

	/* succeeded */
	*result = c;
	return true;

 return_exception:
	/* get the cause */

	cause = *exceptionptr;

	/* convert ClassNotFoundException's to NoClassDefFoundError's */

	if (builtin_instanceof(cause, class_java_lang_ClassNotFoundException)) {
		/* clear exception, because we are calling jit code again */

		*exceptionptr = NULL;

		/* create new error */

		e = new_exception_javastring(string_java_lang_NoClassDefFoundError,
									 ((java_lang_Throwable *) cause)->detailMessage);

		/* we had an exception while creating the error */

		if (*exceptionptr)
			return false;

		/* set new exception */

		*exceptionptr = e;
	}

	*result = NULL;
	return false;
}


bool 
resolve_class_from_typedesc(typedesc *d, bool checkaccess, bool link, classinfo **result)
{
	classinfo *cls;
	
	RESOLVE_ASSERT(d);
	RESOLVE_ASSERT(result);

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
		RESOLVE_ASSERT(cls->loaded);
		if (!cls->linked)
			if (!link_class(cls))
				return false; /* exception */
	}
	RESOLVE_ASSERT(cls);
	RESOLVE_ASSERT(cls->loaded);
	RESOLVE_ASSERT(!link || cls->linked);

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    result = ");utf_fprint(stderr,cls->name);fprintf(stderr,"\n");
#endif

	*result = cls;
	return true;
}

/******************************************************************************/
/* SUBTYPE SET CHECKS                                                         */
/******************************************************************************/

/* for documentation see resolve.h */
bool
resolve_and_check_subtype_set(classinfo *referer,methodinfo *refmethod,
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

	RESOLVE_ASSERT(referer);
	RESOLVE_ASSERT(ref);
	RESOLVE_ASSERT(typeref.any);
	RESOLVE_ASSERT(mode == resolveLazy || mode == resolveEager);
	RESOLVE_ASSERT(error == resolveLinkageError || error == resolveIllegalAccessError);

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

	RESOLVE_ASSERT(type);
	RESOLVE_ASSERT(type->loaded);
	RESOLVE_ASSERT(type->linked);
	TYPEINFO_INIT_CLASSINFO(typeti,type);

	for (; setp->any; ++setp) {
		/* first resolve the set member if necessary */
		if (!resolve_classref_or_classinfo(refmethod,*setp,mode,false,true,&result))
			return false; /* exception */
		if (!result)
			return true; /* be lazy */

		RESOLVE_ASSERT(result);
		RESOLVE_ASSERT(result->loaded);
		RESOLVE_ASSERT(result->linked);

#ifdef RESOLVE_VERBOSE
		fprintf(stderr,"performing subclass test:\n");
		fprintf(stderr,"    ");utf_fprint(stderr,result->name);fputc('\n',stderr);
		fprintf(stderr,"  must be a %s of\n",(reversed) ? "superclass" : "subclass");
		fprintf(stderr,"    ");utf_fprint(stderr,type->name);fputc('\n',stderr);
#endif

		/* now check the subtype relationship */
		TYPEINFO_INIT_CLASSINFO(resultti,result);
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
		*exceptionptr = new_exception_message(string_java_lang_LinkageError,message);
	MFREE(message,char,msglen);
	return false; /* exception */
}

/******************************************************************************/
/* CLASS RESOLUTION                                                           */
/******************************************************************************/

/* for documentation see resolve.h */
bool
resolve_class(unresolved_class *ref,
			  resolve_mode_t mode,
			  bool checkaccess,
			  classinfo **result)
{
	classinfo *cls;
	bool checked;
	
	RESOLVE_ASSERT(ref);
	RESOLVE_ASSERT(result);
	RESOLVE_ASSERT(mode == resolveLazy || mode == resolveEager);

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

	RESOLVE_ASSERT(cls);
	RESOLVE_ASSERT(cls->loaded && cls->linked);

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

/******************************************************************************/
/* FIELD RESOLUTION                                                           */
/******************************************************************************/

/* for documentation see resolve.h */
bool
resolve_field(unresolved_field *ref,
			  resolve_mode_t mode,
			  fieldinfo **result)
{
	classinfo *referer;
	classinfo *container;
	classinfo *declarer;
	constant_classref *fieldtyperef;
	fieldinfo *fi;
	bool checked;
	
	RESOLVE_ASSERT(ref);
	RESOLVE_ASSERT(result);
	RESOLVE_ASSERT(mode == resolveLazy || mode == resolveEager);

	*result = NULL;

#ifdef RESOLVE_VERBOSE
	unresolved_field_debug_dump(ref,stderr);
#endif

	/* the class containing the reference */

	referer = ref->fieldref->classref->referer;
	RESOLVE_ASSERT(referer);

	/* first we must resolve the class containg the field */
	if (!resolve_class_from_name(referer,ref->referermethod,
					   ref->fieldref->classref->name,mode,true,true,&container))
	{
		/* the class reference could not be resolved */
		return false; /* exception */
	}
	if (!container)
		return true; /* be lazy */

	RESOLVE_ASSERT(container);
	RESOLVE_ASSERT(container->loaded && container->linked);

	/* now we must find the declaration of the field in `container`
	 * or one of its superclasses */

#ifdef RESOLVE_VERBOSE
		fprintf(stderr,"    resolving field in class...\n");
#endif

	fi = class_resolvefield(container,
							ref->fieldref->name,ref->fieldref->descriptor,
							referer,true);
	if (!fi)
		return false; /* exception */

	/* { the field reference has been resolved } */
	declarer = fi->class;
	RESOLVE_ASSERT(declarer);
	RESOLVE_ASSERT(declarer->loaded && declarer->linked);

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
		RESOLVE_ASSERT(fieldtyperef);
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
	if (!is_accessible_member(referer,declarer,fi->flags)) {
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
		fprintf(stderr,"    checking protectec access...\n");
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
		RESOLVE_ASSERT(fieldtyperef);
		if (!classcache_add_constraint(declarer->classloader,
									   referer->classloader,
									   fieldtyperef->name))
			return false;
	}

	/* succeed */
#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    success.\n");
#endif
	*result = fi;

	return true;
}

/******************************************************************************/
/* METHOD RESOLUTION                                                          */
/******************************************************************************/

/* for documentation see resolve.h */
bool
resolve_method(unresolved_method *ref, resolve_mode_t mode, methodinfo **result)
{
	classinfo *referer;
	classinfo *container;
	classinfo *declarer;
	methodinfo *mi;
	typedesc *paramtypes;
	int instancecount;
	int i;
	bool checked;
	
	RESOLVE_ASSERT(ref);
	RESOLVE_ASSERT(result);
	RESOLVE_ASSERT(mode == resolveLazy || mode == resolveEager);

#ifdef RESOLVE_VERBOSE
	unresolved_method_debug_dump(ref,stderr);
#endif

	*result = NULL;

	/* the class containing the reference */
	referer = ref->methodref->classref->referer;
	RESOLVE_ASSERT(referer);

	/* first we must resolve the class containg the method */
	if (!resolve_class_from_name(referer,ref->referermethod,
					   ref->methodref->classref->name,mode,true,true,&container))
	{
		/* the class reference could not be resolved */
		return false; /* exception */
	}
	if (!container)
		return true; /* be lazy */

	RESOLVE_ASSERT(container);

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

	if (!mi)
		return false; /* exception */ /* XXX set exceptionptr? */

#ifdef RESOLVE_VERBOSE
	fprintf(stderr,"    flags: %02x\n",mi->flags);
#endif
	/* { the method reference has been resolved } */

	declarer = mi->class;
	RESOLVE_ASSERT(declarer);

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

	RESOLVE_ASSERT(mi->parseddesc->paramcount == ref->methodref->parseddesc.md->paramcount);
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

	if (!is_accessible_member(referer,declarer,mi->flags)) {
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

	if (((mi->flags & ACC_PROTECTED) != 0) && !SAME_PACKAGE(declarer,referer)
			&& (declarer->name->text[0] == '['))
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
			
			if (i < instancecount)
				name = container->name; /* XXX should this be declarer->name? */
			else
				name = paramtypes[i].classref->name;
			
			if (!classcache_add_constraint(referer->classloader,
										   declarer->classloader, name))
				return false; /* exception */
		}
	}

	/* impose loading constraing onto return type */

	if (ref->methodref->parseddesc.md->returntype.type == TYPE_ADR) {
		if (!classcache_add_constraint(referer->classloader,declarer->classloader,
				ref->methodref->parseddesc.md->returntype.classref->name))
			return false; /* exception */
	}

	/* succeed */
	*result = mi;
	return true;
}

/******************************************************************************/
/* CREATING THE DATA STRUCTURES                                               */
/******************************************************************************/

static bool
unresolved_subtype_set_from_typeinfo(classinfo *referer,methodinfo *refmethod,
		unresolved_subtype_set *stset,typeinfo *tinfo,
		constant_classref *declaredtype)
{
	int count;
	int i;
	
	RESOLVE_ASSERT(stset);
	RESOLVE_ASSERT(tinfo);

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

unresolved_class *
create_unresolved_class(methodinfo *refmethod,
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

unresolved_field *
create_unresolved_field(classinfo *referer, methodinfo *refmethod,
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
	
	RESOLVE_ASSERT(fieldref);

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

bool
constrain_unresolved_field(unresolved_field *ref,
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

	RESOLVE_ASSERT(ref);

	fieldref = ref->fieldref;
	RESOLVE_ASSERT(fieldref);

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
	
	RESOLVE_ASSERT(instanceslot || ((ref->flags & RESOLVE_STATIC) != 0));
	fd = fieldref->parseddesc.fd;
	RESOLVE_ASSERT(fd);

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
			RESOLVE_ASSERT(initclass->loaded && initclass->linked);
			TYPEINFO_INIT_CLASSINFO(tinfo,initclass);
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
				RESOLVE_ASSERT(class_java_lang_String);
				RESOLVE_ASSERT(class_java_lang_String->loaded);
				RESOLVE_ASSERT(class_java_lang_String->linked);
				TYPEINFO_INIT_CLASSINFO(tinfo,class_java_lang_String);
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

unresolved_method *
create_unresolved_method(classinfo *referer, methodinfo *refmethod,
						 instruction *iptr)
{
	unresolved_method *ref;
	constant_FMIref *methodref;
	bool staticmethod;

	methodref = (constant_FMIref *) iptr[0].val.a;
	RESOLVE_ASSERT(methodref);
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
	ref->flags = (staticmethod) ? RESOLVE_STATIC : 0;
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

bool
constrain_unresolved_method(unresolved_method *ref,
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

	RESOLVE_ASSERT(ref);
	methodref = ref->methodref;
	RESOLVE_ASSERT(methodref);
	md = methodref->parseddesc.md;
	RESOLVE_ASSERT(md);
	RESOLVE_ASSERT(md->params != NULL);

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
	
	RESOLVE_ASSERT((instanceslot && instancecount==1) || ((ref->flags & RESOLVE_STATIC) != 0));

	/* record subtype constraints for the instance type, if any */
	if (instanceslot) {
		typeinfo *tip;
		
		RESOLVE_ASSERT(instanceslot->type == TYPE_ADR);
		
		if (iptr[0].opc == ICMD_INVOKESPECIAL && 
				TYPEINFO_IS_NEWOBJECT(instanceslot->typeinfo))
		{   /* XXX clean up */
			instruction *ins = (instruction*)TYPEINFO_NEWOBJECT_INSTRUCTION(instanceslot->typeinfo);
			classref_or_classinfo initclass = (ins) ? CLASSREF_OR_CLASSINFO(ins[-1].val.a)
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

		RESOLVE_ASSERT(param);
		RESOLVE_ASSERT(type == param->type);

		if (type == TYPE_ADR) {
			if (!ref->paramconstraints) {
				ref->paramconstraints = MNEW(unresolved_subtype_set,md->paramcount);
				for (j=md->paramcount-1-instancecount; j>i; --j)
					UNRESOLVED_SUBTYPE_SET_EMTPY(ref->paramconstraints[j]);
			}
			RESOLVE_ASSERT(ref->paramconstraints);
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

/******************************************************************************/
/* FREEING MEMORY                                                             */
/******************************************************************************/

inline static void 
unresolved_subtype_set_free_list(classref_or_classinfo *list)
{
	if (list) {
		classref_or_classinfo *p = list;

		/* this is silly. we *only* need to count the elements for MFREE */
		while ((p++)->any)
			;
		MFREE(list,classref_or_classinfo,(p - list));
	}
}

/* unresolved_class_free *******************************************************
 
   Free the memory used by an unresolved_class
  
   IN:
       ref..............the unresolved_class

*******************************************************************************/

void 
unresolved_class_free(unresolved_class *ref)
{
	RESOLVE_ASSERT(ref);

	unresolved_subtype_set_free_list(ref->subtypeconstraints.subtyperefs);
	FREE(ref,unresolved_class);
}

/* unresolved_field_free *******************************************************
 
   Free the memory used by an unresolved_field
  
   IN:
       ref..............the unresolved_field

*******************************************************************************/

void 
unresolved_field_free(unresolved_field *ref)
{
	RESOLVE_ASSERT(ref);

	unresolved_subtype_set_free_list(ref->instancetypes.subtyperefs);
	unresolved_subtype_set_free_list(ref->valueconstraints.subtyperefs);
	FREE(ref,unresolved_field);
}

/* unresolved_method_free ******************************************************
 
   Free the memory used by an unresolved_method
  
   IN:
       ref..............the unresolved_method

*******************************************************************************/

void 
unresolved_method_free(unresolved_method *ref)
{
	RESOLVE_ASSERT(ref);

	unresolved_subtype_set_free_list(ref->instancetypes.subtyperefs);
	if (ref->paramconstraints) {
		int i;
		int count = ref->methodref->parseddesc.md->paramcount;

		for (i=0; i<count; ++i)
			unresolved_subtype_set_free_list(ref->paramconstraints[i].subtyperefs);
		MFREE(ref->paramconstraints,unresolved_subtype_set,count);
	}
	FREE(ref,unresolved_method);
}

/******************************************************************************/
/* DEBUG DUMPS                                                                */
/******************************************************************************/

/* unresolved_subtype_set_debug_dump *******************************************
 
   Print debug info for unresolved_subtype_set to stream
  
   IN:
       stset............the unresolved_subtype_set
	   file.............the stream

*******************************************************************************/

void
unresolved_subtype_set_debug_dump(unresolved_subtype_set *stset,FILE *file)
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

void 
unresolved_class_debug_dump(unresolved_class *ref,FILE *file)
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

void 
unresolved_field_debug_dump(unresolved_field *ref,FILE *file)
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

void 
unresolved_method_debug_dump(unresolved_method *ref,FILE *file)
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

