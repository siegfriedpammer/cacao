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




