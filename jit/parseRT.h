/********************** parseRT.h ******************************************
  Parser and print functions for Rapid Type Analyis
  used to only compile methods that may actually be used.
***************************************************************************/

/*------------ Method /Class Used Markers -------------------------------*/
#define USED 1
#define NOTUSED 0
#define JUSTMARKED -1

/* class only */ 
#define METH_USED_BY_SUB -1
#define MARKEDSUPER -2
 
/*------------ global variables -----------------------------------------*/
int methRT = 0;            
int methRTlast = -1;;      
int methRTmax=5000;        
methodinfo *callgraph[5000];          

/*------------- RTAprint flags ------------------------------------------*/
int pCallgraph  = 1;    /* 0 - dont print 1 - print at end from main                             */ 
                        /* 2 - print at end of RT parse call                                     */
                        /* 3- print after each method RT parse                                   */
int pClassHeir  = 1;    /* 0 - dont print 1 - print at end from main                             */
                        /* 2 - print at end of RT parse call  3- print after each metho RT parse */
int pOpcodes    = 0;    /* 0 - don't print 1- print in parse RT 2- print in parse                */
                        /* 3 - print in both                                                     */
int pWhenMarked = 1;    /* 0 - don't print 1 - print when added to callgraph                     */
                        /* 2 - print when marked+methods called                                  */
                        /* 3 - print when class/method looked at                                 */

/*-----------------------------------------------------------------------*/

void printCallgraph ()
  { int i;

  for (i=0;i<=methRTlast;i++) {
    printf("  (%i): ",i);
    utf_display(callgraph[i]->class->name);
    printf(":");
    method_display(callgraph[i]);
    }

  printf("\n");
  }
/*--------------------------------------------------------------*/
void printRTClassHeirarchy(classinfo  *class) {
  
classinfo  *subs;
methodinfo *meth;
int m,cnt;

if (class == NULL) {return;}

    /* Class Name */
    if (class->classUsed != NOTUSED) {
	printf("\nClass: ");
        utf_display(class->name);    
	printf(" <%i> (depth=%i) ",class->classUsed,class->index);
        if (class->classUsed == METH_USED_BY_SUB)
           printf("\tClass not instanciated - but methods resolved to this class' code\n");
        else {
        if (class->classUsed == MARKEDSUPER)
           printf("\tClass not instanciated - but used by super init\n");
	   else
              printf("\n");
	  }
	/* Print methods used */
	cnt=0;
        for (m=0; m < class->methodscount; m++) {
            meth = &class->methods[m];
	    if (meth->methodUsed == USED) {
	        if (cnt == 0) {
	             printf("Methods used:\n");
	             }
	       cnt++;
		printf("\t");
	       utf_display(meth->class->name); 
		printf(".");
		method_display(meth);
	       }
            }
	 if (cnt > 0) printf("> %i of %i methods used\n",cnt, class->methodscount);
         }

    for (subs = class->sub;subs != NULL;subs = subs->nextsub) {
	printRTClassHeirarchy(subs);
        }
}
/*--------------------------------------------------------------*/

void printRThierarchyInfo(methodinfo *m) {
  printf("\nRT Class Heirarchy for ");
  printf("--- start of RT info --------------- after :\n");
  if (m != NULL) {
  	utf_display(m->class->name); 
   	 printf(".");
 	 method_display(m);
    	printf("\n");
	}
  printRTClassHeirarchy(class_java_lang_Object);
  printf("--- end  of RT info ---------------\n");
}

/*--------------------------------------------------------------*/
/* addToCallgraph - adds to RTA callgraph and                   */ 
/*                  sets  meth->methodUsed  to USED             */
/*                                                              */
/* To avoid unnecessary calls and dup entries in callgraph      */
/*      meth should not be null                                 */
/*      meth->methodUsed should be NOTUSED when called          */
/*      meth's class should be USED                             */
/*                                                              */
/*--------------------------------------------------------------*/

