/* class: java/lang/reflect/Field */


#include "jni.h"
#include "builtin.h"
#include "loader.h"
#include "native.h"
#include "tables.h"
#include "java_lang_Object.h"
#include "java_lang_reflect_Field.h"


/*
 * Class:     java/lang/reflect/Field
 * Method:    get
 * Signature: (Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT struct java_lang_Object* JNICALL Java_java_lang_reflect_Field_get ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj)
{
  jfieldID target_fid;   /* the JNI-fieldid of the wrapping object */
  jfieldID fid;          /* the JNI-fieldid of the field containing the value */
  jobject o;		 /* the object for wrapping the primitive type */
  classinfo *c = (classinfo *) this->declaringClass;
  int st = (this->flag & ACC_STATIC); /* true if field is static */

  /* get the fieldid of the field represented by this Field-object */
  fid = class_findfield_approx((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

  /* The fieldid is used to retrieve the value, for primitive types a new 
     object for wrapping the primitive type is created.  */
  if (st || obj)
    switch ((((struct classinfo*)this->declaringClass)->fields[this->slot]).descriptor->text[0]) {
      case 'I' : {
      		   /* create wrapping class */
	           c = class_java_lang_Integer;
		   o = builtin_new(c);
		   /* get fieldid to store the value */
		   target_fid = (*env)->GetFieldID(env,c,"value","I");
		   if (!target_fid) break;
				   
		   if (st)
		        /* static field */
			SetIntField(env,o,target_fid, (*env)->GetStaticIntField(env, c, fid));
                   else
                        /* instance field */
			SetIntField(env,o,target_fid, (*env)->GetIntField(env,(jobject) obj, fid));

		   /* return the wrapped object */		   
		   return (java_lang_Object*) o;
                 }
      case 'J' : {
	           c = class_java_lang_Long;
		   o = builtin_new(c);
		   target_fid = (*env)->GetFieldID(env,c,"value","J");
		   if (!target_fid) break;
				   
		   if (st)
			SetLongField(env,o,target_fid, (*env)->GetStaticLongField(env, c, fid));
                   else
			SetLongField(env,o,target_fid, (*env)->GetLongField(env,(jobject)  obj, fid));

		   return (java_lang_Object*) o;
                 }
      case 'F' : {
	           c = class_java_lang_Float;
		   o = builtin_new(c);
		   target_fid = (*env)->GetFieldID(env,c,"value","F");
		   if (!target_fid) break;
				   
		   if (st)
			SetFloatField(env,o,target_fid, (*env)->GetStaticFloatField(env, c, fid));
                   else
			SetFloatField(env,o,target_fid, (*env)->GetFloatField(env, (jobject) obj, fid));

		   return (java_lang_Object*) o;
                 }
      case 'D' : {
	           c = class_java_lang_Double;
		   o = builtin_new(c);
		   target_fid = (*env)->GetFieldID(env,c,"value","D");
		   if (!target_fid) break;
				   
		   if (st)
			SetDoubleField(env,o,target_fid, (*env)->GetStaticDoubleField(env, c, fid));
                   else
			SetDoubleField(env,o,target_fid, (*env)->GetDoubleField(env, (jobject) obj, fid));

		   return (java_lang_Object*) o;
                 }
      case 'B' : {
	           c = class_java_lang_Byte;
		   o = builtin_new(c);
		   target_fid = (*env)->GetFieldID(env,c,"value","B");
		   if (!target_fid) break;
				   
		   if (st)
			SetByteField(env,o,target_fid, (*env)->GetStaticByteField(env, c, fid));
                   else
			SetByteField(env,o,target_fid, (*env)->GetByteField(env, (jobject) obj, fid));

		   return (java_lang_Object*) o;
                 }
      case 'C' : {
	           c = class_java_lang_Character;
		   o = builtin_new(c);
		   target_fid = (*env)->GetFieldID(env,c,"value","C");
		   if (!target_fid) break;
				   
		   if (st)
			SetCharField(env,o,target_fid, (*env)->GetStaticCharField(env, c, fid));
                   else
			SetCharField(env,o,target_fid, (*env)->GetCharField(env, (jobject) obj, fid));

		   return (java_lang_Object*) o;
                 }
      case 'S' : {
	           c = class_java_lang_Short;
		   o = builtin_new(c);
		   target_fid = (*env)->GetFieldID(env,c,"value","S");
		   if (!target_fid) break;
				   
		   if (st)
			SetShortField(env,o,target_fid, (*env)->GetStaticShortField(env, c, fid));
                   else
			SetShortField(env,o,target_fid, (*env)->GetShortField(env, (jobject) obj, fid));

		   return (java_lang_Object*) o;
                 }
      case 'Z' : {
	           c = class_java_lang_Boolean;
		   o = builtin_new(c);
		   target_fid = (*env)->GetFieldID(env,c,"value","Z");
		   if (!target_fid) break;
				   
		   if (st)
			SetBooleanField(env,o,target_fid, (*env)->GetStaticBooleanField(env, c, fid));
                   else
			SetBooleanField(env,o,target_fid, (*env)->GetBooleanField(env, (jobject) obj, fid));

		   return (java_lang_Object*) o;
                 }
      case '[' : 
      case 'L' : {
		   if (st)
		        /* static field */
			return (java_lang_Object*) (*env)->GetStaticObjectField(env, c, fid);
                   else
                        /* instance field */
			return (java_lang_Object*) (*env)->GetObjectField(env, (jobject) obj, fid);

                 }
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);

  return NULL;
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    getBoolean
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getBoolean ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      /* get the fieldid represented by the field-object */
      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) 
          /* call the JNI-function to retrieve the field */ 
	  return (*env)->GetBooleanField (env, (jobject) obj, fid);
  }

  /* raise exception */
  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);

  return 0;
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    getByte
 * Signature: (Ljava/lang/Object;)B
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getByte ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) 
	  return (*env)->GetByteField (env, (jobject) obj, fid);
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);

  return 0;
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    getChar
 * Signature: (Ljava/lang/Object;)C
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getChar ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) 
	  return (*env)->GetCharField (env, (jobject) obj, fid);
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);

  return 0;
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    getDouble
 * Signature: (Ljava/lang/Object;)D
 */
