/* Table of native methods calls: nativecalls.h       */
/*     This file should be updated manually if        */
/*     a new Native class/method is added to CACAO    */
/*     with the methods the new native method calls.  */


/*------------------------------------*/

{"java/lang/Object",
{
{"getClass" , "()Ljava/lang/Class;",
        {
        {"java/lang/VMClass" , "<init>",  "(Lgnu/classpath/RawData;)V"},
        }
},


},
1,  {1}
},
/*------------------------------------*/


{"java/lang/reflect/Constructor",
{
{"constructNative" , "([Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;",
        {
        {"gnu/java/io/decode/Decoder" , "<clinit>",  "()V"},
        {"gnu/java/io/decode/Decoder8859_1" , "<clinit>",  "()V"},
        {"gnu/java/io/decode/Decoder8859_1" , "<init>",  "(Ljava/io/InputStream;)V"},
        {"gnu/java/io/encode/Encoder" , "<clinit>",  "()V"},
        {"gnu/java/io/encode/Encoder8859_1" , "<clinit>",  "()V"},
        {"gnu/java/io/encode/Encoder8859_1" , "<init>",  "(Ljava/io/OutputStream;)V"},
        {"gnu/java/net/protocol/file/Handler" , "<init>",  "()V"},
        }
},

},
1,  {7 }
},
/*------------------------------------*/


{"java/lang/Thread",
{
{"sleep" , "(JI)V",
        {
        {"Runner" , "run",  "()V"},
        }
},

},
1,  {1}
},
/*------------------------------------*/


{"java/lang/VMClass",
{
{"getDeclaredConstructors" , "(Z)[Ljava/lang/reflect/Constructor;",
        {
        {"java/lang/reflect/Constructor" , "<init>",  "()V"},
        }
},
{"forName" , "(Ljava/lang/String;)Ljava/lang/Class;",
        {
        {"java/lang/VMClass" , "<init>",  "(Lgnu/classpath/RawData;)V"},
        }
},

},
2,  {1 ,1}
},
/*------------------------------------*/


{"java/lang/VMSystem",
{
{"identityHashCode" , "(Ljava/lang/Object;)I",
	{
        {"java/lang/Class" , "getConstructor",  "([Ljava/lang/Class;)Ljava/lang/reflect/Constructor;"},
        {"java/lang/Class" , "getDeclaredConstructors",  "(Z)[Ljava/lang/reflect/Constructor;"},
        {"java/lang/reflect/Constructor" , "<init>",  "()V"},
	/* really getDeclaredConstructors calls VMClass.getDeclaredConstructors 	*/ 
	/* 	which is not NATIVE & has no constructor to use to flag class as used 	*/
	}
},

},
1,  {3}
},
/*------------------------------------*/


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