void addToCallgraph (methodinfo * meth) {
  int mfound =0;
  int im;
  int i;

if (meth==NULL) {panic("Trying to add a NULL method to callgraph"); return; }
if (meth->methodUsed == USED)  return;  /*This should be test before fn call to avoid needless fn call */

if ((meth->class->classUsed != USED) && (meth->class->classUsed != JUSTMARKED))  {
  printf("AddToCallGraph method's class not used nor marked<%i>\n",meth->class->classUsed); 
  utf_display(meth->class->name);printf(".");
  utf_display(meth->name);printf("\n");
  return;
  }

  /*   Add it to callgraph (mark used) */
       callgraph[++methRTlast] = meth ;
  		/*RTAprint*/if (pWhenMarked >= 1) {
		/*RTAprint*/	printf("\n Added to Call Graph #%i:",methRTlast);
		/*RTAprint*/	printf(" method name =");utf_display(meth->class->name); printf(".");
		/*RTAprint*/	utf_display(meth->name);printf("\n"); fflush(stdout);
		/*RTAprint*/  }
       meth->methodUsed = USED;    
}

/*--------------------------------------------------------------*/

void markMethod(classinfo *class, utf *name) {

  methodinfo *submeth;

  submeth = class_findmethod(class, name, NULL); 
  if (submeth != NULL) {

    if (submeth->class->classUsed == NOTUSED) {
      submeth->methodUsed = JUSTMARKED;
		/*RTAprint*/ if (pWhenMarked >= 2) 
		/*RTAprint*/	printf("Just Marking Method - class not used <%i>\n",class->index);
      }      
    else {
      /*  method not used             && method's class is really used */
      if ((submeth ->methodUsed != USED) && (submeth->class->classUsed == USED)) {
	addToCallgraph(submeth);   
        }  }  
      }
  else {
       if (class->classUsed == USED) {
         classinfo *s = class->super;
         int found = 0;  
	 methodinfo *supermeth;
	 while ((s!=NULL) && (found == 0)) {
	   supermeth = class_findmethod(s, name, NULL);	   
	   if (supermeth != NULL) { 
             found = 1;
	     if (s->classUsed == NOTUSED) {
	       s->classUsed = METH_USED_BY_SUB; /*mark class as having a method used by a used subtype */  
			 /*RTAprint*/if (pWhenMarked >= 2) { 
			 /*RTAprint*/ printf("Class marked Used by Subtype :");utf_display(s->name);printf("\n");}
               } /* end mark */ 
	     if (supermeth->methodUsed !=USED) {   
	     			/*COtest*/ if (strcmp(name->text,"<init>") == 0) 
				/*COtest*/	{panic("HOW did meth <init> get into markMethod???"); return; }
	       addToCallgraph(supermeth);
	       }
             }   /* end if !NULL */
           else {
	     s = s->super;
             } /* end else NULL */ 
           }   /* end while */

	   if ((s == NULL) && (found == 0))
		panic("parse RT: Method not found in class hierarchy");
         }  /* if current class used  */
    } /* end else Null */
} 

/*-------------------------------------------------------------------------------*/

void markSubs(classinfo *class, utf *name) {
		/*RTAprint*/ if (pWhenMarked>=3) {
		/*RTAprint*/ 	printf("markMethod: "); utf_display(class->name);
		/*RTAprint*/ 	printf(".");utf_display(name); printf("\n");}
  markMethod(class, name);   /* Mark method in class where it was found */

  if (class->sub != NULL) {
     classinfo *subs;
     int    subMcnt= 0;

     for (subs = class->sub;subs != NULL;subs = subs->nextsub) {
		/*RTAprint*/ if (pWhenMarked>=3) {
		/*RTAprint*/ 	printf("markSubs: "); utf_display(subs->name);printf(".");
		/*RTAprint*/ 	utf_display(name); printf("\n");}
       markSubs(subs, name); 
       }
    }
return;
}