JNIEXPORT double JNICALL Java_java_lang_reflect_Field_getDouble ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) 
	  return (*env)->GetDoubleField (env, (jobject) obj, fid);
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);

  return 0;
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    getFloat
 * Signature: (Ljava/lang/Object;)F
 */
JNIEXPORT float JNICALL Java_java_lang_reflect_Field_getFloat ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) 
	  return (*env)->GetFloatField (env, (jobject) obj, fid);
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);

  return 0;
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    getInt
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getInt ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) 
	  return (*env)->GetIntField (env, (jobject) obj, fid);
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);

  return 0;
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    getLong
 * Signature: (Ljava/lang/Object;)J
 */
JNIEXPORT s8 JNICALL Java_java_lang_reflect_Field_getLong ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) 
	  return (*env)->GetLongField (env, (jobject) obj, fid);
  }

  return 0;
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    getShort
 * Signature: (Ljava/lang/Object;)S
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getShort ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) 
	  return (*env)->GetShortField (env, (jobject) obj, fid);
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);

  return 0;
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    set
 * Signature: (Ljava/lang/Object;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_set ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj, struct java_lang_Object* val)
{
  jfieldID source_fid;  /* the field containing the value to be written */
  jfieldID fid;         /* the field to be written */
  classinfo *c;
  int st = (this->flag & ACC_STATIC); /* true if the field is static */

  fid = class_findfield_approx((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

  if (val && (st || obj)) {

    c = val->header.vftbl -> class;

    /* The fieldid is used to set the new value, for primitive types the value
       has to be retrieved from the wrapping object */  
    switch ((((struct classinfo*)this->declaringClass)->fields[this->slot]).descriptor->text[0]) {
      case 'I' : {
      		   /* illegal argument specified */
	           if (c != class_java_lang_Integer) break;
	           /* determine the field to read the value */
		   source_fid = (*env)->GetFieldID(env,c,"value","I");
		   if (!source_fid) break;
				   
		   /* set the new value */
		   if (st)
		        /* static field */
			(*env)->SetStaticIntField(env, c, fid, GetIntField(env,(jobject) val,source_fid));
                   else
                        /* instance field */
			(*env)->SetIntField(env, (jobject) obj, fid, GetIntField(env,(jobject) val,source_fid));

		   return;
                 }
      case 'J' : {
		   if (c != class_java_lang_Long) break;
		   source_fid = (*env)->GetFieldID(env,c,"value","J");
		   if (!source_fid) break;
				   
		   if (st)
			(*env)->SetStaticLongField(env, c, fid, GetLongField(env,(jobject) val,source_fid));
                   else
			(*env)->SetLongField(env, (jobject) obj, fid, GetLongField(env,(jobject) val,source_fid));

		   return;
                 }
      case 'F' : {
		   if (c != class_java_lang_Float) break;
		   source_fid = (*env)->GetFieldID(env,c,"value","F");
		   if (!source_fid) break;
				   
		   if (st)
			(*env)->SetStaticFloatField(env, c, fid, GetFloatField(env,(jobject) val,source_fid));
                   else
			(*env)->SetFloatField(env, (jobject) obj, fid, GetFloatField(env,(jobject) val,source_fid));

		   return;
                 }
      case 'D' : {
		   if (c != class_java_lang_Double) break;
		   source_fid = (*env)->GetFieldID(env,c,"value","D");
		   if (!source_fid) break;
				   
		   if (st)
			(*env)->SetStaticDoubleField(env, c, fid, GetDoubleField(env,(jobject) val,source_fid));
                   else
			(*env)->SetDoubleField(env, (jobject) obj, fid, GetDoubleField(env,(jobject) val,source_fid));

		   return;
                 }
      case 'B' : {
		   if (c != class_java_lang_Byte) break;
		   source_fid = (*env)->GetFieldID(env,c,"value","B");
		   if (!source_fid) break;
				   
		   if (st)
			(*env)->SetStaticByteField(env, c, fid, GetByteField(env,(jobject) val,source_fid));
                   else
			(*env)->SetByteField(env, (jobject) obj, fid, GetByteField(env,(jobject) val,source_fid));

		   return;
                 }
      case 'C' : {
		   if (c != class_java_lang_Character) break;
		   source_fid = (*env)->GetFieldID(env,c,"value","C");
		   if (!source_fid) break;
				   
		   if (st)
			(*env)->SetStaticCharField(env, c, fid, GetCharField(env,(jobject) val,source_fid));
                   else
			(*env)->SetCharField(env, (jobject) obj, fid, GetCharField(env,(jobject) val,source_fid));

		   return;
                 }
      case 'S' : {
		   if (c != class_java_lang_Short) break;
		   source_fid = (*env)->GetFieldID(env,c,"value","S");
		   if (!source_fid) break;
				   
		   if (st)
			(*env)->SetStaticShortField(env, c, fid, GetShortField(env,(jobject) val,source_fid));
                   else
			(*env)->SetShortField(env, (jobject) obj, fid, GetShortField(env,(jobject) val,source_fid));

		   return;
                 }
      case 'Z' : {
		   if (c != class_java_lang_Boolean) break;
		   source_fid = (*env)->GetFieldID(env,c,"value","Z");
		   if (!source_fid) break;
				   
		   if (st)
			(*env)->SetStaticBooleanField(env, c, fid, GetBooleanField(env,(jobject) val,source_fid));
                   else
			(*env)->SetBooleanField(env, (jobject) obj, fid, GetBooleanField(env,(jobject) val,source_fid));

		   return;
                 }
      case '[' :
      case 'L' : {
		   if (st)
			(*env)->SetStaticObjectField(env, c, fid, (jobject) val);
                   else
			(*env)->SetObjectField(env, (jobject) obj, fid, (jobject) val);

		   return;
                 }
    }
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    setBoolean
 * Signature: (Ljava/lang/Object;Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setBoolean ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj, s4 val)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) { 
	  (*env)->SetBooleanField (env, (jobject) obj, fid, val);
	  return;
      }
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    setByte
 * Signature: (Ljava/lang/Object;B)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setByte ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj, s4 val)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) {
	  (*env)->SetByteField (env, (jobject) obj, fid, val);
	  return;
      }
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    setChar
 * Signature: (Ljava/lang/Object;C)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setChar ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj, s4 val)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) {
	  (*env)->SetCharField (env, (jobject) obj, fid, val);
	  return;
      }
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    setDouble
 * Signature: (Ljava/lang/Object;D)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setDouble ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj, double val)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) {
	  (*env)->SetDoubleField (env, (jobject) obj, fid, val);
	  return;
      } 
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    setFloat
 * Signature: (Ljava/lang/Object;F)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setFloat ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj, float val)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) {
	  (*env)->SetFloatField (env, (jobject) obj, fid, val);
	  return;
      }
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    setInt
 * Signature: (Ljava/lang/Object;I)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setInt ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj, s4 val)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) {
	  (*env)->SetIntField (env, (jobject) obj, fid, val);
	  return;
      }
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    setLong
 * Signature: (Ljava/lang/Object;J)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setLong ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj, s8 val)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) {
	  (*env)->SetLongField (env, (jobject) obj, fid, val);
	  return;
      }
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
}

