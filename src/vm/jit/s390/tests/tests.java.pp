package tests;

#define TEST(cond) test(#cond, (cond), __LINE__)
#define TODO() test("TODO: Implement", false, __LINE__)

class tests {

	static int s_i, s_i1, s_i2;
	static float s_f, s_f1, s_f2;
	static char s_c, s_c1;
	static short s_s, s_s1;
	static long s_l, s_l1, s_l2;
	static double s_d, s_d1, s_d2;
	static boolean s_b;
	static Object s_a;
	static final char[] s_ca = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	static char[] s_ca2 = { '0', '1', '2' };
	static String s_as0 = "s0", s_as1 = "s1", s_as2 = "s2";
	static Object s_a0 = s_as0, s_a1 = s_as1, s_a2 = s_as2;
	static Object[] s_aa = { s_a0, s_a1, s_a2 };
	static Object[] s_aa2 = { null, null, null };
	static byte[] s_bya = { 10, 11, 12, 13, 14, 15 };
	static byte[] s_bya2 = { 0, 1, 2, 3};
	static byte s_by, s_by1;
	static short[] s_sa2 = { 0, 1, 2, 3};
	static short[] s_sa = { 10, 11, 12, 13, 14, 15 };
	static int[] s_ia2 = { 0, 1, 2, 3 };
	static int[] s_ia = { 10, 11, 12, 13, 14, 15 };

	static class members {
		char c;
		short s;
		int i;
		Object a;
		long l;
		float f;
		double d;
	};

	static class exception extends Exception {
		int line;
		exception(int line) {
			this.line = line;
		}
	};

	static char s_cm() { return s_c; }
	static short s_sm() { return s_s; }
	static int s_im() { return s_i; }
	static long s_lm() { return s_l; }
	static Object s_am() { return s_a; }
	static float s_fm() { return s_f; }
	static double s_dm() { return s_d; }

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

	static void test_DMUL() {
		s_d1 = 1.1337;
		s_d2 = 100.0;
		s_d = s_d1 * s_d2;
		TEST(s_d == 113.37);
		System.out.println(s_d);
	}

	static void test_FDIV() {
		s_f1 = 113.37f;
		s_f2 = 100.0f;
		s_f = s_f1 / s_f2;
		TEST(s_f == 1.1337f);
	}

	static void test_DDIV() {
		s_d1 = 113.37;
		s_d2 = 100.0;
		s_d = s_d1 / s_d2;
		TEST(s_d == 1.1337);
	}

	static void test_FSUB() {
		s_f1 = 1.1337f;
		s_f2 = 0.033f;
		s_f = s_f1 - s_f2;
		TEST(s_f == 1.1007f);
	}

	static void test_DSUB() {
		s_d1 = 1.1337;
		s_d2 = 0.033;
		s_d = s_d1 - s_d2;
		TEST(s_d == 1.1007);
	}

	static void test_FADD() {
		s_f1 = 1.1006f;
		s_f2 = 0.0331f;
		s_f = s_f1 + s_f2;
		TEST(s_f == 1.1337f);
	}

