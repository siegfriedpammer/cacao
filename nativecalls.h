/* Table of native methods calls: nativecalls.h       */
/*     This file should be updated manually if        */
/*     a new Native class/method is added to CACAO    */
/*     with the methods the new native method calls.  */


/*------------------------------------*/

{"java/lang/Object",
//nn 	class_java_lang_Object,
{{"clone" , "()Ljava/lang/Object;",
// Requires manual list of dynamically loaded classes/methods
	{
	{"java/lang/CloneNotSupportedException" , "<init>",  "()V"}, /* c-fn new&init */
	{"java/lang/CloneNotSupportedException" , "",  ""}, /*c-fn - why both method& class alone? */
	},
},

},
1,  {2 }
},
/*------------------------------------*/

{"java/lang/Class",
//nn	class_java_lang_Class  //--> not  extren defined
{{"forName0" , "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;",
	{
	{"java/lang/ClassNotFoundException" , "<init>",  "()V"}, /* c-fn */
	{"java/lang/ClassLoader" , "loadClass",  "(Ljava/lang/String;)Ljava/lang/Class;"}, /* c-fn */
	{"sun/io/CharToByteISO8859_1" , "<init>",  "()V"},     // jctest
	{"java/lang/CloneNotSupportedException" , "",  ""},
	{"java/lang/Class" , "",  ""},
	},
},

{"getClassLoader0" , "()Ljava/lang/ClassLoader;",
	{
	{"java/lang/ClassLoader" , "<init>",  "()V"},
	{"java/lang/ClassLoader" , "",  ""},
	},
},

{"getField0" , "(Ljava/lang/String;I)Ljava/lang/reflect/Field;",
	{
	{"java/lang/reflect/Field" , "<init>",  "()V"},
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/reflect/Field" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"getFields0" , "(I)[Ljava/lang/reflect/Field;",
	{
	{"java/lang/reflect/Field" , "getField0",  "(Ljava/lang/String;I)Ljava/lang/reflect/Field;"},  //java/lang/Class
	{"java/lang/reflect/Field" , "",  ""},
	{"java/lang/String" , "",  ""},
	},
},

{"getMethod0" , "(Ljava/lang/String;[Ljava/lang/Class;I)Ljava/lang/reflect/Method;",
	{
	{"java/lang/reflect/Method" , "<init>",  "()V"},
	{"java/lang/NoSuchMethodException" , "<init>",  "()V"},
	{"java/lang/reflect/Method" , "",  ""},
	{"java/lang/NoSuchMethodException" , "",  ""},
	{"java/lang/String" , "",  ""},
	},
},

{"getMethods0" , "(I)[Ljava/lang/reflect/Method;",
	{
	{"java/lang/reflect/Method" , "<init>",  "()V"},
	{"java/lang/reflect/Method" , "",  ""},
	{"java/lang/String" , "",  ""},
	},
},

{"getPrimitiveClass" , "(Ljava/lang/String;)Ljava/lang/Class;",
	{
	{"java/lang/CloneNotSupportedException" , "<init>",  "()V"},
	{"java/lang/CloneNotSupportedException" , "",  ""},
	{"java/lang/Class" , "",  ""},
	},
},

{"newInstance0" , "()Ljava/lang/Object;",
	{
	{"java/lang/InstantiationException" , "<init>",  "()V"},   // super class??
	{"sun/io/CharToByteISO8859_1" , "<init>",  "()V"},
	{"sun/io/ByteToCharISO8859_1" , "<init>",  "()V"},  /* javac .. */
	{"sun/net/www/protocol/file/Handler" , "<init>",  "()V"},  /* javac .. */
	{"sun/net/www/protocol/jar/Handler" , "<init>",  "()V"},  /* javac .. */
	{"java/lang/InstantiationException" , "",  ""},
	},
},

},
8,  {5, 2, 4, 3, 5, 3, 3, 6 }
},
/*------------------------------------*/

{"java/lang/ClassLoader",
//nn	class_java_lang_ClassLoader;   // in native.c not sure extern def'd
{{"getCallerClassLoader" , "()Ljava/lang/ClassLoader;",
	{
	{"java/lang/ClassLoader" , "<init>",  "()V"},
	{"java/lang/ClassLoader" , "",  ""},
	},
},

},
1,  {2 }
},
/*------------------------------------*/

{"java/security/AccessController",
//nn	class_java_security_AccessController;  // NOT yet defined at all
{{"doPrivileged" , "(Ljava/security/PrivilegedAction;)Ljava/lang/Object;",
	{
	{"java/security/PrivilegedAction" , "run",  "()Ljava/lang/Object;"},    /* c-fn  super class type */
	{"java/security/PrivilegedActionException" , "<init>",  "(Ljava/lang/Exception;)V"},	
								/* c-fn check_forexception call */ 
	{"sun/security/action/GetPropertyAction" , "run",  "()Ljava/lang/Object;"},  // cfg jctest
	{"sun/misc/Launcher$1" , "run",  "()Ljava/lang/Object;"},		     // cfg jctest
	{"sun/misc/Launcher$4" , "run",  "()Ljava/lang/Object;"},		     // cfg javac
	{"java/util/ResourceBundle$1" , "run",  "()Ljava/lang/Object;"},	     // cfg javac
	{"sun/misc/URLClassPath$2" ,    "run",  "()Ljava/lang/Object;"},	     // cfg javac
	{"java/lang/ref/Reference$ReferenceHandler" , "run",  "()V"},	     // cfg javac
	{"java/security/PrivilegedActionException" , "",  ""},  		/* c-fn check_forexception call */
	},
},

{"doPrivileged" , "(Ljava/security/PrivilegedAction;Ljava/security/AccessControlContext;)Ljava/lang/Object;",
	{
	{"java/security/AccessController" , "doPrivileged",  "(Ljava/security/PrivilegedAction;)Ljava/lang/Object;"},
	},									/* c-fn */
},

{"doPrivileged" , "(Ljava/security/PrivilegedExceptionAction;)Ljava/lang/Object;",
	{
	{"java/security/PrivilegedExceptionAction" , "run",  "()Ljava/lang/Object;"}, /* c-fn  super class type */
	{"java/security/PrivilegedActionException" , "<init>",  "(Ljava/lang/Exception;)V"},	
										/* c-fn check_forexception call */
	{"java/security/PrivilegedActionException" , "",  ""},			/* c-fn check_forexception call */
	},
},

{"doPrivileged" , "(Ljava/security/PrivilegedExceptionAction;Ljava/security/AccessControlContext;)Ljava/lang/Object;",
	{
	{"java/security/AccessController" , "doPrivileged",  "(Ljava/security/PrivilegedExceptionAction;)Ljava/lang/Object;"},
	},									/* c-fn */
},

},
4,  {9, 1, 3, 1 }
},
/*------------------------------------*/

{"java/lang/reflect/Field",
//nn	class_java_lang_reflect_Field;   // not yet defined at all
{{"get" , "(Ljava/lang/Object;)Ljava/lang/Object;",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/NoSuchFieldException" , "",  ""},
	{"java/lang/Integer" , "",  ""},
	{"java/lang/Long" , "",  ""},
	{"java/lang/Float" , "",  ""},
	{"java/lang/Double" , "",  ""},
	{"java/lang/Byte" , "",  ""},
	{"java/lang/Character" , "",  ""},
	{"java/lang/Short" , "",  ""},
	{"java/lang/Boolean" , "",  ""},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"getBoolean" , "(Ljava/lang/Object;)Z",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"getByte" , "(Ljava/lang/Object;)B",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"getChar" , "(Ljava/lang/Object;)C",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"getDouble" , "(Ljava/lang/Object;)D",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"getFloat" , "(Ljava/lang/Object;)F",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"getInt" , "Ljava/lang/Object;)I",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"getLong" , "(Ljava/lang/Object;)J",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"getShort" , "(Ljava/lang/Object;)S",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"set" , "(Ljava/lang/Object;Ljava/lang/Object;)V",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"setBoolean" , "(Ljava/lang/Object;Z)V",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"setByte" , "(Ljava/lang/Object;B)V",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"setChar" , "(Ljava/lang/Object;C)V",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"setDouble" , "(Ljava/lang/Object;D)V",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"setFloat" , "(Ljava/lang/Object;F)V",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"setInt" , "(Ljava/lang/Object;I)V",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"setLong" , "(Ljava/lang/Object;J)V",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

{"setShort" , "(Ljava/lang/Object;S)V",
	{
	{"java/lang/NoSuchFieldException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	{"java/lang/NoSuchFieldException" , "",  ""},
	},
},

},
18,  {12, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 }
},
/*------------------------------------*/

{"java/io/FileDescriptor",
//nn	class_java_io_FileDescriptor;   // not yet defined at all
{{"sync" , "()V",
	{
	{"java/io/SyncFailedException" , "<init>",  "()V"},
	{"java/io/SyncFailedException" , "",  ""},
	},
},

},
1,  {2 }
},
/*------------------------------------*/

{"java/io/FileInputStream",
//nn	class_java_io_FileInputStream;   // not yet defined at all

{{"available" , "()I",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"close" , "()V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"open" , "(Ljava/lang/String;)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"read" , "()I",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"readBytes" , "([BII)I",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"skip" , "(J)J",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

},
6,  {2, 2, 2, 2, 2, 2 }
},
/*------------------------------------*/

{"java/io/FileOutputStream",
//nn	class_java_io_FileOutputStream;   // not yet defined at all

{{"close" , "()V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"open" , "(Ljava/lang/String;)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"openAppend" , "(Ljava/lang/String;)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"write" , "(I)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"writeBytes" , "([BII)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

},
5,  {2, 2, 2, 2, 2 }
},
/*------------------------------------*/

{"java/io/FileSystem",
//nn	class_java_io_FileSystem;   // not yet defined at all

{{"getFileSystem" , "()Ljava/io/FileSystem;",
	{
	{"java/io/UnixFileSystem" , "<clinit>",  "()V"},  /* from running javac - maybe c-fn */
	{"java/io/UnixFileSystem" , "<init>",  "()V"},    /* c-fn */
	{"java/io/UnixFileSystem" , "",  ""},             /* c-fn */
	},
},

},
1,  {2 }
},
/*------------------------------------*/

{"java/io/ObjectInputStream",
//nn	class_java_io_ObjectInputStream;   // not yet defined at all

{{"allocateNewArray" , "(Ljava/lang/Class;I)Ljava/lang/Object;",
	{
	{"java/lang/reflect/Array" , "newArray",  "(Ljava/lang/Class;I)Ljava/lang/Object;"},
	},
},

{"allocateNewObject" , "(Ljava/lang/Class;Ljava/lang/Class;)Ljava/lang/Object;",
	{
	{"java/lang/Class" , "newInstance0",  "()Ljava/lang/Object;"},
	},
},

{"loadClass0" , "(Ljava/lang/Class;Ljava/lang/String;)Ljava/lang/Class;",
	{
	{"java/lang/Class" , "forName0",  "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;"},
	},
},

},
3,  {1, 1, 1 }
},
/*------------------------------------*/

{"java/io/RandomAccessFile",
//nn	class_java_io_RandomAccessFile;   // not yet defined at all

{{"close" , "()V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"getFilePointer" , "()J",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"length" , "()J",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"open" , "(Ljava/lang/String;Z)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"read" , "()I",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"readBytes" , "([BII)I",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"seek" , "(J)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"setLength" , "(J)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"write" , "(I)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"writeBytes" , "([BII)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

},
10,  {2, 2, 2, 2, 2, 2, 2, 2, 2, 2 }
},
/*------------------------------------*/

{"java/lang/SecurityManager",
//nn	class_java_lang_SecurityManager;   // not yet defined at all

{{"currentClassLoader0" , "()Ljava/lang/Class;",
	{
	{"java/lang/ClassLoader" , "<init>",  "()V"},
	{"java/lang/ClassLoader" , "",  ""},
	},
},

},
1,  {2 }
},
/*------------------------------------*/

{"java/lang/System",
//nn	class_java_lang_System;   // in native.c not sure if extern defined 

{{"arraycopy" , "(Ljava/lang/Object;ILjava/lang/Object;II)V",
	{
	{"java/lang/NullPointerException" , "<init>",  "()V"},
	{"java/lang/ArrayStoreException" , "<init>",  "()V"},
	{"java/lang/ArrayIndexOutOfBoundsException" , "<init>",  "()V"},
	{"java/lang/NullPointerException" , "",  ""},
	{"java/lang/ArrayStoreException" , "",  ""},
	{"java/lang/ArrayIndexOutOfBoundsException" , "",  ""},
	},
},

{"initProperties" , "(Ljava/util/Properties;)Ljava/util/Properties;",
	{
	//{"java/util/Properties" , "put",  "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"},
	{"java/util/HashTable" , "put",  "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"},
	{"java/util/Properties" , "",  ""},
	},
},

},
2,  {6, 2 }
},
/*------------------------------------*/

{"java/lang/Thread",
//nn	class_java_lang_Thread;  // not yet defined anywhere
{{"currentThread" , "()Ljava/lang/Thread;",
	{
	{"java/lang/ThreadGroup" , "<init>",  "()V"},
	{"java/lang/ThreadGroup" , "",  ""},
	},
},

},
1,  {2 }
},
/*------------------------------------*/

{"java/util/zip/Adler32",
//nn	class_java_util_zip_Adler32;  // not yet defined anywhere

{},
0,  {2 }
},
/*------------------------------------*/

{"java/lang/reflect/Array",
//nn	class_java_lang_reflect_Array;  // not yet defined anywhere

{{"get" , "(Ljava/lang/Object;I)Ljava/lang/Object;",
	{
	{"java/lang/Integer" , "<init>",  "(I)V"},
	{"java/lang/Long" , "<init>",  "(J)V"},
	{"java/lang/Float" , "<init>",  "(F)V"},
	{"java/lang/Double" , "<init>",  "(D)V"},
	{"java/lang/Byte" , "<init>",  "(B)V"},
	{"java/lang/Char" , "<init>",  "(C)V"},
	{"java/lang/Short" , "<init>",  "(S)V"},
	{"java/lang/Boolean" , "<init>",  "(Z)V"},
	{"java/lang/ArrayIndexOutOfBoundsException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/ArrayIndexOutOfBoundsException" , "",  ""},
	{"java/lang/Integer" , "",  ""},
	{"java/lang/Long" , "",  ""},
	{"java/lang/Float" , "",  ""},
	{"java/lang/Double" , "",  ""},
	{"java/lang/Byte" , "",  ""},
	{"java/lang/Char" , "",  ""},
	{"java/lang/Short" , "",  ""},
	{"java/lang/Boolean" , "",  ""},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"getBoolean" , "(Ljava/lang/Object;I)Z",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"getByte" , "(Ljava/lang/Object;I)B",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"getChar" , "(Ljava/lang/Object;I)C",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"getDouble" , "(Ljava/lang/Object;I)D",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"getFloat" , "(Ljava/lang/Object;I)F",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"getInt" , "(Ljava/lang/Object;I)I",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"getLong" , "(Ljava/lang/Object;I)J",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"getShort" , "(Ljava/lang/Object;I)S",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"getLength" , "(Ljava/lang/Object;)I",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"multiNewArray" , "(Ljava/lang/Class;[I)Ljava/lang/Object;",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"newArray" , "(Ljava/lang/Class;I)Ljava/lang/Object;",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"set" , "(Ljava/lang/Object;ILjava/lang/Object;)V",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"setBoolean" , "(Ljava/lang/Object;IZ)V",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"setByte" , "(Ljava/lang/Object;IB)V",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"setChar" , "(Ljava/lang/Object;IC)V",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"setDouble" , "(Ljava/lang/Object;ID)V",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"setFloat" , "(Ljava/lang/Object;IF)V",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"setInt" , "(Ljava/lang/Object;II)V",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"setLong" , "(Ljava/lang/Object;IJ)V",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

{"setShort" , "(Ljava/lang/Object;IS)V",
	{
	{"java/lang/IllegalArgumentException" , "<init>",  "()V"},
	{"java/lang/IllegalArgumentException" , "",  ""},
	},
},

},
21,  {20, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 }
},
/*------------------------------------*/

{"java/net/InetAddressImpl",
//nn 	javar_net_InetAddressImpl;  // not yet defined anywhere


{{"getHostByAddr" , "(I)Ljava/lang/String;",
	{
	{"java/net/UnknownHostException" , "<init>",  "()V"},
	{"java/net/UnknownHostException" , "",  ""},
	},
},

{"lookupAllHostAddr" , "(Ljava/lang/String;)[[B",
	{
	{"java/net/UnknownHostException" , "<init>",  "()V"},
	{"java/net/UnknownHostException" , "",  ""},
	},
},

},
2,  {2, 2 }
},
/*------------------------------------*/

{"java/lang/Math",
//nn 	java_lang_Math;  // not yet defined anywhere 

{{"log" , "(D)D",
	{
	{"java/lang/ArithmeticException" , "<init>",  "()V"},
	{"java/lang/ArithmeticException" , "",  ""},
	},
},

{"sqrt" , "(D)D",
	{
	{"java/lang/ArithmeticException" , "<init>",  "()V"},
	{"java/lang/ArithmeticException" , "",  ""},
	},
},

},
2,  {2, 2 }
},
/*------------------------------------*/

{"java/net/PlainDatagramSocketImpl",
//nn	java_net_PlainDatagramSocketImpl; // not yet defined anywhere

{{"bind" , "(ILjava/net/InetAddress;)V",
	{
	{"java/net/SocketException" , "<init>",  "()V"},
	{"java/net/SocketException" , "",  ""},
	},
},

{"datagramSocketClose" , "()V",
	{
	{"java/net/SocketException" , "<init>",  "()V"},
	{"java/net/SocketException" , "",  ""},
	},
},

{"datagramSocketCreate" , "()V",
	{
	{"java/net/SocketException" , "<init>",  "()V"},
	{"java/net/SocketException" , "",  ""},
	},
},

{"getTTL" , "()B",
	{
	{"java/net/PlainDatagramSocketImpl" , "getTimeToLive",  "()I"},
	},
},

{"getTimeToLive" , "()I",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"join" , "(Ljava/net/InetAddress;)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"leave" , "(Ljava/net/InetAddress;)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"peek" , "(Ljava/net/InetAddress;)I",
	{
	{"java/net/SocketException" , "<init>",  "()V"},
	{"java/net/SocketException" , "",  ""},
	},
},

{"receive" , "(Ljava/net/DatagramPacket;)V",
	{
	{"java/net/SocketException" , "<init>",  "()V"},
	{"java/net/InetAddress" , "<init>",  "()V"},
	{"java/net/SocketException" , "",  ""},
	{"java/net/InetAddress" , "",  ""},
	},
},

{"send" , "(Ljava/net/DatagramPacket;)V",
	{
	{"java/net/SocketException" , "<init>",  "()V"},
	{"java/net/SocketException" , "",  ""},
	},
},

{"setTTL" , "(B)V",
	{
	{"java/net/PlainDatagramSocketImpl" , "setTimeToLive",  "(I)V"},
	},
},

{"setTimeToLive" , "(I)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"socketGetOption" , "(I)I",
	{
	{"java/net/SocketException" , "<init>",  "()V"},
	{"java/net/SocketException" , "",  ""},
	},
},

{"socketSetOption" , "(ILjava/lang/Object;)V",
	{
	{"java/net/SocketException" , "<init>",  "()V"},
	{"java/net/SocketException" , "",  ""},
	},
},

{"leave" , "(Ljava/net/InetAddress;)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

},
15,  {2, 2, 2, 1, 2, 2, 2, 2, 4, 2, 1, 2, 2, 2, 2 }
},
/*------------------------------------*/

{"java/net/PlainSocketImpl",
//nn	 java_net_PlainSocketImpl;  // not yet defined anywhere

{{"socketAccept" , "(Ljava/net/SocketImpl;)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"socketAvailable" , "()I",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"socketBind" , "(Ljava/net/InetAddress;I)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"socketClose" , "()V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"socketConnect" , "(Ljava/net/InetAddress;I)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"socketCreate" , "(Z)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"socketGetOption" , "(I)I",
	{
	{"java/net/SocketException" , "<init>",  "()V"},
	{"java/net/SocketException" , "",  ""},
	},
},

{"socketListen" , "(I)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

{"socketSetOption" , "(IZLjava/lang/Object;)V",
	{
	{"java/net/SocketException" , "<init>",  "()V"},
	{"java/net/SocketException" , "",  ""},
	},
},

},
9,  {2, 2, 2, 2, 2, 2, 2, 2, 2 }
},
/*------------------------------------*/

{"java/net/SocketInputStream",
//nn	java_net_SocketInputStream;  // not yet defined anywhere

{{"socketRead" , "([BII)I",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

},
1,  {2 }
},
/*------------------------------------*/

{"java/net/SocketOutputStream",
//nn 	java_net_SocketOutputStream;  // not yet used anywhere

{{"socketWrite" , "([BII)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

},
1,  {2 }
},
