/* -*- mode: c; tab-width: 4; c-basic-offset: 4 -*- */
/****************************** nat/lang.c *************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the native functions for class java.lang.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at
	         Mark Probst         EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1997/06/10

*******************************************************************************/

#include <math.h>
#include <assert.h>
#include <sys/time.h>

#include "../threads/thread.h"                       /* schani */
#include "../threads/locks.h"

static void use_class_as_object (classinfo *c) 
{
	c->header.vftbl = class_java_lang_Class -> vftbl;
}


/************************************** java.lang.Object ***********************************/

struct java_lang_Class* java_lang_Object_getClass (struct java_lang_Object* this)
{
	classinfo *c = this->header.vftbl -> class;
	use_class_as_object (c);
	return (java_lang_Class*) c;
}

s4 java_lang_Object_hashCode (struct java_lang_Object* this)
{
	return ((char*) this) - ((char*) 0);	
}


struct java_lang_Object* java_lang_Object_clone (struct java_lang_Object* this)
{
	classinfo *c;
	java_lang_Object *new;

	if (((java_objectheader*)this)->vftbl->class == class_array)
	  {
	    static u4 multiplicator[10];
	    static int is_initialized = 0;

	    java_arrayheader *array = (java_arrayheader*)this;
	    u4 size;

	    if (!is_initialized)
	      {
		multiplicator[ARRAYTYPE_INT] = sizeof(s4);
		multiplicator[ARRAYTYPE_LONG] = sizeof(s8);
		multiplicator[ARRAYTYPE_FLOAT] = sizeof(float);
		multiplicator[ARRAYTYPE_DOUBLE] = sizeof(double);
		multiplicator[ARRAYTYPE_BYTE] = sizeof(s1);
		multiplicator[ARRAYTYPE_CHAR] = sizeof(u2);
		multiplicator[ARRAYTYPE_SHORT] = sizeof(s2);
		multiplicator[ARRAYTYPE_BOOLEAN] = sizeof(u1);
		multiplicator[ARRAYTYPE_OBJECT] = sizeof(void*);
		multiplicator[ARRAYTYPE_ARRAY] = sizeof(void*);
		is_initialized = 1;
	      }
	    
	    size = sizeof(java_arrayheader)
	      + array->size * multiplicator[array->arraytype];

	    new = (java_lang_Object*)heap_allocate(size, false, NULL);
	    memcpy(new, this, size);

	    return new;
	  }
	else
	  {
	    if (! builtin_instanceof ((java_objectheader*) this, class_java_lang_Cloneable) ) {
		exceptionptr = native_new_and_init (class_java_lang_CloneNotSupportedException);
		return NULL;
		}
	
	c = this -> header.vftbl -> class;
	new = (java_lang_Object*) builtin_new (c);
	if (!new) {
		exceptionptr = proto_java_lang_OutOfMemoryError;
		return NULL;
		}

	    memcpy (new, this, c->instancesize);
	    return new;
	  }
}
	
void java_lang_Object_notify (struct java_lang_Object* this)
{
	if (runverbose)
		log_text ("java_lang_Object_notify called");

#ifdef USE_THREADS
    signal_cond_for_object(&this->header);
#endif
}

void java_lang_Object_notifyAll (struct java_lang_Object* this)
{
	if (runverbose)
		log_text ("java_lang_Object_notifyAll called");

#ifdef USE_THREADS
	broadcast_cond_for_object(&this->header);
#endif
}

void java_lang_Object_wait (struct java_lang_Object* this, s8 time)
{
	if (runverbose)
		log_text ("java_lang_Object_wait called");

#ifdef USE_THREADS
	wait_cond_for_object(&this->header, time);
#endif
}


/********************************** java.lang.Class **************************************/


struct java_lang_Class* java_lang_Class_forName (struct java_lang_String* s)
{
	java_chararray *a,*b;
	u4 i;
	unicode *u;
	classinfo *c;
	
