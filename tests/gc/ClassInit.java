class ClassInitTest {
	static {
		System.out.println("Static Initializer will call the GC ...");
		System.gc();
	}

	public void test() {
		System.out.println("Object fine.");
	}
}

public class ClassInit {
	public static void main(String[] s) {
		String t;
		ClassInitTest o;

		System.out.println("Preparing a String ...");
		t = new String("Remember Me!");

		System.out.println("Static Initializer will be called ...");
		o = new ClassInitTest();
		o.test();

		System.out.println("String: " + t);
		System.out.println("Test fine.");
	}
}
