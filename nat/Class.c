/* class: java/lang/Class */


/* for selecting public members */
#define MEMBER_PUBLIC  0


/****************************************************************************************** 											   			

	creates method signature (excluding return type) from array of 
	class-objects representing the parameters of the method 

*******************************************************************************************/


utf *create_methodsig(java_objectarray* types)
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
    classinfo *c = NULL;

    if (*utf_ptr>=desc_end)
	panic("illegal methoddescriptor");

    switch (utf_nextu2(utf_ptr)) {
      /* primitive types */
      case 'I' : c = class_java_lang_Integer;
	         break;
      case 'J' : c = class_java_lang_Long;
	         break;
      case 'F' : c = class_java_lang_Float;
	         break;
      case 'D' : c = class_java_lang_Double;
	         break;
      case 'B' : c = class_java_lang_Byte;
	         break;
      case 'C' : c = class_java_lang_Character;
	         break;
      case 'S' : c = class_java_lang_Short;
	         break;
      case 'Z' : c = class_java_lang_Boolean;
	         break;
      case 'V' : c = class_java_lang_Void;
	         break;

      case 'L' : {
	            char *start = *utf_ptr;
		    
		    while (utf_nextu2(utf_ptr)!=';') 
			/* skip */ ;

	            if (skip) return NULL; /* just analyze */

		    /* load the class */	
	            c = loader_load(utf_new(start,*utf_ptr-start-1));
		    break;
                 }

      case '[' : {
      		    /* arrayclass */
	            char *start = *utf_ptr;
		    char ch;

		    while ((ch = utf_nextu2(utf_ptr))=='[') 
			/* skip */ ;

		    if (ch == 'L') {
			while (utf_nextu2(utf_ptr)!=';') 
			    /* skip */ ;
		    }

		    if (skip) return NULL; /* just analyze */

		    /* create the specific arrayclass */
		    c = create_array_class(utf_new(start-1,*utf_ptr-start+1));
		    break;
                 }
    }

    if (!c)
	/* unknown type */
	panic("illegal methoddescriptor");

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
 * Class:     java/lang/Class
 * Method:    forName0
 * Signature: (Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_Class_forName0 ( JNIEnv *env ,  struct java_lang_String* s, s4 initialize, struct java_lang_ClassLoader* loader)
{
	classinfo *c;
	utf *u;
	u4 i;

	if (runverbose)
	{
	    log_text("Java_java_lang_Class_forName0 called");
	    log_text(javastring_tochar((java_objectheader*)s));
	}

	/* illegal argument */
	if (!s) return NULL;
	
	if (s->value->data[s->offset]=='[') {

	  /*  arrayclasses, e.g. [Ljava.lang.String; */	  
	  c = create_array_class(javastring_toutf(s, true));
	  assert(c != 0);

	} else
	{

	  /* create utf string in which '.' is replaced by '/' */
	  u = javastring_toutf(s, true);

	  if ( !(c = class_get(u)) ) {
	      methodinfo *method;
	      java_lang_Class *class;

	      /* class was not found. first check whether we can load it */
	      method = class_findmethod(loader->header.vftbl->class,
					utf_new_char("loadClass"),
					utf_new_char("(Ljava/lang/String;)Ljava/lang/Class;"));
	      if (method == NULL)
	      {
		  log_text("could not find method");
		  exceptionptr = native_new_and_init (class_java_lang_ClassNotFoundException);
		  return NULL;
	      }

	      c = (classinfo*)asm_calljavafunction(method, loader, s, NULL, NULL);

	      if (c == NULL)
	      {
		  /* class was not loaded. raise exception */
		  exceptionptr = 
		      native_new_and_init (class_java_lang_ClassNotFoundException);
		  return NULL;
	      }
	  }
        }

	use_class_as_object (c);
	return (java_lang_Class*) c;
}


/*
 * Class:     java/lang/Class
 * Method:    getClassLoader0
 * Signature: ()Ljava/lang/ClassLoader;
 */
JNIEXPORT struct java_lang_ClassLoader* JNICALL Java_java_lang_Class_getClassLoader0 ( JNIEnv *env ,  struct java_lang_Class* this)
{  
  init_systemclassloader();
  return SystemClassLoader;
}

/*
 * Class:     java/lang/Class
 * Method:    getComponentType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_Class_getComponentType ( JNIEnv *env ,  struct java_lang_Class* this)
{
  classinfo *c = NULL; 

  if ((classinfo*) this == class_array) {
	java_arrayheader *a = (java_arrayheader*) this;

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

/*
 * Class:     java/lang/Class
 * Method:    getConstructor0
 * Signature: ([Ljava/lang/Class;I)Ljava/lang/reflect/Constructor;
 */
JNIEXPORT struct java_lang_reflect_Constructor* JNICALL Java_java_lang_Class_getConstructor0 ( JNIEnv *env ,  struct java_lang_Class* this, java_objectarray* par1, s4 par2)
{
  panic("Java_java_lang_Class_getConstructor0 called");
  return NULL;
}

/*
 * Class:     java/lang/Class
 * Method:    getConstructors0
 * Signature: (I)[Ljava/lang/reflect/Constructor;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Class_getConstructors0 ( JNIEnv *env ,  struct java_lang_Class* this, s4 par1)
{
  panic("Java_java_lang_Class_getConstructors0 called");
  return NULL;
}

/*
 * Class:     java/lang/Class
 * Method:    getDeclaredClasses0
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Class_getDeclaredClasses0 ( JNIEnv *env ,  struct java_lang_Class* this)
{
  classinfo *c = (classinfo *) this;
  int pos = 0;                /* current declared class */
  int declaredclasscount = 0; /* number of declared classes */
  java_objectarray *result;   /* array of declared classes */
  int i;

  if (!this)
    return NULL;

  if (!Java_java_lang_Class_isPrimitive(env, this) && (c->name->text[0]!='[')) {
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
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_Class_getDeclaringClass ( JNIEnv *env ,  struct java_lang_Class* this)
{
  classinfo *c = (classinfo *) this;

  if (this && !Java_java_lang_Class_isPrimitive(env, this) && (c->name->text[0]!='[')) {    
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
JNIEXPORT struct java_lang_reflect_Field* JNICALL Java_java_lang_Class_getField0 ( JNIEnv *env ,  struct java_lang_Class* this, struct java_lang_String* name, s4 which)
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
    f = class_findfield_approx((classinfo*) this, javastring_toutf(name, false));

    if (f) {

	if (which==MEMBER_PUBLIC && !(f->flags & ACC_PUBLIC))
	{
	    /* field is not public */
	    exceptionptr = native_new_and_init(class_java_lang_NoSuchFieldException);
	    return NULL;
	}

      desc = f->descriptor;

      /* use the the field-descriptor to determine the class of the field */
      if (desc->text[0]=='[')
          /* arrayclass */
	  fieldtype = create_array_class(f->descriptor);
      else
 
      if (desc->text[0]=='L') {
	  
	  if (desc->blength<2)
	      return NULL;
	  
	  fieldtype = loader_load(utf_new(desc->text+1,desc->blength-2));

      } else
      switch (desc->text[0]) {
      	  /* primitive type */
	  case 'C': fieldtype = class_java_lang_Character;
	            break;
	  case 'I': fieldtype = class_java_lang_Integer;
	            break;
	  case 'F': fieldtype = class_java_lang_Float;
	            break;
	  case 'D': fieldtype = class_java_lang_Double;
	            break;
	  case 'J': fieldtype = class_java_lang_Long;
	            break;
	  case 'B': fieldtype = class_java_lang_Byte;
	            break;
	  case 'S': fieldtype = class_java_lang_Short;
	            break;
	  case 'Z': fieldtype = class_java_lang_Boolean;
	            break;
           default: /* unknown type */
	            return NULL;
	  }
	 
      /* initialize instance fields */
      setfield_critical(c,o,"clazz",          "Ljava/lang/Class;",  jobject, (jobject) this);
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
 * Class:     java/lang/Class
 * Method:    getFields0
 * Signature: (I)[Ljava/lang/reflect/Field;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Class_getFields0 ( JNIEnv *env ,  struct java_lang_Class* this, s4 which)
{
    classinfo *c = (classinfo *) this;
    classinfo *class_field;
    java_objectarray *array_field; /* result: array of field-objects */
    int public_fields = 0;         /* number of elements in field-array */
    int pos = 0;
    int i;

    /* determine number of fields */
    for (i = 0; i < c->fieldscount; i++) 
	if ((c->fields[i].flags & ACC_PUBLIC) || which!=MEMBER_PUBLIC) public_fields++;

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
	if (c->fields[i].flags & ACC_PUBLIC)
	    array_field->data[pos++] = (java_objectheader*) Java_java_lang_Class_getField0
	                                                     (env,
							      this,
							      (java_lang_String*) javastring_new(c->fields[i].name), 
							      which);
    return array_field;
}

/*
 * Class:     java/lang/Class
 * Method:    getInterfaces
 * Signature: ()[Ljava/lang/Class;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Class_getInterfaces ( JNIEnv *env ,  struct java_lang_Class* this)
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


/*
 * Class:     java/lang/Class
 * Method:    getMethod0
 * Signature: (Ljava/lang/String;[Ljava/lang/Class;I)Ljava/lang/reflect/Method;
 */
JNIEXPORT struct java_lang_reflect_Method* JNICALL Java_java_lang_Class_getMethod0 ( JNIEnv *env ,  struct java_lang_Class* this, struct java_lang_String* name, java_objectarray* types, s4 which)
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
		create_methodsig(types)
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
    setfield_critical(c,o,"clazz",          "Ljava/lang/Class;",  jobject, (jobject) this);
    setfield_critical(c,o,"parameterTypes", "[Ljava/lang/Class;", jobject, (jobject) types);
    setfield_critical(c,o,"exceptionTypes", "[Ljava/lang/Class;", jobject, (jobject) exceptiontypes);
    setfield_critical(c,o,"name",           "Ljava/lang/String;", jstring, javastring_new(m->name));
    setfield_critical(c,o,"modifiers",      "I",		  jint,    m->flags);
    setfield_critical(c,o,"slot",           "I",		  jint,    0); 
    setfield_critical(c,o,"returnType",     "Ljava/lang/Class;",  jclass,  get_returntype(m));

    return o;
}

/*
 * Class:     java/lang/Class
 * Method:    getMethods0
 * Signature: (I)[Ljava/lang/reflect/Method;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Class_getMethods0 ( JNIEnv *env ,  struct java_lang_Class* this, s4 which)
{
    classinfo *c = (classinfo *) this;    
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
	if (((c->methods[i].flags & ACC_PUBLIC)) || which!=MEMBER_PUBLIC) public_methods++;

    class_method = (classinfo*) loader_load(utf_new_char ("java/lang/reflect/Method"));

    if (!class_method) 
	return NULL;
    
    array_method = builtin_anewarray(public_methods, class_method);

    if (!array_method) 
	return NULL;

    for (i = 0; i < c->methodscount; i++) 
	if (c->methods[i].flags & ACC_PUBLIC) {

	    m = &c->methods[i];	    
	    o = native_new_and_init(class_method);     
	    array_method->data[pos++] = o;

	    /* array of exceptions declared to be thrown, information not available !! */
	    exceptiontypes = builtin_anewarray (0, class_java_lang_Class);

	    /* initialize instance fields */
	    setfield_critical(class_method,o,"clazz",          "Ljava/lang/Class;",  jobject, (jobject) this);
	    setfield_critical(class_method,o,"name",           "Ljava/lang/String;", jstring, javastring_new(m->name));
	    setfield_critical(class_method,o,"modifiers",      "I",		     jint,    m->flags);
	    setfield_critical(class_method,o,"slot",           "I",		     jint,    0); 
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
JNIEXPORT s4 JNICALL Java_java_lang_Class_getModifiers ( JNIEnv *env ,  struct java_lang_Class* this)
{
  classinfo *c = (classinfo *) this;
  return c->flags;
}

/*
 * Class:     java/lang/Class
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT struct java_lang_String* JNICALL Java_java_lang_Class_getName ( JNIEnv *env ,  struct java_lang_Class* this)
{
	u4 i;
	classinfo *c = (classinfo*) this;
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
 * Method:    getPrimitiveClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_Class_getPrimitiveClass ( JNIEnv *env ,  struct java_lang_String* name)
{
    classinfo *c;
    utf *u = javastring_toutf(name, false);

    if (u) {    	
      /* get primitive class */
      c = loader_load(u);
      use_class_as_object (c);
      return (java_lang_Class*) c;      
    }

    /* illegal primitive classname specified */
    exceptionptr = native_new_and_init (class_java_lang_ClassNotFoundException);
    return NULL;
}

/*
 * Class:     java/lang/Class
 * Method:    getProtectionDomain0
 * Signature: ()Ljava/security/ProtectionDomain;
 */
JNIEXPORT struct java_security_ProtectionDomain* JNICALL Java_java_lang_Class_getProtectionDomain0 ( JNIEnv *env ,  struct java_lang_Class* this)
{
  log_text("Java_java_lang_Class_getProtectionDomain0  called");
  return NULL;
}

/*
 * Class:     java/lang/Class
 * Method:    getSigners
 * Signature: ()[Ljava/lang/Object;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_lang_Class_getSigners ( JNIEnv *env ,  struct java_lang_Class* this)
{
  log_text("Java_java_lang_Class_getSigners  called");
  return NULL;
}

/*
 * Class:     java/lang/Class
 * Method:    getSuperclass
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_Class_getSuperclass ( JNIEnv *env ,  struct java_lang_Class* this)
{
	classinfo *c = ((classinfo*) this) -> super;
	if (!c) return NULL;

	use_class_as_object (c);
	return (java_lang_Class*) c;
}

/*
 * Class:     java/lang/Class
 * Method:    isArray
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_isArray ( JNIEnv *env ,  struct java_lang_Class* this)
{
        classinfo *c = (classinfo*) this;

	if (c == class_array || c->name->text[0] == '[')  return true;
	return false;
}

/*
 * Class:     java/lang/Class
 * Method:    isAssignableFrom
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_isAssignableFrom ( JNIEnv *env ,  struct java_lang_Class* this, struct java_lang_Class* sup)
{
 	return env->IsAssignableForm(env, (jclass) this, (jclass) sup);
}

/*
 * Class:     java/lang/Class
 * Method:    isInstance
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_isInstance ( JNIEnv *env ,  struct java_lang_Class* this, struct java_lang_Object* obj)
{
	classinfo *clazz = (classinfo*) this;
	return env->IsInstanceOf(env,(jobject) obj,clazz);
}

/*
 * Class:     java/lang/Class
 * Method:    isInterface
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_isInterface ( JNIEnv *env ,  struct java_lang_Class* this)
{
	classinfo *c = (classinfo*) this;
	if (c->flags & ACC_INTERFACE) return true;
	return false;
}

/*
 * Class:     java/lang/Class
 * Method:    isPrimitive
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_Class_isPrimitive ( JNIEnv *env ,  struct java_lang_Class* this)
{
  int i;
  classinfo *c = (classinfo *) this;

  /* search table of primitive classes */
  for (i=0;i<PRIMITIVETYPE_COUNT;i++)
    if (primitivetype_table[i].class_primitive == c) return true;

  return false;
}

/*
 * Class:     java/lang/Class
 * Method:    newInstance0
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_Class_newInstance0 ( JNIEnv *env ,  struct java_lang_Class* this)
{
	java_objectheader *o;

	if (verbose) {		
		char buffer[MAXSTRINGSIZE]; 
		utf_sprint(buffer,((classinfo *) this)->name);      
		strcat(buffer," instantiated. ");
		log_text(buffer);
		}
	
	/* don't allow newInstance for array- and primitive classes */

	if (((classinfo *) this)->name->text[0]=='[')
		panic("newInstance of array_class");

	if (Java_java_lang_Class_isPrimitive(env,this))
		panic("newInstance of primitive class");

	o = native_new_and_init ((classinfo*) this);
	
	if (!o) {
		exceptionptr = 
			native_new_and_init (class_java_lang_InstantiationException);
	}

	return (java_lang_Object*) o;
}

/*
 * Class:     java/lang/Class
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_Class_registerNatives ( JNIEnv *env  )
{
    /* empty */
}

/*
 * Class:     java/lang/Class
 * Method:    setProtectionDomain0
 * Signature: (Ljava/security/ProtectionDomain;)V
 */
JNIEXPORT void JNICALL Java_java_lang_Class_setProtectionDomain0 ( JNIEnv *env ,  struct java_lang_Class* this, struct java_security_ProtectionDomain* par1)
{
  if (verbose)
    log_text("Java_java_lang_Class_setProtectionDomain0 called");
}

/*
 * Class:     java/lang/Class
 * Method:    setSigners
 * Signature: ([Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_Class_setSigners ( JNIEnv *env ,  struct java_lang_Class* this, java_objectarray* par1)
{
  if (verbose)
    log_text("Java_java_lang_Class_setSigners called");
}










