/******************************* loader.h **************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the prototypes for the class loader.

	Author:  Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/11/14

*******************************************************************************/


/************************* program switches ***********************************/

extern bool loadverbose;         /* Debug-Meldungen beim Laden ausgeben */
extern bool linkverbose;
extern bool initverbose;         /* Meldungen ausgeben, wenn Klasse 
                                   initialisiert wird */
extern bool makeinitializations; /* Klassen automatisch initialisieren */

extern bool getloadingtime;
extern long int loadingtime;     /* CPU-Zeit f"urs Laden der Klassen */

extern list linkedclasses;       /* Liste aller fertig gelinkten Klassen */


/************************ prototypes ******************************************/

void loader_init ();
void loader_close ();

classinfo *loader_load (unicode *topname);
void loader_initclasses ();
void loader_compute_subclasses ();

classinfo *class_get (unicode *name);
voidptr class_getconstant (classinfo *class, u4 pos, u4 ctype);
u4 class_constanttype (classinfo *class, u4 pos);

fieldinfo *class_findfield (classinfo *c, unicode *name, unicode *desc);
methodinfo *class_findmethod (classinfo *c, unicode *name, unicode *desc);

methodinfo *class_resolvemethod (classinfo *c, unicode *name, unicode *dest);

bool class_issubclass (classinfo *sub, classinfo *super);

void class_init (classinfo *c);

void class_showmethods (classinfo *c);
void class_showconstantpool (classinfo *c);

