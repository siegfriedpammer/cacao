public class test_param_loading_constraint_violated extends TestController {

    public static void main(String[] args) {
        new test_param_loading_constraint_violated();
    }

    test_param_loading_constraint_violated() {
        // ***** setup

        TestLoader ld1 = new TestLoader("ld1", this);
        TestLoader ld2 = new TestLoader("ld2", this);

        ld1.addClassfile("BarUseFoo", "classes1/BarUseFoo.class");
        ld1.addClassfile("Foo",       "classes1/Foo.class");
        ld1.addParentDelegation("java.lang.Object");
        ld1.addParentDelegation("java.lang.String");

        ld2.addClassfile("BarPassFoo", "classes2/BarPassFoo.class");
        ld2.addClassfile("Foo",        "classes2/Foo.class");
        ld2.addDelegation("BarUseFoo", ld1);
        ld2.addParentDelegation("java.lang.Object");
        ld2.addParentDelegation("java.lang.String");

        // OpenJDKs reflection API causes us to load DerivedFoo
        if (ClassLibrary.getCurrent() == ClassLibrary.OPEN_JDK)
            ld2.addClassfile("DerivedFoo", "classes2/DerivedFoo.class");

        // loading BarPassFoo
        expectRequest(ld2, "BarPassFoo")
            // linking BarPassFoo
            .expectRequest("java.lang.Object")
            .expectDelegateToSystem()
        .expectDefinition();

        Class<?> cls = loadClass(ld2, "BarPassFoo");

        switch (ClassLibrary.getCurrent()) {
        case GNU_CLASSPATH:
            // executing BarPassFoo.passit: new Foo
            expectRequest(ld2, "Foo")
            .expectDefinition();

            // executing BarPassFoo.passit: new BarUseFoo
            expectRequest(ld2, "BarUseFoo")
                .expectDelegation(ld1)
                    // ...linking BarUseFoo
                    .expectRequest("java.lang.Object")
                    .expectDelegateToSystem()
                .expectDefinition()
            .expectLoaded();

            // resolving Foo.virtualId() from BarUseFoo
            expectRequest(ld1, "Foo");
            break;
        case OPEN_JDK:
            // constructor of java.lang.Method checks descriptor of passit
            // this forces loading of Foo and String
            expectRequest(ld2, "java.lang.String")
            .expectDelegateToSystem();

            // executing BarPassFoo.passit: new Foo
            expectRequest(ld2, "Foo")
            .expectDefinition();

            expectRequest(ld2, "DerivedFoo")
            .expectDefinition();

            // executing BarPassFoo.passit: new BarUseFoo
            expectRequest(ld2, "BarUseFoo")
                .expectDelegation(ld1)
                    // ...linking BarUseFoo
                    .expectRequest("java.lang.Object")
                    .expectDelegateToSystem()
                .expectDefinition()
            .expectLoaded();

            // resolving Foo.virtualId() from BarUseFoo
            expectRequest(ld1, "Foo");
            break;
        }

        // the loading constraing (ld1,ld2,Foo) is violated
        expectException(new LinkageError("Foo: loading constraint violated: "));

        checkStringGetterMustFail(cls, "passit");

        exit();
    }

}

// vim: et sw=4
