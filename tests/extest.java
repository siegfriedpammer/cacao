public class extest {
    public static void main(String[] argv) { 
	System.out.println("---------- normal exceptions --------------------");

   	try {
            System.out.print("throw Exception:                ");
    	    sub();
            System.out.println("FAILED");
    	} catch (Exception e) {
      	    System.out.println("PASSED");
    	}

  	try {
            System.out.print("native NullPointerException:    ");
            System.arraycopy(null, 1, null, 1, 1);
            System.out.println("FAILED");
    	} catch (Exception e) {
  	    System.out.println("PASSED");
  	}

        try {
            System.out.print("NullPointerException:           ");
            int[] ia = null;
            int i = ia.length;
            System.out.println("FAILED");
        } catch (NullPointerException e) {
  	    System.out.println("PASSED");
  	}

        try {
            System.out.print("ArithmeticException:            ");
            int i = 1, j = 0, k = i / j;
            System.out.println("FAILED");
        } catch (ArithmeticException e) {
  	    System.out.println("PASSED");
  	}

	System.out.println();
	System.out.println("---------- test soft inline exceptions ----------");

        try {
            System.out.print("ArrayIndexOutOfBoundsException: ");
            int[] ia = new int[1];
            ia[0xcafebabe] = 1;
            System.out.println("FAILED");
        } catch (ArrayIndexOutOfBoundsException e) {
	    String msg = e.getMessage();

	    if (msg != null && msg.compareTo("Array index out of range: -889275714") != 0) {
		System.out.println("FAILED: wrong index");

	    } else {
		System.out.println("PASSED");
	    }
  	}

        try {
            System.out.print("NegativeArraySizeException:     ");
            int[] ia = new int[-1];
            System.out.println("FAILED");
        } catch (NegativeArraySizeException e) {
  	    System.out.println("PASSED");
  	}
        
        try {
            System.out.print("ClassCastException:             ");
            Object o = new Object();
            Integer i = null;
            i = (Integer) o;
            System.out.println("FAILED");
        } catch (ClassCastException e) {
  	    System.out.println("PASSED");
  	}

        try {
            System.out.print("OutOfMemoryError:               ");
	    /* 100 MB should be enough */
	    byte[] ba = new byte[100 * 1024 * 1024];
            System.out.println("FAILED");
        } catch (OutOfMemoryError e) {
  	    System.out.println("PASSED");
  	}
        
	System.out.println();
	System.out.println("---------- no passed beyond this point ----------");

        System.out.println("NullPointerException (without catch): ");
        String s = null;
        int i = s.length();
        System.out.println("FAILED");
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
}
