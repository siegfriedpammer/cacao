/* class: java/lang/Class */


/* for selecting public members */
#define MEMBER_PUBLIC  0


/****************************************************************************************** 											   			

	creates method signature (excluding return type) from array of 
	class-objects representing the parameters of the method 

*******************************************************************************************/


utf *create_methodsig(java_objectarray* types, char *retType)
{
    char *buffer;       /* buffer for building the desciptor */
    char *pos;          /* current position in buffer */
    utf *result;        /* the method signature */
    u4 buffer_size = 3; /* minimal size=3: room for parenthesis and returntype */
    u4 len = 0;		/* current length of the descriptor */
    u4 i,j,n;
 
    if (!types) return NULL;

    /* determine required buffer-size */    
    for (i=0;i<types->header.size;i++) {
      classinfo *c = (classinfo *) types->data[i];
      buffer_size  = buffer_size + c->name->blength+2;
    }

    if (retType) buffer_size+=strlen(retType);

    /* allocate buffer */
    buffer = MNEW(u1, buffer_size);
    pos    = buffer;
    
    /* method-desciptor starts with parenthesis */
    *pos++ = '(';

    for (i=0;i<types->header.size;i++) {

            char ch;	   
            /* current argument */
	    classinfo *c = (classinfo *) types->data[i];
	    /* current position in utf-text */
	    char *utf_ptr = c->name->text; 
	    
	    /* determine type of argument */
	    if ( (ch = utf_nextu2(&utf_ptr)) == '[' ) {
	
	    	/* arrayclass */
	        for ( utf_ptr--; utf_ptr<utf_end(c->name); utf_ptr++)
		   *pos++ = *utf_ptr; /* copy text */

	    } else
	    {	   	
	      	/* check for primitive types */
		for (j=0; j<PRIMITIVETYPE_COUNT; j++) {

			char *utf_pos	= utf_ptr-1;
			char *primitive = primitivetype_table[j].wrapname;

			/* compare text */
			while (utf_pos<utf_end(c->name))
		   		if (*utf_pos++ != *primitive++) goto nomatch;

			/* primitive type found */
			*pos++ = primitivetype_table[j].typesig;
			goto next_type;

		nomatch:
		}

		/* no primitive type and no arrayclass, so must be object */
	      	*pos++ = 'L';
	      	/* copy text */
	        for ( utf_ptr--; utf_ptr<utf_end(c->name); utf_ptr++)
		   	*pos++ = *utf_ptr;
	      	*pos++ = ';';

		next_type:
	    }  
    }	    

    *pos++ = ')';

    if (retType) {
	for (i=0;i<strlen(retType);i++) {
		*pos++=retType[i];
	}
    }
    /* create utf-string */
    result = utf_new(buffer,(pos-buffer));
    MFREE(buffer, u1, buffer_size);

    return result;
}

/******************************************************************************************

	retrieve the next argument or returntype from a descriptor
	and return the corresponding class 

*******************************************************************************************/


classinfo *get_type(char **utf_ptr,char *desc_end, bool skip)
{
    classinfo *c = class_from_descriptor(*utf_ptr,desc_end,utf_ptr,
                                         (skip) ? CLASSLOAD_SKIP : CLASSLOAD_LOAD);
    if (!c)
	/* unknown type */
	panic("illegal descriptor");

    if (skip) return NULL;

    use_class_as_object(c);
    return c;
}

/******************************************************************************************

	use the descriptor of a method to generate a java/lang/Class array
	which contains the classes of the parametertypes of the method

*******************************************************************************************/

java_objectarray* get_parametertypes(methodinfo *m) 
{
    utf  *descr    =  m->descriptor;    /* method-descriptor */ 
    char *utf_ptr  =  descr->text;      /* current position in utf-text */
    char *desc_end =  utf_end(descr);   /* points behind utf string     */
    java_objectarray* result;
    int parametercount = 0;
    int i;

    /* skip '(' */
    utf_nextu2(&utf_ptr);
  
    /* determine number of parameters */
    while ( *utf_ptr != ')' ) {
    	get_type(&utf_ptr,desc_end,true);
	parametercount++;
    }

    /* create class-array */
    result = builtin_anewarray(parametercount, class_java_lang_Class);

    utf_ptr  =  descr->text;
    utf_nextu2(&utf_ptr);

    /* get returntype classes */
    for (i = 0; i < parametercount; i++)
	    result->data[i] = (java_objectheader *) get_type(&utf_ptr,desc_end, false);

    return result;
}


