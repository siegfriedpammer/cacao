/****************************** native.c ***************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Enth"alt die Tabellen f"ur die native-methods.
	Die vom Headerfile-Generator erzeugten -.hh - Dateien werden hier
	eingebunden, und ebenso alle C-Funktionen, mit denen diese
	Methoden implementiert werden.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/11/14

*******************************************************************************/

#include <unistd.h>
#include <time.h>

#include "global.h"
#include "native.h"

#include "builtin.h"
#include "nativetypes.hh"
#include "asmpart.h"
#include "tables.h"
#include "loader.h"


java_objectheader* exceptionptr = NULL;



static char *classpath;


/******************** die f"r die native-Methoden n"otigen Systemklassen *****/

static classinfo *class_java_lang_Class;
static classinfo *class_java_lang_Cloneable;
static classinfo *class_java_lang_CloneNotSupportedException;
static classinfo *class_java_lang_Double;
static classinfo *class_java_lang_Float;
static classinfo *class_java_io_IOException;
static classinfo *class_java_lang_ClassNotFoundException;
static classinfo *class_java_lang_InstantiationException;



/************************** alle Funktionen einbinden ************************/ 

#include "nat/lang.c"
#include "nat/io.c"
#include "nat/util.c"


/********************** Tabellen f"ur die Methoden ***************************/

static struct nativeref {
	char *classname;
	char *methodname;
	char *descriptor;
	bool isstatic;
	functionptr func;
} nativetable [] = {

#include "nativetable.hh"

};


#define NATIVETABLESIZE  (sizeof(nativetable)/sizeof(struct nativeref))

static struct nativecompref {
	unicode *classname;
	unicode *methodname;
	unicode *descriptor;
	bool isstatic;
	functionptr func;
	} nativecomptable [NATIVETABLESIZE];

static bool nativecompdone = false;


/*********************** Funktion: native_loadclasses **************************

	L"adt alle Klassen, die die native Methoden zus"atzlich ben"otigen 

*******************************************************************************/

void native_loadclasses()
{
	class_java_lang_Cloneable = 
		loader_load ( unicode_new_char ("java/lang/Cloneable") );
	class_java_lang_CloneNotSupportedException = 
		loader_load ( unicode_new_char ("java/lang/CloneNotSupportedException") );
	class_java_lang_Class =
		loader_load ( unicode_new_char ("java/lang/Class") );
	class_java_lang_Double =
		loader_load ( unicode_new_char ("java/lang/Double") );
	class_java_lang_Float =
		loader_load ( unicode_new_char ("java/lang/Float") );
	class_java_io_IOException = 
		loader_load ( unicode_new_char ("java/io/IOException") );
	class_java_lang_ClassNotFoundException =
		loader_load ( unicode_new_char ("java/lang/ClassNotFoundException") );
	class_java_lang_InstantiationException=
		loader_load ( unicode_new_char ("java/lang/InstantiationException") );
	
}

/********************* Funktion: native_setclasspath ***************************/
 
void native_setclasspath (char *path)
{
	classpath = path;
}


/*********************** Funktion: native_findfunction ************************

	Sucht in der Tabelle die passende Methode (muss mit Klassennamen,
	Methodennamen, Descriptor und 'static'-Status "ubereinstimmen),
	und gibt den Funktionszeiger darauf zur"uck.
	Return: Funktionszeiger oder NULL  (wenn es keine solche Methode gibt)

	Anmerkung: Zu Beschleunigung des Suchens werden die als C-Strings
	   vorliegenden Namen/Descriptors in entsprechende unicode-Symbole
	   umgewandelt (beim ersten Aufruf dieser Funktion).

*******************************************************************************/

functionptr native_findfunction (unicode *cname, unicode *mname, 
                                 unicode *desc, bool isstatic)
{
	u4 i;
	struct nativecompref *n;

	isstatic = isstatic ? true : false;

	if (!nativecompdone) {
		for (i=0; i<NATIVETABLESIZE; i++) {
			nativecomptable[i].classname   = 
					unicode_new_char(nativetable[i].classname);
			nativecomptable[i].methodname  = 
					unicode_new_char(nativetable[i].methodname);
			nativecomptable[i].descriptor  = 
					unicode_new_char(nativetable[i].descriptor);
			nativecomptable[i].isstatic    = 
					nativetable[i].isstatic;
			nativecomptable[i].func        = 
					nativetable[i].func;
			}
		nativecompdone = true;
		}

	for (i=0; i<NATIVETABLESIZE; i++) {
		n = &(nativecomptable[i]);

		if (cname==n->classname && mname==n->methodname &&
		    desc==n->descriptor && isstatic==n->isstatic)  return n->func;
		}

	return NULL;
}


/********************** Funktion: javastring_new *****************************

	Legt ein neues Objekt vom Typ java/lang/String an, und tr"agt als Text
	das "ubergebene unicode-Symbol ein. 
	Return: Zeiger auf den String, oder NULL (wenn Speicher aus)

*****************************************************************************/

