#include  <stdio.h>
#include  <string.h>

/*---------- Define Constants ---------------------------*/
#define MAX 256
#define MAXCALLS 30 

/*---------- global variables ---------------------------*/
typedef struct classMeth classMeth;
typedef struct nativeCall   nativeCall;
typedef struct methodCall   methodCall;
typedef struct nativeMethod nativeMethod;

typedef struct nativeCompCall   nativeCompCall;
typedef struct methodCompCall   methodCompCall;
typedef struct nativeCompMethod nativeCompMethod;

int classCnt;

struct classMeth {
  int i_class;
  int j_method;
  int methCnt;
};

struct  methodCall{
        char *classname;
        char *methodname;
        char *descriptor;
} ;

struct  nativeMethod  {
        char *methodname;
        char *descriptor;
        struct methodCall methodCalls[MAXCALLS];
} ;


static struct nativeCall {
        char *classname;
//	classinfo* nclass;
        struct nativeMethod methods[MAXCALLS];
        int methCnt;
        int callCnt[MAXCALLS];
} nativeCalls[] =
 {

#include "nativecalls.h"
}
;

#define NATIVECALLSSIZE  (sizeof(nativeCalls)/sizeof(struct nativeCall))


struct  methodCompCall{
        utf *classname;
        utf *methodname;
        utf *descriptor;
} ;

struct  nativeCompMethod  {
        utf *methodname;
        utf *descriptor;
        struct methodCompCall methodCalls[MAXCALLS];
} ;


struct nativeCompCall {
        utf *classname;
        struct nativeCompMethod methods[MAXCALLS];
        int methCnt;
        int callCnt[MAXCALLS];
} nativeCompCalls[NATIVECALLSSIZE];


bool natcall2utf(bool);
void printNativeCall(nativeCall);
void markNativeMethodsRT(utf *, utf* , utf* ); 