/******************************************************************************************

	get the returntype class of a method

*******************************************************************************************/

classinfo *get_returntype(methodinfo *m) 
{
	char *utf_ptr;   /* current position in utf-text */
	char *desc_end;  /* points behind utf string     */
        utf *desc = m->descriptor; /* method-descriptor  */

	utf_ptr  = desc->text;
	desc_end = utf_end(desc);

	/* ignore parametertypes */
        while ((utf_ptr<desc_end) && utf_nextu2(&utf_ptr)!=')')
		/* skip */ ;

	return get_type(&utf_ptr,desc_end, false);
}


/*
 * Class:     java_lang_VMClass
 * Method:    forName
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClass_forName (JNIEnv *env , jclass clazz,struct java_lang_String* s)
{
	classinfo *c;
	utf *u;
	u4 i;

	if (runverbose)
	{
	    log_text("Java_java_lang_VMClass_forName0 called");
	    log_text(javastring_tochar((java_objectheader*)s));
	}

	/* illegal argument */
	if (!s) return NULL;
	
        /* create utf string in which '.' is replaced by '/' */
        u = javastring_toutf(s, true);
        
        if ( !(c = class_get(u)) ) {
            methodinfo *method;
            java_lang_Class *class;
            
            log_text("forName: would need classloader");
            /*utf_display(u);*/
            
            c = loader_load(u);
            if (c == NULL)
            {
                /* class was not loaded. raise exception */
                exceptionptr = 
                    native_new_and_init (class_java_lang_ClassNotFoundException);
                return NULL;
            }
        }

	use_class_as_object (c);
	return (java_lang_Class*) c;
}

/*
 * Class:     java_lang_VMClass
 * Method:    getClassLoader
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT struct java_lang_ClassLoader* JNICALL Java_java_lang_VMClass_getClassLoader (JNIEnv *env ,  struct java_lang_VMClass* this )
{  
  init_systemclassloader();
  return SystemClassLoader;
}

/*
 * Class:     java_lang_VMClass
 * Method:    getModifiers
 * Signature: ()I
 */
/* XXX delete */
#if 0
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClass_getComponentType (JNIEnv *env ,  struct java_lang_VMClass* this )
{
  classinfo *c = NULL; 

  if ((classinfo*) (this->vmData) == class_array) {
	java_arrayheader *a = (java_arrayheader*) (this->vmData);

	/* determine componenttype */
	switch (a->arraytype) {
	   case ARRAYTYPE_BYTE:    c = class_java_lang_Byte; break;  
           case ARRAYTYPE_BOOLEAN: c = class_java_lang_Boolean; break;   
	   case ARRAYTYPE_CHAR:    c = class_java_lang_Character; break;   
 	   case ARRAYTYPE_SHORT:   c = class_java_lang_Short; break;    
	   case ARRAYTYPE_INT:     c = class_java_lang_Integer; break;   
	   case ARRAYTYPE_LONG:    c = class_java_lang_Long; break;   
	   case ARRAYTYPE_FLOAT:   c = class_java_lang_Float; break;   
	   case ARRAYTYPE_DOUBLE:  c = class_java_lang_Double; break;   
	   case ARRAYTYPE_OBJECT:  c = ((java_objectarray*) a) -> elementtype; break;
	   case ARRAYTYPE_ARRAY:   c = (classinfo *) ((java_arrayarray*) a) -> data[0]; break;
	   default: panic("illegal arraytype");
	}		

	/* set vftbl */
	use_class_as_object (c);
  }
  
  return (java_lang_Class*) c;
}
#endif
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClass_getComponentType (JNIEnv *env ,  struct java_lang_VMClass* this )
{
    classinfo *thisclass = (classinfo*) (this->vmData);
    classinfo *c = NULL;
    arraydescriptor *desc;
    
    if ((desc = thisclass->vftbl->arraydesc) != NULL) {
        if (desc->arraytype == ARRAYTYPE_OBJECT)
            c = desc->componentvftbl->class;
        else
            c = primitivetype_table[desc->arraytype].class_primitive;
        
        /* set vftbl */
	use_class_as_object (c);
    }
    
    return (java_lang_Class*) c;
}


