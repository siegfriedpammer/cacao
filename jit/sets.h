#ifndef __SET__
#define __SET__

typedef struct methSet     methSet;
typedef struct methSetNode  methSetNode;
typedef struct fldSet      fldSet;
typedef struct fldSetNode   fldSetNode;
typedef struct classSet    classSet;
typedef struct classSetNode classSetNode;


/*------------------------------------------------------------*/
/*-- flds used by a method set fns */
/*------------------------------------------------------------*/
struct fldSet {
  fldSetNode *head;
  fldSetNode *tail;
  fldSetNode *pos;
  s4 length;
  };


struct fldSetNode {
  fieldinfo *fldRef;
  fldSetNode *nextfldRef;
  bool readPUT;
  bool writeGET;
  s2 index;
  };
fldSetNode      *inFldSet (fldSetNode *, fieldinfo *);
fldSetNode 	*addFldRef(fldSetNode *, fieldinfo *);
fldSet          *add2FldSet(fldSet    *, fieldinfo *);
fldSet          *createFldSet();
int 		 printFldSet  (fldSetNode *);
int 		 printFieldSet (fldSet *);


/*------------------------------------------------------------*/
/*-- methodinfo call set fns */
/*------------------------------------------------------------*/
struct methSet {
  methSetNode *head;
  methSetNode *tail;
  methSetNode *pos;
  s4 length;
  };

struct methSetNode {
  methodinfo   *methRef;
  methSetNode  *nextmethRef;
  classSetNode *lastptrIntoClassSet2;
  s2            index;
  s4            monoPoly;
  };

int  		 inMethSet (methSetNode *, methodinfo *);
methSetNode 	*addMethRef(methSetNode *, methodinfo *);
methSet         *add2MethSet(methSet    *, methodinfo *);
methSet         *createMethSet();
int		 printMethSet   (methSetNode *);
int		 printMethodSet (methSet *);

/*------------------------------------------------------------*/
/*-- classinfo XTA set fns  */
/*------------------------------------------------------------*/

struct classSet {
  classSetNode *head;
  classSetNode *tail;
  classSetNode *pos;
  s4 length;
  };

struct classSetNode {
  classinfo *classType;
  classSetNode *nextClass;
  s2 index;
  };

int  		inSet    (classSetNode *, classinfo *);
classSetNode *	addElement(classSetNode *,  classinfo *);
classSet     *  add2ClassSet(classSet *,  classinfo *);
classSet     *  createClassSet();
int             inRange (classSetNode *, classinfo *);
classSetNode *	addClassCone(classSetNode *,  classinfo *);
classSetNode *   intersectSubtypesWithSet(classinfo *, classSetNode *); 
int 		setSize(classSetNode *);
int 		printSet(classSetNode *);
int 		printClassSet(classSet *);

#endif

