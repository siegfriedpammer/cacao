/* jni.c - implementation of the Java Native Interface functions

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

   Authors: ?

   Changes: Joseph Wenninger

   $Id: jni.c 1112 2004-05-31 15:47:20Z jowenn $

*/


#include <string.h>
#include "main.h"
#include "jni.h"
#include "global.h"
#include "loader.h"
#include "tables.h"
#include "native.h"
#include "builtin.h"
#include "threads/thread.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"
#include "nat/java_lang_Byte.h"
#include "nat/java_lang_Character.h"
#include "nat/java_lang_Short.h"
#include "nat/java_lang_Integer.h"
#include "nat/java_lang_Boolean.h"
#include "nat/java_lang_Long.h"
#include "nat/java_lang_Float.h"
#include "nat/java_lang_Double.h"
#include "nat/java_lang_Throwable.h"
#include "jit/jit.h"
#include "asmpart.h"	
#define JNI_VERSION       0x00010002


#define PTR_TO_ITEM(ptr)   ((u8)(size_t)(ptr))

static utf* utf_char = 0;
static utf* utf_bool = 0;
static utf* utf_byte  =0;
static utf* utf_short = 0;
static utf* utf_int = 0;
static utf* utf_long = 0;
static utf* utf_float = 0;
static utf* utf_double = 0;


/********************* accessing instance-fields **********************************/

#define setField(obj,typ,var,val) *((typ*) ((long int) obj + (long int) var->offset))=val;  
#define getField(obj,typ,var)     *((typ*) ((long int) obj + (long int) var->offset))
#define setfield_critical(clazz,obj,name,sig,jdatatype,val) setField(obj,jdatatype,getFieldID_critical(env,clazz,name,sig),val); 



u4 get_parametercount(methodinfo *m)
{
	utf  *descr    =  m->descriptor;    /* method-descriptor */
	char *utf_ptr  =  descr->text;      /* current position in utf-text */
	char *desc_end =  utf_end(descr);   /* points behind utf string     */
	u4 parametercount = 0;

	/* skip '(' */
	utf_nextu2(&utf_ptr);

    /* determine number of parameters */
	while (*utf_ptr != ')') {
		get_type(&utf_ptr, desc_end, true);
		parametercount++;
	}

	return parametercount;
}



void fill_callblock(void *obj, utf *descr, jni_callblock blk[], va_list data, char ret)
{
    char *utf__ptr = descr->text;      /* current position in utf-text */
    char **utf_ptr = &utf__ptr;
    char *desc_end = utf_end(descr);   /* points behind utf string     */
    int cnt;
    u4 dummy;
    char c;

	/*
    log_text("fill_callblock");
    utf_display(descr);
    log_text("====");
	*/
    /* skip '(' */
    utf_nextu2(utf_ptr);

    /* determine number of parameters */
	if (obj) {
		blk[0].itemtype = TYPE_ADR;
		blk[0].item = PTR_TO_ITEM(obj);
		cnt = 1;
	} else cnt = 0;

	while (**utf_ptr != ')') {
		if (*utf_ptr >= desc_end)
        	panic("illegal method descriptor");

		switch (utf_nextu2(utf_ptr)) {
			/* primitive types */
		case 'B':
		case 'C':
		case 'S': 
		case 'Z':
			blk[cnt].itemtype = TYPE_INT;
			blk[cnt].item = (u8) va_arg(data, int);
			break;

		case 'I':
			blk[cnt].itemtype = TYPE_INT;
			dummy = va_arg(data, u4);
			/*printf("fill_callblock: pos:%d, value:%d\n",cnt,dummy);*/
			blk[cnt].item = (u8) dummy;
			break;

		case 'J':
			blk[cnt].itemtype = TYPE_LNG;
			blk[cnt].item = (u8) va_arg(data, jlong);
			break;

		case 'F':
			blk[cnt].itemtype = TYPE_FLT;
			*((jfloat *) (&blk[cnt].item)) = (jfloat) va_arg(data, jdouble);
			break;

		case 'D':
			blk[cnt].itemtype = TYPE_DBL;
			*((jdouble *) (&blk[cnt].item)) = (jdouble) va_arg(data, jdouble);
			break;

		case 'V':
			panic ("V not allowed as function parameter");
			break;
			
		case 'L':
			while (utf_nextu2(utf_ptr) != ';')
 			    blk[cnt].itemtype = TYPE_ADR;
			blk[cnt].item = PTR_TO_ITEM(va_arg(data, void*));
			break;
			
		case '[':
			{
				/* XXX */
				/* arrayclass */
/*  				char *start = *utf_ptr; */
				char ch;
				while ((ch = utf_nextu2(utf_ptr)) == '[')
					if (ch == 'L') {
						while (utf_nextu2(utf_ptr) != ';') {}
					}
	
				ch = utf_nextu2(utf_ptr);
				blk[cnt].itemtype = TYPE_ADR;
				blk[cnt].item = PTR_TO_ITEM(va_arg(data, void*));
				break;			
			}
		}
		cnt++;
	}

	/*the standard doesn't say anything about return value checking, but it appears to be usefull*/
	c = utf_nextu2(utf_ptr);
	c = utf_nextu2(utf_ptr);
	/*printf("%c  %c\n",ret,c);*/
	if (ret == 'O') {
		if (!((c == 'L') || (c == '[')))
			log_text("\n====\nWarning call*Method called for function with wrong return type\n====");
	} else if (ret != c)
		log_text("\n====\nWarning call*Method called for function with wrong return type\n====");
}