/*
 * Class:     java_lang_VMClass
 * Method:    getDeclaredConstructors
 * Signature: (Z)[Ljava/lang/reflect/Constructor;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredConstructors (JNIEnv *env ,  struct java_lang_VMClass* this , s4 public_only)
{
  
    classinfo *c = (classinfo *) (this->vmData);    
    java_objectheader *o;
    classinfo *class_constructor;
    java_objectarray *array_constructor;     /* result: array of Method-objects */
    java_objectarray *exceptiontypes;   /* the exceptions thrown by the method */
    methodinfo *m;			/* the current method to be represented */    
    int public_methods = 0;		/* number of public methods of the class */
    int pos = 0;
    int i;
    utf *utf_constr=utf_new_char("<init>");


    /*
    log_text("Java_java_lang_VMClass_getDeclaredConstructors");
    utf_display(c->name);
    printf("\n");
    */


    /* determine number of constructors */
    for (i = 0; i < c->methodscount; i++) 
	if ((((c->methods[i].flags & ACC_PUBLIC)) || public_only) && 
		(c->methods[i].name==utf_constr)) public_methods++;

    class_constructor = (classinfo*) loader_load(utf_new_char ("java/lang/reflect/Constructor"));

    if (!class_constructor) 
	return NULL;

    array_constructor = builtin_anewarray(public_methods, class_constructor);

    if (!array_constructor) 
	return NULL;

    for (i = 0; i < c->methodscount; i++) 
	if ((c->methods[i].flags & ACC_PUBLIC) || (!public_only)){
	
	    m = &c->methods[i];	    
	    if (m->name!=utf_constr) continue;
	    o = native_new_and_init(class_constructor);     
	    array_constructor->data[pos++] = o;

	    /* array of exceptions declared to be thrown, information not available !! */
	    exceptiontypes = builtin_anewarray (0, class_java_lang_Class);

/*	    class_showconstantpool(class_constructor);*/
	    /* initialize instance fields */
	    ((java_lang_reflect_Constructor*)o)->flag=(m->flags & (ACC_PRIVATE | ACC_PUBLIC | ACC_PROTECTED));
	    setfield_critical(class_constructor,o,"clazz",          "Ljava/lang/Class;",  jobject, (jobject) c /*this*/);
	    setfield_critical(class_constructor,o,"slot",           "I",		     jint,    i); 
	    setfield_critical(class_constructor,o,"exceptionTypes", "[Ljava/lang/Class;", jobject, (jobject) exceptiontypes);
    	    setfield_critical(class_constructor,o,"parameterTypes", "[Ljava/lang/Class;", jobject, (jobject) get_parametertypes(m));
        }	     
    
log_text("leaving Java_java_lang_VMClass_getDeclaredConstructors");
return array_constructor;




/*  panic("Java_java_lang_Class_getConstructors0 called");
  return NULL;*/
}


/*
 * Class:     java_lang_VMClass
 * Method:    getDeclaredClasses
 * Signature: (Z)[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredClasses (JNIEnv *env ,  struct java_lang_VMClass* this , s4 publicOnly)
{
#warning fix the public only case
  classinfo *c = (classinfo *) (this->vmData);
  int pos = 0;                /* current declared class */
  int declaredclasscount = 0; /* number of declared classes */
  java_objectarray *result;   /* array of declared classes */
  int i;

  if (!this)
    return NULL;

  if (!this->vmData)
    return NULL;

  if (!Java_java_lang_VMClass_isPrimitive(env, this) && (c->name->text[0]!='[')) {
    /* determine number of declared classes */
    for (i = 0; i < c->innerclasscount; i++) {
      if (c->innerclass[i].outer_class == c) 
        /* outer class is this class */
	declaredclasscount++;
    }
  }

  result = builtin_anewarray(declaredclasscount, class_java_lang_Class);    	

  for (i = 0; i < c->innerclasscount; i++) {
    
    classinfo *inner =  c->innerclass[i].inner_class;
    classinfo *outer =  c->innerclass[i].outer_class;
      
    if (outer == c) {
      /* outer class is this class, store innerclass in array */
      use_class_as_object (inner);
      result->data[pos++] = (java_objectheader *) inner;
    }
  }

  return result;
}