/*-------------------------------------------------------------------------------*/

#define rt_code_get_u1(p)  rt_jcode[p]
#define rt_code_get_s1(p)  ((s1)rt_jcode[p])
#define rt_code_get_u2(p)  ((((u2)rt_jcode[p])<<8)+rt_jcode[p+1])
#define rt_code_get_s2(p)  ((s2)((((u2)rt_jcode[p])<<8)+rt_jcode[p+1]))
#define rt_code_get_u4(p)  ((((u4)rt_jcode[p])<<24)+(((u4)rt_jcode[p+1])<<16)\
                           +(((u4)rt_jcode[p+2])<<8)+rt_jcode[p+3])
#define rt_code_get_s4(p)  ((s4)((((u4)rt_jcode[p])<<24)+(((u4)rt_jcode[p+1])<<16)\
                           +(((u4)rt_jcode[p+2])<<8)+rt_jcode[p+3]))


/*-------------------------------------------------------------------------------*/

static void parseRT()
{
	int  p;                     /* java instruction counter                   */
	int  nextp;                 /* start of next java instruction             */
	int  opcode;                /* java opcode                                */
	int  i;                     /* temporary for different uses (counters)    */
        bool iswide = false;        /* true if last instruction was a wide        */

		/*RTAprint*/ if ((pOpcodes == 1) || (pOpcodes == 3)) 
		/*RTAprint*/ 	{printf("*********************************\n");
		/*RTAprint*/ 	printf("PARSE RT method name =");
		/*RTAprint*/ 	utf_display(rt_method->class->name);printf(".");
		/*RTAprint*/ 	utf_display(rt_method->name);printf("\n\n"); 
		/*RTAprint*/ 	method_display(rt_method); printf(">\n\n");fflush(stdout);}

	/* scan all java instructions */


	for (p = 0; p < rt_jcodelength; p = nextp) {
		opcode = rt_code_get_u1 (p);           /* fetch op code                  */

				/*RTAprint*/ if ((pOpcodes == 1) || (pOpcodes == 3)) 
				/*RTAprint*/ 	{printf("Parse RT p=%i<%i<   opcode=<%i> %s\n",
				/*RTAprint*/                             p,rt_jcodelength,opcode,icmd_names[opcode]);
				/*RTAprint*/	fflush(stdout); }   

		nextp = p + jcommandsize[opcode];   /* compute next instruction start */
   switch (opcode) {

/*--------------------------------*/
/* Code just to get the correct  next instruction */
			/* 21- 25 */
                        case JAVA_ILOAD:
                        case JAVA_LLOAD:
                        case JAVA_FLOAD:
                        case JAVA_DLOAD:
                        case JAVA_ALOAD:
                                if (iswide)
                                  {
                                  nextp = p+3;
                                  iswide = false;
                                  }
                                break;

			/* 54 -58 */
		        case JAVA_ISTORE:
                        case JAVA_LSTORE:
                        case JAVA_FSTORE:
                        case JAVA_DSTORE:
                        case JAVA_ASTORE:
                                if (iswide)
                                  {
                                  iswide=false;
                                  nextp = p+3;
                                  }
                                break;
			/* 132 */
		 	case JAVA_IINC:
                                {
                                int v;

                                if (iswide) {
                                        iswide = false;
                                        nextp = p+5;
                                        }
                                }
                                break;

                        /* wider index for loading, storing and incrementing */
			/* 196 */
                        case JAVA_WIDE:
                                iswide = true;
                                nextp = p + 1;
                                break;
			/* 169 */
			case JAVA_RET:
                                if (iswide) {
                                        nextp = p+3;
                                        iswide = false;
                                        }
                                break;

   /* table jumps ********************************/

                        case JAVA_LOOKUPSWITCH:
                                {
				s4 num;
				nextp = ALIGN((p + 1), 4);
                                num = rt_code_get_u4(nextp + 4);
                                nextp = nextp + 8 + 8 * num;
                                break;
                                }


                       case JAVA_TABLESWITCH:
                                {
				s4 num;
				nextp = ALIGN ((p + 1),4);
                                num = rt_code_get_s4(nextp + 4);
                                num = rt_code_get_s4(nextp + 8) - num;
                                nextp = nextp + 16 + 4 * num;
                                break;
                                }

/*-------------------------------*/
                        /* method invocation *****/

                        case JAVA_INVOKESTATIC:
                                i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *mr;
                                methodinfo *mi;

                                mr = class_getconstant (rt_class, i, CONSTANT_Methodref);
                                mi = class_findmethod (mr->class, mr->name, mr->descriptor);

					/*RTAprint*/ if ((pOpcodes >= 1)  || (pWhenMarked >= 2)) {
					/*RTAprint*/ 	printf(" method name =");
					/*RTAprint*/ 	utf_display(mi->class->name); printf(".");
					/*RTAprint*/	utf_display(mi->name);printf("\t<%i>INVOKE STATIC\n",mi->methodUsed);
					/*RTAprint*/	fflush(stdout);}

                                if (mi->class->classUsed == NOTUSED) {
                                    mi->class->classUsed = USED;

					/*RTAprint*/ if (pWhenMarked >= 2) {
					/*RTAprint*/ 	printf("Class marked Used :");
					/*RTAprint*/ 	utf_display(rt_class->name);
					/*RTAprint*/ 	 printf("INVOKESTATIC\n"); fflush(stdout);}
                                    }

                                if (mi->methodUsed  != USED) {  /* if static method not in callgraph */
				    addToCallgraph(mi);
                                    }
                                }
                                break;


                        case JAVA_INVOKESPECIAL:
                        case JAVA_INVOKEVIRTUAL:
                                i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *mr;
                                methodinfo *mi;

                                mr = class_getconstant (rt_class, i, CONSTANT_Methodref);
                                mi = class_findmethod (mr->class, mr->name, mr->descriptor);

					/*RTAprint*/ if ((pOpcodes >= 1)  || (pWhenMarked >= 2)) {
					/*RTAprint*/	 printf(" method name =");
					/*RTAprint*/	 utf_display(mi->class->name); printf(".");
					/*RTAprint*/ 	 utf_display(mi->name);
					/*RTAprint*/ 	 printf("\tINVOKESPECIAL/VIRTUAL\n"); fflush(stdout); }
 
                                if (strcmp(mi->name->text,"<init>") == 0) {     /* new class in method */
				    if ( mi->methodUsed != USED) {
                                        
					/*RTAprint*/ if (pWhenMarked >= 3) {
					/*RTAprint*/ 	printf("Tests to see if <init> is only called as super <init>\n");
					/*RTAprint*/ 	printf("\n/ method>");      
					/*RTAprint*/ 	utf_display(rt_method->class->name);
					/*RTAprint*/ 	printf(".");utf_display(rt_method->name);

					/*RTAprint*/ 	printf("\n/ super>");  
					/*RTAprint*/ 	if (rt_method->class->super == NULL) 
					/*RTAprint*/ 		printf("Super is NULL!  (class index=%i)\n",
					/*RTAprint*/ 			rt_method->class->index);
					/*RTAprint*/ 	else
					/*RTAprint*/ 		{utf_display(rt_method->class->super->name);printf(".");
					/*RTAprint*/ 		utf_display(method->name);}

					/*RTAprint*/	printf("\n/ calledMethod");
					/*RTAprint*/	utf_display(mi->class->name);printf(".");
					/*RTAprint*/	utf_display(mi->name); printf("\n");
					/*RTAprint*/ 	} 

                               	      if    (((strcmp(rt_method->name->text,"<init>") == 0) 
				        &&   (rt_method->class->super->name == mi->class->name))    /*  super? */
                                        && (mi->class->classUsed == NOTUSED))     /* class in heirarchy? */
				  	 {
                                            mi->class->classUsed = MARKEDSUPER;     /* mark as used as super*/
						/*RTAprint*/ if (pWhenMarked >= 2) { 
						/*RTAprint*/ 	printf("Class marked Used as a Super :");;
						/*RTAprint*/ 	utf_display(mi->class->name);printf("\n");}
					 }
				      else {	  
					if (mi->class->classUsed == NOTUSED) {
                                        	int ii;
                                        	mi->class->classUsed = USED;    /* add to heirarchy    */
				   		addToCallgraph(mi);   /* class was already marked - that init is not new */
								      /* A class that was just super - has a new    5/11 */

							/*RTAprint*/ if (pWhenMarked >= 2) { 
							/*RTAprint*/ 	printf("Class marked Used :");;
							/*RTAprint*/ 	utf_display(mi->class->name);printf("\n");}
						/* add marked methods to callgraph */ 
                                        	for (ii=0; ii<mi->class->methodscount; ii++) { 
                                           		if (mi->class->methods[ii].methodUsed == JUSTMARKED) { 
				                	addToCallgraph(&mi->class->methods[ii]); 
                                    	} }   }
				  }   }  }                                   	 
				/*--------------------------------------------------------------*/
				else {
						/*RTAprint*/ if (pWhenMarked >= 3) { 
						/*RTAprint*/ 	printf("Calling MarkSubs from SPECIAL/VIRTUAL :");
						/*RTAprint*/ 	utf_display(mi->class->name);printf(":");
						/*RTAprint*/ 	utf_display(mi->name);printf("\n"); }
				  markSubs(mi->class,mi->name); 
				  }
                                }
                                break;

                        default:
                                break;

                        } /* end switch */


		} /* end for */

	if (p != rt_jcodelength)
		panic("Command-sequence crosses code-boundary");

}

