public class test_retval_loading_constraint_violated extends TestController {

	public static void main(String[] args) {
		new test_retval_loading_constraint_violated();
	}

	test_retval_loading_constraint_violated() {
		// ***** setup

		TestLoader ld1 = new TestLoader("ld1", this);
		TestLoader ld2 = new TestLoader("ld2", this);

		ld1.addClassfile("BarUseFoo", "classes1/BarUseFoo.class");
		ld1.addClassfile("Foo",       "classes1/Foo.class");
		ld1.addDelegation("BarPassFoo", ld2);
		ld1.addParentDelegation("java.lang.Object");
		ld1.addParentDelegation("java.lang.String");

		ld2.addClassfile("BarPassFoo", "classes2/BarPassFoo.class");
		ld2.addClassfile("Foo",        "classes2/Foo.class");
		ld2.addParentDelegation("java.lang.Object");
		ld2.addParentDelegation("java.lang.String");

		// ***** test

		// loading & linking BarUseFoo
		expectRequest(ld1, "BarUseFoo")
			.expectRequest("java.lang.Object")
			.expectDelegateToSystem()
		.expectDefinition();

		Class<?> cls = loadClass(ld1, "BarUseFoo");

		switch (ClassLibrary.getCurrent()) {
		case GNU_CLASSPATH:
			// executing BarUseFoo.useReturnedFoo: new BarPassFoo
			expectRequest(ld1, "BarPassFoo")
				.expectDelegation(ld2)
					// ...linking BarPassFoo
					.expectRequest("java.lang.Object")
					.expectDelegateToSystem()
				.expectDefinition()
			.expectLoaded();

			// resolving BarPassFoo.createFoo
			expectRequest(ld2, "Foo")
			.expectDefinition();

			expectRequest(ld1,"Foo");
			break;
		case OPEN_JDK:
			// constructor of java.lang.Method checks descriptor of useReturnedFoo
			// this forces loading of Foo and String
			expectRequest(ld1, "java.lang.String")
			.expectDelegateToSystem();

			// resolving BarPassFoo.createFoo
			expectRequest(ld1, "Foo")
			.expectDefinition();

			expectRequest(ld1, "BarPassFoo")
				.expectDelegation(ld2)
					.expectRequest("java.lang.Object")
					.expectDelegateToSystem()
				.expectDefinition()
			.expectLoaded();

			expectRequest(ld2, "Foo");
			break;
		}

		// ...the loading constraing (ld1,ld2,Foo) is violated
		expectException(new LinkageError("Foo: loading constraint violated: "));

		checkStringGetterMustFail(cls, "useReturnedFoo");

		exit();
    }

}

// vim: et sw=4