/* XXX it could be considered if we should do typechecking here in the future */
char fill_callblock_objA(void *obj, utf *descr, jni_callblock blk[], java_objectarray* params)
{
    char *utf__ptr = descr->text;      /* current position in utf-text */
    char **utf_ptr = &utf__ptr;
    char *desc_end = utf_end(descr);   /* points behind utf string     */

    jobject param;
    int cnt;
    int cnts;
    char c;

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
    intsDisable();
#endif
	if (utf_char==0) {
		utf_char=utf_new_char("java/lang/Character");
		utf_bool=utf_new_char("java/lang/Boolean");
		utf_byte=utf_new_char("java/lang/Byte");
		utf_short=utf_new_char("java/lang/Short");
		utf_int=utf_new_char("java/lang/Integer");
		utf_long=utf_new_char("java/lang/Long");
		utf_float=utf_new_char("java/lang/Float");
		utf_double=utf_new_char("java/lang/Double");
	}
#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
    intsRestore();
#endif

	/*
	  log_text("fill_callblock");
	  utf_display(descr);
	  log_text("====");
	*/
    /* skip '(' */
    utf_nextu2(utf_ptr);

    /* determine number of parameters */
	if (obj) {
		blk[0].itemtype = TYPE_ADR;
		blk[0].item = PTR_TO_ITEM(obj);
		cnt=1;

	} else {
		cnt = 0;
	}

	cnts = 0;
	while (**utf_ptr != ')') {
		if (*utf_ptr >= desc_end)
        	panic("illegal method descriptor");

		/* primitive types */
		switch (utf_nextu2(utf_ptr)) {
		case 'B':
			param = params->data[cnts];
			if (param == 0) {
				*exceptionptr = new_exception("java/lang/IllegalArgumentException");
				return 0;
			}
			if (param->vftbl->class->name == utf_byte) {
				blk[cnt].itemtype = TYPE_INT;
				blk[cnt].item = (u8) ((java_lang_Byte *) param)->value;

			} else  {
				*exceptionptr = new_exception("java/lang/IllegalArgumentException");
				return 0;
			}
			break;

		case 'C':
			param = params->data[cnts];
			if (param == 0) {
				*exceptionptr = new_exception("java/lang/IllegalArgumentException");
				return 0;
			}
			if (param->vftbl->class->name == utf_char) {
				blk[cnt].itemtype = TYPE_INT;
				blk[cnt].item = (u8) ((java_lang_Character *) param)->value;

			} else  {
				*exceptionptr = new_exception("java/lang/IllegalArgumentException");
				return 0;
			}
			break;

		case 'S':
			param = params->data[cnts];
			if (param == 0) {
				*exceptionptr = new_exception("java/lang/IllegalArgumentException");
				return 0;
			}
			if (param->vftbl->class->name == utf_short) {
				blk[cnt].itemtype = TYPE_INT;
				blk[cnt].item = (u8) ((java_lang_Short *) param)->value;

			} else  {
				if (param->vftbl->class->name == utf_byte) {
					blk[cnt].itemtype = TYPE_INT;
					blk[cnt].item = (u8) ((java_lang_Byte *) param)->value;

				} else {
					*exceptionptr = new_exception("java/lang/IllegalArgumentException");
					return 0;
				}
			}
			break;

		case 'Z':
			param = params->data[cnts];
			if (param == 0) {
				*exceptionptr = new_exception("java/lang/IllegalArgumentException");
				return 0;
			}
			if (param->vftbl->class->name == utf_bool) {
				blk[cnt].itemtype = TYPE_INT;
				blk[cnt].item = (u8) ((java_lang_Boolean *) param)->value;

			} else {
				*exceptionptr = new_exception("java/lang/IllegalArgumentException");
				return 0;
			}
			break;

		case 'I':
			/*log_text("fill_callblock_objA: param 'I'");*/
			param = params->data[cnts];
			if (param == 0) {
				*exceptionptr = new_exception("java/lang/IllegalArgumentException");
				return 0;
			}
			if (param->vftbl->class->name == utf_int) {
				blk[cnt].itemtype = TYPE_INT;
				blk[cnt].item = (u8) ((java_lang_Integer *) param)->value;
				/*printf("INT VALUE :%d\n",((struct java_lang_Integer * )param)->value);*/
			} else {
				if (param->vftbl->class->name == utf_short) {
					blk[cnt].itemtype = TYPE_INT;
					blk[cnt].item = (u8) ((java_lang_Short *) param)->value;

				} else  {
					if (param->vftbl->class->name == utf_byte) {
						blk[cnt].itemtype = TYPE_INT;
						blk[cnt].item = (u8) ((java_lang_Byte *) param)->value;

					} else  {
						*exceptionptr = new_exception("java/lang/IllegalArgumentException");
						return 0;
					}
				}
			}
			break;

		case 'J':
			param = params->data[cnts];
			if (param == 0) {
				*exceptionptr = new_exception("java/lang/IllegalArgumentException");
				return 0;
			}
			if (param->vftbl->class->name == utf_long) {
				blk[cnt].itemtype = TYPE_LNG;
				blk[cnt].item = (u8) ((java_lang_Long *) param)->value;

			} else  {
				if (param->vftbl->class->name == utf_int) {
					blk[cnt].itemtype = TYPE_LNG;
					blk[cnt].item = (u8) ((java_lang_Integer *) param)->value;

				} else {
					if (param->vftbl->class->name == utf_short) {
						blk[cnt].itemtype = TYPE_LNG;
						blk[cnt].item = (u8) ((java_lang_Short *) param)->value;

					} else  {
						if (param->vftbl->class->name == utf_byte) {
							blk[cnt].itemtype = TYPE_LNG;
							blk[cnt].item = (u8) ((java_lang_Byte *) param)->value;
						} else  {
							*exceptionptr = new_exception("java/lang/IllegalArgumentException");
							return 0;
						}
					}
				}

			}
			break;

		case 'F':
			param = params->data[cnts];
			if (param == 0) {
				*exceptionptr = new_exception("java/lang/IllegalArgumentException");
				return 0;
			}

			if (param->vftbl->class->name == utf_float) {
				blk[cnt].itemtype = TYPE_FLT;
				*((jfloat *) (&blk[cnt].item)) = (jfloat) ((java_lang_Float *) param)->value;

			} else  {
				*exceptionptr = new_exception("java/lang/IllegalArgumentException");
				return 0;
			}
			break;

		case 'D':
			param = params->data[cnts];
			if (param == 0) {
				*exceptionptr = new_exception("java/lang/IllegalArgumentException");
				return 0;
			}

			if (param->vftbl->class->name == utf_double) {
				blk[cnt].itemtype = TYPE_DBL;
				*((jdouble *) (&blk[cnt].item)) = (jdouble) ((java_lang_Float *) param)->value;

			} else  {
				if (param->vftbl->class->name == utf_float) {
					blk[cnt].itemtype = TYPE_DBL;
					*((jdouble *) (&blk[cnt].item)) = (jdouble) ((java_lang_Float *) param)->value;

				} else  {
					*exceptionptr = new_exception("java/lang/IllegalArgumentException");
					return 0;
				}
			}
			break;

		case 'V':
			panic("V not allowed as function parameter");
			break;

		case 'L':
			{
				char *start = (*utf_ptr) - 1;
				char *end = NULL;

				while (utf_nextu2(utf_ptr) != ';')
					end = (*utf_ptr) + 1;

				if (!builtin_instanceof(params->data[cnts], class_from_descriptor(start, end, 0, CLASSLOAD_LOAD))) {
					if (params->data[cnts] != 0) {
						*exceptionptr = new_exception("java/lang/IllegalArgumentException");
						return 0;
					}			
				}

				blk[cnt].itemtype = TYPE_ADR;
				blk[cnt].item = PTR_TO_ITEM(params->data[cnts]);
				break;			
			}

		case '[':
			{
				char *start = (*utf_ptr) - 1;
				char *end;

				char ch;
				while ((ch = utf_nextu2(utf_ptr)) == '[')
					if (ch == 'L') {
						while (utf_nextu2(utf_ptr) != ';') {}
					}

				end = (*utf_ptr) - 1;
				ch = utf_nextu2(utf_ptr);

				if (!builtin_arrayinstanceof(params->data[cnts], class_from_descriptor(start, end, 0, CLASSLOAD_LOAD)->vftbl)) {
					*exceptionptr = new_exception("java/lang/IllegalArgumentException");
					return 0;
				}
	
				blk[cnt].itemtype = TYPE_ADR;
				blk[cnt].item = PTR_TO_ITEM(params->data[cnts]);
				break;
			}
		}
		cnt++;
		cnts++;
	}

	c = utf_nextu2(utf_ptr);
	c = utf_nextu2(utf_ptr);
	return c; /*return type needed usage of the right lowlevel methods*/
}















jmethodID get_virtual(jobject obj,jmethodID methodID) {
	if (obj->vftbl->class==methodID->class) return methodID;
	return class_resolvemethod (obj->vftbl->class, methodID->name, methodID->descriptor);
}

jmethodID get_nonvirtual(jclass clazz,jmethodID methodID) {
	if (clazz==methodID->class) return methodID;
/*class_resolvemethod -> classfindmethod? (JOWENN)*/
	return class_resolvemethod (clazz, methodID->name, methodID->descriptor);
}



jobject callObjectMethod (jobject obj, jmethodID methodID, va_list args)
{ 	
	int argcount;
	jni_callblock *blk;
	jobject ret;

	/*
	  log_text("JNI-Call: CallObjectMethodV");
	  utf_display(methodID->name);
	  utf_display(methodID->descriptor);
	  printf("\nParmaeter count: %d\n",argcount);
	  utf_display(obj->vftbl->class->name);
	  printf("\n");
	*/

	if (methodID == 0) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError); 
		return 0;
	}

	argcount = get_parametercount(methodID);

	if (!( ((methodID->flags & ACC_STATIC) && (obj == 0)) ||
		((!(methodID->flags & ACC_STATIC)) && (obj != 0)) )) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}
	
	if (obj && !builtin_instanceof(obj, methodID->class)) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. CallObjectMethod does not support that");
		return 0;
	}

	blk = MNEW(jni_callblock, 4 /*argcount+2*/);

	fill_callblock(obj, methodID->descriptor, blk, args, 'O');

	/*      printf("parameter: obj: %p",blk[0].item); */
	ret = asm_calljavafunction2(methodID,
								argcount + 1,
								(argcount + 1) * sizeof(jni_callblock),
								blk);

	MFREE(blk, jni_callblock, argcount + 1);
	/*      printf("(CallObjectMethodV)-->%p\n",ret); */
	return ret;
}


/*
  core function for integer class methods (bool, byte, short, integer)
  This is basically needed for i386
*/
jint callIntegerMethod(jobject obj, jmethodID methodID, char retType, va_list args)
{
	int argcount;
	jni_callblock *blk;
	jint ret;

/*	printf("%p,     %c\n",retType,methodID,retType);*/

        /*
        log_text("JNI-Call: CallObjectMethodV");
        utf_display(methodID->name);
        utf_display(methodID->descriptor);
        printf("\nParmaeter count: %d\n",argcount);
        utf_display(obj->vftbl->class->name);
        printf("\n");
        */
	if (methodID == 0) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError); 
		return 0;
	}
        
	argcount = get_parametercount(methodID);

	if (!( ((methodID->flags & ACC_STATIC) && (obj == 0)) ||
		((!(methodID->flags & ACC_STATIC)) && (obj != 0)) )) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

	if (obj && !builtin_instanceof(obj, methodID->class)) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}


	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. CallIntegerMethod does not support that");
		return 0;
	}

	blk = MNEW(jni_callblock, 4 /*argcount+2*/);

	fill_callblock(obj, methodID->descriptor, blk, args, retType);

	/*      printf("parameter: obj: %p",blk[0].item); */
	ret = (jint) asm_calljavafunction2(methodID,
									   argcount + 1,
									   (argcount + 1) * sizeof(jni_callblock),
									   blk);

	MFREE(blk, jni_callblock, argcount + 1);
	/*      printf("(CallObjectMethodV)-->%p\n",ret); */

	return ret;
}


/*core function for long class functions*/
jlong callLongMethod(jobject obj, jmethodID methodID, va_list args)
{
	int argcount;
	jni_callblock *blk;
	jlong ret;

	/*
        log_text("JNI-Call: CallObjectMethodV");
        utf_display(methodID->name);
        utf_display(methodID->descriptor);
        printf("\nParmaeter count: %d\n",argcount);
        utf_display(obj->vftbl->class->name);
        printf("\n");
        */
	if (methodID == 0) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError); 
		return 0;
	}

	argcount = get_parametercount(methodID);

	if (!( ((methodID->flags & ACC_STATIC) && (obj == 0)) ||
		   ((!(methodID->flags & ACC_STATIC)) && (obj!=0)) )) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}

	if (obj && !builtin_instanceof(obj,methodID->class)) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
		return 0;
	}


	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. CallObjectMethod does not support that");
		return 0;
	}

	blk = MNEW(jni_callblock, 4 /*argcount+2*/);

	fill_callblock(obj, methodID->descriptor, blk, args, 'L');

	/*      printf("parameter: obj: %p",blk[0].item); */
	ret = asm_calljavafunction2long(methodID,
									argcount + 1,
									(argcount + 1) * sizeof(jni_callblock),
									blk);

	MFREE(blk, jni_callblock, argcount + 1);
	/*      printf("(CallObjectMethodV)-->%p\n",ret); */

	return ret;
}