	if (!s) return NULL;
	if (!(a = s->value) ) return NULL;
	b = builtin_newarray_char (s->count);
	if (!b) return NULL;
	for (i=0; i<s->count; i++) {
		if (a->data[s->offset+i]=='.') b->data[i] = '/';
	    else                           b->data[i] = a->data[s->offset+i];
	    }
	u = unicode_new_u2 (b->data, b->header.size);
	c = u->class;
	if (!c) {
		c = loader_load (u);
		loader_initclasses ();

		if (!c) {	
			exceptionptr = 
			native_new_and_init (class_java_lang_ClassNotFoundException);
	
			return NULL;
			}

		}
	
	use_class_as_object (c);
	return (java_lang_Class*) c;
}

struct java_lang_Object* java_lang_Class_newInstance (struct java_lang_Class* this)
{
	java_objectheader *o = native_new_and_init ((classinfo*) this);
	if (!o) {
		exceptionptr = 
			native_new_and_init (class_java_lang_InstantiationException);
		}
	return (java_lang_Object*) o;
}

struct java_lang_String* java_lang_Class_getName (struct java_lang_Class* this)
{
	u4 i;
	classinfo *c = (classinfo*) this;
	java_lang_String *s = (java_lang_String*) javastring_new(c->name);

	if (!s) return NULL;

	for (i=0; i<s->value->header.size; i++) {
		if (s->value->data[i] == '/') s->value->data[i] = '.';
		}
	
	return s;
}

struct java_lang_Class* java_lang_Class_getSuperclass (struct java_lang_Class* this)
{
	classinfo *c = ((classinfo*) this) -> super;
	if (!c) return NULL;

	use_class_as_object (c);
	return (java_lang_Class*) c;
}

java_objectarray* java_lang_Class_getInterfaces (struct java_lang_Class* this)
{
	classinfo *c = (classinfo*) this;
	u4 i;
	java_objectarray *a = builtin_anewarray (c->interfacescount, class_java_lang_Class);
	if (!a) return NULL;
	for (i=0; i<c->interfacescount; i++) {
		use_class_as_object (c->interfaces[i]);

		a->data[i] = (java_objectheader*) c->interfaces[i];
		}
	return a;
}

struct java_lang_ClassLoader* java_lang_Class_getClassLoader (struct java_lang_Class* this)
{
	log_text ("java_lang_Class_getClassLoader called");
	return NULL;
}

s4 java_lang_Class_isInterface (struct java_lang_Class* this)
{
	classinfo *c = (classinfo*) this;
	if (c->flags & ACC_INTERFACE) return 1;
	return 0;
}

/************************************ java.lang.ClassLoader *******************************/


struct java_lang_Class* java_lang_ClassLoader_defineClass (struct java_lang_ClassLoader* this, java_bytearray* par1, s4 par2, s4 par3)
{
	log_text ("java_lang_ClassLoader_defineClass called");
	return NULL;
}
void java_lang_ClassLoader_resolveClass (struct java_lang_ClassLoader* this, struct java_lang_Class* par1)
{
	log_text ("java_lang_ClassLoader_resolveClass called");
}
struct java_lang_Class* java_lang_ClassLoader_findSystemClass (struct java_lang_ClassLoader* this, struct java_lang_String* par1)
{
	log_text ("java_lang_ClassLoader_findSystemClass called");
	return NULL;
}	
void java_lang_ClassLoader_init (struct java_lang_ClassLoader* this)
{
	log_text ("java_lang_ClassLoader_init called");
}
struct java_lang_Class* java_lang_ClassLoader_findSystemClass0 (struct java_lang_ClassLoader* this, struct java_lang_String* par1)
{
	log_text ("java_lang_ClassLoader_findSystemClass0 called");
	return NULL;
}

