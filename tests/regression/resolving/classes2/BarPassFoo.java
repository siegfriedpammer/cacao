public class BarPassFoo {
    public static String id() {
        return "classes2/BarPassFoo";
    }

    public static String passit() {
        Foo foo = new Foo();
        BarUseFoo bar = new BarUseFoo();

        return bar.useFoo(foo);
    }

    public static String passDerivedFoo() {
        DerivedFoo dfoo = new DerivedFoo();
        BarUseFoo bar = new BarUseFoo();

        return bar.useFoo(dfoo);
    }

    public static String passDerivedFooInstance() {
        Foo foo = new DerivedFoo();

        return foo.virtualId();
    }

    public Foo createFoo() {
        return new Foo();
    }
}

// vim: et sw=4