/*core function for float class methods (float,double)*/
jdouble callFloatMethod(jobject obj, jmethodID methodID, va_list args,char retType)
{
	int argcount = get_parametercount(methodID);
	jni_callblock *blk;
	jdouble ret;

        /*
        log_text("JNI-Call: CallObjectMethodV");
        utf_display(methodID->name);
        utf_display(methodID->descriptor);
        printf("\nParmaeter count: %d\n",argcount);
        utf_display(obj->vftbl->class->name);
        printf("\n");
        */

	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. CallObjectMethod does not support that");
		return 0;
	}

	blk = MNEW(jni_callblock, 4 /*argcount+2*/);

	fill_callblock(obj, methodID->descriptor, blk, args, retType);

	/*      printf("parameter: obj: %p",blk[0].item); */
	ret = asm_calljavafunction2double(methodID,
									  argcount + 1,
									  (argcount + 1) * sizeof(jni_callblock),
									  blk);

	MFREE(blk, jni_callblock, argcount + 1);
	/*      printf("(CallObjectMethodV)-->%p\n",ret); */

	return ret;
}


/*************************** function: jclass_findfield ****************************
	
	searches for field with specified name and type in a 'classinfo'-structur
	if no such field is found NULL is returned 

************************************************************************************/

fieldinfo *jclass_findfield (classinfo *c, utf *name, utf *desc)
{
	s4 i;
/*	printf(" FieldCount: %d\n",c->fieldscount);
	utf_display(c->name); */
		for (i = 0; i < c->fieldscount; i++) {
/*		utf_display(c->fields[i].name);
		printf("\n");
		utf_display(c->fields[i].descriptor);
		printf("\n");*/
		if ((c->fields[i].name == name) && (c->fields[i].descriptor == desc))
			return &(c->fields[i]);
		}

	if (c->super) return jclass_findfield(c->super,name,desc);

	return NULL;
}

/********************* returns version of native method interface *****************/

jint GetVersion (JNIEnv* env)
{
	return JNI_VERSION;
}


/****************** loads a class from a buffer of raw class data *****************/

jclass DefineClass(JNIEnv* env, const char *name, jobject loader, const jbyte *buf, jsize len) 
{
	jclass clazz; 
	classbuffer *cb;
	s8 starttime;
	s8 stoptime;

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	compiler_lock();
	tables_lock();
#else
	intsDisable();
#endif
#endif

	/* measure time */
	if (getloadingtime)
		starttime = getcputime();

	clazz = class_new(utf_new_char((char *) name));

	/* build a classbuffer with the given data */
	cb = NEW(classbuffer);
	cb->class = clazz;
	cb->size = len;
	cb->data = (u1 *) buf;
	cb->pos = cb->data - 1;

	class_load_intern(cb);

	/* free memory */
	FREE(cb, classbuffer);

	/* measure time */
	if (getloadingtime) {
		stoptime = getcputime();
		loadingtime += (stoptime - starttime);
	}

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	tables_unlock();
	compiler_unlock();
#else
	intsRestore();
#endif
#endif

	if (*exceptionptr)
		return NULL;

	/* XXX link the class here? */
	class_link(clazz);

	if (*exceptionptr)
		return NULL;

	if (clazz)
		clazz->classloader = loader;

	return clazz;
}


/*************** loads locally defined class with the specified name **************/

jclass FindClass(JNIEnv* env, const char *name) 
{
	classinfo *c;  
  
	c = class_new(utf_new_char_classname((char *) name));

	class_load(c);

	if (*exceptionptr)
		return NULL;

	class_link(c);

	if (*exceptionptr)
		return NULL;

  	return c;
}
  

/*********************************************************************************** 

	converts java.lang.reflect.Method or 
 	java.lang.reflect.Constructor object to a method ID  
  
 **********************************************************************************/   
  
jmethodID FromReflectedMethod(JNIEnv* env, jobject method)
{
	/* log_text("JNI-Call: FromReflectedMethod"); */

	return 0;
}


/*************** return superclass of the class represented by sub ****************/
 
jclass GetSuperclass(JNIEnv* env, jclass sub) 
{
	classinfo *c;

	c = ((classinfo*) sub)->super;

	if (!c) return NULL; 

	use_class_as_object(c);

	return c;		
}
  
 
/*********************** check whether sub can be cast to sup  ********************/
  
jboolean IsAssignableForm(JNIEnv* env, jclass sub, jclass sup)
{
	return builtin_isanysubclass(sub, sup);
}


/***** converts a field ID derived from cls to a java.lang.reflect.Field object ***/

jobject ToReflectedField(JNIEnv* env, jclass cls, jfieldID fieldID, jboolean isStatic)
{
	/* log_text("JNI-Call: ToReflectedField"); */

	return NULL;
}


/***************** throw java.lang.Throwable object  ******************************/

jint Throw(JNIEnv* env, jthrowable obj)
{
	*exceptionptr = (java_objectheader*) obj;

	return 0;
}


/*******************************************************************************

	create exception object from the class clazz with the 
	specified message and cause it to be thrown

*******************************************************************************/

jint ThrowNew(JNIEnv* env, jclass clazz, const char *msg) 
{
	java_lang_Throwable *o;

  	/* instantiate exception object */
	o = (java_lang_Throwable *) native_new_and_init((classinfo*) clazz);

	if (!o) return (-1);

  	o->detailMessage = (java_lang_String *) javastring_new_char((char *) msg);

	*exceptionptr = (java_objectheader *) o;

	return 0;
}


/************************* check if exception occured *****************************/

jthrowable ExceptionOccurred (JNIEnv* env) 
{
	return (jthrowable) *exceptionptr;
}

/********** print exception and a backtrace of the stack (for debugging) **********/

void ExceptionDescribe (JNIEnv* env) 
{
	utf_display((*exceptionptr)->vftbl->class->name);
	printf ("\n");
	fflush (stdout);	
}


/******************* clear any exception currently being thrown *******************/

void ExceptionClear (JNIEnv* env) 
{
	*exceptionptr = NULL;	
}


/********** raises a fatal error and does not expect the VM to recover ************/

void FatalError (JNIEnv* env, const char *msg)
{
	panic((char *) msg);	
}

/******************* creates a new local reference frame **************************/ 

jint PushLocalFrame(JNIEnv* env, jint capacity)
{
	/* empty */

	return 0;
}

/**************** Pops off the current local reference frame **********************/

jobject PopLocalFrame(JNIEnv* env, jobject result)
{
	/* empty */

	return NULL;
}
    

/** Creates a new global reference to the object referred to by the obj argument **/
    
jobject NewGlobalRef(JNIEnv* env, jobject lobj)
{
	return lobj;
}

/*************  Deletes the global reference pointed to by globalRef **************/

void DeleteGlobalRef (JNIEnv* env, jobject gref)
{
	/* empty */
}


/*************** Deletes the local reference pointed to by localRef ***************/

void DeleteLocalRef (JNIEnv* env, jobject localRef)
{
	/* empty */
}

/********** Tests whether two references refer to the same Java object ************/

jboolean IsSameObject (JNIEnv* env, jobject obj1, jobject obj2)
{
 	return (obj1==obj2);
}

/***** Creates a new local reference that refers to the same object as ref  *******/

jobject NewLocalRef (JNIEnv* env, jobject ref)
{
	return ref;
}

/*********************************************************************************** 

	Ensures that at least a given number of local references can 
	be created in the current thread

 **********************************************************************************/   

jint EnsureLocalCapacity (JNIEnv* env, jint capacity)
{
	return 0; /* return 0 on success */
}


/********* Allocates a new Java object without invoking a constructor *************/

jobject AllocObject (JNIEnv* env, jclass clazz)
{
        java_objectheader *o = builtin_new(clazz);	
	return o;
}


/*********************************************************************************** 

	Constructs a new Java object
	arguments that are to be passed to the constructor are placed after methodID

***********************************************************************************/

jobject NewObject (JNIEnv* env, jclass clazz, jmethodID methodID, ...)
{
	java_objectheader *o;
	void* args[3];
	int argcount=get_parametercount(methodID);
	int i;
	va_list vaargs;

	/* log_text("JNI-Call: NewObject"); */

	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. NewObject does not support that");
		return 0;
	}

	
	o = builtin_new (clazz);         /*          create object */
	
	if (!o) return NULL;

	va_start(vaargs,methodID);
	for (i=0;i<argcount;i++) {
		args[i]=va_arg(vaargs,void*);
	}
	va_end(vaargs);
	asm_calljavafunction(methodID,o,args[0],args[1],args[2]);

	return o;
}


/*********************************************************************************** 

       Constructs a new Java object
       arguments that are to be passed to the constructor are placed in va_list args 

***********************************************************************************/

jobject NewObjectV(JNIEnv* env, jclass clazz, jmethodID methodID, va_list args)
{
	/* log_text("JNI-Call: NewObjectV"); */

	return NULL;
}


/*********************************************************************************** 

	Constructs a new Java object
	arguments that are to be passed to the constructor are placed in 
	args array of jvalues 

***********************************************************************************/

jobject NewObjectA(JNIEnv* env, jclass clazz, jmethodID methodID, jvalue *args)
{
	/* log_text("JNI-Call: NewObjectA"); */

	return NULL;
}


/************************ returns the class of an object **************************/ 

jclass GetObjectClass(JNIEnv* env, jobject obj)
{
 	classinfo *c = obj->vftbl->class;

	use_class_as_object(c);

	return c;
}


/************* tests whether an object is an instance of a class ******************/

jboolean IsInstanceOf(JNIEnv* env, jobject obj, jclass clazz)
{
	return builtin_instanceof(obj,clazz);
}


/***************** converts a java.lang.reflect.Field to a field ID ***************/
 
jfieldID FromReflectedField(JNIEnv* env, jobject field)
{
	log_text("JNI-Call: FromReflectedField");

	return 0;
}