/*
 * Class:     java/lang/reflect/Field
 * Method:    setShort
 * Signature: (Ljava/lang/Object;S)V
 */
JNIEXPORT void JNICALL Java_java_lang_reflect_Field_setShort ( JNIEnv *env ,  struct java_lang_reflect_Field* this, struct java_lang_Object* obj, s4 val)
{
  jfieldID fid;

  if (this->declaringClass && obj) {

      fid = class_findfield_approx ((classinfo *) this->declaringClass,javastring_toutf(this->name, false));

      if (fid) {
	  (*env)->SetShortField (env, (jobject) obj, fid, val);
	  return;
      }
  }

  exceptionptr = native_new_and_init(class_java_lang_IllegalArgumentException);
}


/*
 * Class:     java_lang_reflect_Field
 * Method:    getType
 * Signature: ()Ljava/lang/Class;
 */
JNIEXPORT struct java_lang_Class* JNICALL Java_java_lang_reflect_Field_getType (JNIEnv *env ,  struct java_lang_reflect_Field* this )
{
/*	log_text("Java_java_lang_reflect_Field_getType");*/
	utf *desc=(((struct classinfo*)this->declaringClass)->fields[this->slot]).descriptor;
	if (!desc) return NULL;
	return class_from_descriptor(desc->text,utf_end(desc),NULL,true);
}


/*
 * Class:     java_lang_reflect_Field
 * Method:    getModifiers
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_reflect_Field_getModifiers (JNIEnv *env ,  struct java_lang_reflect_Field* this ) 
{
/*	log_text("Java_java_lang_reflect_Field_getType");*/
	return (((struct classinfo*)this->declaringClass)->fields[this->slot]).flags;
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
