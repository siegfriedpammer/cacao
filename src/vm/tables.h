/****************************** tables.h ***************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Author:  Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/11/20

*******************************************************************************/

extern bool collectverbose;


#define CLASS(name)     (unicode_getclasslink(unicode_new_char(name)))

typedef void (*stringdeleter) ( java_objectheader *string );


void suck_init (char *classpath);
bool suck_start (unicode *name);
void suck_stop ();
void suck_nbytes (u1 *buffer, u4 len);
void skip_nbytes (u4 len);
u1 suck_u1 ();
s1 suck_s1 ();
u2 suck_u2 ();
s2 suck_s2 ();
u4 suck_u4 ();
s4 suck_s4 ();
u8 suck_u8 ();
s8 suck_s8 ();
float suck_float ();
double suck_double ();


void unicode_init ();
void unicode_close (stringdeleter del);
void unicode_display (unicode *u);
void unicode_sprint (char *buffer, unicode *u);
void unicode_fprint (FILE *file, unicode *u);
unicode *unicode_new_u2 (u2 *text, u2 length);
unicode *unicode_new_char (char *text);
void unicode_setclasslink (unicode *u, classinfo *class);
classinfo *unicode_getclasslink (unicode *u);
void unicode_unlinkclass (unicode *u);
void unicode_setstringlink (unicode *u, java_objectheader *str);
void unicode_unlinkstring (unicode *u);
void unicode_show ();

u2 desc_to_type (unicode *descriptor);
u2 desc_typesize (unicode *descriptor);


void heap_init (u4 size, u4 startsize, void **stackbottom);
void heap_close ();
void *heap_allocate (u4 bytelength, bool references, methodinfo *finalizer);
void heap_addreference (void **reflocation);

void gc_init (void);
void gc_thread (void);
void gc_call (void);
