public class classcontextnativeTest {

	public classcontextnativeTest() {}

	private static void z() {
		try {
		Object o=Class.forName("blup",true,null).newInstance();
		} catch (Throwable e) {
			System.out.println("TEST: "+e.toString());
		}
	}

	private static native void y();

	private static void x() {
		y();
	}
	private static void w() {
		x();
	}
	private static void v() {
		w();
	}
	private static void u() {
		v();
	}
	private static void t() {
		u();
	}
	private static void s() {
		t();
	}
	private static void r() {
		s();
	}
	private static void q() {
		r();
	}
	private static void p() {
		q();
	}
	private static void o() {
		p();
	}
	private static void n() {
		o();
	}
	private static void m() {
		n();
	}
	private static void l() {
		m();
	}
	private native static void k();

	private static void j() {
		k();
	}
	private static void i() {
		j();
	}
	private static void h() {
		i();
	}
	private static void g() {
		h();
	}
	private static void f() {
		g();
	}
	private static void e() {
		f();
	}
	private static void d() {
		e();
	}
	private static void c() {
		d();
	}
	private static void b() {
		c();
	}

	private native static void a();

	public static void main(String args[]) {
		System.setSecurityManager(new SecurityManager());
		a();
	}
}
