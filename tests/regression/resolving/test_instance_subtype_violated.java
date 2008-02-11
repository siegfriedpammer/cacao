public class test_instance_subtype_violated {

    public static void main(String[] args) {
        TestController ct = new TestController();

        TestLoader ld1 = new TestLoader(ClassLoader.getSystemClassLoader(), "ld1", ct);
        TestLoader ld2 = new TestLoader(ClassLoader.getSystemClassLoader(), "ld2", ct);
        TestLoader ld3 = new TestLoader(ClassLoader.getSystemClassLoader(), "ld3", ct);

        ld1.addClassfile("BarUseFoo", "classes1/BarUseFoo.class");
        ld1.addClassfile("Foo", "classes1/Foo.class");
        ld1.addParentDelegation("java.lang.Object");
        ld1.addParentDelegation("java.lang.String");

        ld2.addClassfile("BarPassFoo", "classes2/BarPassFoo.class");
        ld2.addDelegation("BarUseFoo", ld1);
        ld2.addDelegation("Foo", ld1);
        ld2.addDelegation("DerivedFoo", ld3);
        ld2.addParentDelegation("java.lang.Object");
        ld2.addParentDelegation("java.lang.String");

        ld3.addClassfile("Foo", "classes3/Foo.class");
        ld3.addClassfile("DerivedFoo", "classes3/DerivedFoo.class");
        ld3.addParentDelegation("java.lang.Object");
        ld3.addParentDelegation("java.lang.String");


        // loading and linking BarPassFoo
        ct.expect("requested", ld2, "BarPassFoo");
        ct.expectLoadFromSystem(ld2, "java.lang.Object");
        ct.expect("defined", ld2, "<BarPassFoo>");
        ct.expect("loaded", ld2, "<BarPassFoo>");

        Class cls = ct.loadClass(ld2, "BarPassFoo");

        // executing BarPassFoo.passDerivedFooInstance: new DerivedFoo
        ct.expectDelegation(ld2, ld3, "DerivedFoo");

        // linking (ld3, DerivedFoo)
        ct.expect("requested", ld3, "Foo");
        ct.expectLoadFromSystem(ld3, "java.lang.Object");
        ct.expect("defined", ld3, "<Foo>");
        ct.expectDelegationDefinition(ld2, ld3, "DerivedFoo");

        // resolving Foo.virtualId
        // the deferred subtype check ((ld2, DerivedFoo) subtypeof (ld2, Foo)) is done
        ct.expectDelegation(ld2, ld1, "Foo");
        // ...linking (ld2, Foo) == (ld1, Foo)
        ct.expectLoadFromSystem(ld1, "java.lang.Object");
        ct.expectDelegationDefinition(ld2, ld1, "Foo");

        // the subtype constraint ((ld2, DerivedFoo) subtypeof (ld2, Foo)) is violated
        ct.expect("exception", "java.lang.LinkageError", "<BarPassFoo>");

        ct.checkStringGetterMustFail(cls, "passDerivedFooInstance");

        ct.exit();
    }

}

// vim: et sw=4
