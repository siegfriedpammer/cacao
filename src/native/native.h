/****************************** native.h ***************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the codegenerator for an Alpha processor.
	This module generates Alpha machine code for a sequence of
	pseudo commands (PCMDs).

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/03/12

*******************************************************************************/


extern java_objectheader* exceptionptr;

void native_loadclasses ();
void native_setclasspath (char *path);

functionptr native_findfunction (unicode *cname, unicode *mname, 
                                 unicode *desc, bool isstatic);

java_objectheader *javastring_new (unicode *text);
java_objectheader *javastring_new_char (char *text);
char *javastring_tochar (java_objectheader *s);

java_objectheader *native_new_and_init (classinfo *c);

java_objectheader *literalstring_new (unicode *text);
void literalstring_free (java_objectheader*);

void attach_property(char *name, char *value);
