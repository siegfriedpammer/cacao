/****************************** tables.h ***************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Author:  Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/11/20

*******************************************************************************/

#include "global.h" /* for unicode. -- phil */

#define CLASS(name)     (unicode_getclasslink(unicode_new_char(name)))

/* to determine the end of utf strings */
#define utf_end(utf) ((char *) utf->text+utf->blength)

/* switches for debug messages */
extern bool collectverbose;
extern list unloadedclasses;

/* function for disposing javastrings */
typedef void (*stringdeleter) ( java_objectheader *string );
    
/* creates hashtables for symboltables */
void tables_init ();

/* free memory for hashtables */ 
void tables_close (stringdeleter del);

/* write utf symbol to file/buffer */
void utf_sprint (char *buffer, utf *u);
void utf_fprint (FILE *file, utf *u);
void utf_display (utf *u);

/* create new utf-symbol */
utf *utf_new (char *text, u2 length);
utf *utf_new_char (char *text);

/* show utf-table */
void utf_show ();

/* get next unicode character of a utf-string */
u2 utf_nextu2(char **utf);

/* get number of unicode characters of a utf string */
u4 utf_strlen(utf *u);

/* search for class and create it if not found */
classinfo *class_new (utf *u);

/* get javatype according to a typedescriptor */
u2 desc_to_type (utf *descriptor);

/* get length of a datatype */
u2 desc_typesize (utf *descriptor);

/* determine hashkey of a unicode-symbol */
u4 unicode_hashkey (u2 *text, u2 length);

/* create hashtable */
void init_hashtable(hashtable *hash, u4 size);

/* search for class in classtable */
classinfo *class_get (utf *u);


void heap_init (u4 size, u4 startsize, void **stackbottom);
void heap_close ();
void *heap_allocate (u4 bytelength, bool references, methodinfo *finalizer);
void heap_addreference (void **reflocation);

void gc_init (void);
void gc_thread (void);
void gc_call (void);





