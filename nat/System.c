/* class: java/lang/System */

/*
 * Class:     java/lang/System
 * Method:    currentTimeMillis
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_System_currentTimeMillis ( JNIEnv *env )
{
	struct timeval tv;

	(void) gettimeofday(&tv, NULL);
	return ((s8) tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

/*
 * Class:     java/lang/System
 * Method:    arraycopy
 * Signature: (Ljava/lang/Object;ILjava/lang/Object;II)V
 */
JNIEXPORT void JNICALL Java_java_lang_System_arraycopy (JNIEnv *env, struct java_lang_Object* source, s4 sp, struct java_lang_Object* dest, s4 dp, s4 len)
{
	s4 i;
	java_arrayheader *s = (java_arrayheader*) source;
	java_arrayheader *d = (java_arrayheader*) dest;

	if (((s == NULL) | (d == NULL)) != 0) { 
		exceptionptr = proto_java_lang_NullPointerException; 
		return; 
		}

	if (s->objheader.vftbl->class != class_array) {
		exceptionptr = proto_java_lang_ArrayStoreException; 
		return; 
		}

	if (((sp<0) | (sp+len > s->size) | (dp<0) | (dp+len > d->size)) != 0) {
		exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException; 
		return; 
		}

	switch (s->arraytype) {
	case ARRAYTYPE_BYTE:
		if (s->objheader.vftbl != d->objheader.vftbl) {
			exceptionptr = proto_java_lang_ArrayStoreException; 
			return; 
			}
		memmove(((java_bytearray*) d)->data + dp,
		        ((java_bytearray*) s)->data + sp,
		        (size_t) len);
		return;
	case ARRAYTYPE_BOOLEAN:
		if (s->objheader.vftbl != d->objheader.vftbl) {
			exceptionptr = proto_java_lang_ArrayStoreException; 
			return; 
			}
		memmove(((java_booleanarray*) d)->data + dp,
		        ((java_booleanarray*) s)->data + sp,
		        (size_t) len);
		return;
	case ARRAYTYPE_CHAR:
		if (s->objheader.vftbl != d->objheader.vftbl) {
			exceptionptr = proto_java_lang_ArrayStoreException; 
			return; 
			}
		memmove(((java_chararray*) d)->data + dp,
		        ((java_chararray*) s)->data + sp,
		        (size_t) len * sizeof(u2));
		return;
	case ARRAYTYPE_SHORT:
		if (s->objheader.vftbl != d->objheader.vftbl) {
			exceptionptr = proto_java_lang_ArrayStoreException; 
			return; 
			}
		memmove(((java_shortarray*) d)->data + dp,
		        ((java_shortarray*) s)->data + sp,
		        (size_t) len * sizeof(s2));
		return;
	case ARRAYTYPE_INT:
		if (s->objheader.vftbl != d->objheader.vftbl) {
			exceptionptr = proto_java_lang_ArrayStoreException; 
			return; 
			}
		memmove(((java_intarray*) d)->data + dp,
		        ((java_intarray*) s)->data + sp,
		        (size_t) len * sizeof(s4));
		return;
	case ARRAYTYPE_LONG:
		if (s->objheader.vftbl != d->objheader.vftbl) {
			exceptionptr = proto_java_lang_ArrayStoreException; 
			return; 
			}
		memmove(((java_longarray*) d)->data + dp,
		        ((java_longarray*) s)->data + sp,
		        (size_t) len * sizeof(s8));
		return;
	case ARRAYTYPE_FLOAT:
		if (s->objheader.vftbl != d->objheader.vftbl) {
			exceptionptr = proto_java_lang_ArrayStoreException; 
			return; 
			}
		memmove(((java_floatarray*) d)->data + dp,
		        ((java_floatarray*) s)->data + sp,
		        (size_t) len * sizeof(float));
		return;
	case ARRAYTYPE_DOUBLE:
		if (s->objheader.vftbl != d->objheader.vftbl) {
			exceptionptr = proto_java_lang_ArrayStoreException; 
			return; 
			}
		memmove(((java_doublearray*) d)->data + dp,
		        ((java_doublearray*) s)->data + sp,
		        (size_t) len * sizeof(double));
		return;
	case ARRAYTYPE_OBJECT:
		{
		java_objectarray *oas = (java_objectarray*) s;
		java_objectarray *oad = (java_objectarray*) d;

		if (d->objheader.vftbl->class != class_array) {
			exceptionptr = proto_java_lang_ArrayStoreException; 
			return; 
			}
		if (s->arraytype != d->arraytype) {
			exceptionptr = proto_java_lang_ArrayStoreException; 
			return; 
			}

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

		if (d->objheader.vftbl->class != class_array) {
			exceptionptr = proto_java_lang_ArrayStoreException; 
			return; 
			}
		if (s->arraytype != d->arraytype) {
			exceptionptr = proto_java_lang_ArrayStoreException; 
			return; 
			}

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
	{ "java.version", "cacao:0.3" },
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

/*
 * Class:     java/lang/System
 * Method:    initProperties
 * Signature: (Ljava/util/Properties;)Ljava/util/Properties;
 */
JNIEXPORT struct java_util_Properties* JNICALL Java_java_lang_System_initProperties (JNIEnv *env,struct java_util_Properties* p)
{
	#define BUFFERSIZE 200
	u4 i;
	methodinfo *m;
	char buffer[BUFFERSIZE];
	java_objectheader *o;
	
	proplist[0][1] = classpath;
	proplist[1][1] = getenv("JAVA_HOME");
	proplist[2][1] = getenv("HOME");
	proplist[3][1] = getenv("USER");
	proplist[4][1] = getcwd(buffer,BUFFERSIZE);
	
	if (!p) panic ("initProperties called with NULL-Argument");

	/* search for method to add properties */
	m = class_resolvemethod (
		p->header.vftbl->class, 
		utf_new_char ("put"), 
		utf_new_char ("(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;")
    	);

	if (!m) panic ("Can not find method 'put' for class Properties");
       
       	/* add the properties */
	for (i=0; i<activeprops; i++) {

	    if (proplist[i][1]==NULL) proplist[i][1]=""; 
       	
	    asm_calljavamethod(m,  p, 
    	                        javastring_new_char(proplist[i][0]),
    	                        javastring_new_char(proplist[i][1]),  
    	                   		NULL
				);
    	}

	return p;

}


JNIEXPORT void JNICALL Java_java_lang_System_registerNatives (JNIEnv *env )
{
    /* empty */
}

/*
 * Class:     java/lang/System
 * Method:    setErr0
 * Signature: (Ljava/io/PrintStream;)V
 */
JNIEXPORT void JNICALL Java_java_lang_System_setErr0 (JNIEnv *env , struct java_io_PrintStream* stream)
{
    /* set static field 'err' of class java.lang.System */	 
	
    jfieldID fid =
        env->GetStaticFieldID(env,class_java_lang_System,"err","Ljava/io/PrintStream;");

    if (!fid) panic("unable to access static field of class System");

    env->SetStaticObjectField(env,class_java_lang_System,fid,(jobject) stream);
}

/*
 * Class:     java/lang/System
 * Method:    setIn0
 * Signature: (Ljava/io/InputStream;)V
 */
JNIEXPORT void JNICALL Java_java_lang_System_setIn0 (JNIEnv *env , struct java_io_InputStream* stream)
{
    /* set static field 'in' of class java.lang.System */	 
	
    jfieldID fid =
        env->GetStaticFieldID(env,class_java_lang_System,"in","Ljava/io/InputStream;");

    if (!fid) panic("unable to access static field of class System");

    env->SetStaticObjectField(env,class_java_lang_System,fid,(jobject) stream);
}

/*
 * Class:     java/lang/System
 * Method:    setOut0
 * Signature: (Ljava/io/PrintStream;)V
 */
JNIEXPORT void JNICALL Java_java_lang_System_setOut0 (JNIEnv *env , struct java_io_PrintStream* stream)
{
    /* set static field 'out' of class java.lang.System */	 	
	
    jfieldID fid =
        env->GetStaticFieldID(env,class_java_lang_System,"out","Ljava/io/PrintStream;");

    if (!fid) panic("unable to access static field of class System");

    env->SetStaticObjectField(env,class_java_lang_System,fid,(jobject) stream);
}

/*
 * Class:     java/lang/System
 * Method:    mapLibraryName
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_System_mapLibraryName (JNIEnv *env , struct java_lang_String* s)
{
  char somefile[MAXSTRINGSIZE];
  char *java_home;

  /* return name of any file that exists (relative to root),
     so ClassLoader believes we dynamically load the native library */ 

  if (strlen(classpath)+24>MAXSTRINGSIZE)
    panic("filename too long");

  java_home = getenv("JAVA_HOME");
  if (java_home == 0)
      java_home = "/tmp";
  strcpy(somefile,java_home);
  strcat(somefile,"/dummy");

  return (java_lang_String* ) javastring_new_char(&somefile[1]);
}



/*
 * Class:     java/lang/System
 * Method:    getCallerClass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_System_getCallerClass (JNIEnv *env )
{
  /* determine the callerclass by getting the method which called getCallerClass */
  classinfo *c;
  methodinfo *m = asm_getcallingmethod(); 

  if (m && (c = m->class)) {
    	  use_class_as_object (c);
	  return (java_lang_Class*) c;
  }

  /* caller class could not be determined */
  return NULL;
}

/*
 * Class:     java/lang/System
 * Method:    identityHashCode
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_System_identityHashCode (JNIEnv *env , struct java_lang_Object* par1)
{
	return ((char*) par1) - ((char*) 0);	
}




