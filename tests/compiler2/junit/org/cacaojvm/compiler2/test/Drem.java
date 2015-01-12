/** tests/compiler2/junit/Drem.java
 *
 * Copyright (C) 1996-2014
 * CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
 *
 * This file is part of CACAO.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
package org.cacaojvm.compiler2.test;

import org.junit.Test;

public class Drem extends Compiler2TestBase {

	@Test
	public void test0() {
		testResultEqual("drem", "(DD)D", 0.10, 0.05);
	}

	@Test
	public void test1() {
		testResultEqual("drem", "(DD)D", 0.11, 0.05);
	}

	@Test
	public void test2() {
		testResultEqual("drem", "(DD)D", 0.5, 0.5);
	}

	@Test
	public void test3() {
		testResultEqual("drem", "(DD)D", 0.3, 0.5);
	}

	@Test
	public void test4() {
		testResultEqual("drem", "(DD)D", 3.0, 0.5);
	}

	@Test
	public void test5() {
		testResultEqual("drem", "(DD)D", 3.3, 0.5);
	}

	@Test
	public void test6() {
		testResultEqual("drem", "(DD)D",-0.10, 0.05);
	}

	@Test
	public void test7() {
		testResultEqual("drem", "(DD)D", 0.10, -0.05);
	}

	@Test
	public void test8() {
		testResultEqual("drem", "(DD)D", -0.10, -0.05);
	}

	@Test
	public void test9() {
		testResultEqual("drem", "(DD)D", -0.13, 0.05);
	}

	@Test
	public void test10() {
		testResultEqual("drem", "(DD)D", 0.13, -0.05);
	}

	@Test
	public void test11() {
		testResultEqual("drem", "(DD)D", -0.13, -0.05);
	}
	
	@Test
	public void test12() {
		testResultEqual("drem", "(DD)D", 0.0, 0.5);
	}
	
	/**
	 * This is the method under test.
	 */
	static double drem(double x, double y) {
		return x % y;
	}
}
