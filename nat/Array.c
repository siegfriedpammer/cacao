/* class: java/lang/reflect/Array */



/* creates a new object for wrapping a primitive type and calls its constructor */

java_lang_Object *create_primitive_object (classinfo *c,void *value,char *sig)
{
	methodinfo *m;
	java_objectheader *o = builtin_new (c);
	
	if (!o) return NULL;
	
	/* find constructor */
	m = class_findmethod (c, 				  
	                      utf_new_char ("<init>"), 	      /*  method name  */
	                      utf_new_char (sig));	      /*  descriptor   */

	/* constructor not found */
	if (!m) panic("invalid object for wrapping primitive type");

	/* call constructor and pass the value of the primitive type */
	exceptionptr = asm_calljavamethod (m, o, value,NULL,NULL);

	if (exceptionptr) return NULL;

	return (java_lang_Object*) o;
}


/* checks whether an array index is valid and throws an exception otherwise */

int arrayindex_valid(java_arrayheader * array, s4 index)
{
  if (!array) {  	
  	/* NULL passed as argument */
  	exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
  	return 0;
  }

  if (index >= array->size || index<0) {
  	/* out of bounds */
	exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
	return 0;
  }

  return 1;
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    get
 * Signature: (Ljava/lang/Object;I)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_Array_get ( JNIEnv *env ,  struct java_lang_Object* array, s4 index)
{
  java_arrayheader *a = (java_arrayheader *) array;

  /* verify validity of the arrayindex */ 
  if (!arrayindex_valid(a,index))
	return NULL;
  
  switch (a->arraytype) {
      /* for primitive types an object for wrapping the primitive type is created */
      case ARRAYTYPE_INT      : {
		 	           java_intarray *b = (java_intarray *) array;
			           return create_primitive_object(class_java_lang_Integer, (void*) (long) b->data[index],"(I)V");
			        }
      case ARRAYTYPE_LONG     : {
				   java_longarray *b = (java_longarray *) array;
				   return create_primitive_object(class_java_lang_Long, (void*) b->data[index],"(J)V");
				}
      case ARRAYTYPE_FLOAT    : {
				   java_floatarray *b = (java_floatarray *) array;
				   return create_primitive_object(class_java_lang_Float, (void*) (long) b->data[index],"(F)V");
				}
      case ARRAYTYPE_DOUBLE   : {
				   java_doublearray *b = (java_doublearray *) array;
				   return create_primitive_object(class_java_lang_Double, (void*) (long) b->data[index],"(D)V");
				}
      case ARRAYTYPE_BYTE     : {
				   java_bytearray *b = (java_bytearray *) array;
				   return create_primitive_object(class_java_lang_Byte, (void*) (long) b->data[index],"(B)V");
				}
      case ARRAYTYPE_CHAR     : {
				   java_chararray *b = (java_chararray *) array;
				   return create_primitive_object(class_java_lang_Character, (void*) (long) b->data[index],"(C)V");
				}
      case ARRAYTYPE_SHORT    : {
				   java_shortarray *b = (java_shortarray *) array;
				   return create_primitive_object(class_java_lang_Short, (void*) (long) b->data[index],"(S)V");
				}
      case ARRAYTYPE_BOOLEAN  : {
				   java_booleanarray *b = (java_booleanarray *) array;
				   return create_primitive_object(class_java_lang_Boolean, (void*) (long) b->data[index],"(Z)V");
				}
      case ARRAYTYPE_ARRAY    : {
				   java_arrayarray *b = (java_arrayarray *) array;
				   return (java_lang_Object*) b->data[index];
			        }
      case ARRAYTYPE_OBJECT   : /* use JNI-function to get the object */
            			return (java_lang_Object*) GetObjectArrayElement (env, (jobjectArray) array, index);
  }

  /* unknown arraytype */
  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
  return NULL;
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    getBoolean
 * Signature: (Ljava/lang/Object;I)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Array_getBoolean ( JNIEnv *env ,  struct java_lang_Object* array, s4 index)
{
  java_booleanarray *a = (java_booleanarray *) array;

  /* verify validity of the arrayindex */ 
  if (arrayindex_valid(&a->header,index))
	  return a->data[index];
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    getByte
 * Signature: (Ljava/lang/Object;I)B
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Array_getByte ( JNIEnv *env ,  struct java_lang_Object* array, s4 index)
{
  java_bytearray *a = (java_bytearray *) array;

  /* verify validity of the arrayindex */ 
  if (arrayindex_valid(&a->header,index))
	  return a->data[index];
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    getChar
 * Signature: (Ljava/lang/Object;I)C
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Array_getChar ( JNIEnv *env ,  struct java_lang_Object* array, s4 index)
{
  java_chararray *a = (java_chararray *) array;

  if (arrayindex_valid(&a->header,index))
	  return a->data[index];
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    getDouble
 * Signature: (Ljava/lang/Object;I)D
 */
JNIEXPORT double JNICALL Java_java_lang_reflect_Array_getDouble ( JNIEnv *env ,  struct java_lang_Object* array, s4 index)
{
  java_doublearray *a = (java_doublearray *) array;

  if (arrayindex_valid(&a->header,index))
	  return a->data[index];
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    getFloat
 * Signature: (Ljava/lang/Object;I)F
 */
JNIEXPORT float JNICALL Java_java_lang_reflect_Array_getFloat ( JNIEnv *env ,  struct java_lang_Object* array, s4 index)
{
  java_floatarray *a = (java_floatarray *) array;

  if (arrayindex_valid(&a->header,index))
	  return a->data[index];
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    getInt
 * Signature: (Ljava/lang/Object;I)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Array_getInt ( JNIEnv *env ,  struct java_lang_Object* array, s4 index)
{
  java_intarray *a = (java_intarray *) array;

  if (arrayindex_valid(&a->header,index))
	  return a->data[index];
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    getLength
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Array_getLength ( JNIEnv *env ,  struct java_lang_Object* array)
{
  java_arrayheader *a = (java_arrayheader *) array;  

  if (!a)
  	exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
  else
      	return a->size;
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    getLong
 * Signature: (Ljava/lang/Object;I)J
 */
JNIEXPORT s8 JNICALL Java_java_lang_reflect_Array_getLong ( JNIEnv *env ,  struct java_lang_Object* array, s4 index)
{
  java_longarray *a = (java_longarray *) array;

  if (arrayindex_valid(&a->header,index))
	  return a->data[index];
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    getShort
 * Signature: (Ljava/lang/Object;I)S
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Array_getShort ( JNIEnv *env ,  struct java_lang_Object* array, s4 index)
{
  java_shortarray *a = (java_shortarray *) array;

  if (arrayindex_valid(&a->header,index))
	  return a->data[index];
}


/* get one character representation of componenttype class */

char identify_componenttype(classinfo *c)
{
  int i;

  if (c->name->text[0] == '[')
  	/* array */
	return '[';

  /* check whether primitive type */ 
  for (i=0;i<PRIMITIVETYPE_COUNT;i++)
    if (primitivetype_table[i].class_primitive == c) 
	return primitivetype_table[i].typesig;


  /* no array and no primitive type, must be object */
  return 'L';
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    multiNewArray
 * Signature: (Ljava/lang/Class;[I)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_Array_multiNewArray ( JNIEnv *env ,  struct java_lang_Class* componenttype, java_intarray* dimensions)
{
  classinfo *c = (classinfo*) componenttype;
  constant_arraydescriptor *desc;
  utf *classname;
  u4 buf_len,i,dims;
  char *buffer,*pos;
  char ch;

  /* check arguments */
  if (!dimensions || !componenttype) {
  	exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
  	return 0; 
  }

  /* build constant_arraydescriptor, so builtin_multianewarray can be called */
  dims = dimensions->header.size;
  classname = c->name;

  /* allocate buffer for desciptor */
  buf_len = classname->blength + dims + 2;
  buffer = MNEW(u1, buf_len);
  pos = buffer;

  for (i=0;i<dims;i++) *pos++ = '[';

  switch (ch = identify_componenttype(c)) {
	case ']':
		/* component is array */
		memcpy(pos,classname->text,classname->blength);
		pos+=classname->blength;
  		desc = buildarraydescriptor(buffer,pos-buffer);
		break;
	case 'L':
		/* component is object */
		*pos++ = 'L';
  		memcpy(pos,classname->text,classname->blength);
		pos+=classname->blength;
		*pos++ = ';';
	  	desc = buildarraydescriptor(buffer,pos-buffer);
		break;	
	 default:
		/* primitive type */
		*pos++ = ch;
  		desc = buildarraydescriptor(buffer,pos-buffer);
  }
	
  /* dispose buffer */
  MFREE(buffer, u1, buf_len);
  return (java_lang_Object*) builtin_multianewarray(dimensions,desc);
}


/*
 * Class:     java/lang/reflect/Array
 * Method:    newArray
 * Signature: (Ljava/lang/Class;I)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_Array_newArray ( JNIEnv *env ,  struct java_lang_Class* componenttype, s4 size)
{
  classinfo *c = (classinfo *) componenttype;

  if (componenttype) {
      switch (identify_componenttype(c)) {
	case 'Z':return (java_lang_Object*) builtin_newarray_boolean (size); 
  	case 'C':return (java_lang_Object*) builtin_newarray_char (size); 
  	case 'F':return (java_lang_Object*) builtin_newarray_float (size); 
  	case 'D':return (java_lang_Object*) builtin_newarray_double (size); 
  	case 'B':return (java_lang_Object*) builtin_newarray_byte (size); 
  	case 'S':return (java_lang_Object*) builtin_newarray_short (size); 
  	case 'I':return (java_lang_Object*) builtin_newarray_int (size); 
  	case 'J':return (java_lang_Object*) builtin_newarray_long (size); 
  	case 'L':return (java_lang_Object*) builtin_anewarray (size, c);		
	case '[':{
		   /* arrayarray */
  	           constant_arraydescriptor *desc;
		   desc = buildarraydescriptor(c->name->text,c->name->blength);
		   return (java_lang_Object*) builtin_newarray_array (size, desc);
		 }
	}
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
  return NULL; 
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    set
 * Signature: (Ljava/lang/Object;ILjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Array_set ( JNIEnv *env ,  struct java_lang_Object* array, s4 index, struct java_lang_Object* val)
{
  java_arrayheader *a = (java_arrayheader *) array;
  classinfo *c;
  jfieldID fid;

  /* check arguments */
  if (!arrayindex_valid(a,index) || !val)
	return;

  /* determine class of the object containing the value */
  c = val->header.vftbl -> class;  

  /* The new value of the arrayelement is retrieved from the argument 'val' using a JNI-function 'GetxxxField'.
     For primitive types the argument-object is first unwrapped to retrieve this value. */

  switch (a->arraytype) {
      case ARRAYTYPE_INT      : {
	                           /* unwrap */
		 	           java_intarray *b = (java_intarray *) array;
				   if (c!=class_java_lang_Integer) break;
				   fid = env->GetFieldID(env,c,"value","I");
				   if (!fid) break;
				   b->data[index] = GetIntField(env,(jobject) val,fid);
				   return;							
			        }
      case ARRAYTYPE_LONG     : {
				   java_longarray *b = (java_longarray *) array;
				   if (c!=class_java_lang_Long) break;
				   fid = env->GetFieldID(env,c,"value","J");
				   if (!fid) break;
				   b->data[index] = GetLongField(env,(jobject) val,fid);
				   return;			
				}
      case ARRAYTYPE_FLOAT    : {
				   java_floatarray *b = (java_floatarray *) array;
				   if (c!=class_java_lang_Float) break;
				   fid = env->GetFieldID(env,c,"value","F");
				   if (!fid) break;
				   b->data[index] = GetFloatField(env,(jobject) val,fid);
				   return;			
				}
      case ARRAYTYPE_DOUBLE   : {
				   java_doublearray *b = (java_doublearray *) array;
				   if (c!=class_java_lang_Double) break;
				   fid = env->GetFieldID(env,c,"value","D");
				   if (!fid) break;
				   b->data[index] = GetDoubleField(env,(jobject) val,fid);
				   return;			
				}
      case ARRAYTYPE_BYTE     : {
				   java_bytearray *b = (java_bytearray *) array;
				   if (c!=class_java_lang_Byte) break;
				   fid = env->GetFieldID(env,c,"value","B");
				   if (!fid) break;
				   b->data[index] = GetIntField(env,(jobject) val,fid);
				   return;			
				}
      case ARRAYTYPE_CHAR     : {
				   java_chararray *b = (java_chararray *) array;
				   if (c!=class_java_lang_Character) break;
				   fid = env->GetFieldID(env,c,"value","C");
				   if (!fid) break;
				   b->data[index] = GetByteField(env,(jobject) val,fid);
				   return;			
				}
      case ARRAYTYPE_SHORT    : {
				   java_shortarray *b = (java_shortarray *) array;
				   if (c!=class_java_lang_Short) break;
				   fid = env->GetFieldID(env,c,"value","S");
				   if (!fid) break;
				   b->data[index] = GetShortField(env,(jobject) val,fid);
				   return;			
				}
      case ARRAYTYPE_BOOLEAN  : {
				   java_booleanarray *b = (java_booleanarray *) array;
				   if (c!=class_java_lang_Boolean) break;
				   fid = env->GetFieldID(env,c,"value","Z");
				   if (!fid) break;
				   b->data[index] = GetBooleanField(env,(jobject) val,fid);
				   return;			
				}
      case ARRAYTYPE_ARRAY    : {
				   java_arrayarray *b = (java_arrayarray *) array;
				   b->data[index] = (java_arrayheader*) val;
			        }
      case ARRAYTYPE_OBJECT   : {
				   java_objectarray *b = (java_objectarray *) array;
				   b->data[index] = (java_objectheader*) val;
			        }
  }

  /* unknown arraytype */
  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    setBoolean
 * Signature: (Ljava/lang/Object;IZ)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Array_setBoolean ( JNIEnv *env ,  struct java_lang_Object* array, s4 index, s4 val)
{
  java_booleanarray *a = (java_booleanarray *) array;

  if (arrayindex_valid(&a->header,index)) 
	a->data[index]=val;
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    setByte
 * Signature: (Ljava/lang/Object;IB)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Array_setByte ( JNIEnv *env ,  struct java_lang_Object* array, s4 index, s4 val)
{
  java_bytearray *a = (java_bytearray *) array;

  if (arrayindex_valid(&a->header,index))
	a->data[index]=val;
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    setChar
 * Signature: (Ljava/lang/Object;IC)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Array_setChar ( JNIEnv *env ,  struct java_lang_Object* array, s4 index, s4 val)
{
  java_chararray *a = (java_chararray *) array;

  if (arrayindex_valid(&a->header,index))
	a->data[index]=val;
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    setDouble
 * Signature: (Ljava/lang/Object;ID)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Array_setDouble ( JNIEnv *env ,  struct java_lang_Object* array, s4 index, double val)
{
  java_doublearray *a = (java_doublearray *) array;

  if (arrayindex_valid(&a->header,index))
	a->data[index]=val;

}

/*
 * Class:     java/lang/reflect/Array
 * Method:    setFloat
 * Signature: (Ljava/lang/Object;IF)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Array_setFloat ( JNIEnv *env ,  struct java_lang_Object* array, s4 index, float val)
{
  java_floatarray *a = (java_floatarray *) array;

  if (arrayindex_valid(&a->header,index))
	a->data[index]=val;
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    setInt
 * Signature: (Ljava/lang/Object;II)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Array_setInt ( JNIEnv *env ,  struct java_lang_Object* array, s4 index, s4 val)
{
  java_intarray *a = (java_intarray *) array;

  if (arrayindex_valid(&a->header,index))
	a->data[index]=val;
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    setLong
 * Signature: (Ljava/lang/Object;IJ)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Array_setLong ( JNIEnv *env ,  struct java_lang_Object* array, s4 index, s8 val)
{
  java_longarray *a = (java_longarray *) array;

  if (arrayindex_valid(&a->header,index))
	a->data[index]=val;
}

/*
 * Class:     java/lang/reflect/Array
 * Method:    setShort
 * Signature: (Ljava/lang/Object;IS)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Array_setShort ( JNIEnv *env ,  struct java_lang_Object* array, s4 index, s4 val)
{
  java_shortarray *a = (java_shortarray *) array;

  if (arrayindex_valid(&a->header,index))
	a->data[index]=val;
}
