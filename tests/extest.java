public class extest {
    public static void main(String[] argv) {
    	try {
            System.out.print("throw Exception: ");
    	    sub();
            System.out.println("failed.");
    	} catch (Exception e) {
      	    System.out.println("passed.");
    	}

  	try {
            System.out.print("native Exception: ");
            System.arraycopy(null, 1, null, 1, 1);
            System.out.println("failed.");
    	} catch (Exception e) {
  	    System.out.println("passed.");
  	}

        try {
            System.out.print("NullPointerException: ");
            int[] ia = null;
            int i = ia.length;
            System.out.println("failed.");
        } catch (NullPointerException e) {
  	    System.out.println("passed.");
  	}

        try {
            System.out.print("ArithmeticException: ");
            int i = 1, j = 0, k = i / j;
            System.out.println("failed.");
        } catch (ArithmeticException e) {
  	    System.out.println("passed.");
  	}

        try {
            System.out.print("ArrayIndexOutOfBoundsException: ");
            int[] ia = new int[1];
            ia[1] = 1;
            System.out.println("failed.");
        } catch (ArrayIndexOutOfBoundsException e) {
  	    System.out.println("passed.");
  	}

        try {
            System.out.print("NegativeArraySizeException: ");
            int[] ia = new int[-1];
            System.out.println("failed.");
        } catch (NegativeArraySizeException e) {
  	    System.out.println("passed.");
  	}
        
        try {
            System.out.print("ClassCastException: ");
            Object o = new Object();
            Integer i = null;
            i = (Integer) o;
            System.out.println("failed.");
        } catch (ClassCastException e) {
  	    System.out.println("passed.");
  	}
        
        System.out.println("NullPointerException (without catch): ");
        String s = null;
        int i = s.length();
        System.out.println("failed.");
    }

    public synchronized static void sub() throws Exception {
        int a, b, c, d;
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
