/********************** parseRT.h ******************************************
  Parser and print functions for X Type Analyis (XTA)
  used to only compile methods that may actually be used.

  5/12/2003 only prints info that will need to put in the various sets
            Goal: find which opcodes need to get info from...
***************************************************************************/
/********** internal function: printflags  (only for debugging) ***************/

static void printflags (u2 f)
{
   if ( f & ACC_PUBLIC )       printf (" PUBLIC");
   if ( f & ACC_PRIVATE )      printf (" PRIVATE");
   if ( f & ACC_PROTECTED )    printf (" PROTECTED");
   if ( f & ACC_STATIC )       printf (" STATIC");
   if ( f & ACC_FINAL )        printf (" FINAL");
   if ( f & ACC_SYNCHRONIZED ) printf (" SYNCHRONIZED");
   if ( f & ACC_VOLATILE )     printf (" VOLATILE");
   if ( f & ACC_TRANSIENT )    printf (" TRANSIENT");
   if ( f & ACC_NATIVE )       printf (" NATIVE");
   if ( f & ACC_INTERFACE )    printf (" INTERFACE");
   if ( f & ACC_ABSTRACT )     printf (" ABSTRACT");
}

/**************** Function: xfield_display (debugging only) ********************/
static void xfield_display (fieldinfo *f)
{
printf ("   ");
printflags (f -> flags);
printf (" ");
utf_display (f -> name);
printf (" ");
utf_display (f -> descriptor);	
printf (" offset: %ld\n", (long int) (f -> offset) );
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

static void parseXTA()
{
	int  p;                     /* java instruction counter                   */
	int  nextp;                 /* start of next java instruction             */
	int  opcode;                /* java opcode                                */
	int  i;                     /* temporary for different uses (counters)    */
        bool iswide = false;        /* true if last instruction was a wide        */

		/*XTAprint*/ 	{printf("*********************************\n");
		/*XTAprint*/ 	printf("PARSE XTA method name =");
		/*XTAprint*/ 	utf_display(rt_method->class->name);printf(".");
		/*XTAprint*/ 	utf_display(rt_method->name);printf("\n\n"); 
		/*XTAprint*/ 	method_display(rt_method); printf(">\n\n");fflush(stdout);}

	/* scan all java instructions */


	for (p = 0; p < rt_jcodelength; p = nextp) {
		opcode = rt_code_get_u1 (p);           /* fetch op code                  */

				/*XTAprint*/ 	{printf("Parse RT p=%i<%i<   opcode=<%i> %s\n",
				/*XTAprint*/                             p,rt_jcodelength,opcode,opcode_names[opcode]);
				/*XTAprint*/	fflush(stdout); }   

		nextp = p + jcommandsize[opcode];   /* compute next instruction start */
   switch (opcode) {

/* Opcodes with no info for XTA */
/*  NOP 			*/
/*  BIPUSH, SIPUSH - and all CONST opcodes just not read/write of field*/ 
/* illegal opcodes             */

/*--------------------------------*/
/* LOADCONST opcodes              */
                        case JAVA_BIPUSH:
				printf("BIPUSH value=%i\n",(rt_code_get_s1(p+1)));
                                /*LOADCONST_I(code_get_s1(p+1));*/
                                break;

                        case JAVA_SIPUSH:
				printf("SIPUSH value=%i\n",(rt_code_get_s2(p+1)));
                                /*LOADCONST_I(code_get_s2(p+1));*/
                                break;
			/*
			case JAVA_LDC1:
			...
                        case JAVA_LDC2:
                        case JAVA_LDC2W:
			...
			*/
                        case JAVA_ACONST_NULL:
				printf("ACONST_NULL value=NULL\n" );
                                /* LOADCONST_A(NULL);*/
                                break;

                        case JAVA_ICONST_M1:
                        case JAVA_ICONST_0:
                        case JAVA_ICONST_1:
                        case JAVA_ICONST_2:
                        case JAVA_ICONST_3:
                        case JAVA_ICONST_4:
                        case JAVA_ICONST_5:
				printf("JAVA_ICONST_x value=%i\n",(opcode - JAVA_ICONST_0 ) );
                                /* LOADCONST_I(opcode - JAVA_ICONST_0);*/
                                break;

                        case JAVA_LCONST_0:
                        case JAVA_LCONST_1:
				printf("JAVA_LCONST_x value=%d\n",(opcode - JAVA_LCONST_0 ) );
                                /* LOADCONST_L(opcode - JAVA_LCONST_0);*/
                                break;

                        case JAVA_FCONST_0:
                        case JAVA_FCONST_1:
                        case JAVA_FCONST_2:
printf("JAVA_FCONST_x value=%f\n",(opcode - JAVA_FCONST_0 ) );
                                /* LOADCONST_F(opcode - JAVA_FCONST_0);*/
                                break;

                        case JAVA_DCONST_0:
                        case JAVA_DCONST_1:
printf("JAVA_DCONST_x value=%d\n",(opcode - JAVA_DCONST_0 ) );
                                /* LOADCONST_D(opcode - JAVA_DCONST_0);*/
                                break;

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

                        case JAVA_PUTSTATIC:
                        case JAVA_GETSTATIC:
                                i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *fr;
                                fieldinfo *fi;

                                fr = class_getconstant (rt_class, i, CONSTANT_Fieldref);
                                fi = class_findfield (fr->class,fr->name, fr->descriptor);
					/*COtest*/ xfield_display (fi);
                                        /*COtest */ printf(" in class.field =");utf_display(fr->class->name); printf(".");
                                        /*COtest */ utf_display(fr->name);printf("\tPUT/GET STATIC\n");

                                }
                                break;

                        case JAVA_PUTFIELD:
                        case JAVA_GETFIELD:
                                i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *fr;
                                fieldinfo *fi;

                                fr = class_getconstant (rt_class, i, CONSTANT_Fieldref);
                                fi = class_findfield (fr->class,fr->name, fr->descriptor);
					/*COtest*/ xfield_display (fi);
                                        /*COtest */ printf(" in class.field =");utf_display(fr->class->name); printf(".");
                                        /*COtest */ utf_display(fr->name);printf("\tPUT/GET FIELD\n");


                                }
                                break;


/*-------------------------------*/
                        /* managing arrays ************************************************/

                        case JAVA_NEWARRAY:
                                switch (rt_code_get_s1(p+1)) {
                                        case 4:  /* boolean */
						/*COtest*/ printf("***** NEW boolean array\n");
                                                break;
                                        case 5:
						/*COtest*/ printf("***** NEW char array\n");
                                                break;
                                        case 6:
						/*COtest*/ printf("***** NEW float array\n");
                                                break;
                                        case 7:
						/*COtest*/ printf("***** NEW double array\n");
                                                break;
                                        case 8:
						/*COtest*/ printf("***** NEW byte array\n");
                                                break;
                                        case 9:
						/*COtest*/ printf("***** NEW short array\n");
                                                break;
                                        case 10:
						/*COtest*/ printf("***** NEW int array\n");
                                                break;
                                        case 11:
						/*COtest*/ printf("***** NEW long array\n");
                                                break;
                                        default: panic("XTA: Invalid array-type to create");
                                        }
                                break;


                        case JAVA_ANEWARRAY:
                                i = rt_code_get_u2(p+1);
                                {
                                constant_FMIref *ar;
                                voidptr e;
/*COtest*/ printf("ARRAY<%i> type=%i=",i,(int) rt_class->cptags[i] ); utf_display(rt_class->name);printf("\n");
                                /* array or class type ? */
                                if (class_constanttype (rt_class, i) != CONSTANT_Arraydescriptor) {
                                        e = class_getconstant(rt_class, i, CONSTANT_Class);
/*COtest*/ utf_display ( ((classinfo*)e) -> name );printf(">b\n");
					/******
                                        /*COtest if (ar == NULL) printf(" ARRAY2xNULL\n");
                                        /*COtest else {
                                        /*COtest      printf(" ARRAY2 Name=");utf_display(ar->name);printf("\n");
                                        /*COtest      }
					****/
                                        }
                                }
                                break;
                        case JAVA_MULTIANEWARRAY:
                                i = rt_code_get_u2(p+1);
                                {
                                constant_FMIref *ar;
                                        ar = class_getconstant(rt_class, i, CONSTANT_Arraydescriptor);
					/******
                                        /*COtest if (ar == NULL) printf(" ARRAY2xNULL\n");
                                        /*COtest else {
                                        /*COtest      printf(" ARRAY2 Name=");utf_display(ar->name);printf("\n");
                                        /*COtest      }
                                constant_arraydescriptor *desc =
                                    class_getconstant (rt_class, i, CONSTANT_Arraydescriptor);
				****/
                                }
                                break;


/*-------------------------------*/
                        /* method invocation *****/

                        case JAVA_INVOKESTATIC:
                                i = rt_code_get_u2(p + 1);
                                {
                                constant_FMIref *mr;
                                methodinfo *mi;

                                mr = class_getconstant (rt_class, i, CONSTANT_Methodref);
                                mi = class_findmethod (mr->class, mr->name, mr->descriptor);

					/*RTAprint*/ 	printf(" method name =");
					/*RTAprint*/ 	utf_display(mi->class->name); printf(".");
					/*RTAprint*/	utf_display(mi->name);printf("\tINVOKE STATIC\n");
					/*RTAprint*/	fflush(stdout);
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

					/*RTAprint*/	 {printf(" method name =");
					/*RTAprint*/	 utf_display(mi->class->name); printf(".");
					/*RTAprint*/ 	 utf_display(mi->name);
					/*RTAprint*/ 	 printf("\tbINVOKESPECIAL/VIRTUAL\n"); fflush(stdout); }
 
                                }
                                break;

                        case JAVA_INVOKEINTERFACE:
                                i = code_get_u2(p + 1);
                                {
                                constant_FMIref *mr;
                                methodinfo *mi;

                                mr = class_getconstant (rt_class, i, CONSTANT_InterfaceMethodref);
                                mi = class_findmethod (mr->class, mr->name, mr->descriptor);

					/*RTAprint*/	 {printf(" method name =");
					/*RTAprint*/	 utf_display(mi->class->name); printf(".");
					/*RTAprint*/ 	 utf_display(mi->name); printf("\n");
  /*RTAprint*/    method_display(mi); printf("INVOKE INTERFACE>\n\n");fflush(stdout);}

 
                                if (mi->flags & ACC_STATIC)
                                        panic ("Static/Nonstatic mismatch calling static method");
                                /*descriptor2types(mi); */
                                }
                                break;

                        /* miscellaneous object operations *******/

                        case JAVA_NEW:
                                i = rt_code_get_u2 (p+1);
				{
                                constant_FMIref *cr;
				classinfo *ci;	

                                ci = class_getconstant (rt_class, i, CONSTANT_Class);
					/*RTAprint*/	 printf(" NEW fmi info = ");
					/*RTAprint*/ utf_display (ci->name);fflush(stdout); printf("\n");
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

void XTA_jit_parse(methodinfo *m)
{
	rt_method      = m;
	rt_class       = rt_method->class;
	rt_descriptor  = rt_method->descriptor;
	rt_jcodelength = rt_method->jcodelength;
	rt_jcode       = rt_method->jcode;

	parseXTA();
}
