import java.util.*;

public class testarguments {
    static Random r;

    public static native void nisub(int a, int b, int c, int d, int e,
                                    int f, int g, int h, int i, int j);
    public static native void nlsub(long a, long b, long c, long d, long e,
                                    long f, long g, long h, long i, long j);
    public static native void nfsub(float a, float b, float c, float d, float e,
                                    float f, float g, float h, float i, float j);
    public static native void ndsub(double a, double b, double c, double d, double e,
                                    double f, double g, double h, double i, double j);

    public static void main(String[] argv) {
        r = new Random(0);

        System.loadLibrary("testarguments");

//          nisub(i(), i(), i(), i(), i(), i(), i(), i(), i(), i());
//          nlsub(l(), l(), l(), l(), l(), l(), l(), l(), l(), l());
        nfsub(f(), f(), f(), f(), f(), f(), f(), f(), f(), f());
//          ndsub(d(), d(), d(), d(), d(), d(), d(), d(), d(), d());

//          try {
//              ndsub(d(), d(), d(), d(), d(), d(), d(), d(), d(), d());
//          } catch (LinkageError e) {
//              System.out.println("catched");
//          }
    }

    public static void jisub(int a, int b, int c, int d, int e,
                             int f, int g, int h, int i, int j) {
        p("java  : int:");
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
        pln();
    }

    public static void jlsub(long a, long b, long c, long d, long e,
                             long f, long g, long h, long i, long j) {
        p("java  : long:");
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
        pln();
    }

    public static void jfsub(float a, float b, float c, float d, float e,
                             float f, float g, float h, float i, float j) {
        p("java  : float:");
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
        pln();
    }

    public static void jdsub(double a, double b, double c, double d, double e,
                             double f, double g, double h, double i, double j) {
        p("java  : double:");
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
        pln();
    }

    static int i() { return r.nextInt(); }
    static long l() { return r.nextLong(); }
    static float f() { return r.nextFloat(); }
    static double d() { return r.nextDouble(); }

    static void p(String s) { System.out.print(s); }

    static void p(int i) { System.out.print(" " + i); }
    static void p(long l) { System.out.print(" " + l); }
    static void p(float f) { System.out.print(" " + f); }
    static void p(double d) { System.out.print(" " + d); }

    static void pln() { System.out.println(); }
}
