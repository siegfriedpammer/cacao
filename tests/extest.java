public class extest {
    public static void main(String[] argv) {
    	try {
            System.out.print("THROWS: ");
    	    sub();
            System.out.println("failed.");
    	} catch (Exception e) {
      	    System.out.println("passed.");
    	}

  	try {
            System.out.print("NATIVE: ");
            System.arraycopy(null, 1, null, 1, 1);
            System.out.println("failed.");
    	} catch (Exception e) {
  	    System.out.println("passed");
  	}

        try {
            System.out.print("NULL: ");
            int[] ia = null;
            int i = ia.length;
            System.out.println("failed.");
        } catch (NullPointerException e) {
  	    System.out.println("passed.");
  	}

        try {
            System.out.print("ARITHMETIC: ");
            int i = 1;
            int j = 0;
            int k = i / j;
            System.out.println("failed.");
        } catch (ArithmeticException e) {
  	    System.out.println("passed.");
  	}

        System.out.println("NULL (without catch): ");
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
