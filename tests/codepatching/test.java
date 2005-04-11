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

        invokespecial();
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
        p("putfield:");

        int i = 123;
        long l = 456L;
        float f = 123.456F;
        double d = 789.012;
        Object o = null;

        putfieldI pfi = new putfieldI();
        pfi.i = i;
        check(pfi.i, i);

        putfieldJ pfj = new putfieldJ();
        pfj.l = l;
        check(pfj.l, l);

        putfieldF pff = new putfieldF();
        pff.f = f;
        check(pff.f, f);

        putfieldD pfd = new putfieldD();
        pfd.d = d;
        check(pfd.d, d);

        putfieldL pfl = new putfieldL();
        pfl.o = o;
        check(pfl.o, o);
    }

    public void putfieldconst() {
        p("putfieldconst:");

        putfieldconstIF pfcif = new putfieldconstIF();
        pfcif.i = 123;
        check(pfcif.i, 123);

        putfieldconstJDL pfcjdl = new putfieldconstJDL();
        pfcjdl.l = 456;
        check(pfcjdl.l, 456);
    }

    public void invokespecial() {
        p("invokespecial:");
        new invokespecial();
    }

    public void check(int a, int b) {
        if (a == b)
            p("OK");
        else
            p("FAILED: " + a + " != " + b);
    }

    public void check(long a, long b) {
        if (a == b)
            p("OK");
        else
            p("FAILED: " + a + " != " + b);
    }

    public void check(float a, float b) {
        if (a == b)
            p("OK");
        else
            p("FAILED: " + a + " != " + b);
    }

    public void check(double a, double b) {
        if (a == b)
            p("OK");
        else
            p("FAILED: " + a + " != " + b);
    }

    public void check(Object a, Object b) {
        if (a == b)
            p("OK");
        else
            p("FAILED: " + a + " != " + b);
    }

    private final void p(String s) {
        System.out.println(s);
    }
}
