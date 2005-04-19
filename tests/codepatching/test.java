public class test {
    public static void main(String[] argv) {
        new test();
    }

    public test() {
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

    public void getstatic() {
        p("getstatic:");
        check(getstaticI.i, 123);
        check(getstaticJ.l, 456L);
        check(getstaticF.f, 123.456F);
        check(getstaticD.d, 789.012);
        check(getstaticL.o, null);
    }


    public void putstatic() {
        p("putstatic:");

        int i = 123;
        long l = 456L;
        float f = 123.456F;
        double d = 789.012;
        Object o = null;

        putstaticI.i = i;
        check(putstaticI.i, i);

        putstaticJ.l = l;
        check(putstaticJ.l, l);

        putstaticF.f = f;
        check(putstaticF.f, f);

        putstaticD.d = d;
        check(putstaticD.d, d);

        putstaticL.o = o;
        check(putstaticL.o, o);
    }

    public void getfield() {
        p("getfield:");
        check(new getfieldI().i, 123);
        check(new getfieldJ().l, 456L);
        check(new getfieldF().f, 123.456F);
        check(new getfieldD().d, 789.012);
        check(new getfieldL().o, null);
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
            newarray[] na = new newarray[1];
            ok();
        } catch (Throwable t) {
            failed(t);
        }
    }

    private void multianewarray() {
        try {
            p("multianewarray: ");
            multianewarray[][] ma = new multianewarray[1][1];
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

        try {
            p("checkcast class: ");
            checkcastC cc = (checkcastC) o;
            failed();
        } catch (ClassCastException e) {
            ok();
        }

        try {
            p("checkcast interface: ");
            checkcastI ci = (checkcastI) o;
            failed();
        } catch (ClassCastException e) {
            ok();
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
