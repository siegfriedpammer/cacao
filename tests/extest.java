public class extest {
    public static void main(String[] argv) { 
	p("---------- normal exceptions --------------------");

   	try {
            System.out.print("throw new Exception():          ");
    	    sub();
            p("FAILED");
    	} catch (Exception e) {
      	    p("PASSED");
    	}

        try {
            System.out.print("NullPointerException:           ");
            int[] ia = null;
            int i = ia.length;
            p("FAILED");
        } catch (NullPointerException e) {
  	    p("PASSED");
  	}

  	try {
            System.out.print("NullPointerException (native):  ");
            System.arraycopy(null, 1, null, 1, 1);
            p("FAILED");
    	} catch (Exception e) {
  	    p("PASSED");
  	}

	p();


	p("---------- test soft inline exceptions ----------");
	p("/* throw and catch inline exceptions twice to check the inline jump code */");

        try {
            System.out.print("ArrayIndexOutOfBoundsException: ");
            int[] ia = new int[1];
            ia[0xcafebabe] = 1;
            p("FAILED");
        } catch (ArrayIndexOutOfBoundsException e) {
	    String msg = e.getMessage();

	    if (msg != null && msg.compareTo("Array index out of range: -889275714") != 0) {
		p("FAILED: wrong index");

	    } else {
		p("PASSED");
	    }
  	}

        try {
            System.out.print("ArrayIndexOutOfBoundsException: ");
            int[] ia = new int[1];
            ia[0xcafebabe] = 1;
            p("FAILED");
        } catch (ArrayIndexOutOfBoundsException e) {
	    String msg = e.getMessage();

	    if (msg != null && msg.compareTo("Array index out of range: -889275714") != 0) {
		p("FAILED: wrong index");

	    } else {
		p("PASSED");
	    }
  	}


        try {
            System.out.print("NegativeArraySizeException:     ");
            int[] ia = new int[-1];
            p("FAILED");
        } catch (NegativeArraySizeException e) {
  	    p("PASSED");
  	}

        try {
            System.out.print("NegativeArraySizeException:     ");
            int[] ia = new int[-1];
            p("FAILED");
        } catch (NegativeArraySizeException e) {
  	    p("PASSED");
  	}

        
        try {
            System.out.print("ClassCastException:             ");
            Object o = new Object();
            Integer i = null;
            i = (Integer) o;
            p("FAILED");
        } catch (ClassCastException e) {
  	    p("PASSED");
  	}

        try {
            System.out.print("ClassCastException:             ");
            Object o = new Object();
            Integer i = null;
            i = (Integer) o;
            p("FAILED");
        } catch (ClassCastException e) {
  	    p("PASSED");
  	}


        try {
            System.out.print("OutOfMemoryError:               ");
	    /* 100 MB should be enough */
	    byte[] ba = new byte[100 * 1024 * 1024];
            p("FAILED");
        } catch (OutOfMemoryError e) {
  	    p("PASSED");
  	}

        try {
            System.out.print("OutOfMemoryError:               ");
	    /* 100 MB should be enough */
	    byte[] ba = new byte[100 * 1024 * 1024];
            p("FAILED");
        } catch (OutOfMemoryError e) {
  	    p("PASSED");
  	}
        
	p();


	p("---------- some asmpart exceptions --------------");

        try {
            System.out.print("ArithmeticException (idiv):     ");
            int i = 1, j = 0, k = i / j;
            p("FAILED");
        } catch (ArithmeticException e) {
  	    p("PASSED");
  	}

        try {
            System.out.print("ArithmeticException (ldiv):     ");
            long i = 1, j = 0, k = i / j;
            p("FAILED");
        } catch (ArithmeticException e) {
  	    p("PASSED");
  	}

        try {
            System.out.print("ArithmeticException (irem):     ");
            int i = 1, j = 0, k = i % j;
            p("FAILED");
        } catch (ArithmeticException e) {
  	    p("PASSED");
  	}

        try {
            System.out.print("ArithmeticException (lrem):     ");
            long i = 1, j = 0, k = i % j;
            p("FAILED");
        } catch (ArithmeticException e) {
  	    p("PASSED");
  	}

        try {
            System.out.print("NullPointerException:           ");
            Object o = new Object();
	    Object[] oa = null;
	    oa[0] = o;
            p("FAILED");
        } catch (ArithmeticException e) {
  	    p("PASSED");
  	}

	p();


	p("---------- no passed beyond this point ----------");

        p("NullPointerException (without catch): ");
        String s = null;
        int i = s.length();
        p("FAILED");
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

    public static void p() {
	System.out.println();
    }

    public static void p(String s) {
	System.out.println(s);
    }
}