/*
 * Class:     java/lang/Class
 * Method:    getDeclaringClass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClass_getDeclaringClass ( JNIEnv *env ,  struct java_lang_VMClass* this)
{
#warning fixme
  classinfo *c = (classinfo *) (this->vmData);
  log_text("Java_java_lang_VMClass_getDeclaringClass");

  if (this && this->vmData && !Java_java_lang_VMClass_isPrimitive(env, this) && (c->name->text[0]!='[')) {    
    int i, j;

    if (c->innerclasscount == 0)  /* no innerclasses exist */
	return NULL;
    
    for (i = 0; i < c->innerclasscount; i++) {

      classinfo *inner =  c->innerclass[i].inner_class;
      classinfo *outer =  c->innerclass[i].outer_class;
      
      if (inner == c) {
      	/* innerclass is this class */
	use_class_as_object (outer);
	return (java_lang_Class*) outer;
      }
    }
  }

  /* return NULL for arrayclasses and primitive classes */
  return NULL;
}

/*
 * Class:     java/lang/Class
 * Method:    getField0
 * Signature: (Ljava/lang/String;I)Ljava/lang/reflect/Field;
 */
JNIEXPORT struct java_lang_reflect_Field* JNICALL Java_java_lang_VMClass_getField0 ( JNIEnv *env ,  struct java_lang_VMClass* this, struct java_lang_String* name, s4 public_only)
{
    classinfo *c, *fieldtype;   
    fieldinfo *f;               /* the field to be represented */
    java_lang_reflect_Field *o; /* result: field-object */
    utf *desc;			/* the fielddescriptor */
    char buffer[MAXSTRINGSIZE];
    
    /* create Field object */
    c = (classinfo*) loader_load(utf_new_char ("java/lang/reflect/Field"));
    o = (java_lang_reflect_Field*) native_new_and_init(c);

    /* get fieldinfo entry */
    f = class_findfield_approx((classinfo*) (this->vmData), javastring_toutf(name, false));

    if (f) {

	if ( public_only && !(f->flags & ACC_PUBLIC))
	{
	    /* field is not public */
	    exceptionptr = native_new_and_init(class_java_lang_NoSuchFieldException);
	    return NULL;
	}

      desc = f->descriptor;
      fieldtype = class_from_descriptor(desc->text,utf_end(desc),NULL,true);
      if (!fieldtype) return NULL;
	 
      /* initialize instance fields */
      setfield_critical(c,o,"clazz",          "Ljava/lang/Class;",  jobject, (jobject) (this->vmData) /*this*/);
      setfield_critical(c,o,"modifiers",      "I",		    jint,    f->flags);
      /* save type in slot-field for faster processing */
      setfield_critical(c,o,"slot",           "I",		    jint,    (jint) f->descriptor->text[0]);  
      setfield_critical(c,o,"name",           "Ljava/lang/String;", jstring, (jstring) name);
      setfield_critical(c,o,"type",           "Ljava/lang/Class;",  jclass,  fieldtype);

      return o;
    }

    return NULL;
}


/*
 * Class:     java_lang_VMClass
 * Method:    getDeclaredFields
 * Signature: (Z)[Ljava/lang/reflect/Field;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredFields (JNIEnv *env ,  struct java_lang_VMClass* this , s4 public_only)
{
    classinfo *c = (classinfo *) (this->vmData);
    classinfo *class_field;
    java_objectarray *array_field; /* result: array of field-objects */
    int public_fields = 0;         /* number of elements in field-array */
    int pos = 0;
    int i;

    /* determine number of fields */
    for (i = 0; i < c->fieldscount; i++) 
	if ((c->fields[i].flags & ACC_PUBLIC) || (!public_only)) public_fields++;

    class_field = loader_load(utf_new_char("java/lang/reflect/Field"));

    if (!class_field) 
	return NULL;

    /* create array of fields */
    array_field = builtin_anewarray(public_fields, class_field);

    /* creation of array failed */
    if (!array_field) 
	return NULL;

    /* get the fields and store in the array */    
    for (i = 0; i < c->fieldscount; i++) 
	if ( (c->fields[i].flags & ACC_PUBLIC) || (!public_only))
	    array_field->data[pos++] = (java_objectheader*) Java_java_lang_VMClass_getField0
	                                                     (env,
							      this,
							      (java_lang_String*) javastring_new(c->fields[i].name), 
							      public_only);
    return array_field;
}