struct java_lang_Class* java_lang_ClassLoader_defineClass0 (struct java_lang_ClassLoader* this, java_bytearray* par1, s4 par2, s4 par3)
{
	log_text ("java_lang_ClassLoader_defineClass0 called");
	return NULL;
}
void java_lang_ClassLoader_resolveClass0 (struct java_lang_ClassLoader* this, struct java_lang_Class* par1)
{
	log_text ("java_lang_ClassLoader_resolveClass0 called");
	return;
}

/************************************** java.lang.Compiler  *******************************/

void java_lang_Compiler_initialize ()
{
	log_text ("java_lang_Compiler_initialize called");
}
s4 java_lang_Compiler_compileClass (struct java_lang_Class* par1) 
{
	log_text ("java_lang_Compiler_compileClass called");
	return 0;
}
s4 java_lang_Compiler_compileClasses (struct java_lang_String* par1)
{
	log_text ("java_lang_Compiler_compileClasses called");
	return 0;
}
struct java_lang_Object* java_lang_Compiler_command (struct java_lang_Object* par1)
{
	log_text ("java_lang_Compiler_command called");
	return NULL;
}
void java_lang_Compiler_enable ()
{
	log_text ("java_lang_Compiler_enable called");
}
void java_lang_Compiler_disable ()
{
	log_text ("java_lang_Compiler_disable called");
}


/******************************** java.lang.Double **************************************/

struct java_lang_String* java_lang_Double_toString (double par1)
{
	char b[400];
	sprintf (b, "%-.6g", par1);
	return (java_lang_String*) javastring_new_char (b);
}
struct java_lang_Double* java_lang_Double_valueOf (struct java_lang_String* par1)
{	
	float val;
	java_lang_Double *d = (java_lang_Double*) builtin_new (class_java_lang_Double);
	if (d) {	
		sscanf (javastring_tochar((java_objectheader*) par1), "%f", &val);
		d->value = val;
		return d;
	}
	return NULL;
}
s8 java_lang_Double_doubleToLongBits (double par1)
{
	s8 l;
	double d = par1;
	memcpy ((u1*) &l, (u1*) &d, 8);
	return l;
}
double java_lang_Double_longBitsToDouble (s8 par1)
{
	s8 l = par1;
	double d;
	memcpy ((u1*) &d, (u1*) &l, 8);
	return d;
}

/******************************** java.lang.Float ***************************************/

struct java_lang_String* java_lang_Float_toString (float par1)
{
	char b[50];
	sprintf (b, "%-.6g", (double) par1);
	return (java_lang_String*) javastring_new_char (b);
}
struct java_lang_Float* java_lang_Float_valueOf (struct java_lang_String* par1)
{
	float val;
	java_lang_Float *d = (java_lang_Float*) builtin_new (class_java_lang_Float);
	if (d) {	
		sscanf (javastring_tochar((java_objectheader*) par1), "%f", &val);
		d->value = val;
		return d;
	}
	return NULL;
}
s4 java_lang_Float_floatToIntBits (float par1)
{
	s4 i;
	float f = par1;
	memcpy ((u1*) &i, (u1*) &f, 4);
	return i;
}
float java_lang_Float_intBitsToFloat (s4 par1)
{
	s4 i = par1;
	float f;
	memcpy ((u1*) &f, (u1*) &i, 4);
	return f;
}


/******************************** java.lang.Math ****************************************/

double java_lang_Math_sin (double par1)
{
	return sin(par1);
}

double java_lang_Math_cos (double par1)
{
	return cos(par1);
}

double java_lang_Math_tan (double par1)
{
	return tan(par1);
}

double java_lang_Math_asin (double par1)
{
	return asin(par1);
}

double java_lang_Math_acos (double par1)
{
	return acos(par1);
}

double java_lang_Math_atan (double par1)
{
	return atan(par1);
}

double java_lang_Math_exp (double par1)
{
	return exp(par1);
}

double java_lang_Math_log (double par1)
{
	if (par1<0.0) {
		exceptionptr = proto_java_lang_ArithmeticException;
		return 0.0;
		}
	return log(par1);
}

