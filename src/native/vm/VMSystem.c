/* class: java/lang/System */

#if 0
/*
 * Class:     java/lang/System
 * Method:    currentTimeMillis
 * Signature: ()J
 */
JNIEXPORT s8 JNICALL Java_java_lang_VMSystem_currentTimeMillis ( JNIEnv *env )
{
	struct timeval tv;

	(void) gettimeofday(&tv, NULL);
	return ((s8) tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}
#endif

/*
 * Class:     java/lang/System
 * Method:    arraycopy
 * Signature: (Ljava/lang/Object;ILjava/lang/Object;II)V
 */
/* XXX delete */
#if 0
JNIEXPORT void JNICALL Java_java_lang_VMSystem_arraycopy (JNIEnv *env, jclass clazz,struct java_lang_Object* source, s4 sp, struct java_lang_Object* dest, s4 dp, s4 len)
{
	s4 i;
	java_arrayheader *s = (java_arrayheader*) source;
	java_arrayheader *d = (java_arrayheader*) dest;

	printf("arraycopy: %p:%x->%p:%x||len=%d\n",source,sp,dest,dp,len);
	fflush(stdout);


	if (((s == NULL) || (d == NULL)) != 0) { 
		exceptionptr = proto_java_lang_NullPointerException; 
		return; 
		}
	log_text("Passed nullpointer check");
	if (s->objheader.vftbl->class != class_array) {
		exceptionptr = proto_java_lang_ArrayStoreException; 
		return; 
		}

	log_text("Passed array storeexception");
	if (((sp<0) | (sp+len > s->size) | (dp<0) | (dp+len > d->size)) != 0) {
		exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException; 
		return; 
		}

	log_text("Passed array out of bounds exception");
	printf("ARRAY TYPE: %d\n",s->arraytype);

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
	printf("CHARARRAY:");
	if (len>0) {
		utf_display(utf_new_u2(((java_chararray*)d)->data+sp,len*sizeof(u2), 0));
	}
	printf("\n");

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
#endif
JNIEXPORT void JNICALL Java_java_lang_VMSystem_arraycopy (JNIEnv *env, jclass clazz,struct java_lang_Object* source, s4 sp, struct java_lang_Object* dest, s4 dp, s4 len)
{
	s4 i;
	java_arrayheader *s = (java_arrayheader*) source;
	java_arrayheader *d = (java_arrayheader*) dest;
        arraydescriptor *sdesc;
        arraydescriptor *ddesc;

/*	printf("arraycopy: %p:%x->%p:%x\n",source,sp,dest,dp);
	fflush(stdout);*/

	if (!s || !d) { 
            exceptionptr = proto_java_lang_NullPointerException; 
            return; 
        }

        sdesc = s->objheader.vftbl->arraydesc;
        ddesc = d->objheader.vftbl->arraydesc;

        if (!sdesc || !ddesc || (sdesc->arraytype != ddesc->arraytype)) {
            exceptionptr = proto_java_lang_ArrayStoreException; 
            return; 
        }

	if ((len<0) || (sp<0) || (sp+len > s->size) || (dp<0) || (dp+len > d->size)) {
            exceptionptr = proto_java_lang_ArrayIndexOutOfBoundsException; 
            return; 
        }

        if (sdesc->componentvftbl == ddesc->componentvftbl) {
            /* We copy primitive values or references of exactly the same type */
            s4 dataoffset = sdesc->dataoffset;
            s4 componentsize = sdesc->componentsize;
            memmove(((u1*)d) + dataoffset + componentsize * dp,
                    ((u1*)s) + dataoffset + componentsize * sp,
                    (size_t) len * componentsize);
        }
        else {
            /* We copy references of different type */
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
                /* XXX this does not completely obey the specification!
                 * If an exception is thrown only the elements above
                 * the current index have been copied. The
                 * specification requires that only the elements
                 * *below* the current index have been copied before
                 * the throw.
                 */
                for (i=len-1; i>=0; i--) {
                    java_objectheader *o = oas->data[sp+i];
                    if (!builtin_canstore(oad, o)) {
                        exceptionptr = proto_java_lang_ArrayStoreException;
                        return;
                    }
                    oad->data[dp+i] = o;
                }
        }
}

void attach_property (char *name, char *value)
{
	log_text("attach_property not supported");
#if 0
	if (activeprops >= MAXPROPS) panic ("Too many properties defined");
	proplist[activeprops][0] = name;
	proplist[activeprops][1] = value;
	activeprops++;
#endif
}

/*
 * Class:     java/lang/System
 * Method:    identityHashCode
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMSystem_identityHashCode (JNIEnv *env , jclass clazz, struct java_lang_Object* par1)
{
	return ((char*) par1) - ((char*) 0);	
}




