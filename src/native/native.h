/****************************** native.h ***************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the codegenerator for an Alpha processor.
	This module generates Alpha machine code for a sequence of
	pseudo commands (PCMDs).

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/03/12

*******************************************************************************/

/* Java Native Interface */
#include "jni.h"

/* structure for class java.lang.String */
struct java_lang_String;

/* for raising exceptions from native methods */
extern java_objectheader* exceptionptr;

/* javastring-hashtable */
extern hashtable string_hash; 

/* load classes required for native methods */
void native_loadclasses ();

/* set searchpath for classfiles */
void native_setclasspath (char *path);

/* find native function */
functionptr native_findfunction (utf *cname, utf *mname, 
                                 utf *desc, bool isstatic);

/* creates a new object of type java/lang/String from a utf-text */
java_objectheader *javastring_new (utf *text);

/* creates a new object of type java/lang/String from a c-string */
java_objectheader *javastring_new_char (char *text);

/* make c-string from a javastring (debugging) */
char *javastring_tochar (java_objectheader *s);

/* create new object on the heap and call the initializer */
java_objectheader *native_new_and_init (classinfo *c);

/* add property to system-property vector */
void attach_property(char *name, char *value);

/* correct vftbl-entries of javastring-hash */
void stringtable_update();

/* throw classnotfoundexcetion */
void throw_classnotfoundexception();

void throw_classnotfoundexception2(utf* classname);

/* make utf symbol from javastring */
utf *javastring_toutf(struct java_lang_String *string, bool isclassname);

/* make utf symbol from u2 array */
utf *utf_new_u2(u2 *unicodedata, u4 unicodelength, bool isclassname);

/* determine utf length in bytes of a u2 array */
u4 u2_utflength(u2 *text, u4 u2_length);

/* create systemclassloader object and initialize its instance fields  */
void init_systemclassloader();

/* search 'classinfo'-structure for a field with the specified name */
fieldinfo *class_findfield_approx (classinfo *c, utf *name);

/* creates a new javastring with the text of the utf-symbol */
java_objectheader *literalstring_new (utf *u);

/* creates a new javastring with the text of the u2-array */
java_objectheader *literalstring_u2 (java_chararray *a, u4 length, bool copymode);

/* dispose a javastring */
void literalstring_free (java_objectheader*);

void systemclassloader_addlibname(java_objectheader *o);
void systemclassloader_addlibrary(java_objectheader *o);

void copy_vftbl(vftbl **dest, vftbl *src);


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