/*
 * Class:     java/lang/Class
 * Method:    getInterfaces
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getInterfaces ( JNIEnv *env ,  struct java_lang_VMClass* this)
{
	classinfo *c = (classinfo*) (this->vmData);
	u4 i;
	java_objectarray *a = builtin_anewarray (c->interfacescount, class_java_lang_Class);
	if (!a) return NULL;
	for (i=0; i<c->interfacescount; i++) {
		use_class_as_object (c->interfaces[i]);

		a->data[i] = (java_objectheader*) c->interfaces[i];
		}
	return a;
}


/*
 * Class:     java/lang/Class
 * Method:    getMethod0
 * Signature: (Ljava/lang/String;[Ljava/lang/Class;I)Ljava/lang/reflect/Method;
 */
JNIEXPORT struct java_lang_reflect_Method* JNICALL Java_java_lang_VMClass_getMethod0 ( JNIEnv *env ,  struct java_lang_Class* 
	this, struct java_lang_String* name, java_objectarray* types, s4 which)
{
    classinfo *c; 
    classinfo *clazz = (classinfo *) this;
    java_lang_reflect_Method* o;         /* result: Method-object */ 
    java_objectarray *exceptiontypes;    /* the exceptions thrown by the method */
    methodinfo *m;			 /* the method to be represented */

    c = (classinfo*) loader_load(utf_new_char ("java/lang/reflect/Method"));
    o = (java_lang_reflect_Method*) native_new_and_init(c);

    /* find the method */
    m = class_resolvemethod_approx (
		clazz, 
		javastring_toutf(name, false),
		create_methodsig(types,0)
    	);

    if (!m || (which==MEMBER_PUBLIC && !(m->flags & ACC_PUBLIC)))
    {
    	/* no apropriate method was found */
	exceptionptr = native_new_and_init (class_java_lang_NoSuchMethodException);
	return NULL;
    }
   
    /* array of exceptions declared to be thrown, information not available !! */
    exceptiontypes = builtin_anewarray (0, class_java_lang_Class);

    /* initialize instance fields */
    setfield_critical(c,o,"clazz",          "Ljava/lang/Class;",  jobject, (jobject) clazz /*this*/);
    setfield_critical(c,o,"parameterTypes", "[Ljava/lang/Class;", jobject, (jobject) types);
    setfield_critical(c,o,"exceptionTypes", "[Ljava/lang/Class;", jobject, (jobject) exceptiontypes);
    setfield_critical(c,o,"name",           "Ljava/lang/String;", jstring, javastring_new(m->name));
    setfield_critical(c,o,"modifiers",      "I",		  jint,    m->flags);
    setfield_critical(c,o,"slot",           "I",		  jint,    0); 
    setfield_critical(c,o,"returnType",     "Ljava/lang/Class;",  jclass,  get_returntype(m));

    return o;
}