java_objectheader *javastring_new (unicode *text)
{
	u4 i;
	java_lang_String *s;
	java_chararray *a;
	
	s = (java_lang_String*) builtin_new (class_java_lang_String);
	a = builtin_newarray_char (text->length);

	if ( (!a) || (!s) ) return NULL;

	for (i=0; i<text->length; i++) a->data[i] = text->text[i];
	s -> value = a;
	s -> offset = 0;
	s -> count = text->length;

	return (java_objectheader*) s;
}


/********************** Funktion: javastring_new_char ************************

	Legt ein neues Objekt vom Typ java/lang/String an, und tr"agt als Text
	den "ubergebenen C-String ein. 
	Return: Zeiger auf den String, oder NULL (wenn Speicher aus)

*****************************************************************************/

java_objectheader *javastring_new_char (char *text)
{
	u4 i;
	u4 len = strlen(text);
	java_lang_String *s;
	java_chararray *a;
	
	s = (java_lang_String*) builtin_new (class_java_lang_String);
	a = builtin_newarray_char (len);

	if ( (!a) || (!s) ) return NULL;

	for (i=0; i<len; i++) a->data[i] = text[i];
	s -> value = a;
	s -> offset = 0;
	s -> count = len;

	return (java_objectheader*) s;
}


/************************* Funktion: javastring_tochar *****************************

	Macht aus einem java-string einen C-String, und liefert den Zeiger
	darauf zur"uck. 
	Achtung: Beim n"achsten Aufruf der Funktion wird der vorige String 
	"uberschrieben
	
***********************************************************************************/

#define MAXSTRINGSIZE 1000
char stringbuffer[MAXSTRINGSIZE];

char *javastring_tochar (java_objectheader *so) 
{
	java_lang_String *s = (java_lang_String*) so;
	java_chararray *a;
	u4 i;
	
	if (!s) return "";
	a = s->value;
	if (!a) return "";
	if (s->count > MAXSTRINGSIZE) return "";
	for (i=0; i<s->count; i++) stringbuffer[i] = a->data[s->offset+i];
	stringbuffer[i] = '\0';
	return stringbuffer;
}


/******************** Funktion: native_new_and_init *************************

	Legt ein neues Objekt einer Klasse am Heap an, und ruft automatisch
	die Initialisierungsmethode auf.
	Return: Der Zeiger auf das Objekt, oder NULL, wenn kein Speicher
			mehr frei ist.
			
*****************************************************************************/

java_objectheader *native_new_and_init (classinfo *c)
{
	methodinfo *m;
	java_objectheader *o = builtin_new (c);

	if (!o) return NULL;
	
	m = class_findmethod (c, 
	                      unicode_new_char ("<init>"), 
	                      unicode_new_char ("()V"));
	if (!m) {
		log_text ("warning: class has no instance-initializer:");
		unicode_sprint (logtext, c->name);
		dolog();
		return o;
		}

	asm_calljavamethod (m, o,NULL,NULL,NULL);
	return o;
}


/********************* Funktion: literalstring_new ****************************

	erzeugt einen Java-String mit dem angegebenen Text, allerdings nicht
	auf dem HEAP, sondern in einem anderen Speicherbereich (der String
	muss dann sp"ater explizit wieder freigegeben werden).
	Alle Strings, die auf diese Art erzeugt werden, werden in einer
	gemeinsamen Struktur gespeichert (n"amlich auch "uber die
	Unicode-Hashtabelle), sodass identische Strings auch wirklich den
	gleichen Zeiger liefern.
	
******************************************************************************/

java_objectheader *literalstring_new (unicode *text)
{
	u4 i;
	java_lang_String *s;
	java_chararray *a;

	if (text->string) return text->string;
	
	a = lit_mem_alloc (sizeof(java_chararray) + sizeof(u2)*(text->length-1) );
	a -> header.objheader.vftbl = class_array -> vftbl;
	a -> header.size = text->length;
	a -> header.arraytype = ARRAYTYPE_CHAR;
	for (i=0; i<text->length; i++) a->data[i] = text->text[i];

	s = LNEW (java_lang_String);
	s -> header.vftbl = class_java_lang_String -> vftbl;
	s -> value = a;
	s -> offset = 0;
	s -> count = text->length;

	unicode_setstringlink (text, (java_objectheader*) s);
	return (java_objectheader*) s;
}


/********************** Funktion: literalstring_free **************************

	L"oscht einen Java-String wieder aus dem Speicher (wird zu Systemende
	vom Hashtabellen-Verwalter aufgerufen)

******************************************************************************/

void literalstring_free (java_objectheader* sobj)
{
	java_lang_String *s = (java_lang_String*) sobj;
	java_chararray *a = s->value;
	
	LFREE (s, java_lang_String);
	LFREE (a, sizeof(java_chararray) + sizeof(u2)*(a->header.size-1));
}



