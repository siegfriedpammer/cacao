public class test extends Thread {
    static boolean doit = true;

    public static void main(String[] argv) {
        int threadcount = 1;

        if (argv.length > 0) {
            for (int i = 0; i < argv.length; i++) {
                if (argv[i].equals("--help")) {
                    usage();

                } else if (argv[i].equals("skip")) {
                    doit = false;

                } else {
                    threadcount = Integer.valueOf(argv[i]).intValue();
                }
            }
        }

        System.out.println("Running with " + threadcount + " threads.");

        for (int i = 0; i < threadcount; i++) {
            new test().start();
        }
    }

    static void usage() {
        System.out.println("test [number of threads] [skip]");
        System.exit(1);
    }

    public test() {
    }

    public void start() {
        run();
    }

    public void run() {
        invokestatic();

        getstatic();
        putstatic();

        getfield();
        putfield();
        putfieldconst();

        newarray();
        multianewarray();

        invokespecial();

        checkcast();
        _instanceof();
    }


    final private static void invokestatic() {
        System.out.print("invokestatic: ");
        try {
            if (doit)
                invokestatic.sub();
            else
                System.out.println("OK");
        } catch (Throwable t) {
            System.out.println("FAILED: " + t);
        }
    }


    private void getstatic() {
        try {
            p("getstatic (I): ");
            if (doit)
                check(getstaticI.i, 123);
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("getstatic (J): ");
            if (doit)
                check(getstaticJ.l, 1234567890123L);
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("getstatic (F): ");
            if (doit)
                check(getstaticF.f, 123.456F);
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("getstatic (D): ");
            if (doit)
                check(getstaticD.d, 789.012);
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("getstatic (L): ");
            if (doit)
                check(getstaticL.o, null);
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }
    }

    private void putstatic() {
        try {
            p("putstatic (I): ");
            if (doit) {
                int i = 123;
                putstaticI.i = i;
                check(putstaticI.i, i);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("putstatic (J): ");
            if (doit) {
                long l = 1234567890123L;
                putstaticJ.l = l;
                check(putstaticJ.l, l);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("putstatic (F): ");
            if (doit) {
                float f = 123.456F;
                putstaticF.f = f;
                check(putstaticF.f, f);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("putstatic (D): ");
            if (doit) {
                double d = 789.012;
                putstaticD.d = d;
                check(putstaticD.d, d);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }


        try {
            p("putstatic (L): ");
            if (doit) {
                Object o = null;
                putstaticL.o = o;
                check(putstaticL.o, o);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }
    }

    private void getfield() {
        try {
            p("getfield (I): ");
            if (doit)
                check(new getfieldI().i, 123);
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("getfield (J): ");
            if (doit)
                check(new getfieldJ().l, 1234567890123L);
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("getfield (F): ");
            if (doit)
                check(new getfieldF().f, 123.456F);
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("getfield (D): ");
            if (doit)
                check(new getfieldD().d, 789.012);
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("getfield (L): ");
            if (doit)
                check(new getfieldL().o, null);
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }
    }

    private void putfield() {
        try {
            p("putfield (I): ");
            if (doit) {
                putfieldI pfi = new putfieldI();
                int i = 123;
                pfi.i = i;
                check(pfi.i, i);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("putfield (J): ");
            if (doit) {
                putfieldJ pfj = new putfieldJ();
                long l = 1234567890123L;
                pfj.l = l;
                check(pfj.l, l);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("putfield (F): ");
            if (doit) {
                putfieldF pff = new putfieldF();
                float f = 123.456F;
                pff.f = f;
                check(pff.f, f);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("putfield (D): ");
            if (doit) {
                putfieldD pfd = new putfieldD();
                double d = 789.012;
                pfd.d = d;
                check(pfd.d, d);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("putfield (L): ");
            if (doit) {
                putfieldL pfl = new putfieldL();
                Object o = null;
                pfl.o = o;
                check(pfl.o, o);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }
    }

    private void putfieldconst() {
        try {
            p("putfieldconst (I,F): ");
            if (doit) {
                putfieldconstIF pfcif = new putfieldconstIF();
                pfcif.i = 123;
                check(pfcif.i, 123);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }
 
        try {
            p("putfieldconst zero (I,F): ");
            if (doit) {
                putfieldconstIF pfcif = new putfieldconstIF();
                pfcif.i = 0;
                check(pfcif.i, 0);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }
 
        try {
            p("putfieldconst (J,D,L): ");
            if (doit) {
                putfieldconstJDL pfcjdl = new putfieldconstJDL();
                pfcjdl.l = 1234567890123L;
                check(pfcjdl.l, 1234567890123L);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("putfieldconst zero (J,D,L): ");
            if (doit) {
                putfieldconstJDL pfcjdl = new putfieldconstJDL();
                pfcjdl.l = 0L;
                check(pfcjdl.l, 0L);
            } else
                ok();
        } catch (Throwable t) {
            failed(t);
        }
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

    private void invokespecial() {
        try {
            p("invokespecial: ");
            if (doit)
                new invokespecial();
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }
    }

    private void checkcast() {
        Object o = new Object();

        // class
        try {
            p("checkcast class: ");
            if (doit) {
                checkcastC cc = (checkcastC) o;
                failed();
            } else
                ok();
        } catch (ClassCastException e) {
            ok();
        } catch (Throwable t) {
            failed(t);
        }

        // interface
        try {
            p("checkcast interface: ");
            if (doit) {
                checkcastI ci = (checkcastI) o;
                failed();
            } else
                ok();
        } catch (ClassCastException e) {
            ok();
        } catch (Throwable t) {
            failed(t);
        }


        // array

        Object[] oa = new Object[1];

        try {
            p("checkcast class array: ");
            if (doit) {
                checkcastC[] cca = (checkcastC[]) oa;
                failed();
            } else
                ok();
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
            if (doit)
                if (o instanceof instanceofC)
                    failed();
                else
                    ok();
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }

        try {
            p("instanceof interface: ");
            if (doit)
                if (o instanceof instanceofI)
                    failed();
                else
                    ok();
            else
                ok();
        } catch (Throwable t) {
            failed(t);
        }


        // array

        Object[] oa = new Object[1];

        try {
            p("instanceof class array: ");
            if (doit)
                if (oa instanceof instanceofC[])
                    failed();
                else
                    ok();
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

    private void check(int a, int b) {
        if (a == b)
            ok();
        else
            pln("FAILED: " + a + " != " + b);
    }

    private void check(long a, long b) {
        if (a == b)
            ok();
        else
            pln("FAILED: " + a + " != " + b);
    }

    private void check(float a, float b) {
        if (a == b)
            ok();
        else
            pln("FAILED: " + a + " != " + b);
    }

    private void check(double a, double b) {
        if (a == b)
            ok();
        else
            pln("FAILED: " + a + " != " + b);
    }

    private void check(Object a, Object b) {
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