/*
 * Class:     java_lang_VMClass
 * Method:    getDeclaredMethods
 * Signature: (Z)[Ljava/lang/reflect/Method;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getDeclaredMethods (JNIEnv *env ,  struct java_lang_VMClass* this , s4 public_only)
{
    classinfo *c = (classinfo *) this->vmData;    
    java_objectheader *o;
    classinfo *class_method;
    java_objectarray *array_method;     /* result: array of Method-objects */
    java_objectarray *exceptiontypes;   /* the exceptions thrown by the method */
    methodinfo *m;			/* the current method to be represented */    
    int public_methods = 0;		/* number of public methods of the class */
    int pos = 0;
    int i;

    /* determine number of methods */
    for (i = 0; i < c->methodscount; i++) 
	if (((c->methods[i].flags & ACC_PUBLIC)) || public_only) public_methods++;

    class_method = (classinfo*) loader_load(utf_new_char ("java/lang/reflect/Method"));

    if (!class_method) 
	return NULL;
    
    array_method = builtin_anewarray(public_methods, class_method);

    if (!array_method) 
	return NULL;

    for (i = 0; i < c->methodscount; i++) 
	if ((c->methods[i].flags & ACC_PUBLIC) || (!public_only)){

	    m = &c->methods[i];	    
	    o = native_new_and_init(class_method);     
	    array_method->data[pos++] = o;

	    /* array of exceptions declared to be thrown, information not available !! */
	    exceptiontypes = builtin_anewarray (0, class_java_lang_Class);

	    /* initialize instance fields */
	    setfield_critical(class_method,o,"clazz",          "Ljava/lang/Class;",  jobject, (jobject) c /*this*/);
	    setfield_critical(class_method,o,"name",           "Ljava/lang/String;", jstring, javastring_new(m->name));
	    setfield_critical(class_method,o,"flag",      "I",		     jint,    m->flags);
	    setfield_critical(class_method,o,"slot",           "I",		     jint,    i); 
	    setfield_critical(class_method,o,"returnType",     "Ljava/lang/Class;",  jclass,  get_returntype(m));
	    setfield_critical(class_method,o,"exceptionTypes", "[Ljava/lang/Class;", jobject, (jobject) exceptiontypes);
    	    setfield_critical(class_method,o,"parameterTypes", "[Ljava/lang/Class;", jobject, (jobject) get_parametertypes(m));
        }	     

    return array_method;
}

/*
 * Class:     java/lang/Class
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_getModifiers ( JNIEnv *env ,  struct java_lang_VMClass* this)
{
  classinfo *c = (classinfo *) (this->vmData);
  return c->flags;
}

/*
 * Class:     java/lang/Class
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_VMClass_getName ( JNIEnv *env ,  struct java_lang_VMClass* this)
{

	u4 i;
	classinfo *c = (classinfo*) (this->vmData);
	java_lang_String *s = (java_lang_String*) javastring_new(c->name);

	if (!s) return NULL;

	/* return string where '/' is replaced by '.' */
	for (i=0; i<s->value->header.size; i++) {
		if (s->value->data[i] == '/') s->value->data[i] = '.';
		}
	
	return s;
}



/*
 * Class:     java/lang/Class
 * Method:    getProtectionDomain0
 * Signature: ()Ljava/security/ProtectionDomain;
 */
JNIEXPORT struct java_security_ProtectionDomain* JNICALL Java_java_lang_VMClass_getProtectionDomain0 ( JNIEnv *env ,  struct java_lang_Class* this)
{
  log_text("Java_java_lang_VMClass_getProtectionDomain0  called");
  return NULL;
}

/*
 * Class:     java/lang/Class
 * Method:    getSigners
 * Signature: ()[Ljava/lang/Object;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_VMClass_getSigners ( JNIEnv *env ,  struct java_lang_Class* this)
{
  log_text("Java_java_lang_VMClass_getSigners  called");
  return NULL;
}

/*
 * Class:     java/lang/Class
 * Method:    getSuperclass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClass_getSuperclass ( JNIEnv *env ,  struct java_lang_VMClass* this)
{
	classinfo *c = ((classinfo*) this->vmData) -> super;
	if (!c) return NULL;

	use_class_as_object (c);
	return (java_lang_Class*) c;
}

/*
 * Class:     java/lang/Class
 * Method:    isArray
 * Signature: ()Z
 */
/* XXX delete */
#if 0
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isArray ( JNIEnv *env ,  struct java_lang_VMClass* this)
{
        classinfo *c = (classinfo*) (this->vmData);

	if (c == class_array || c->name->text[0] == '[')  return true;
	return false;
}
#endif
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isArray ( JNIEnv *env ,  struct java_lang_VMClass* this)
{
    classinfo *c = (classinfo*) (this->vmData);
    return c->vftbl->arraydesc != NULL;
}

