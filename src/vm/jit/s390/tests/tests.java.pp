#define TEST(cond) test(#cond, (cond), __LINE__)
#define TODO() test("TODO: Implement", false, __LINE__)

class tests {

	static int s_i, s_i1, s_i2;
	static float s_f, s_f1, s_f2;
	static char s_c, s_c1;
	static short s_s, s_s1;
	static long s_l, s_l1;
	static double s_d, s_d1;
	static boolean s_b;
	static Object s_a;
	static final char[] s_ca = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	static char[] s_ca2 = { '0', '1', '2' };
	static String s_a0 = "s0", s_a1 = "s1", s_a2 = "s2";
	static Object[] s_aa = { s_a0, s_a1, s_a2 };
	static Object[] s_aa2 = { null, null, null };

	static int s_testsOk = 0, s_testsFailure = 0;

	static void test(String description, boolean success, int line) {
		String code = 
			success ? 
			"[OK]      " : 
			"[FAILURE] " ;
		System.out.println(code + description + " (at line " + line + ")");
		if (success) {
			s_testsOk ++;
		} else {
			s_testsFailure ++;
		}
	}

	static void summary() {
		System.out.println(
			"Summary>  " + 
			(s_testsOk + s_testsFailure) + " tests, OK: " +
			s_testsOk + ", FAILURE: " + s_testsFailure + "."
		);
	}

	static void test_ICONST() {

		// load some constant

		s_i = 23;

		TEST(s_i == 23);
		TEST(s_i != 22);

		// load corner values

		s_i = 0x7fffffff;
		TEST(s_i == 0x7fffffff);
		TEST(s_i != 0x7ffffffe);

		s_i = 0xffffffff;
		TEST(s_i == 0xffffffff);
		TEST(s_i != 0x7fffffff);
	}

	static void test_FCONST() {

		// load some float constant

		s_f = 12.3f;

		TEST(s_f == 12.3f);
		TEST(s_f != 12.4f);
	}

	static void test_INEG() {

		// negate (+) value

		s_i = 99;
		s_i = -s_i;
		TEST(s_i == -99);

		// negate corner (-) value

		s_i = -0x7fffffff;
		s_i = -s_i;
		TEST(s_i == 0x7fffffff);
	}

	static void test_INT2CHAR() {
		// Normal test
		s_i = 97;
		s_c = (char)s_i;
		TEST(s_c == 'a');
		// Negative values
		s_i = -1;
		s_c = (char)s_i;
		TEST(s_c == (char)-1);
		s_i = -3;
		s_c = (char)s_i;
		TEST(s_c == (char)-3);
	}

	static void test_IADD() {

		// add 2 (+) values
		
		s_i1 = 1983;
		s_i2 = 2000;
		s_i  = s_i1 + s_i2;
		TEST(s_i == 3983);

		// add 2 (-) values

		s_i1 = -1983;
		s_i2 = -2000;
		s_i  = s_i1 + s_i2;
		TEST(s_i == -3983);
	}

	static void test_IADDCONST() {
	
		// valid (+) immediate

		s_i = 1983;
		s_i += 2000;
		TEST(s_i == 3983);

		// increment

		s_i = 1983;
		++s_i;
		TEST(s_i == 1984);

		// valid (-) immediate

		s_i = -1983;
		s_i += -2000;
		TEST(s_i == -3983);

		// big (+) immediate (datasegment)

		s_i = 1983;
		s_i += 1000000;
		TEST(s_i == 1001983);

		// big (-) immediate (datasegment)

		s_i = 1983;
		s_i += -20001983;
		TEST(s_i == -20000000);

	}

	static void test_ISUB() {

		// substract 2 (+) values

		s_i1 = 33987;
		s_i2 = 9455325;
		s_i = s_i1 - s_i2;
		TEST(s_i == -9421338);

		// substract 2 (-) values

		s_i1 = -33987;
		s_i2 = -9455325;
		s_i = s_i1 - s_i2;
		TEST(s_i == 9421338);
	}