/**********************************************************************************

	converts a method ID to a java.lang.reflect.Method or 
	java.lang.reflect.Constructor object

**********************************************************************************/

jobject ToReflectedMethod(JNIEnv* env, jclass cls, jmethodID methodID, jboolean isStatic)
{
	log_text("JNI-Call: ToReflectedMethod");

	return NULL;
}


/**************** returns the method ID for an instance method ********************/

jmethodID GetMethodID(JNIEnv* env, jclass clazz, const char *name, const char *sig)
{
	jmethodID m;

 	m = class_resolvemethod (
		clazz, 
		utf_new_char ((char*) name), 
		utf_new_char ((char*) sig)
    	);

	if (!m) *exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
	else if (m->flags & ACC_STATIC)   {
		m=0;
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
	}
	return m;
}


/******************** JNI-functions for calling instance methods ******************/

jobject CallObjectMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jobject ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallObjectMethod");*/

	va_start(vaargs, methodID);
	ret = callObjectMethod(obj, methodID, vaargs);
	va_end(vaargs);

	return ret;
}


jobject CallObjectMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	return callObjectMethod(obj,methodID,args);
}


jobject CallObjectMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallObjectMethodA");

	return NULL;
}




jboolean CallBooleanMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jboolean ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallBooleanMethod");*/

	va_start(vaargs,methodID);
	ret = (jboolean)callIntegerMethod(obj,get_virtual(obj,methodID),'Z',vaargs);
	va_end(vaargs);
	return ret;

}

jboolean CallBooleanMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	return (jboolean)callIntegerMethod(obj,get_virtual(obj,methodID),'Z',args);

}

jboolean CallBooleanMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallBooleanMethodA");

	return 0;
}

jbyte CallByteMethod (JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jbyte ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallVyteMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj,get_virtual(obj,methodID),'B',vaargs);
	va_end(vaargs);
	return ret;

}

jbyte CallByteMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
/*	log_text("JNI-Call: CallByteMethodV");*/
	return callIntegerMethod(obj,methodID,'B',args);
}


jbyte CallByteMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallByteMethodA");

	return 0;
}


jchar CallCharMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jchar ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallCharMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj, get_virtual(obj, methodID), 'C', vaargs);
	va_end(vaargs);

	return ret;
}


jchar CallCharMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
/*	log_text("JNI-Call: CallCharMethodV");*/
	return callIntegerMethod(obj,get_virtual(obj,methodID),'C',args);
}


jchar CallCharMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallCharMethodA");

	return 0;
}


jshort CallShortMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jshort ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallShortMethod");*/

	va_start(vaargs, methodID);
	ret = callIntegerMethod(obj, get_virtual(obj, methodID), 'S', vaargs);
	va_end(vaargs);

	return ret;
}


jshort CallShortMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	return callIntegerMethod(obj, get_virtual(obj, methodID), 'S', args);
}


jshort CallShortMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallShortMethodA");

	return 0;
}



jint CallIntMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jint ret;
	va_list vaargs;

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj, get_virtual(obj, methodID), 'I', vaargs);
	va_end(vaargs);

	return ret;
}


jint CallIntMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	return callIntegerMethod(obj, get_virtual(obj, methodID), 'I', args);
}


jint CallIntMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallIntMethodA");

	return 0;
}



jlong CallLongMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallLongMethod");

	return 0;
}


jlong CallLongMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallLongMethodV");

	return 0;
}


jlong CallLongMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallLongMethodA");

	return 0;
}



jfloat CallFloatMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jfloat ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallFloatMethod");*/

	va_start(vaargs,methodID);
	ret = callFloatMethod(obj, get_virtual(obj, methodID), vaargs, 'F');
	va_end(vaargs);

	return ret;
}


jfloat CallFloatMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallFloatMethodV");
	return callFloatMethod(obj, get_virtual(obj, methodID), args, 'F');
}


jfloat CallFloatMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallFloatMethodA");

	return 0;
}



jdouble CallDoubleMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	jdouble ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallDoubleMethod");*/

	va_start(vaargs,methodID);
	ret = callFloatMethod(obj, get_virtual(obj, methodID), vaargs, 'D');
	va_end(vaargs);

	return ret;
}


jdouble CallDoubleMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallDoubleMethodV");
	return callFloatMethod(obj, get_virtual(obj, methodID), args, 'D');
}


jdouble CallDoubleMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallDoubleMethodA");
	return 0;
}



void CallVoidMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
{
	va_list vaargs;

/*      log_text("JNI-Call: CallVoidMethod");*/

	va_start(vaargs,methodID);
	(void) callIntegerMethod(obj, get_virtual(obj, methodID), 'V', vaargs);
	va_end(vaargs);
}


void CallVoidMethodV (JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallVoidMethodV");
	(void)callIntegerMethod(obj,get_virtual(obj,methodID),'V',args);
}


void CallVoidMethodA (JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallVoidMethodA");
}



jobject CallNonvirtualObjectMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualObjectMethod");

	return NULL;
}


jobject CallNonvirtualObjectMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualObjectMethodV");

	return NULL;
}


jobject CallNonvirtualObjectMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallNonvirtualObjectMethodA");

	return NULL;
}



jboolean CallNonvirtualBooleanMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jboolean ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallNonvirtualBooleanMethod");*/

	va_start(vaargs,methodID);
	ret = (jboolean)callIntegerMethod(obj,get_nonvirtual(clazz,methodID),'Z',vaargs);
	va_end(vaargs);
	return ret;

}


jboolean CallNonvirtualBooleanMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
/*	log_text("JNI-Call: CallNonvirtualBooleanMethodV");*/
	return (jboolean)callIntegerMethod(obj,get_nonvirtual(clazz,methodID),'Z',args);
}


jboolean CallNonvirtualBooleanMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallNonvirtualBooleanMethodA");

	return 0;
}



jbyte CallNonvirtualByteMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jbyte ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallNonvirutalByteMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj,get_nonvirtual(clazz,methodID),'B',vaargs);
	va_end(vaargs);
	return ret;
}


jbyte CallNonvirtualByteMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	/*log_text("JNI-Call: CallNonvirtualByteMethodV"); */
	return callIntegerMethod(obj,get_nonvirtual(clazz,methodID),'B',args);

}


jbyte CallNonvirtualByteMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualByteMethodA");

	return 0;
}



jchar CallNonvirtualCharMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jchar ret;
	va_list vaargs;

/*	log_text("JNI-Call: CallNonVirtualCharMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj,get_nonvirtual(clazz,methodID),'C',vaargs);
	va_end(vaargs);
	return ret;
}


jchar CallNonvirtualCharMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	/*log_text("JNI-Call: CallNonvirtualCharMethodV");*/
	return callIntegerMethod(obj,get_nonvirtual(clazz,methodID),'C',args);
}


jchar CallNonvirtualCharMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualCharMethodA");

	return 0;
}



jshort CallNonvirtualShortMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jshort ret;
	va_list vaargs;

	/*log_text("JNI-Call: CallNonvirtualShortMethod");*/

	va_start(vaargs,methodID);
	ret = callIntegerMethod(obj,get_nonvirtual(clazz,methodID),'S',vaargs);
	va_end(vaargs);
	return ret;
}


jshort CallNonvirtualShortMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	/*log_text("JNI-Call: CallNonvirtualShortMethodV");*/
	return callIntegerMethod(obj,get_nonvirtual(clazz,methodID),'S',args);
}


jshort CallNonvirtualShortMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualShortMethodA");

	return 0;
}



jint CallNonvirtualIntMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{

        jint ret;
        va_list vaargs;

	/*log_text("JNI-Call: CallNonvirtualIntMethod");*/

        va_start(vaargs,methodID);
        ret = callIntegerMethod(obj,get_nonvirtual(clazz,methodID),'I',vaargs);
        va_end(vaargs);
        return ret;
}


jint CallNonvirtualIntMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	/*log_text("JNI-Call: CallNonvirtualIntMethodV");*/
        return callIntegerMethod(obj,get_nonvirtual(clazz,methodID),'I',args);
}


jint CallNonvirtualIntMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualIntMethodA");

	return 0;
}



jlong CallNonvirtualLongMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallNonvirtualLongMethod");

	return 0;
}


jlong CallNonvirtualLongMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualLongMethodV");

	return 0;
}


jlong CallNonvirtualLongMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualLongMethodA");

	return 0;
}



jfloat CallNonvirtualFloatMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jfloat ret;
	va_list vaargs;

	/*log_text("JNI-Call: CallNonvirtualFloatMethod");*/


	va_start(vaargs,methodID);
	ret = callFloatMethod(obj,get_nonvirtual(clazz,methodID),vaargs,'F');
	va_end(vaargs);
	return ret;

}


jfloat CallNonvirtualFloatMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallNonvirtualFloatMethodV");
	return callFloatMethod(obj,get_nonvirtual(clazz,methodID),args,'F');
}


jfloat CallNonvirtualFloatMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualFloatMethodA");

	return 0;
}



jdouble CallNonvirtualDoubleMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
	jdouble ret;
	va_list vaargs;
	log_text("JNI-Call: CallNonvirtualDoubleMethod");

	va_start(vaargs,methodID);
	ret = callFloatMethod(obj,get_nonvirtual(clazz,methodID),vaargs,'D');
	va_end(vaargs);
	return ret;

}


jdouble CallNonvirtualDoubleMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
/*	log_text("JNI-Call: CallNonvirtualDoubleMethodV");*/
	return callFloatMethod(obj,get_nonvirtual(clazz,methodID),args,'D');
}


jdouble CallNonvirtualDoubleMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallNonvirtualDoubleMethodA");

	return 0;
}



void CallNonvirtualVoidMethod (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
{
        va_list vaargs;

/*      log_text("JNI-Call: CallNonvirtualVoidMethod");*/

        va_start(vaargs,methodID);
        (void)callIntegerMethod(obj,get_nonvirtual(clazz,methodID),'V',vaargs);
        va_end(vaargs);

}


void CallNonvirtualVoidMethodV (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
/*	log_text("JNI-Call: CallNonvirtualVoidMethodV");*/

        (void)callIntegerMethod(obj,get_nonvirtual(clazz,methodID),'V',args);

}


void CallNonvirtualVoidMethodA (JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallNonvirtualVoidMethodA");
}

/************************* JNI-functions for accessing fields ************************/

jfieldID GetFieldID (JNIEnv *env, jclass clazz, const char *name, const char *sig) 
{
	jfieldID f;

/*	log_text("========================= searching for:");
	log_text(name);
	log_text(sig);*/
	f = jclass_findfield(clazz,
			    utf_new_char ((char*) name), 
			    utf_new_char ((char*) sig)
		  	    ); 
	
	if (!f) { 
/*		utf_display(clazz->name);
		log_text(name);
		log_text(sig);*/
		*exceptionptr =	new_exception(string_java_lang_NoSuchFieldError);  
	}
	return f;
}

/*************************** retrieve fieldid, abort on error ************************/

jfieldID getFieldID_critical(JNIEnv *env, jclass clazz, char *name, char *sig)
{
    jfieldID id = GetFieldID(env, clazz, name, sig);

    if (!id) {
       log_text("class:");
       utf_display(clazz->name);
       log_text("\nfield:");
       log_text(name);
       log_text("sig:");
       log_text(sig);

       panic("setfield_critical failed"); 
    }
    return id;
}

jobject GetObjectField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jobject,fieldID);
}

jboolean GetBooleanField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jboolean,fieldID);
}


jbyte GetByteField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
       	return getField(obj,jbyte,fieldID);
}


jchar GetCharField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jchar,fieldID);
}


jshort GetShortField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jshort,fieldID);
}


jint GetIntField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jint,fieldID);
}


jlong GetLongField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jlong,fieldID);
}


jfloat GetFloatField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jfloat,fieldID);
}


jdouble GetDoubleField (JNIEnv *env, jobject obj, jfieldID fieldID)
{
	return getField(obj,jdouble,fieldID);
}

void SetObjectField (JNIEnv *env, jobject obj, jfieldID fieldID, jobject val)
{
        setField(obj,jobject,fieldID,val);
}


void SetBooleanField (JNIEnv *env, jobject obj, jfieldID fieldID, jboolean val)
{
        setField(obj,jboolean,fieldID,val);
}


void SetByteField (JNIEnv *env, jobject obj, jfieldID fieldID, jbyte val)
{
        setField(obj,jbyte,fieldID,val);
}


void SetCharField (JNIEnv *env, jobject obj, jfieldID fieldID, jchar val)
{
        setField(obj,jchar,fieldID,val);
}


void SetShortField (JNIEnv *env, jobject obj, jfieldID fieldID, jshort val)
{
        setField(obj,jshort,fieldID,val);
}


void SetIntField (JNIEnv *env, jobject obj, jfieldID fieldID, jint val)
{
        setField(obj,jint,fieldID,val);
}


void SetLongField (JNIEnv *env, jobject obj, jfieldID fieldID, jlong val)
{
        setField(obj,jlong,fieldID,val);
}


void SetFloatField (JNIEnv *env, jobject obj, jfieldID fieldID, jfloat val)
{
        setField(obj,jfloat,fieldID,val);
}


void SetDoubleField (JNIEnv *env, jobject obj, jfieldID fieldID, jdouble val)
{
        setField(obj,jdouble,fieldID,val);
}


/**************** JNI-functions for calling static methods **********************/ 

jmethodID GetStaticMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig)
{
	jmethodID m;

 	m = class_resolvemethod(clazz,
							utf_new_char((char *) name),
							utf_new_char((char *) sig));

	if (!m) *exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
	else if (!(m->flags & ACC_STATIC))   {
		m=0;
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError);
	}

	return m;
}


jobject CallStaticObjectMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	log_text("JNI-Call: CallStaticObjectMethod");

	return NULL;
}


jobject CallStaticObjectMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticObjectMethodV");

	return NULL;
}


jobject CallStaticObjectMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticObjectMethodA");

	return NULL;
}


jboolean CallStaticBooleanMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jboolean ret;
	va_list vaargs;

	va_start(vaargs, methodID);
	ret = (jboolean) callIntegerMethod(0, methodID, 'Z', vaargs);
	va_end(vaargs);

	return ret;
}


jboolean CallStaticBooleanMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	return (jboolean) callIntegerMethod(0, methodID, 'Z', args);
}


jboolean CallStaticBooleanMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticBooleanMethodA");

	return 0;
}


jbyte CallStaticByteMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jbyte ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticByteMethod");*/

	va_start(vaargs, methodID);
	ret = (jbyte) callIntegerMethod(0, methodID, 'B', vaargs);
	va_end(vaargs);

	return ret;
}


jbyte CallStaticByteMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	return (jbyte) callIntegerMethod(0, methodID, 'B', args);
}


jbyte CallStaticByteMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticByteMethodA");

	return 0;
}


jchar CallStaticCharMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jchar ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticByteMethod");*/

	va_start(vaargs, methodID);
	ret = (jchar) callIntegerMethod(0, methodID, 'C', vaargs);
	va_end(vaargs);

	return ret;
}


jchar CallStaticCharMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	return (jchar) callIntegerMethod(0, methodID, 'C', args);
}


jchar CallStaticCharMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticCharMethodA");

	return 0;
}



jshort CallStaticShortMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jshort ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticByteMethod");*/

	va_start(vaargs, methodID);
	ret = (jshort) callIntegerMethod(0, methodID, 'S', vaargs);
	va_end(vaargs);

	return ret;
}


jshort CallStaticShortMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	/*log_text("JNI-Call: CallStaticShortMethodV");*/
	return (jshort) callIntegerMethod(0, methodID, 'S', args);
}


jshort CallStaticShortMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticShortMethodA");

	return 0;
}



jint CallStaticIntMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jint ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticIntMethod");*/

	va_start(vaargs, methodID);
	ret = callIntegerMethod(0, methodID, 'I', vaargs);
	va_end(vaargs);

	return ret;
}


jint CallStaticIntMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticIntMethodV");

	return callIntegerMethod(0, methodID, 'I', args);
}


jint CallStaticIntMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticIntMethodA");

	return 0;
}



jlong CallStaticLongMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jlong ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticLongMethod");*/

	va_start(vaargs, methodID);
	ret = callLongMethod(0, methodID, vaargs);
	va_end(vaargs);

	return ret;
}


jlong CallStaticLongMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticLongMethodV");
	
	return callLongMethod(0,methodID,args);
}


jlong CallStaticLongMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticLongMethodA");

	return 0;
}



jfloat CallStaticFloatMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jfloat ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticLongMethod");*/

	va_start(vaargs, methodID);
	ret = callFloatMethod(0, methodID, vaargs, 'F');
	va_end(vaargs);

	return ret;
}


jfloat CallStaticFloatMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{

	return callFloatMethod(0, methodID, args, 'F');

}


jfloat CallStaticFloatMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticFloatMethodA");

	return 0;
}



jdouble CallStaticDoubleMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	jdouble ret;
	va_list vaargs;

	/*      log_text("JNI-Call: CallStaticDoubleMethod");*/

	va_start(vaargs,methodID);
	ret = callFloatMethod(0, methodID, vaargs, 'D');
	va_end(vaargs);

	return ret;
}


jdouble CallStaticDoubleMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticDoubleMethodV");

	return callFloatMethod(0, methodID, args, 'D');
}


jdouble CallStaticDoubleMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
{
	log_text("JNI-Call: CallStaticDoubleMethodA");

	return 0;
}


void CallStaticVoidMethod(JNIEnv *env, jclass cls, jmethodID methodID, ...)
{
	va_list vaargs;

/*      log_text("JNI-Call: CallStaticVoidMethod");*/

	va_start(vaargs, methodID);
	(void) callIntegerMethod(0, methodID, 'V', vaargs);
	va_end(vaargs);
}


void CallStaticVoidMethodV(JNIEnv *env, jclass cls, jmethodID methodID, va_list args)
{
	log_text("JNI-Call: CallStaticVoidMethodV");
	(void)callIntegerMethod(0, methodID, 'V', args);
}


void CallStaticVoidMethodA(JNIEnv *env, jclass cls, jmethodID methodID, jvalue * args)
{
	log_text("JNI-Call: CallStaticVoidMethodA");
}


/****************** JNI-functions for accessing static fields ********************/

jfieldID GetStaticFieldID (JNIEnv *env, jclass clazz, const char *name, const char *sig) 
{
	jfieldID f;

	f = jclass_findfield(clazz,
			    utf_new_char ((char*) name), 
			    utf_new_char ((char*) sig)
		 	    ); 
	
	if (!f) *exceptionptr =	new_exception(string_java_lang_NoSuchFieldError);  

	return f;
}


jobject GetStaticObjectField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.a;       
}


jboolean GetStaticBooleanField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jbyte GetStaticByteField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jchar GetStaticCharField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jshort GetStaticShortField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jint GetStaticIntField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.i;       
}


jlong GetStaticLongField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.l;
}


jfloat GetStaticFloatField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
 	return fieldID->value.f;
}