/*-------------------------------------------------------------------------------*/

void RT_jit_parse(methodinfo *m)
{
	/* initialise parameter type descriptor */

        callgraph[++methRTlast] = m;
  		/*RTAprint*/if (pWhenMarked >= 1) {
		/*RTAprint*/	printf("\n<Added to Call Graph #%i:",methRTlast);
		/*RTAprint*/	printf(" method name =");utf_display(m->class->name); printf(".");
		/*RTAprint*/	utf_display(m->name);printf("\n");
		/*RTAprint*/  }
	m->methodUsed = USED;

        while (methRT <= methRTlast) {
            rt_method      = callgraph[methRT];
	    rt_class       = rt_method->class;
	    rt_descriptor  = rt_method->descriptor;
	    rt_jcodelength = rt_method->jcodelength;
	    rt_jcode       = rt_method->jcode;

            if (! (  (rt_method->flags & ACC_NATIVE  )
		||   (rt_method->flags & ACC_ABSTRACT) ) )
              parseRT();

	    methRT++;
			/*RTprint*/ if (pCallgraph >= 3) {
			/*RTprint*/ 	printf("RTA Callgraph after RTA Call\n");
			/*RTprint*/ 	printCallgraph ();
			/*RTprint*/ 	}

			/*RTprint*/ if (pClassHeir >= 3) {
			/*RTprint*/ 	printf("RTA Classheirarchy after RTA Call\n");
			/*RTprint*/ 	printRThierarchyInfo(m); }
        } /* while */

	if (m->class->classUsed == NOTUSED)
		m->class->classUsed = JUSTMARKED; /* say Main's class has a method used */ 

			/*RTprint*/ if (pCallgraph >= 2) {
			/*RTprint*/ 	printf("RTA Callgraph after last method in callgraph - so far\n");
			/*RTprint*/ 	printCallgraph ();
			/*RTprint*/ 	}

			/*RTprint*/ if (pClassHeir >= 2) {
			/*RTprint*/ 	printf("RTA Classheirarchy after last method in callgraph - so far\n");
			/*RTprint*/ 	printRThierarchyInfo(m); }

}