	static void test_ISUBCONST() {

		// substract valid immediate

		s_i = 32000;
		s_i -= 2000;
		TEST(s_i == 30000);

		// substract big (+) immediate (datasegment)

		s_i = 33987;
		s_i -= 9455325;
		TEST(s_i == -9421338);

		// decrement

		s_i = 33987;
		--s_i;
		TEST(s_i == 33986);

		// substract big (-) immediate (datasegment)

		s_i = -33987;
		s_i -= -9455325;
		TEST(s_i == 9421338);
	}

	static void test_IMULCONST() {

		// by 2 (will shift)

		s_i = 2000;
		s_i *= 2;
		TEST(s_i == 4000);

		// valid immediate

		s_i = 200;
		s_i *= 10000;
		TEST(s_i == 2000000);

		// big immediate (datasegment)

		s_i = 20;
		s_i *= 100000;
		TEST(s_i == 2000000);
	}

	static void test_IDIV() {
		s_i1 = 33;
		s_i2 = 3;
		s_i = s_i1 / s_i2;
		TEST(s_i == 11);

		s_i1 = 5570000;
		s_i2 = 10000;
		s_i = s_i1 / s_i2;
		TEST(s_i == 557);
	}

	static void test_IREM() {
		s_i1 = 5570664;
		s_i2 = 10000;
		s_i = s_i1 % s_i2;
		TEST(s_i == 664);
	}

	static void test_FMUL() {
		s_f1 = 1.1337f;
		s_f2 = 100.0f;
		s_f = s_f1 * s_f2;
		TEST(s_f == 113.37f);
	}

	static void test_I2F() {
		s_i = 1234567;
		s_f = (float)s_i;
		TEST(s_f == 1234567.0f);
		s_i = 0;
		s_f = (float)s_i;
		TEST(s_f == 0.0f);
	}

	static void test_I2D() {
		TODO();
	}

	static void test_F2I() {
		s_f = 1337.1337f;
		s_i = (int)s_f;
		TEST(s_i == 1337);
		s_f = 0.0f;
		s_i = (int)s_f;
		TEST(s_i == 0);
	}

	static void test_FCMP() {

		// tests FCMPL and FCMPG

		s_f1 = 1000.0f;
		s_f2 = 2000.0f;

		// With ecj, FCMPG is generated for < and reverse !

		s_b = (s_f1 < s_f2);
		TEST(s_b);

		s_b = (s_f2 < s_f1);
		TEST(! s_b);

		s_b = (s_f1 > s_f2);
		TEST(! s_b);

		s_b = (s_f2 > s_f1);
		TEST(s_b);

		s_b = (s_f2 == s_f1);
		TEST(! s_b);

		s_f2 = s_f1;
		s_b = (s_f2 == s_f1);
		TEST(s_b);

		// Corner cases
		// This might not work with compilers other than ecj

		s_f2 = Float.NaN;
		s_b = (s_f1 < s_f2); // this generates FCMPG with NaN -> GT
		TEST(! s_b);

		s_b = (s_f1 > s_f2); // this generates FCMPL whith NAN -> LT
		TEST(! s_b);

		s_b = (s_f1 == s_f2); // this generates FCMPXX with NAN -> XX
		TEST(! s_b);

		// Infinity

		s_f1 = Float.NEGATIVE_INFINITY;
		s_f2 = Float.POSITIVE_INFINITY;

		s_b = (s_f1 < s_f2);
		TEST(s_b);
		s_b = (s_f1 > s_f2);
		TEST(! s_b);
		s_b = (s_f1 == s_f2);
		TEST(! s_b);
	
		s_f1 = Float.NEGATIVE_INFINITY;
		s_f2 = -9887.33f;

		s_b = (s_f1 < s_f2);
		TEST(s_b);
		s_b = (s_f1 > s_f2);
		TEST(! s_b);
		s_b = (s_f1 == s_f2);
		TEST(! s_b);

		s_f1 = 9999877.44f;
		s_f2 = Float.POSITIVE_INFINITY;

		s_b = (s_f1 < s_f2);
		TEST(s_b);
		s_b = (s_f1 > s_f2);
		TEST(! s_b);
		s_b = (s_f1 == s_f2);
		TEST(! s_b);
	}

