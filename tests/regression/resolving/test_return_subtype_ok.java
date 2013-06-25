public class test_return_subtype_ok extends TestController {

    public static void main(String[] args) {
        new test_return_subtype_ok();
    }

    test_return_subtype_ok() {
        // ***** setup

        TestLoader ld1 = new TestLoader("ld1", this);
        TestLoader ld2 = new TestLoader("ld2", this);

        ld1.addClassfile("Foo",        "classes1/Foo.class");
        ld1.addClassfile("DerivedFoo", "classes2/DerivedFoo.class");
        ld1.addParentDelegation("java.lang.Object");
        ld1.addParentDelegation("java.lang.String");

        ld2.addClassfile("BarPassFoo", "classes2/BarPassFoo.class");
        ld2.addDelegation("Foo",        ld1);
        ld2.addDelegation("DerivedFoo", ld1);
        ld2.addParentDelegation("java.lang.Object");
        ld2.addParentDelegation("java.lang.String");

        // ***** test

		// loading & linking BarPassFoo
		expectRequest(ld2, "BarPassFoo")
			.expectRequest("java.lang.Object")
			.expectDelegateToSystem()
		.expectDefinition();

        Class<?> cls = loadClass(ld2, "BarPassFoo");

		switch (ClassLibrary.getCurrent()) {
		case GNU_CLASSPATH:
			// executing createDerivedFoo
			expectRequest(ld2, "DerivedFoo")
				.expectDelegation(ld1)
					// ...linking (ld2, DerivedFoo)
					.expectRequest("Foo")
						.expectRequest("java.lang.Object")
						.expectDelegateToSystem()
					.expectDefinition()
				.expectDefinition()
			.expectLoaded();

			checkStringGetter(cls, "getDerivedFoo", "no exception");

			// subtype check (DerivedFoo subtypeof Foo)
			expectRequest(ld2, "Foo")
				.expectDelegation(ld1)
				.expectFound()
			.expectLoaded();

			checkStringGetter(cls, "getDerivedFooAsFoo", "no exception");
			break;
		case OPEN_JDK:
			// constructor of java.lang.Method checks descriptor of getDerivedFoo
			// this forces loading of Foo and String
			expectRequest(ld2, "java.lang.String")
			.expectDelegateToSystem();

			expectRequest(ld2, "Foo")
				.expectDelegation(ld1)
					.expectRequest("java.lang.Object")
					.expectDelegateToSystem()
				.expectDefinition()
			.expectLoaded();

			// executing createDerivedFoo
			expectRequest(ld2, "DerivedFoo")
				.expectDelegation(ld1)
				.expectDefinition()
			.expectLoaded();

			checkStringGetter(cls, "getDerivedFoo", "no exception");

			checkStringGetter(cls, "getDerivedFooAsFoo", "no exception");
			break;
		}

        exit();
    }

}

// vim: et sw=4