double java_lang_Math_sqrt (double par1)
{
	if (par1<0.0) {
		exceptionptr = proto_java_lang_ArithmeticException;
		return 0.0;
		}
	return sqrt(par1);
}

static u8 dbl_nan    = 0xffffffffffffffffL ;

#define DBL_NAN    (*((double*) (&dbl_nan)))

double java_lang_Math_IEEEremainder (double a, double b)
{
	double d;
	if (finite(a) && finite(b)) {
		d = a / b;
		if (finite(d))
			return a - floor(d) * b;
		return DBL_NAN;
		}
	if (isnan(b))
		return DBL_NAN;
	if (finite(a))
		return a;
	return DBL_NAN;
}

double java_lang_Math_ceil (double par1)
{
	return ceil(par1);
}

double java_lang_Math_floor (double par1)
{
	return floor(par1);
}

double java_lang_Math_rint (double par1)
{
	panic ("native Methode java_lang_rint not implemented yet");
	return 0.0;
}

double java_lang_Math_atan2 (double par1, double par2)
{
	return atan2(par1,par2);
}

double java_lang_Math_pow (double par1, double par2)
{
	return pow(par1,par2);
}


/******************************* java.lang.Runtime **************************************/

void java_lang_Runtime_exitInternal (struct java_lang_Runtime* this, s4 par1)
{
	cacao_shutdown (par1);
}
struct java_lang_Process* java_lang_Runtime_execInternal (struct java_lang_Runtime* this, java_objectarray* par1, java_objectarray* par2)
{
	log_text ("java_lang_Runtime_execInternal called");
	return NULL;
}
s8 java_lang_Runtime_freeMemory (struct java_lang_Runtime* this)
{
	log_text ("java_lang_Runtime_freeMemory called");
	return builtin_i2l (0);
}
s8 java_lang_Runtime_totalMemory (struct java_lang_Runtime* this)
{
	log_text ("java_lang_Runtime_totalMemory called");
	return builtin_i2l (0);
}
void java_lang_Runtime_gc (struct java_lang_Runtime* this)
{
	gc_call();
}
void java_lang_Runtime_runFinalization (struct java_lang_Runtime* this)
{
	log_text ("java_lang_Runtime_runFinalization called");
}
void java_lang_Runtime_traceInstructions (struct java_lang_Runtime* this, s4 par1)
{
	log_text ("java_lang_Runtime_traceInstructions called");
}
void java_lang_Runtime_traceMethodCalls (struct java_lang_Runtime* this, s4 par1)
{
	log_text ("java_lang_Runtime_traceMethodCalls called");
}
struct java_lang_String* java_lang_Runtime_initializeLinkerInternal (struct java_lang_Runtime* this)
{	
	log_text ("java_lang_Runtime_initializeLinkerInternal called");
	return (java_lang_String*)javastring_new_char(".");
}
struct java_lang_String* java_lang_Runtime_buildLibName (struct java_lang_Runtime* this, struct java_lang_String* par1, struct java_lang_String* par2)
{
	log_text ("java_lang_Runtime_buildLibName called");
	return NULL;
}
s4 java_lang_Runtime_loadFileInternal (struct java_lang_Runtime* this, struct java_lang_String* par1)
{
	log_text ("java_lang_Runtime_loadFileInternal called");
	return 1;
}


/**************************************** java.lang.SecurityManager ***********************/

java_objectarray* java_lang_SecurityManager_getClassContext (struct java_lang_SecurityManager* this)
{
	log_text ("called: java_lang_SecurityManager_getClassContext");
	return NULL;
}
struct java_lang_ClassLoader* java_lang_SecurityManager_currentClassLoader (struct java_lang_SecurityManager* this)
{
	log_text ("called: java_lang_SecurityManager_currentClassLoader");
	return NULL;
}
s4 java_lang_SecurityManager_classDepth (struct java_lang_SecurityManager* this, struct java_lang_String* par1)
{
	log_text ("called: java_lang_SecurityManager_classDepth");
	return 0;
}
s4 java_lang_SecurityManager_classLoaderDepth (struct java_lang_SecurityManager* this)
{
	log_text ("called: java_lang_SecurityManager_classLoaderDepth");
	return 0;
}