/*
 * Class:     java/lang/Class
 * Method:    isAssignableFrom
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isAssignableFrom ( JNIEnv *env ,  struct java_lang_VMClass* this, struct java_lang_Class* sup)
{
#warning fixme
	log_text("Java_java_lang_VMClass_isAssignableFrom");
 	return (*env)->IsAssignableForm(env, (jclass) this, (jclass) sup->vmClass);
}

/*
 * Class:     java/lang/Class
 * Method:    isInstance
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isInstance ( JNIEnv *env ,  struct java_lang_VMClass* this, struct java_lang_Object* obj)
{
	classinfo *clazz = (classinfo*) this;
	return (*env)->IsInstanceOf(env,(jobject) obj,clazz);
}

/*
 * Class:     java/lang/Class
 * Method:    isInterface
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isInterface ( JNIEnv *env ,  struct java_lang_VMClass* this)
{
	classinfo *c = (classinfo*) this->vmData;
	if (c->flags & ACC_INTERFACE) return true;
	return false;
}

/*
 * Class:     java/lang/Class
 * Method:    isPrimitive
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isPrimitive ( JNIEnv *env ,  struct java_lang_VMClass* this)
{
  int i;
  classinfo *c = (classinfo *) this->vmData;

  /* search table of primitive classes */
  for (i=0;i<PRIMITIVETYPE_COUNT;i++)
    if (primitivetype_table[i].class_primitive == c) return true;

  return false;
}


/*
 * Class:     java/lang/Class
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_registerNatives ( JNIEnv *env  )
{
    /* empty */
}

/*
 * Class:     java/lang/Class
 * Method:    setProtectionDomain0
 * Signature: (Ljava/security/ProtectionDomain;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_setProtectionDomain0 ( JNIEnv *env ,  struct java_lang_Class* this, struct java_security_ProtectionDomain* par1)
{
  if (verbose)
    log_text("Java_java_lang_VMClass_setProtectionDomain0 called");
}

/*
 * Class:     java/lang/Class
 * Method:    setSigners
 * Signature: ([Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_setSigners ( JNIEnv *env ,  struct java_lang_Class* this, java_objectarray* par1)
{
  if (verbose)
    log_text("Java_java_lang_VMClass_setSigners called");
}






/*
 * Class:     java_lang_VMClass
 * Method:    initialize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_initialize (JNIEnv *env ,  struct java_lang_VMClass* this ){
	log_text("Java_java_lang_VMClass_initialize");
}
/*
 * Class:     java_lang_VMClass
 * Method:    loadArrayClass
 * Signature: (Ljava/lang/String;Ljava/lang/ClassLoader;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_VMClass_loadArrayClass (JNIEnv *env , jclass clazz, struct java_lang_String* par1, struct 
	java_lang_ClassLoader* par2) {
		log_text("Java_java_lang_VMClass_loadArrayClass");
		return 0;
}
/*
 * Class:     java_lang_VMClass
 * Method:    throwException
 * Signature: (Ljava/lang/Throwable;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_throwException (JNIEnv *env , jclass clazz, struct java_lang_Throwable* par1) {
	log_text("Java_java_lang_VMClass_throwException");
}

/*
 * Class:     java_lang_VMClass
 * Method:    step7
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_step7 (JNIEnv *env ,  struct java_lang_VMClass* this ) {
	log_text("Java_java_lang_VMClass_step7");
}
/*
 * Class:     java_lang_VMClass
 * Method:    step8
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_step8 (JNIEnv *env ,  struct java_lang_VMClass* this ) {
	log_text("Java_java_lang_VMClass_step8");
}



/*
 * Class:     java_lang_VMClass
 * Method:    isInitialized
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMClass_isInitialized (JNIEnv *env ,  struct java_lang_VMClass* this ) {
	log_text("Java_java_lang_VMClass_isInitialized");
	return 1;
}
/*
 * Class:     java_lang_VMClass
 * Method:    setInitialized
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClass_setInitialized (JNIEnv *env ,  struct java_lang_VMClass* this ) {
	log_text("Java_java_lang_VMClass_setInitialized");
}
