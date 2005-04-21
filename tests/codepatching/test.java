public class test {
    boolean doit = true;

    public static void main(String[] argv) {
//          invokestatic();

        new test(argv);
    }

    public test(String[] argv) {
        if (argv.length > 0) {
            for (int i = 0; i < argv.length; i++) {
                if (argv[i].equals("skip")) {
                    doit = false;
                }
            }
        }
//          getstatic();
//          putstatic();

//          getfield();
//         putfield();
//         putfieldconst();

//          newarray();
//          multianewarray();

//         invokespecial();

        checkcast();
        _instanceof();
    }


    final private static void invokestatic() {
        System.out.println("invokestatic:");
        invokestatic.sub();
    }


    public void getstatic() {
        try {
            p("getstatic (I): ");
            check(getstaticI.i, 123);
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("getstatic (J): ");
            check(getstaticJ.l, 456L);
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("getstatic (F): ");
            check(getstaticF.f, 123.456F);
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("getstatic (D): ");
            check(getstaticD.d, 789.012);
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("getstatic (L): ");
            check(getstaticL.o, null);
        } catch (Throwable t) {
            failed(t);
        }
    }


    public void putstatic() {
        try {
            p("putstatic (I): ");
            int i = 123;
            putstaticI.i = i;
            check(putstaticI.i, i);
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("putstatic (J): ");
            long l = 456L;
            putstaticJ.l = l;
            check(putstaticJ.l, l);
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("putstatic (F): ");
            float f = 123.456F;
            putstaticF.f = f;
            check(putstaticF.f, f);
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("putstatic (D): ");
            double d = 789.012;
            putstaticD.d = d;
            check(putstaticD.d, d);
        } catch (Throwable t) {
            failed(t);
        }


        try {
            p("putstatic (L): ");
            Object o = null;
            putstaticL.o = o;
            check(putstaticL.o, o);
        } catch (Throwable t) {
            failed(t);
        }
    }

    public void getfield() {
//         try {
//             p("getfield (I): ");
//             check(new getfieldI().i, 123);
//         } catch (Throwable t) {
//             failed(t);
//         }

        try {
            p("getfield (J): ");
            check(new getfieldJ().l, 456L);
        } catch (Throwable t) {
            failed(t);
        }

//         try {
//             p("getfield (F): ");
//             check(new getfieldF().f, 123.456F);
//         } catch (Throwable t) {
//             failed(t);
//         }

//         try {
//             p("getfield (D): ");
//             check(new getfieldD().d, 789.012);
//         } catch (Throwable t) {
//             failed(t);
//         }

//         try {
//             p("getfield (L): ");
//             check(new getfieldL().o, null);
//         } catch (Throwable t) {
//             failed(t);
//         }
    }

    public void putfield() {
        p("putfield (I): ");
        putfieldI pfi = new putfieldI();
        int i = 123;
        pfi.i = i;
        check(pfi.i, i);

        p("putfield (J): ");
        putfieldJ pfj = new putfieldJ();
        long l = 456L;
        pfj.l = l;
        check(pfj.l, l);

        p("putfield (F): ");
        putfieldF pff = new putfieldF();
        float f = 123.456F;
        pff.f = f;
        check(pff.f, f);

        p("putfield (D): ");
        putfieldD pfd = new putfieldD();
        double d = 789.012;
        pfd.d = d;
        check(pfd.d, d);

        p("putfield (L): ");
        putfieldL pfl = new putfieldL();
        Object o = null;
        pfl.o = o;
        check(pfl.o, o);
    }

    public void putfieldconst() {
        p("putfieldconst (I,F): ");
        putfieldconstIF pfcif = new putfieldconstIF();
        pfcif.i = 123;
        check(pfcif.i, 123);
 
        p("putfieldconst (J,D,L): ");
        putfieldconstJDL pfcjdl = new putfieldconstJDL();
        pfcjdl.l = 456;
        check(pfcjdl.l, 456);
    }

    private void newarray() {
        try {
            p("newarray: ");
            if (doit) {
                newarray[] na = new newarray[1];
            }
            ok();
        } catch (Throwable t) {
            failed(t);
        }
    }

    private void multianewarray() {
        try {
            p("multianewarray: ");
            if (doit) {
                multianewarray[][] ma = new multianewarray[1][1];
            }
            ok();
        } catch (Throwable t) {
            failed(t);
        }
    }

    public void invokespecial() {
        try {
            p("invokespecial: ");
            new invokespecial();
        } catch (Throwable t) {
            failed(t);
        }
    }

    private void checkcast() {
        Object o = new Object();

        // class
        try {
            p("checkcast class: ");
            checkcastC cc = (checkcastC) o;
            failed();
        } catch (ClassCastException e) {
            ok();
        } catch (Throwable t) {
            failed(t);
        }

        // interface
        try {
            p("checkcast interface: ");
            checkcastI ci = (checkcastI) o;
            failed();
        } catch (ClassCastException e) {
            ok();
        } catch (Throwable t) {
            failed(t);
        }


        // array

        Object[] oa = new Object[1];

        try {
            p("checkcast class array: ");
            checkcastC[] cca = (checkcastC[]) oa;
            failed();
        } catch (ClassCastException e) {
            ok();
        } catch (Throwable t) {
            failed(t);
        }
    }

    private void _instanceof() {
        Object o = new Object();

        try {
            p("instanceof class: ");
            if (o instanceof instanceofC)
                failed();
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("instanceof interface: ");
            if (o instanceof instanceofI)
                failed();
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }


        // array

        Object[] oa = new Object[1];

        try {
            p("instanceof class array: ");
            if (oa instanceof instanceofC[])
                failed();
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }
    }

    private void ok() {
        pln("OK");
    }

    private void failed() {
        pln("FAILED");
    }

    private void failed(Throwable t) {
        pln("FAILED: " + t);
    }

    public void check(int a, int b) {
        if (a == b)
            ok();
        else
            pln("FAILED: " + a + " != " + b);
    }

    public void check(long a, long b) {
        if (a == b)
            ok();
        else
            pln("FAILED: " + a + " != " + b);
    }

    public void check(float a, float b) {
        if (a == b)
            ok();
        else
            pln("FAILED: " + a + " != " + b);
    }

    public void check(double a, double b) {
        if (a == b)
            ok();
        else
            pln("FAILED: " + a + " != " + b);
    }

    public void check(Object a, Object b) {
        if (a == b)
            ok();
        else
            pln("FAILED: " + a + " != " + b);
    }

    private final void p(String s) {
        System.out.print(s);
    }

    private final void pln(String s) {
        System.out.println(s);
    }
}
