/*  
/* Defines of debug / trace /info  prints 
/*     to make the actual code more readable
/*
/* COtest prints are just debug
/* RTAprint prints are trace /info prints
*/



#define RTAPRINTcallgraph1  if(pWhenMarked>=1) \
        {printf("\n Added to Call Graph #%i:",  \
        methRTlast);printf(" method name =");   \
        utf_display(meth->class->name);printf("."); \
        method_display(meth);fflush(stdout);}


#define RTAPRINTmarkMethod1 if (pWhenMarked >= 2) { \
        printf("Just Marking Method - class:%s not used <%i>\n",class->name->text,class->index); \
        method_display(submeth);        \
        }

#define RTAPRINTmarkMethod1aa if (pWhenMarked >= 2) { \
        printf("Top Method/Class Abstract --  init found\n"); \
        method_display(submeth);        \
        }

#define RTAPRINTmarkMethod1A if (pWhenMarked >= 2) { \
        printf("Top Method/Class Abstract -- Marking Method & init - & marking class used <%i>\n",class->index); \
        method_display(submeth);        \
        }

#define RTAPRINTmarkMethod2 if (pWhenMarked >= 2) { \
        printf("Class marked Used by Subtype :");utf_display(s->name);printf("\n");}

#define RTAPRINTmarkSubs1 if (pWhenMarked>=3) { \
  utf *name = topmethod -> name; \
  utf *descriptor = topmethod -> descriptor; \
  s4  flags = topmethod -> flags; \
	printf("markMethod: "); utf_display(class->name); \
	printf(".");utf_display(name); printf("\n");}

#define RTAPRINTmarkSubs2 if (pWhenMarked>=3) { \
        printf("markSubs: "); utf_display(subs->name);printf("."); \
        utf_display(name); printf("\n");}

#define RTAPRINT01method if ((pOpcodes == 1) || (pOpcodes == 3)) \
        {printf("*********************************\n"); \
        printf("PARSE RT method name ="); \
        utf_display(rt_method->class->name);printf("."); \
        utf_display(rt_method->name);printf("\n\n"); \
        method_display(rt_method); printf(">\n\n");fflush(stdout);}

#define RTAPRINT02opcode if ((pOpcodes == 1) || (pOpcodes == 3)) \
        {printf("Parse RT p=%i<%i<   opcode=<%i> %s\n", \
                p,rt_jcodelength,opcode,opcode_names[opcode]); \
        fflush(stdout); }

#define RTAPRINT03putstatic1  if (pWhenMarked >= 5) { \
	printf("FMIref = ");  \
        utf_display ( fr->class->name );  \
        printf (".");  \
        utf_display ( fr->name);  \
        printf (" type=");  \
        utf_display ( fr->descriptor );  \
        printf("\n Field =");  \
        field_display (fi);  \
        printf(" in class.field =");utf_display(ci->name); printf(".");  \
        utf_display(fr->name);printf("\tPUT/GET STATIC\n"); fflush(stdout);  \
        printf("For %s:",ci->name->text);fflush(stdout);  \
        printf("#methods=%i\n",ci->methodscount); fflush(stdout); \
	}

#define RTAPRINT04invokestatic1  if (((pOpcodes == 1)  ||(pOpcodes == 3))  || (pWhenMarked >= 2)) { \
        printf(" method name ="); \
        utf_display(mi->class->name); printf("."); \
        utf_display(mi->name);printf("\t<%i>INVOKE STATIC\n",mi->methodUsed); \
        fflush(stdout);}

#define RTAPRINT05invokestatic2  if (pWhenMarked >= 2) { \
        printf("Class marked Used :"); \
        utf_display(rt_class->name); \
        printf("INVOKESTATIC\n"); fflush(stdout);}


#define RTAPRINT06invoke_spec_virt1 if ((pOpcodes == 1) ||(pOpcodes == 3)  || (pWhenMarked >= 2)) { \
        printf(" method name ="); \
	method_display(mi); \
        utf_display(mi->class->name); printf("."); \
        utf_display(mi->name); \
        printf("\taINVOKESPECIAL/VIRTUAL\n"); fflush(stdout); }

#define RTAPRINT07invoke_spec_virt2  if (pWhenMarked >= 3) { \
        printf("Calling MarkSubs from SPECIAL/VIRTUAL :"); \
        utf_display(mi->class->name);printf(":"); \
        utf_display(mi->name);printf("\n"); }

#define RTAPRINT08AinvokeInterface0 if (pWhenMarked >= 2) { \
	utf_display(mi->class->name); \
	method_display(mi); printf("\n");} 

#define RTAPRINT08invokeInterface1  if (pWhenMarked >= 3) { \
	printf("Implemented By classes :\n");  \
	if (subs == NULL) printf(" \tNOT IMPLEMENTED !!!\n"); \
	}

#define RTAPRINT09invokeInterface2  if (pWhenMarked >= 3) { \
	printf("\t");utf_display(subs->name);  printf(" <%i>\n",subs->classUsed); \
	}

#define RTAPRINT10new  if (pWhenMarked >= 2) { \
        printf("NEW Class marked Used :"); \
        utf_display(ci->name); \
        fflush(stdout);}

#define RTAPRINT11addedtoCallgraph  if (pWhenMarked >= 1){ \
	printf("\n<Added to Call Graph #%i:",methRTlast); \
	method_display(m); \
	printf(" method name =");utf_display(m->class->name); printf("."); \
	utf_display(m->name);printf("\n"); \
	}

#define RTAPRINT11addedtoCallgraph2  if (pWhenMarked >= 1){ \
	utf_display(ci->name); printf("."); \
	method_display(m); printf("\t\t"); \
	printf("<init> W/O new -- Add marked methods...\n<"); \
	}

#define RTAPRINT12aNative if (pWhenMarked >= 1) { \
	printf("Native = ");			   fflush(stdout); \
	utf_display(rt_class->name);  printf("."); fflush(stdout); \
	utf_display(rt_method->name); printf(" "); fflush(stdout); \
	utf_display(rt_descriptor);   		   fflush(stdout); printf("\n"); \
	}

#define RTAPRINT12Callgraph  if (pCallgraph >= 3) { \
	printf("RTA Callgraph after RTA Call\n"); \
	printCallgraph (); \
	}

#define RTAPRINT13Heirarchy  if (pClassHeir >= 3) { \
        printf("RTA Classheirarchy after RTA Call\n"); \
	        printRThierarchyInfo(m); \
	} 

#define RTAPRINT14CallgraphLast  if (pCallgraph >= 2) { \
	printf("RTA Callgraph after last method in callgraph - so far >888888888888888888\n"); \
	printCallgraph (); \
	}

#define RTAPRINT15HeirarchyiLast if (pClassHeir >= 2) { \
        printf("RTA Classheirarchy after last method in callgraph - so far >888888888888888888\n"); \
        	printRThierarchyInfo(m); \
	} 

#define RTAPRINT16stats1 if (pStats == 2) { \
	printf("OBJECT SUBS ARE_THERE 1\n"); \
	unRTclassHeirCnt=0; \
	unRTmethodCnt = 0; \
		printObjectClassHeirarchy(class_java_lang_Object); \
	printf("\n END of unanalysed Class Heirarchy: #%i classes /  #%i methods\n\n", \
		unRTclassHeirCnt,unRTmethodCnt); \
	}
