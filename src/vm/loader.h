/******************************* loader.h **************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the prototypes for the class loader.

	Author:  Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/11/14

*******************************************************************************/



/************************* program switches ***********************************/

extern bool loadverbose;         /* Print debug messages during loading */
extern bool linkverbose;
extern bool initverbose;         /* Log class initialization */ 
extern bool makeinitializations; /* Initialize classes automatically */

extern bool getloadingtime;
extern long int loadingtime;     /* CPU time for class loading */

extern list linkedclasses;       /* List containing all linked classes */


/************************ prototypes ******************************************/

/* initialize laoder, load important systemclasses */
void loader_init ();

void suck_init(char *cpath);

/* free resources */
void loader_close ();

/* load a class and all referenced classes */
classinfo *loader_load (utf *topname);

/* initializes all loaded classes */
void loader_initclasses ();

void loader_compute_subclasses ();

/* retrieve constantpool element */
voidptr class_getconstant (classinfo *class, u4 pos, u4 ctype);

/* determine type of a constantpool element */
u4 class_constanttype (classinfo *class, u4 pos);

/* search class for a field */
fieldinfo *class_findfield (classinfo *c, utf *name, utf *desc);

/* search for a method with a specified name and descriptor */
methodinfo *class_findmethod (classinfo *c, utf *name, utf *desc);
methodinfo *class_resolvemethod (classinfo *c, utf *name, utf *dest);

/* search for a method with specified name and arguments (returntype ignored) */
methodinfo *class_findmethod_approx (classinfo *c, utf *name, utf *desc);
methodinfo *class_resolvemethod_approx (classinfo *c, utf *name, utf *dest);

bool class_issubclass (classinfo *sub, classinfo *super);

/* call initializer of class */
void class_init (classinfo *c);

/* debug purposes */
void class_showmethods (classinfo *c);
void class_showconstantpool (classinfo *c);

/* set buffer for reading classdata */
void classload_buffer(u1 *buf,int len);

/* create class representing specific arraytype */
classinfo *create_array_class(utf *u);

/* create the arraydescriptor for the arraytype specified by the utf-string */
constant_arraydescriptor * buildarraydescriptor(char *utf, u4 namelen);