jdouble GetStaticDoubleField (JNIEnv *env, jclass clazz, jfieldID fieldID)
{
	class_init(clazz);
	return fieldID->value.d;
}



void SetStaticObjectField (JNIEnv *env, jclass clazz, jfieldID fieldID, jobject value)
{
	class_init(clazz);
	fieldID->value.a = value;
}


void SetStaticBooleanField (JNIEnv *env, jclass clazz, jfieldID fieldID, jboolean value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticByteField (JNIEnv *env, jclass clazz, jfieldID fieldID, jbyte value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticCharField (JNIEnv *env, jclass clazz, jfieldID fieldID, jchar value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticShortField (JNIEnv *env, jclass clazz, jfieldID fieldID, jshort value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticIntField (JNIEnv *env, jclass clazz, jfieldID fieldID, jint value)
{
	class_init(clazz);
	fieldID->value.i = value;
}


void SetStaticLongField (JNIEnv *env, jclass clazz, jfieldID fieldID, jlong value)
{
	class_init(clazz);
	fieldID->value.l = value;
}


void SetStaticFloatField (JNIEnv *env, jclass clazz, jfieldID fieldID, jfloat value)
{
	class_init(clazz);
	fieldID->value.f = value;
}


void SetStaticDoubleField (JNIEnv *env, jclass clazz, jfieldID fieldID, jdouble value)
{
	class_init(clazz);
	fieldID->value.d = value;
}


/*****  create new java.lang.String object from an array of Unicode characters ****/ 

jstring NewString (JNIEnv *env, const jchar *buf, jsize len)
{
	u4 i;
	java_lang_String *s;
	java_chararray *a;
	
	s = (java_lang_String*) builtin_new (class_java_lang_String);
	a = builtin_newarray_char (len);

	/* javastring or characterarray could not be created */
	if ( (!a) || (!s) ) return NULL;

	/* copy text */
	for (i=0; i<len; i++) a->data[i] = buf[i];
	s -> value = a;
	s -> offset = 0;
	s -> count = len;

	return (jstring) s;
}


static char emptyString[]="";
static jchar emptyStringJ[]={0,0};

/******************* returns the length of a Java string ***************************/

jsize GetStringLength (JNIEnv *env, jstring str)
{
	return ((java_lang_String*) str)->count;
}


/********************  convertes javastring to u2-array ****************************/
	
u2 *javastring_tou2 (jstring so) 
{
	java_lang_String *s = (java_lang_String*) so;
	java_chararray *a;
	u4 i;
	u2 *stringbuffer;
	
	if (!s) return NULL;

	a = s->value;
	if (!a) return NULL;

	/* allocate memory */
	stringbuffer = MNEW( u2 , s->count + 1 );

	/* copy text */
	for (i=0; i<s->count; i++) stringbuffer[i] = a->data[s->offset+i];
	
	/* terminate string */
	stringbuffer[i] = '\0';

	return stringbuffer;
}

/********* returns a pointer to an array of Unicode characters of the string *******/

const jchar *GetStringChars (JNIEnv *env, jstring str, jboolean *isCopy)
{	
	jchar *jc=javastring_tou2(str);

	if (jc)	{
		if (isCopy) *isCopy=JNI_TRUE;
		return jc;
	}
	if (isCopy) *isCopy=JNI_TRUE;
	return emptyStringJ;
}

/**************** native code no longer needs access to chars **********************/

void ReleaseStringChars (JNIEnv *env, jstring str, const jchar *chars)
{
	if (chars==emptyStringJ) return;
	MFREE(((jchar*) chars),jchar,((java_lang_String*) str)->count+1);
}

/************ create new java.lang.String object from utf8-characterarray **********/

jstring NewStringUTF (JNIEnv *env, const char *utf)
{
/*    log_text("NewStringUTF called");*/
    return (jstring) javastring_new(utf_new_char((char *) utf));
}

/****************** returns the utf8 length in bytes of a string *******************/

jsize GetStringUTFLength (JNIEnv *env, jstring string)
{   
    java_lang_String *s = (java_lang_String*) string;

    return (jsize) u2_utflength(s->value->data, s->count); 
}


/************ converts a Javastring to an array of UTF-8 characters ****************/

const char* GetStringUTFChars(JNIEnv *env, jstring string, jboolean *isCopy)
{
    utf *u;

    u = javastring_toutf((java_lang_String *) string, false);

    if (isCopy)
		*isCopy = JNI_FALSE;
	
    if (u)
		return u->text;

    return emptyString;
	
}


/***************** native code no longer needs access to utf ***********************/

void ReleaseStringUTFChars (JNIEnv *env, jstring str, const char* chars)
{
    /*we don't release utf chars right now, perhaps that should be done later. Since there is always one reference
	the garbage collector will never get them*/
	/*
    log_text("JNI-Call: ReleaseStringUTFChars");
    utf_display(utf_new_char(chars));
	*/
}

/************************** array operations ***************************************/

jsize GetArrayLength(JNIEnv *env, jarray array)
{
    return array->size;
}


jobjectArray NewObjectArray (JNIEnv *env, jsize len, jclass clazz, jobject init)
{
	java_objectarray *j;

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_anewarray(len, clazz);

    return j;
}


jobject GetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index)
{
    jobject j = NULL;

    if (index < array->header.size)	
		j = array->data[index];
    else
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);
    
    return j;
}


void SetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index, jobject val)
{
    if (index >= array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else {
		/* check if the class of value is a subclass of the element class of the array */
		if (!builtin_canstore((java_objectarray *) array, (java_objectheader *) val))
			*exceptionptr = new_exception(string_java_lang_ArrayStoreException);

		else
			array->data[index] = val;
    }
}



jbooleanArray NewBooleanArray(JNIEnv *env, jsize len)
{
	java_booleanarray *j;

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_boolean(len);

    return j;
}


jbyteArray NewByteArray(JNIEnv *env, jsize len)
{
	java_bytearray *j;

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_byte(len);

    return j;
}


jcharArray NewCharArray(JNIEnv *env, jsize len)
{
	java_chararray *j;

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_char(len);

    return j;
}


jshortArray NewShortArray(JNIEnv *env, jsize len)
{
	java_shortarray *j;

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_short(len);

    return j;
}


jintArray NewIntArray(JNIEnv *env, jsize len)
{
	java_intarray *j;

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_int(len);

    return j;
}


jlongArray NewLongArray(JNIEnv *env, jsize len)
{
	java_longarray *j;

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_long(len);

    return j;
}


jfloatArray NewFloatArray(JNIEnv *env, jsize len)
{
	java_floatarray *j;

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_float(len);

    return j;
}


jdoubleArray NewDoubleArray(JNIEnv *env, jsize len)
{
	java_doublearray *j;

    if (len < 0) {
		*exceptionptr = new_exception(string_java_lang_NegativeArraySizeException);
		return NULL;
    }

    j = builtin_newarray_double(len);

    return j;
}


jboolean * GetBooleanArrayElements (JNIEnv *env, jbooleanArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jbyte * GetByteArrayElements (JNIEnv *env, jbyteArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jchar * GetCharArrayElements (JNIEnv *env, jcharArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jshort * GetShortArrayElements (JNIEnv *env, jshortArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jint * GetIntArrayElements (JNIEnv *env, jintArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jlong * GetLongArrayElements (JNIEnv *env, jlongArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jfloat * GetFloatArrayElements (JNIEnv *env, jfloatArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}


jdouble * GetDoubleArrayElements (JNIEnv *env, jdoubleArray array, jboolean *isCopy)
{
    if (isCopy) *isCopy = JNI_FALSE;
    return array->data;
}



void ReleaseBooleanArrayElements (JNIEnv *env, jbooleanArray array, jboolean *elems, jint mode)
{
    /* empty */
}


void ReleaseByteArrayElements (JNIEnv *env, jbyteArray array, jbyte *elems, jint mode)
{
    /* empty */
}


void ReleaseCharArrayElements (JNIEnv *env, jcharArray array, jchar *elems, jint mode)
{
    /* empty */
}


void ReleaseShortArrayElements (JNIEnv *env, jshortArray array, jshort *elems, jint mode)
{
    /* empty */
}


void ReleaseIntArrayElements (JNIEnv *env, jintArray array, jint *elems, jint mode)
{
    /* empty */
}


void ReleaseLongArrayElements (JNIEnv *env, jlongArray array, jlong *elems, jint mode)
{
    /* empty */
}


void ReleaseFloatArrayElements (JNIEnv *env, jfloatArray array, jfloat *elems, jint mode)
{
    /* empty */
}


void ReleaseDoubleArrayElements (JNIEnv *env, jdoubleArray array, jdouble *elems, jint mode)
{
    /* empty */
}


void GetBooleanArrayRegion(JNIEnv* env, jbooleanArray array, jsize start, jsize len, jboolean *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetByteArrayRegion(JNIEnv* env, jbyteArray array, jsize start, jsize len, jbyte *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size) 
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetCharArrayRegion(JNIEnv* env, jcharArray array, jsize start, jsize len, jchar *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetShortArrayRegion(JNIEnv* env, jshortArray array, jsize start, jsize len, jshort *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else	
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetIntArrayRegion(JNIEnv* env, jintArray array, jsize start, jsize len, jint *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetLongArrayRegion(JNIEnv* env, jlongArray array, jsize start, jsize len, jlong *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetFloatArrayRegion(JNIEnv* env, jfloatArray array, jsize start, jsize len, jfloat *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void GetDoubleArrayRegion(JNIEnv* env, jdoubleArray array, jsize start, jsize len, jdouble *buf)
{
    if (start < 0 || len < 0 || start+len>array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(buf, &array->data[start], len * sizeof(array->data[0]));
}


void SetBooleanArrayRegion(JNIEnv* env, jbooleanArray array, jsize start, jsize len, jboolean *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));
}


void SetByteArrayRegion(JNIEnv* env, jbyteArray array, jsize start, jsize len, jbyte *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));
}


void SetCharArrayRegion(JNIEnv* env, jcharArray array, jsize start, jsize len, jchar *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));

}


void SetShortArrayRegion(JNIEnv* env, jshortArray array, jsize start, jsize len, jshort *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));
}


void SetIntArrayRegion(JNIEnv* env, jintArray array, jsize start, jsize len, jint *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));

}


void SetLongArrayRegion(JNIEnv* env, jlongArray array, jsize start, jsize len, jlong *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));

}


void SetFloatArrayRegion(JNIEnv* env, jfloatArray array, jsize start, jsize len, jfloat *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));

}


void SetDoubleArrayRegion(JNIEnv* env, jdoubleArray array, jsize start, jsize len, jdouble *buf)
{
    if (start < 0 || len < 0 || start + len > array->header.size)
		*exceptionptr = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

    else
		memcpy(&array->data[start], buf, len * sizeof(array->data[0]));
}


jint RegisterNatives (JNIEnv* env, jclass clazz, const JNINativeMethod *methods, jint nMethods)
{
    log_text("JNI-Call: RegisterNatives");
    return 0;
}


jint UnregisterNatives (JNIEnv* env, jclass clazz)
{
    log_text("JNI-Call: UnregisterNatives");
    return 0;
}

/******************************* monitor operations ********************************/

jint MonitorEnter (JNIEnv* env, jobject obj)
{
    builtin_monitorenter(obj);
    return 0;
}


jint MonitorExit (JNIEnv* env, jobject obj)
{
    builtin_monitorexit(obj);
    return 0;
}


/************************************* JavaVM interface ****************************/
#ifdef __cplusplus
#error CPP mode not supported yet
#else
jint GetJavaVM (JNIEnv* env, JavaVM **vm)
{
    log_text("JNI-Call: GetJavaVM");
    *vm=&javaVM;
    return 0;
}
#endif /*__cplusplus*/

void GetStringRegion (JNIEnv* env, jstring str, jsize start, jsize len, jchar *buf)
{
    log_text("JNI-Call: GetStringRegion");

}

void GetStringUTFRegion (JNIEnv* env, jstring str, jsize start, jsize len, char *buf)
{
    log_text("JNI-Call: GetStringUTFRegion");

}

/************** obtain direct pointer to array elements ***********************/

void * GetPrimitiveArrayCritical (JNIEnv* env, jarray array, jboolean *isCopy)
{
	java_objectheader *s = (java_objectheader*) array;
	arraydescriptor *desc = s->vftbl->arraydesc;

	if (!desc) return NULL;

	return ((u1*)s) + desc->dataoffset;
}


void ReleasePrimitiveArrayCritical (JNIEnv* env, jarray array, void *carray, jint mode)
{
	log_text("JNI-Call: ReleasePrimitiveArrayCritical");

	/* empty */
}

/**** returns a pointer to an array of Unicode characters of the string *******/

const jchar * GetStringCritical (JNIEnv* env, jstring string, jboolean *isCopy)
{
	log_text("JNI-Call: GetStringCritical");

	return GetStringChars(env,string,isCopy);
}

/*********** native code no longer needs access to chars **********************/

void ReleaseStringCritical (JNIEnv* env, jstring string, const jchar *cstring)
{
	log_text("JNI-Call: ReleaseStringCritical");

	ReleaseStringChars(env,string,cstring);
}


jweak NewWeakGlobalRef (JNIEnv* env, jobject obj)
{
	log_text("JNI-Call: NewWeakGlobalRef");

	return obj;
}


void DeleteWeakGlobalRef (JNIEnv* env, jweak ref)
{
	log_text("JNI-Call: DeleteWeakGlobalRef");

	/* empty */
}


/******************************* check for pending exception ***********************/


jboolean ExceptionCheck(JNIEnv* env)
{
	log_text("JNI-Call: ExceptionCheck");

	return *exceptionptr ? JNI_TRUE : JNI_FALSE;
}






jint DestroyJavaVM(JavaVM *vm)
{
	log_text("DestroyJavaVM called");

	return 0;
}


jint AttachCurrentThread(JavaVM *vm, void **par1, void *par2)
{
	log_text("AttachCurrentThread called");

	return 0;
}


jint DetachCurrentThread(JavaVM *vm)
{
	log_text("DetachCurrentThread called");

	return 0;
}


jint GetEnv(JavaVM *vm, void **environment, jint jniversion)
{
	*environment = &env;

	return 0;
}


jint AttachCurrentThreadAsDaemon(JavaVM *vm, void **par1, void *par2)
{
	log_text("AttachCurrentThreadAsDaemon called");

	return 0;
}


/********************************* JNI invocation table ******************************/

struct _JavaVM javaVMTable={
   NULL,
   NULL,
   NULL,
   &DestroyJavaVM,
   &AttachCurrentThread,
   &DetachCurrentThread,
   &GetEnv,
   &AttachCurrentThreadAsDaemon
};

JavaVM javaVM = &javaVMTable;


/********************************* JNI function table ******************************/

struct JNI_Table envTable = {   
    NULL,
    NULL,
    NULL,
    NULL,    
    &GetVersion,
    &DefineClass,
    &FindClass,
    &FromReflectedMethod,
    &FromReflectedField,
    &ToReflectedMethod,
    &GetSuperclass,
    &IsAssignableForm,
    &ToReflectedField,
    &Throw,
    &ThrowNew,
    &ExceptionOccurred,
    &ExceptionDescribe,
    &ExceptionClear,
    &FatalError,
    &PushLocalFrame,
    &PopLocalFrame,
    &NewGlobalRef,
    &DeleteGlobalRef,
    &DeleteLocalRef,
    &IsSameObject,
    &NewLocalRef,
    &EnsureLocalCapacity,
    &AllocObject,
    &NewObject,
    &NewObjectV,
    &NewObjectA,
    &GetObjectClass,
    &IsInstanceOf,
    &GetMethodID,
    &CallObjectMethod,
    &CallObjectMethodV,
    &CallObjectMethodA,
    &CallBooleanMethod,
    &CallBooleanMethodV,
    &CallBooleanMethodA,
    &CallByteMethod,
    &CallByteMethodV,
    &CallByteMethodA,
    &CallCharMethod,
    &CallCharMethodV,
    &CallCharMethodA,
    &CallShortMethod,
    &CallShortMethodV,
    &CallShortMethodA,
    &CallIntMethod,
    &CallIntMethodV,
    &CallIntMethodA,
    &CallLongMethod,
    &CallLongMethodV,
    &CallLongMethodA,
    &CallFloatMethod,
    &CallFloatMethodV,
    &CallFloatMethodA,
    &CallDoubleMethod,
    &CallDoubleMethodV,
    &CallDoubleMethodA,
    &CallVoidMethod,
    &CallVoidMethodV,
    &CallVoidMethodA,
    &CallNonvirtualObjectMethod,
    &CallNonvirtualObjectMethodV,
    &CallNonvirtualObjectMethodA,
    &CallNonvirtualBooleanMethod,
    &CallNonvirtualBooleanMethodV,
    &CallNonvirtualBooleanMethodA,
    &CallNonvirtualByteMethod,
    &CallNonvirtualByteMethodV,
    &CallNonvirtualByteMethodA,
    &CallNonvirtualCharMethod,
    &CallNonvirtualCharMethodV,
    &CallNonvirtualCharMethodA,
    &CallNonvirtualShortMethod,
    &CallNonvirtualShortMethodV,
    &CallNonvirtualShortMethodA,
    &CallNonvirtualIntMethod,
    &CallNonvirtualIntMethodV,
    &CallNonvirtualIntMethodA,
    &CallNonvirtualLongMethod,
    &CallNonvirtualLongMethodV,
    &CallNonvirtualLongMethodA,
    &CallNonvirtualFloatMethod,
    &CallNonvirtualFloatMethodV,
    &CallNonvirtualFloatMethodA,
    &CallNonvirtualDoubleMethod,
    &CallNonvirtualDoubleMethodV,
    &CallNonvirtualDoubleMethodA,
    &CallNonvirtualVoidMethod,
    &CallNonvirtualVoidMethodV,
    &CallNonvirtualVoidMethodA,
    &GetFieldID,
    &GetObjectField,
    &GetBooleanField,
    &GetByteField,
    &GetCharField,
    &GetShortField,
    &GetIntField,
    &GetLongField,
    &GetFloatField,
    &GetDoubleField,
    &SetObjectField,
    &SetBooleanField,
    &SetByteField,
    &SetCharField,
    &SetShortField,
    &SetIntField,
    &SetLongField,
    &SetFloatField,
    &SetDoubleField,
    &GetStaticMethodID,
    &CallStaticObjectMethod,
    &CallStaticObjectMethodV,
    &CallStaticObjectMethodA,
    &CallStaticBooleanMethod,
    &CallStaticBooleanMethodV,
    &CallStaticBooleanMethodA,
    &CallStaticByteMethod,
    &CallStaticByteMethodV,
    &CallStaticByteMethodA,
    &CallStaticCharMethod,
    &CallStaticCharMethodV,
    &CallStaticCharMethodA,
    &CallStaticShortMethod,
    &CallStaticShortMethodV,
    &CallStaticShortMethodA,
    &CallStaticIntMethod,
    &CallStaticIntMethodV,
    &CallStaticIntMethodA,
    &CallStaticLongMethod,
    &CallStaticLongMethodV,
    &CallStaticLongMethodA,
    &CallStaticFloatMethod,
    &CallStaticFloatMethodV,
    &CallStaticFloatMethodA,
    &CallStaticDoubleMethod,
    &CallStaticDoubleMethodV,
    &CallStaticDoubleMethodA,
    &CallStaticVoidMethod,
    &CallStaticVoidMethodV,
    &CallStaticVoidMethodA,
    &GetStaticFieldID,
    &GetStaticObjectField,
    &GetStaticBooleanField,
    &GetStaticByteField,
    &GetStaticCharField,
    &GetStaticShortField,
    &GetStaticIntField,
    &GetStaticLongField,
    &GetStaticFloatField,
    &GetStaticDoubleField,
    &SetStaticObjectField,
    &SetStaticBooleanField,
    &SetStaticByteField,
    &SetStaticCharField,
    &SetStaticShortField,
    &SetStaticIntField,
    &SetStaticLongField,
    &SetStaticFloatField,
    &SetStaticDoubleField,
    &NewString,
    &GetStringLength,
    &GetStringChars,
    &ReleaseStringChars,
    &NewStringUTF,
    &GetStringUTFLength,
    &GetStringUTFChars,
    &ReleaseStringUTFChars,
    &GetArrayLength,
    &NewObjectArray,
    &GetObjectArrayElement,
    &SetObjectArrayElement,
    &NewBooleanArray,
    &NewByteArray,
    &NewCharArray,
    &NewShortArray,
    &NewIntArray,
    &NewLongArray,
    &NewFloatArray,
    &NewDoubleArray,
    &GetBooleanArrayElements,
    &GetByteArrayElements,
    &GetCharArrayElements,
    &GetShortArrayElements,
    &GetIntArrayElements,
    &GetLongArrayElements,
    &GetFloatArrayElements,
    &GetDoubleArrayElements,
    &ReleaseBooleanArrayElements,
    &ReleaseByteArrayElements,
    &ReleaseCharArrayElements,
    &ReleaseShortArrayElements,
    &ReleaseIntArrayElements,
    &ReleaseLongArrayElements,
    &ReleaseFloatArrayElements,
    &ReleaseDoubleArrayElements,
    &GetBooleanArrayRegion,
    &GetByteArrayRegion,
    &GetCharArrayRegion,
    &GetShortArrayRegion,
    &GetIntArrayRegion,
    &GetLongArrayRegion,
    &GetFloatArrayRegion,
    &GetDoubleArrayRegion,
    &SetBooleanArrayRegion,
    &SetByteArrayRegion,
    &SetCharArrayRegion,
    &SetShortArrayRegion,
    &SetIntArrayRegion,
    &SetLongArrayRegion,
    &SetFloatArrayRegion,
    &SetDoubleArrayRegion,
    &RegisterNatives,
    &UnregisterNatives,
    &MonitorEnter,
    &MonitorExit,
    &GetJavaVM,
    &GetStringRegion,
    &GetStringUTFRegion,
    &GetPrimitiveArrayCritical,
    &ReleasePrimitiveArrayCritical,
    &GetStringCritical,
    &ReleaseStringCritical,
    &NewWeakGlobalRef,
    &DeleteWeakGlobalRef,
    &ExceptionCheck
};

JNIEnv env = &envTable;


jobject *jni_method_invokeNativeHelper(JNIEnv *env, struct methodinfo *methodID, jobject obj, java_objectarray *params)
{
	int argcount;
	jni_callblock *blk;
	char retT;
	jobject retVal;

	if (methodID == 0) {
		*exceptionptr = new_exception(string_java_lang_NoSuchMethodError); 
		return NULL;
	}

	argcount = get_parametercount(methodID);

	if (obj && (!builtin_instanceof((java_objectheader *) obj, methodID->class))) {
		*exceptionptr = new_exception_message(string_java_lang_IllegalArgumentException,
											  "Object parameter of wrong type in Java_java_lang_reflect_Method_invokeNative");
		return 0;
	}



	if (argcount > 3) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		log_text("Too many arguments. invokeNativeHelper does not support that");
		return 0;
	}

	if (((!params) && (argcount != 0)) || (params && (params->header.size != argcount))) {
		*exceptionptr = new_exception(string_java_lang_IllegalArgumentException);
		return 0;
	}


	if (!(methodID->flags & ACC_STATIC) && (!obj))  {
		*exceptionptr = new_exception_message(string_java_lang_NullPointerException,
											  "Static mismatch in Java_java_lang_reflect_Method_invokeNative");
		return 0;
	}

	if ((methodID->flags & ACC_STATIC) && (obj)) obj = 0;

	blk = MNEW(jni_callblock, 4 /*argcount+2*/);

	retT = fill_callblock_objA(obj, methodID->descriptor, blk, params);

	switch (retT) {
	case 'V':
		(void) asm_calljavafunction2(methodID,
									 argcount + 1,
									 (argcount + 1) * sizeof(jni_callblock),
									 blk);
		retVal = NULL; /*native_new_and_init(loader_load(utf_new_char("java/lang/Void")));*/
		break;

	case 'I': {
		s4 intVal;	
		intVal = (s4) asm_calljavafunction2(methodID,
											argcount + 1,
											(argcount + 1) * sizeof(jni_callblock),
											blk);
		retVal = builtin_new(class_new(utf_new_char("java/lang/Integer")));
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
										   utf_new_char("<init>"),
										   utf_new_char("(I)V")),
					   intVal);
	}
	break;

	case 'B': {
		s4 intVal;	
		intVal = (s4) asm_calljavafunction2(methodID,
											argcount + 1,
											(argcount + 1) * sizeof(jni_callblock),
											blk);
		retVal = builtin_new(class_new(utf_new_char("java/lang/Byte")));
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
										   utf_new_char("<init>"),
										   utf_new_char("(B)V")),
					   intVal);
	}
	break;

	case 'C': {
		s4 intVal;	
		intVal = (s4) asm_calljavafunction2(methodID,
											argcount + 1,
											(argcount + 1) * sizeof(jni_callblock),
											blk);
		retVal = builtin_new(class_new(utf_new_char("java/lang/Character")));
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
										   utf_new_char("<init>"),
										   utf_new_char("(C)V")),
					   intVal);
	}
	break;

	case 'S': {
		s4 intVal;	
		intVal = (s4) asm_calljavafunction2(methodID,
											argcount + 1,
											(argcount + 1) * sizeof(jni_callblock),
											blk);
		retVal = builtin_new(class_new(utf_new_char("java/lang/Short")));
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
										   utf_new_char("<init>"),
										   utf_new_char("(S)V")),
					   intVal);
	}
	break;

	case 'Z': {
		s4 intVal;	
		intVal = (s4) asm_calljavafunction2(methodID,
											argcount + 1,
											(argcount + 1) * sizeof(jni_callblock),
											blk);
		retVal = builtin_new(class_new(utf_new_char("java/lang/Boolean")));
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
										   utf_new_char("<init>"),
										   utf_new_char("(Z)V")),
					   intVal);
	}
	break;

	case 'J': {
		jlong intVal;	
		intVal = asm_calljavafunction2long(methodID,
										   argcount + 1,
										   (argcount + 1) * sizeof(jni_callblock),
										   blk);
		retVal = builtin_new(class_new(utf_new_char("java/lang/Long")));
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
										   utf_new_char("<init>"),
										   utf_new_char("(J)V")),
					   intVal);
	}
	break;

	case 'F': {
		jdouble floatVal;	
		floatVal = asm_calljavafunction2double(methodID,
											   argcount + 1,
											   (argcount + 1) * sizeof(jni_callblock),
											   blk);
		retVal = builtin_new(class_new(utf_new_char("java/lang/Float")));
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
										   utf_new_char("<init>"),
										   utf_new_char("(F)V")),
					   floatVal);
	}
	break;

	case 'D': {
		jdouble floatVal;	
		floatVal = asm_calljavafunction2double(methodID,
											   argcount + 1,
											   (argcount + 1) * sizeof(jni_callblock),
											   blk);
		retVal = builtin_new(class_new(utf_new_char("java/lang/Double")));
		CallVoidMethod(env,
					   retVal,
					   class_resolvemethod(retVal->vftbl->class,
										   utf_new_char("<init>"),
										   utf_new_char("(D)V")),
					   floatVal);
	}
	break;

	case 'L': /* fall through */
	case '[':
		retVal = asm_calljavafunction2(methodID,
									   argcount + 1,
									   (argcount + 1) * sizeof(jni_callblock),
									   blk);
		break;

	default:
		/* if this happens the acception has already been set by fill_callblock_objA*/
		MFREE(blk, jni_callblock, 4 /*argcount+2*/);
		return (jobject *) 0;
	}

	MFREE(blk, jni_callblock, 4 /*argcount+2*/);

	if (*exceptionptr) {
		java_objectheader *exceptionToWrap = *exceptionptr;
		classinfo *ivtec;
		java_objectheader *ivte;

		*exceptionptr = NULL;
		ivtec = class_new(utf_new_char("java/lang/reflect/InvocationTargetException"));
		ivte = builtin_new(ivtec);
		asm_calljavafunction(class_resolvemethod(ivtec,
												 utf_new_char("<init>"),
												 utf_new_char("(Ljava/lang/Throwable;)V")),
							 ivte,
							 exceptionToWrap,
							 0,
							 0);

		if (*exceptionptr != NULL)
			panic("jni.c: error while creating InvocationTargetException wrapper");

		*exceptionptr = ivte;
	}

	return (jobject *) retVal;
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
