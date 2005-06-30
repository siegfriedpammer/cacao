public class extest {
    static boolean printStackTrace;

    public static void main(String[] argv) {
	printStackTrace = false;
	if (argv.length > 0) 
            if (argv[0].equals("stacktrace"))
                printStackTrace = true;

        boolean catched = false;

	pln("---------- normal exceptions --------------------");

   	try {
            p("throw new Exception():");
    	    throw new Exception();
    	} catch (Exception e) {
            catched = true;
      	    pln("OK");
	    pstacktrace(e);

    	} finally {
            /* check if catch block was executed */
            if (!catched) {
                pln("FAILED");
            }
        }

   	try {
            p("throw new Exception() (from subroutines):");
    	    sub();
            pln("FAILED");
    	} catch (Exception e) {
      	    pln("OK");
	    pstacktrace(e);
    	}

        try {
            p("NullPointerException:");
            int[] ia = null;
            int i = ia.length;
            pln("FAILED");
        } catch (NullPointerException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

	pln();


	pln("---------- test soft inline exceptions ----------");
	pln("/* thrown twice to check the inline jump code */");

        try {
            p("ArrayIndexOutOfBoundsException:");
            int[] ia = new int[1];
            ia[0xcafebabe] = 1;
            pln("FAILED");
        } catch (ArrayIndexOutOfBoundsException e) {
	    String msg = e.getMessage();

	    if (msg == null || !msg.equals(String.valueOf(0xcafebabe))) {
		pln("FAILED: wrong index: " + msg + ", should be: " + 0xcafebabe);
		pstacktrace(e);
	    } else {
		pln("OK");
	        pstacktrace(e);
	    }
  	}

        try {
            p("ArrayIndexOutOfBoundsException:");
            int[] ia = new int[1];
            ia[0xbabecafe] = 1;
            pln("FAILED");
        } catch (ArrayIndexOutOfBoundsException e) {
	    String msg = e.getMessage();

	    if (msg == null || !msg.equals(String.valueOf(0xbabecafe))) {
		pln("FAILED: wrong index: " + msg + ", should be: " + 0xbabecafe);
		pstacktrace(e);
	    } else {
		pln("OK");
	        pstacktrace(e);

	    }
  	}


        try {
            p("NegativeArraySizeException:");
            int[] ia = new int[-1];
            pln("FAILED");
        } catch (NegativeArraySizeException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

        try {
            p("NegativeArraySizeException:");
            int[] ia = new int[-1];
            pln("FAILED");
        } catch (NegativeArraySizeException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

        
        try {
            p("ClassCastException:");
            Object o = new Object();
            Integer i = (Integer) o;
            pln("FAILED");
        } catch (ClassCastException e) {
  	    pln("OK");
  	}

        try {
            p("ClassCastException:");
            Object o = new Object();
            Integer i = null;
            i = (Integer) o;
            pln("FAILED");
        } catch (ClassCastException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}


        try {
            p("ArithmeticException (only w/ -softnull):");
            int i = 1, j = 0, k = i / j;
            pln("FAILED");
        } catch (ArithmeticException e) {
	    String msg = e.getMessage();

	    if (msg == null || !msg.equals("/ by zero")) {
		pln("FAILED: wrong message: " + msg + ", should be: / by zero");
		pstacktrace(e);
	    } else {
                pln("OK");
                pstacktrace(e);
            }
  	}

        try {
            p("ArithmeticException (only w/ -softnull):");
            long i = 1, j = 0, k = i / j;
            pln("FAILED");
        } catch (ArithmeticException e) {
	    String msg = e.getMessage();

	    if (msg == null || !msg.equals("/ by zero")) {
		pln("FAILED: wrong message: " + msg + ", should be: / by zero");

	    } else {
                pln("OK");
                pstacktrace(e);
            }
  	}


        try {
            p("OutOfMemoryError:");
	    /* 100 MB should be enough */
	    byte[] ba = new byte[100 * 1024 * 1024];
            pln("FAILED");
        } catch (OutOfMemoryError e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

        try {
            p("OutOfMemoryError:");
	    /* 100 MB should be enough */
	    byte[] ba = new byte[100 * 1024 * 1024];
            pln("FAILED");
        } catch (OutOfMemoryError e) {
  	    pln("OK");
	    pstacktrace(e);
  	}
        

        try {
            p("NullPointerException (only w/ -softnull):");
            int[] ia = null;
            int i = ia.length;
            pln("FAILED");
        } catch (NullPointerException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

        try {
            p("NullPointerException (only w/ -softnull):");
            int[] ia = null;
            int i = ia.length;
            pln("FAILED");
        } catch (NullPointerException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

	pln();


	pln("---------- some asmpart exceptions --------------");

        try {
            p("NullPointerException in <clinit> (PUTSTATIC):");
            extest_clinit_1.i = 1;
            pln("FAILED");
        } catch (ExceptionInInitializerError e) {
            if (e.getCause().getClass() != NullPointerException.class) {
                pln("FAILED");
            } else {
                pln("OK");
  	        pstacktrace(e);
            }
        }

        try {
            p("NullPointerException in <clinit> (GETSTATIC):");
            int i = extest_clinit_2.i;
            pln("FAILED");
        } catch (ExceptionInInitializerError e) {
            if (e.getCause().getClass() != NullPointerException.class) {
                pln("FAILED");
            } else {
                pln("OK");
  	        pstacktrace(e);
            }
        }

        try {
            p("ArithmeticException (idiv):");
            int i = 1, j = 0, k = i / j;
            pln("FAILED");
        } catch (ArithmeticException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

        try {
            p("ArithmeticException (ldiv):");
            long i = 1, j = 0, k = i / j;
            pln("FAILED");
        } catch (ArithmeticException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

        try {
            p("ArithmeticException (irem):");
            int i = 1, j = 0, k = i % j;
            pln("FAILED");
        } catch (ArithmeticException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

        try {
            p("ArithmeticException (lrem):");
            long i = 1, j = 0, k = i % j;
            pln("FAILED");
        } catch (ArithmeticException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

        try {
            p("NullPointerException (aastore):");
	    Object[] oa = null;
	    oa[0] = new Object();
            pln("FAILED");
        } catch (NullPointerException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

        try {
            p("ArrayIndexOutOfBoundsException (aastore):");
	    Object[] oa = new Object[1];
	    oa[0xaa] = new Object();
            pln("FAILED");
        } catch (ArrayIndexOutOfBoundsException e) {
	    String msg = e.getMessage();

	    if (msg == null || !msg.equals(String.valueOf(0xaa))) {
		pln("FAILED: wrong index: " + msg + ", should be: " + 0xaa);

	    } else {
                pln("OK");
                pstacktrace(e);
            }
  	}

        try {
            p("ArrayStoreException (aastore):");
	    Integer[] ia = new Integer[1];
            Object[] oa = (Object[]) ia;
	    oa[0] = new Object();
            pln("FAILED");
        } catch (ArrayStoreException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

        try {
            p("ClassCastException (checkarraycast):");
            Object[] oa = new Object[1];
	    Integer[] ia = (Integer[]) oa;
            pln("FAILED");
        } catch (ClassCastException e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

	pln();


	pln("---------- exception related things -------------");

        try {
            p("load/link an exception class in asmpart:");
            throw new Exception();
        } catch (UnknownError e) {
            /* this exception class MUST NOT be loaded before!!!
               otherwise this test in useless */
        } catch (Exception e) {
  	    pln("OK");
	    pstacktrace(e);
  	}

        pln();


	pln("---------- native stub exceptions ---------------");

  	try {
            p("NullPointerException (native):");
            System.arraycopy(null, 1, null, 1, 1);
            pln("FAILED");
    	} catch (Exception e) {
  	    pln("OK");
            pstacktrace(e);
  	}

        try {
            p("NullPointerException in <clinit>:");
            extest_clinit_3.sub();
            pln("FAILED");
        } catch (ExceptionInInitializerError e) {
            pln("OK");
            pstacktrace(e);
        } catch (UnsatisfiedLinkError e) {
            /* catch this one for staticvm and say it's ok */
            pln("OK");
            pstacktrace(e);
        }

        pln();


	pln("---------- no OK beyond this point --------------");

        pln("NullPointerException (without catch):");
        String s = null;
        int i = s.length();
        pln("FAILED");
    }

    public synchronized static void sub() throws Exception {
	sub2();
    }

    public static void sub2() throws Exception {
	sub3();
    }

    public synchronized static void sub3() throws Exception {
	sub4();
    }

    public static void sub4() throws Exception {
	throw new Exception();
    }

    public static void p(String s) {
	System.out.print(s);
        for (int i = s.length(); i < 46; i++) {
            System.out.print(" ");
        }
    }

    public static void pln() {
	System.out.println();
    }

    public static void pln(String s) {
	System.out.println(s);
    }

    public static void pstacktrace(Throwable e) {
	if (!printStackTrace)
            return;
	e.printStackTrace();
	System.out.println();
    }
}

public class extest_clinit_1 {
    static {
        String s = null;
        s.length();
    }

    public static int i;
}

public class extest_clinit_2 {
    static {
        String s = null;
        s.length();
    }

    public static int i;
}

public class extest_clinit_3 {
    static {
        String s = null;
        s.length();
    }

    public static native void sub();
}