	static void test_ARRAYLENGTH() {
		TEST(s_ca.length == 10);
		TEST(s_ca.length != 11);
	}

	static void test_CALOAD() {
		s_c = s_ca[4];
		TEST(s_c == '4');
	}

	static void test_AALOAD() {
		s_a = s_aa[1];
		TEST(s_a != s_a0);
		TEST(s_a == s_a1);
	}

	static void test_CASTORE() {
		s_ca2[1] = 'X';
		s_c = s_ca2[1];
		TEST(s_c == 'X');
	}

	static void test_AASTORE() {
		s_aa2[1] = s_a1;
		s_a = s_aa2[1];
		TEST(s_a == s_a1);
	}

	static void test_GETPUTSTATIC() {
		s_c1 = 'X';
		s_c = s_c1;
		TEST(s_c == 'X');

		s_s1 = -34;
		s_s = s_s1;
		TEST(s_s == -34);

		s_i1 = 987;
		s_i = s_i1;
		TEST(s_i == 987);

		s_a = s_a1;
		TEST(s_a == s_a1);

		s_l1 = 987l;
		s_l = s_l1;
		TEST(s_l == 987l);

		s_f1 = 98.12f;
		s_f = s_f1;
		TEST(s_f == 98.12f);

		s_d1 = 98.12;
		s_d = s_d1;
		TEST(s_d == 98.12);
	}

	static void test_IF_LXX() {
		// the tests generated are the negated tests
		// (witj ecj)

		// short long (one long long)

		s_l = 0xffffl;

		s_i = (s_l < 0xfffffl ? 10 : 20);
		TEST(s_i == 10);

		s_i = (s_l <= 0xfffffl ? 10 : 20);
		TEST(s_i == 10);

		s_i = (s_l > 0xfffl ? 10 : 20);
		TEST(s_i == 10);

		s_i = (s_l >= 0xfffl ? 10 : 20);
		TEST(s_i == 10);

		s_i = (s_l == 0xffffl ? 10 : 20);
		TEST(s_i == 10);

		s_i = (s_l != 0xffffl ? 10 : 20);
		TEST(s_i == 20);

		s_i = (s_l != 0xfffl ? 10 : 20);
		TEST(s_i == 10);

		// long long (two words long)

		s_l = 0xffFFFFFFFFl;

		s_i = (s_l < 0xfffFFFFFFFFl ? 10 : 20);
		TEST(s_i == 10); //

		s_i = (s_l <= 0xfffFFFFFFFFl ? 10 : 20);
		TEST(s_i == 10);

		s_i = (s_l > 0xfffFFFFFFFFl ? 10 : 20);
		TEST(s_i == 10); //

		s_i = (s_l >= 0xfffFFFFFFFFl ? 10 : 20);
		TEST(s_i == 10); //

		s_i = (s_l == 0xffFFFFFFFFl ? 10 : 20);
		TEST(s_i == 10);

		s_i = (s_l != 0xffFFFFFFFFl ? 10 : 20);
		TEST(s_i == 20);

		s_i = (s_l != 0xfffFFFFFFFFl ? 10 : 20);
		TEST(s_i == 10); //
	}
	
	static void main(String[] args) {
		test_ICONST();
		test_FCONST();
		test_INEG();
		test_INT2CHAR();
		test_IADD();
		test_IADDCONST();
		test_ISUB();
		test_ISUBCONST();
		test_IMULCONST();
		test_IDIV();
		test_IREM();
		test_FMUL();
		test_I2F();
		test_I2D();
		test_F2I();
		test_FCMP();
		test_ARRAYLENGTH();
		test_CALOAD();
		test_AALOAD();
		test_CASTORE();
		test_AASTORE();
		test_GETPUTSTATIC();
		test_IF_LXX();

		summary();
	}


};

// vim: syntax=java