	static void test_DADD() {
		s_d1 = 1007.1;
		s_d2 = 0330.0;
		s_d = s_d1 + s_d2;
		TEST(s_d == 1337.1);
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

	static void test_BALOAD() {
		s_by = s_bya[3];
		TEST(s_by == 13);
	}

	static void test_SALOAD() {
		s_s = s_sa[4];
		TEST(s_s == 14);
	}

	static void test_IALOAD() {
		s_i = s_ia[4];
		TEST(s_i == 14);
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

	static void test_BASTORE() {
		s_by1 = 77;
		s_bya2[1] = s_by1;
		s_by = s_bya2[1];
		TEST(s_by == 77);
	}

	static void test_SASTORE() {
		s_s1 = (short)0xbcde;
		s_sa2[2] = s_s1;
		s_s = s_sa2[2];
		TEST(s_s == (short)0xbcde);
	}

	static void test_IASTORE() {
		s_i1 = 0xbcde1234;
		s_ia2[2] = s_i1;
		s_i = s_ia2[2];
		TEST(s_i == 0xbcde1234);
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

		s_l1 = 0x987AABBCCDDl;
		s_l = s_l1;
		TEST(s_l == 0x987AABBCCDDl);

		s_f1 = 98.12f;
		s_f = s_f1;
		TEST(s_f == 98.12f);

		s_d1 = 98.12;
		s_d = s_d1;
		TEST(s_d == 98.12);
	}

	static void test_IF_LXX_LCMPXX() {
		// the tests generated are the negated tests
		// (witj ecj)

#		define YES 10
#		define NO 20
#		define LTEST(val1, op, val2, expect) \
			s_l = val1; \
			s_i = (s_l op val2 ? YES : NO); \
			TEST(s_i == expect); \
			s_l1 = val1; \
			s_l2 = val2; \
			s_i = (s_l1 op s_l2 ? YES : NO); \
			TEST(s_i == expect); 

		// HIGH words equal

		LTEST(0xffABCDl, <,  0xffABCDl, NO);
		LTEST(0xffABCDl, <=, 0xffABCDl, YES);
		LTEST(0xffABCDl, >,  0xffABCDl, NO);
		LTEST(0xffABCDl, >=, 0xffABCDl, YES);
		LTEST(0xffABCDl, ==, 0xffABCDl, YES);
		LTEST(0xffABCDl, !=, 0xffABCDl, NO);

		LTEST(0xffABCDl, <,  0xfffABCDl, YES);
		LTEST(0xffABCDl, <=, 0xfffABCDl, YES);
		LTEST(0xffABCDl, >,  0xfffABCDl, NO);
		LTEST(0xffABCDl, >=, 0xfffABCDl, NO);
		LTEST(0xffABCDl, ==, 0xfffABCDl, NO);
		LTEST(0xffABCDl, !=, 0xfffABCDl, YES);

		LTEST(0xffABCDl, <,  0xfABCDl, NO);
		LTEST(0xffABCDl, <=, 0xfABCDl, NO);
		LTEST(0xffABCDl, >,  0xfABCDl, YES);
		LTEST(0xffABCDl, >=, 0xfABCDl, YES);
		LTEST(0xffABCDl, ==, 0xfABCDl, NO);
		LTEST(0xffABCDl, !=, 0xfABCDl, YES);

		// LOW words equal

		LTEST(0xffAABBCCDDl, <,  0xffAABBCCDDl, NO);
		LTEST(0xffAABBCCDDl, <=, 0xffAABBCCDDl, YES);
		LTEST(0xffAABBCCDDl, >,  0xffAABBCCDDl, NO);
		LTEST(0xffAABBCCDDl, >=, 0xffAABBCCDDl, YES);
		LTEST(0xffAABBCCDDl, ==, 0xffAABBCCDDl, YES);
		LTEST(0xffAABBCCDDl, !=, 0xffAABBCCDDl, NO);

		LTEST(0xffAABBCCDDl, <,  0xfffAABBCCDDl, YES);
		LTEST(0xffAABBCCDDl, <=, 0xfffAABBCCDDl, YES);
		LTEST(0xffAABBCCDDl, >,  0xfffAABBCCDDl, NO);
		LTEST(0xffAABBCCDDl, >=, 0xfffAABBCCDDl, NO);
		LTEST(0xffAABBCCDDl, ==, 0xfffAABBCCDDl, NO);
		LTEST(0xffAABBCCDDl, !=, 0xfffAABBCCDDl, YES);

		LTEST(0xffAABBCCDDl, <,  0xfAABBCCDDl, NO);
		LTEST(0xffAABBCCDDl, <=, 0xfAABBCCDDl, NO);
		LTEST(0xffAABBCCDDl, >,  0xfAABBCCDDl, YES);
		LTEST(0xffAABBCCDDl, >=, 0xfAABBCCDDl, YES);
		LTEST(0xffAABBCCDDl, ==, 0xfAABBCCDDl, NO);
		LTEST(0xffAABBCCDDl, !=, 0xfAABBCCDDl, YES);

		// Greater in absolute value is negative

		LTEST(0xffABCDl, <,  -0xfffABCDl, NO);
		LTEST(0xffABCDl, <=, -0xfffABCDl, NO);
		LTEST(0xffABCDl, >,  -0xfffABCDl, YES);
		LTEST(0xffABCDl, >=, -0xfffABCDl, YES);
		LTEST(0xffABCDl, ==, -0xfffABCDl, NO);
		LTEST(0xffABCDl, !=, -0xfffABCDl, YES);

		LTEST(-0xffABCDl, <,  0xfABCDl, YES);
		LTEST(-0xffABCDl, <=, 0xfABCDl, YES);
		LTEST(-0xffABCDl, >,  0xfABCDl, NO);
		LTEST(-0xffABCDl, >=, 0xfABCDl, NO);
		LTEST(-0xffABCDl, ==, 0xfABCDl, NO);
		LTEST(-0xffABCDl, !=, 0xfABCDl, YES);

#		undef LTEST
#		undef YES
#		undef NO
	}

	static void test_GETPUTFIELD() {
		members m = new members();

		s_c1 = 'X';
		m.c = s_c1;
		TEST(m.c == 'X');

		s_s1 = -34;
		m.s = s_s1;
		TEST(m.s == -34);

		s_i1 = 987;
		m.i = s_i1;
		TEST(m.i == 987);

		m.a = s_a1;
		TEST(m.a == s_a1);

		s_l1 = 0x987AABBCCDDl;
		m.l = s_l1;
		TEST(m.l == 0x987AABBCCDDl);

		s_f1 = 98.12f;
		m.f = s_f1;
		TEST(m.f == 98.12f);

		s_d1 = 98.12;
		m.d = s_d1;
		TEST(m.d == 98.12);
	}

	static void doThrow(int line) throws exception {
		throw new exception(line);
	}

	static void test_ATHROW() {
		s_b = false;
		try {
			/* Propagate line in java source with exception.
			 * Then compare with line provided by exception stack trace.
			 */
			s_i =
			__JAVA_LINE__
			;
			throw new exception(s_i + 2);
		} catch (exception e) {
			s_b = true;
			TODO();
			//TEST(e.line == e.getStackTrace()[0].getLineNumber());
		}
		TEST(s_b); /* exception catched ? */
	}

	static void test_IFNULL() {
		TODO();
	}

	static void test_IFNONNULL() {
		TODO();
	}

	static void test_IFXX_ICMPXX() {
#		define YES 10
#		define NO 20
#		define ITEST(val1, op, val2, expect) \
			s_i1 = val1; \
			s_i = (s_i1 op val2 ? YES: NO); \
			TEST(s_i == expect); \
			s_i1 = val1; \
			s_i2 = val2; \
			s_i = (s_i1 op s_i2 ? YES: NO); \
			TEST(s_i == expect); 

		ITEST(0xffABCD, <,  0xffABCD, NO);
		ITEST(0xffABCD, <=, 0xffABCD, YES);
		ITEST(0xffABCD, >,  0xffABCD, NO);
		ITEST(0xffABCD, >=, 0xffABCD, YES);
		ITEST(0xffABCD, ==, 0xffABCD, YES);
		ITEST(0xffABCD, !=, 0xffABCD, NO);

		ITEST(0xffABCD, <,  0xfffABCD, YES);
		ITEST(0xffABCD, <=, 0xfffABCD, YES);
		ITEST(0xffABCD, >,  0xfffABCD, NO);
		ITEST(0xffABCD, >=, 0xfffABCD, NO);
		ITEST(0xffABCD, ==, 0xfffABCD, NO);
		ITEST(0xffABCD, !=, 0xfffABCD, YES);

		ITEST(0xffABCD, <,  0xfABCD, NO);
		ITEST(0xffABCD, <=, 0xfABCD, NO);
		ITEST(0xffABCD, >,  0xfABCD, YES);
		ITEST(0xffABCD, >=, 0xfABCD, YES);
		ITEST(0xffABCD, ==, 0xfABCD, NO);
		ITEST(0xffABCD, !=, 0xfABCD, YES);

		// Greater in absolute value is negative

		ITEST(0xffABCD, <,  -0xfffABCD, NO);
		ITEST(0xffABCD, <=, -0xfffABCD, NO);
		ITEST(0xffABCD, >,  -0xfffABCD, YES);
		ITEST(0xffABCD, >=, -0xfffABCD, YES);
		ITEST(0xffABCD, ==, -0xfffABCD, NO);
		ITEST(0xffABCD, !=, -0xfffABCD, YES);

		ITEST(-0xffABCD, <,  0xfABCD, YES);
		ITEST(-0xffABCD, <=, 0xfABCD, YES);
		ITEST(-0xffABCD, >,  0xfABCD, NO);
		ITEST(-0xffABCD, >=, 0xfABCD, NO);
		ITEST(-0xffABCD, ==, 0xfABCD, NO);
		ITEST(-0xffABCD, !=, 0xfABCD, YES);

#		undef YES
#		undef NO
#		undef ITEST
	}

	static void test_IF_ACMPXX() {
		s_i = (s_a1 == s_a1 ? 10 : 20);
		TEST(s_i == 10);
		s_i = (s_a1 != s_a1 ? 10 : 20);
		TEST(s_i == 20);
		s_i = (s_a1 == s_a2 ? 10 : 20);
		TEST(s_i == 20);
		s_i = (s_a1 != s_a2 ? 10 : 20);
		TEST(s_i == 10);
	}

	static void test_XRETURN() {
#		define RTEST(type, value) \
			s_##type = value; \
			s_##type##1 = s_##type##m(); \
			TEST(s_##type##1 == value);

		RTEST(c, 'a');
		RTEST(s, 99);
		RTEST(i, 0xFFEEDDCC);
		RTEST(l, 0xAABBCCDD11223344l);
		RTEST(a, s_a2);
		RTEST(f, 1337.1f);
		RTEST(d, 1983.1975);

#		undef RTEST
	}

	static interface i1 { };
	static interface i2 { };
	static interface i3 extends i2 { };
	static interface i4 extends i3 { };
	static interface i5 { };
	static class c1 { };
	static class c2 extends c1 implements i1 { };
	static class c3 extends c2 implements i4 { };
	static class c4 { };

	static void test_INSTANCEOF() {
		Object x = new c3();
		TEST(x instanceof i1);
		TEST(x instanceof i2);
		TEST(x instanceof i3);
		TEST(x instanceof i4);
		TEST(! (x instanceof i5));
		TEST(! (x instanceof java.lang.Runnable));
		TEST(x instanceof c1);
		TEST(x instanceof c2);
		TEST(x instanceof c3);
		TEST(! (x instanceof c4));
		TEST(! (x instanceof java.lang.String));
		TEST(x instanceof java.lang.Object);
	}

	static void test_CHECKCAST() {
		Object x = new c3();

#		define TESTCAST(klass, res) \
			s_b = true; \
			try { \
				klass y = (klass)x; \
			} catch (ClassCastException e) { \
				s_b = false; \
			} \
			TEST(s_b == res);

		TESTCAST(i1, true);
		TESTCAST(i2, true);
		TESTCAST(i3, true);
		TESTCAST(i4, true);
		TESTCAST(i5, false);
		TESTCAST(java.lang.Runnable, false);
		TESTCAST(c1, true);
		TESTCAST(c2, true);
		TESTCAST(c3, true);
		TESTCAST(c4, false);
		TESTCAST(java.lang.String, false);
		TESTCAST(java.lang.Object, true);

#		undef TESTCAST
	}
	

	static void test_emit_exception_stubs() {
		s_b = false;
		try {
			s_c = s_ca[10];	
		} catch (ArrayIndexOutOfBoundsException e) {
			s_b = true;
		}
		TEST(s_b);
		s_b = false;
		try {
			s_a = s_aa[10];
		} catch (ArrayIndexOutOfBoundsException e) {
			s_b = true;
		}
		TEST(s_b);
	}

	static void test_IAND() {
		s_i1 =      0xcccccccc;
		s_i2 =      0x0f080400;
		s_i = s_i1 & s_i2;
		TEST(s_i == 0x0c080400);
	}
	static void test_IOR() {
		s_i1 =      0x0a0b0c1d;
		s_i2 =      0x10203040;
		s_i = s_i1 | s_i2;
		TEST(s_i == 0x1a2b3c5d);
	}
	static void test_IXOR() {
		s_i1 =      0x0f0f1700;
		s_i2 =      0xf00f3300;
		s_i = s_i1 ^ s_i2;
		TEST(s_i == 0xff002400);

		// xor swapping algorithm

		s_i1 = 0xa75bced8;
		s_i2 = 0x1458aa56;

		s_i1 ^= s_i2;
		s_i2 ^= s_i1;
		s_i1 ^= s_i2;

		TEST(s_i2 == 0xa75bced8);
		TEST(s_i1 == 0x1458aa56);
	}

	static void main(String[] args) {
		/*
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
		test_IF_LXX_LCMPXX();
		test_GETPUTFIELD();
		test_ATHROW();
		test_IFNULL();
		test_IFNONNULL();
		test_IFXX_ICMPXX();
		test_IF_ACMPXX();
		test_XRETURN();
		test_INSTANCEOF();
		test_emit_exception_stubs();
		test_CHECKCAST();
		test_IAND();
		test_IOR();
		test_IXOR();
		test_BASTORE();
		test_BALOAD();
		test_SASTORE();
		test_SALOAD();
		test_IALOAD();
		test_IASTORE();
		*/
		test_FADD();
		test_FMUL();
		test_FSUB();
		test_FDIV();
		test_DADD();
		test_DMUL();
		test_DSUB();
		test_DDIV();

		summary();
	}


};

// vim: syntax=java
