public class test_simple_lazy_load extends TestController {

    public static void main(String[] args) {
        new test_simple_lazy_load();
    }

    test_simple_lazy_load() {
        // ***** setup

        TestLoader ld1 = new TestLoader("ld1", this);

        ld1.addClassfile("BarUseFoo", "classes1/BarUseFoo.class");
        ld1.addParentDelegation("java.lang.Object");

        // OpenJDKs reflection API forces eager loading of classes
        if (ClassLibrary.getCurrent() == ClassLibrary.OPEN_JDK) {
            ld1.addClassfile("Foo", "classes1/Foo.class");
            ld1.addParentDelegation("java.lang.String");
        }

        // ***** test

        expectRequest(ld1, "BarUseFoo")
            .expectRequest("java.lang.Object")
            .expectDelegateToSystem()
        .expectDefinition();

        if (ClassLibrary.getCurrent() == ClassLibrary.OPEN_JDK) {
            // constructor of java.lang.Method checks descriptor of idOfFoo
            // this forces loading of Foo and String
            expectRequest(ld1, "java.lang.String")
            .expectDelegateToSystem();

            expectRequest(ld1, "Foo")
            .expectDefinition();
        }

        Class<?> cls = loadClass(ld1, "BarUseFoo");
        checkClassId(cls, "classes1/BarUseFoo");
        expectEnd();

        ld1.addClassfile("Foo", "classes1/Foo.class");
        setReportClassIDs(true);

        if (ClassLibrary.getCurrent() == ClassLibrary.GNU_CLASSPATH) {
            // while OpenJDKs reflection API already forced eager loading of Foo,
            // classpath allows us to do this lazily
            expectRequest(ld1, "Foo")
            .expectDefinitionWithClassId("classes1/Foo");
        }

        checkStringGetter(cls, "idOfFoo", "classes1/Foo");

        exit();
    }

}

// vim: et sw=4
