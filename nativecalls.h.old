/* Table of native methods calls: nativecalls.h       */
/*     This file should be updated manually if        */
/*     a new Native class/method is added to CACAO    */
/*     with the methods the new native method calls.  */


/*------------------------------------*/

{"java/lang/Object",
{{"clone" , "()Ljava/lang/Object;",
	{
	{"java/lang/CloneNotSupportedException" , "<init>",  "()V"}, /* c-fn new&init */
	{"java/lang/CloneNotSupportedException" , "",  ""}, /*c-fn - why both method& class alone? */
	},
},

{"wait" , "(J)V",
        {
        {"Runner" , "run",  "()V"},
        },
},

},
2,  {2,1 }
},
/*------------------------------------*/

{"java/lang/Class",
{{"forName0" , "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;",
	{
	{"java/lang/ClassNotFoundException" , "<init>",  "()V"}, /* c-fn */
	{"java/lang/ClassLoader" , "loadClass",  "(Ljava/lang/String;)Ljava/lang/Class;"}, /* c-fn */
	{"sun/io/CharToByteISO8859_1" , "<init>",  "()V"},     /* jctest */
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
	{"java/lang/reflect/Field" , "getField0",  "(Ljava/lang/String;I)Ljava/lang/reflect/Field;"},  /* java/lang/Class */
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
	{"java/lang/InstantiationException" , "<init>",  "()V"},   /* super class?? */
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
{{"doPrivileged" , "(Ljava/security/PrivilegedAction;)Ljava/lang/Object;",
	{
	{"java/security/PrivilegedAction" , "run",  "()Ljava/lang/Object;"},    /* c-fn  super class type */
	{"java/security/PrivilegedActionException" , "<init>",  "(Ljava/lang/Exception;)V"},	
								/* c-fn check_forexception call */ 
	{"sun/security/action/GetPropertyAction" , "run",  "()Ljava/lang/Object;"},  /* cfg jctest */
	{"sun/misc/Launcher$1" , "run",  "()Ljava/lang/Object;"},		     /* cfg jctest */
	{"sun/misc/Launcher$4" , "run",  "()Ljava/lang/Object;"},		     /* cfg javac  */
	{"java/util/ResourceBundle$1" , "run",  "()Ljava/lang/Object;"},	     /* cfg javac  */
	{"sun/misc/URLClassPath$2" ,    "run",  "()Ljava/lang/Object;"},	     /* cfg javac  */
	{"java/lang/ref/Reference$ReferenceHandler" , "run",  "()V"},	     /* cfg javac */
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
	{"java/util/Properties" , "",  ""},
	},
},

},
2,  {6, 1 }
},
/*------------------------------------*/

{"java/lang/Thread",
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

{},
0,  {2 }
},
/*------------------------------------*/

{"java/lang/reflect/Array",

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

{{"socketWrite" , "([BII)V",
	{
	{"java/io/IOException" , "<init>",  "()V"},
	{"java/io/IOException" , "",  ""},
	},
},

},
1,  {2 }
},


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