/*********************************** java.lang.System ************************************/

s8 java_lang_System_currentTimeMillis ()
{
	struct timeval tv;

	(void) gettimeofday(&tv, NULL);
	return ((s8) tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}


void java_lang_System_arraycopy (struct java_lang_Object* source, s4 sp, 
                                 struct java_lang_Object* dest, s4 dp, s4 len)
{
	s4 i;
	java_arrayheader *s = (java_arrayheader*) source;
	java_arrayheader *d = (java_arrayheader*) dest;

	if ( (!s) || (!d) ) { 
		exceptionptr = proto_java_lang_NullPointerException; 
		return; 
		}
	if ( (s->objheader.vftbl->class != class_array) || (d->objheader.vftbl->class != class_array) ) {
		exceptionptr = proto_java_lang_ArrayStoreException; 
		return; 
		}
		
	if (s->arraytype != d->arraytype) {
		exceptionptr = proto_java_lang_ArrayStoreException; 
		return; 
		}

	if ((sp<0) || (sp+len > s->size) || (dp<0) || (dp+len > d->size)) {
		exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException; 
		return; 
		}

	switch (s->arraytype) {
	case ARRAYTYPE_BYTE:
		{
		java_bytearray *bas = (java_bytearray*) s;
		java_bytearray *bad = (java_bytearray*) d;
		if (dp<=sp) 
			for (i=0; i<len; i++) bad->data[dp+i] = bas->data[sp+i];
		else 
			for (i=len-1; i>=0; i--) bad->data[dp+i] = bas->data[sp+i];
		}
		break;
	case ARRAYTYPE_BOOLEAN:
		{
		java_booleanarray *bas = (java_booleanarray*) s;
		java_booleanarray *bad = (java_booleanarray*) d;
		if (dp<=sp) 
			for (i=0; i<len; i++) bad->data[dp+i] = bas->data[sp+i];
		else 
			for (i=len-1; i>=0; i--) bad->data[dp+i] = bas->data[sp+i];
		}
		break;
	case ARRAYTYPE_CHAR:
		{
		java_chararray *cas = (java_chararray*) s;
		java_chararray *cad = (java_chararray*) d;
		if (dp<=sp) 
			for (i=0; i<len; i++) cad->data[dp+i] = cas->data[sp+i];
		else 
			for (i=len-1; i>=0; i--) cad->data[dp+i] = cas->data[sp+i];
		}
		break;
	case ARRAYTYPE_SHORT:
		{
		java_shortarray *sas = (java_shortarray*) s;
		java_shortarray *sad = (java_shortarray*) d;
		if (dp<=sp) 
			for (i=0; i<len; i++) sad->data[dp+i] = sas->data[sp+i];
		else
			for (i=len-1; i>=0; i--) sad->data[dp+i] = sas->data[sp+i];
		}
		break;
	case ARRAYTYPE_INT:
		{
		java_intarray *ias = (java_intarray*) s;
		java_intarray *iad = (java_intarray*) d;
		if (dp<=sp) 
			for (i=0; i<len; i++) iad->data[dp+i] = ias->data[sp+i];
		else
			for (i=len-1; i>=0; i--) iad->data[dp+i] = ias->data[sp+i];
		}
		break;
	case ARRAYTYPE_LONG:
		{
		java_longarray *las = (java_longarray*) s;
		java_longarray *lad = (java_longarray*) d;
		if (dp<=sp) 
			for (i=0; i<len; i++) lad->data[dp+i] = las->data[sp+i];
		else
			for (i=len-1; i>=0; i--) lad->data[dp+i] = las->data[sp+i];
		}
		break;
	case ARRAYTYPE_FLOAT:
		{
		java_floatarray *fas = (java_floatarray*) s;
		java_floatarray *fad = (java_floatarray*) d;
		if (dp<=sp) 
			for (i=0; i<len; i++) fad->data[dp+i] = fas->data[sp+i];
		else
			for (i=len-1; i>=0; i--) fad->data[dp+i] = fas->data[sp+i];
		}
		break;
	case ARRAYTYPE_DOUBLE:
		{
		java_doublearray *das = (java_doublearray*) s;
		java_doublearray *dad = (java_doublearray*) d;
		if (dp<=sp) 
			for (i=0; i<len; i++) dad->data[dp+i] = das->data[sp+i];
		else
			for (i=len-1; i>=0; i--) dad->data[dp+i] = das->data[sp+i];
		}
		break;
	case ARRAYTYPE_OBJECT:
		{
		java_objectarray *oas = (java_objectarray*) s;
		java_objectarray *oad = (java_objectarray*) d;
		if (dp<=sp) 
			for (i=0; i<len; i++) {
				java_objectheader *o = oas->data[sp+i];
				if (!builtin_canstore(oad, o)) {
					exceptionptr = proto_java_lang_ArrayStoreException;
					return;
					}
				oad->data[dp+i] = o;
				}
		else 
			for (i=len-1; i>=0; i--) {
				java_objectheader *o = oas->data[sp+i];
				if (!builtin_canstore(oad, o)) {
					exceptionptr = proto_java_lang_ArrayStoreException;
					return;
					}
				oad->data[dp+i] = o;
				}
		
		}
		break;
	case ARRAYTYPE_ARRAY:
		{
		java_arrayarray *aas = (java_arrayarray*) s;
		java_arrayarray *aad = (java_arrayarray*) d;
		if (dp<=sp) 
			for (i=0; i<len; i++) {
				java_arrayheader *o = aas->data[sp+i];
				if (!builtin_canstore( (java_objectarray*)aad, 
				        (java_objectheader*)o )) {
					exceptionptr = proto_java_lang_ArrayStoreException;
					return;
					}
				aad->data[dp+i] = o;
				}
		else
			for (i=len-1; i>=0; i--) {
				java_arrayheader *o = aas->data[sp+i];
				if (!builtin_canstore( (java_objectarray*)aad, 
				        (java_objectheader*)o )) {
					exceptionptr = proto_java_lang_ArrayStoreException;
					return;
					}
				aad->data[dp+i] = o;
				}

		}
		break;

	default:
		panic ("Unknown data type for arraycopy");
	}

}


#define MAXPROPS 100
static int activeprops = 15;

static char *proplist[MAXPROPS][2] = {
	{ "java.class.path", NULL },
	{ "java.home", NULL }, 
	{ "user.home", NULL }, 
	{ "user.name", NULL }, 
	{ "user.dir",  NULL }, 
	
	{ "java.class.version", "45.3" },
	{ "java.version", "cacao:0.2" },
	{ "java.vendor", "CACAO Team" },
	{ "java.vendor.url", "http://www.complang.tuwien.ac.at/java/cacao/" },
	{ "os.arch", "Alpha" },
	{ "os.name", "Linux/Digital Unix" },
	{ "os.version", "4.0/3.2C/V4.0" },
	{ "path.separator", ":" },
	{ "file.separator", "/" },
	{ "line.separator", "\n" }
};	

void attach_property (char *name, char *value)
{
	if (activeprops >= MAXPROPS) panic ("Too many properties defined");
	proplist[activeprops][0] = name;
	proplist[activeprops][1] = value;
	activeprops++;
}


struct java_util_Properties* java_lang_System_initProperties (struct java_util_Properties* p)
{
	u4 i;
	methodinfo *m;
#define BUFFERSIZE 200
	char buffer[BUFFERSIZE];
	
	proplist[0][1] = classpath;
	proplist[1][1] = getenv("JAVAHOME");
	proplist[2][1] = getenv("HOME");
	proplist[3][1] = getenv("USER");
	proplist[4][1] = getcwd(buffer,BUFFERSIZE);
	
	if (!p) panic ("initProperties called with NULL-Argument");

	m = class_resolvemethod (
		p->header.vftbl->class, 
		unicode_new_char ("put"), 
		unicode_new_char ("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;")
    	);
    if (!m) panic ("Can not find method 'put' for class Properties");
    
    for (i=0; i<activeprops; i++) {
    	if (proplist[i][1]==NULL) proplist[i][1]="";
    	    	
    	asm_calljavamethod (m,  p, 
    	                        javastring_new_char(proplist[i][0]),
    	                        javastring_new_char(proplist[i][1]),  
    	                   		NULL
    	                   );
    	}

	return p;
}




/*********************************** java.lang.Thread ***********************************/

struct java_lang_Thread* java_lang_Thread_currentThread ()
{
  if (runverbose)
    log_text ("java_lang_Thread_currentThread called");
#ifdef USE_THREADS
	return (struct java_lang_Thread*)currentThread;
#else
	return 0;
#endif
}

void java_lang_Thread_yield ()
{
  if (runverbose)
    log_text ("java_lang_Thread_yield called");
#ifdef USE_THREADS
	yieldThread();
#endif
}

void java_lang_Thread_sleep (s8 par1)
{
  if (runverbose)
    log_text ("java_lang_Thread_sleep called");
#ifdef USE_THREADS
	yieldThread();
#endif
	/* not yet implemented */
}

void java_lang_Thread_start (struct java_lang_Thread* this)
{
  if (runverbose)
    log_text ("java_lang_Thread_start called");
#ifdef USE_THREADS
	startThread((thread*)this);
#endif
}

s4 java_lang_Thread_isAlive (struct java_lang_Thread* this)
{
  if (runverbose)
    log_text ("java_lang_Thread_isAlive called");
#ifdef USE_THREADS
	return aliveThread((thread*)this);
#else
	return 0;
#endif
}

s4 java_lang_Thread_countStackFrames (struct java_lang_Thread* this)
{
  log_text ("java_lang_Thread_countStackFrames called");
  return 0;         /* not yet implemented */
}

void java_lang_Thread_setPriority0 (struct java_lang_Thread* this, s4 par1)
{
  if (runverbose)
    log_text ("java_lang_Thread_setPriority0 called");
#ifdef USE_THREADS
  setPriorityThread((thread*)this, par1);
#endif
}

void java_lang_Thread_stop0 (struct java_lang_Thread* this, struct java_lang_Object* par1)
{
  if (runverbose)
    log_text ("java_lang_Thread_stop0 called");
#ifdef USE_THREADS
	if (currentThread == (thread*)this)
	{
	    log_text("killing");
	    killThread(0);
	    /*
	        exceptionptr = proto_java_lang_ThreadDeath;
	        return;
	    */
	}
	else
	{
	        CONTEXT((thread*)this).flags |= THREAD_FLAGS_KILLED;
	        resumeThread((thread*)this);
	}
#endif
}

void java_lang_Thread_suspend0 (struct java_lang_Thread* this)
{
  if (runverbose)
    log_text ("java_lang_Thread_suspend0 called");
#ifdef USE_THREADS
	suspendThread((thread*)this);
#endif
}

void java_lang_Thread_resume0 (struct java_lang_Thread* this)
{
  if (runverbose)
    log_text ("java_lang_Thread_resume0 called");
#ifdef USE_THREADS
	resumeThread((thread*)this);
#endif
}



/************************************ java.lang.Throwable *********************************/

void java_lang_Throwable_printStackTrace0 (struct java_lang_Throwable* this, struct java_io_PrintStream* par1)
{
	log_text ("java_lang_Throwable_printStackTrace0 called");
	return;
}

struct java_lang_Throwable* java_lang_Throwable_fillInStackTrace (struct java_lang_Throwable *this)
{
	this -> detailMessage = 
	  (java_lang_String*) (javastring_new_char ("no backtrace info!") );
	return this;
}

