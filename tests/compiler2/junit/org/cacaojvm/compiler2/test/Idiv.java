/** tests/compiler2/junit/Idiv.java
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
import org.junit.Ignore;

public class Idiv extends Compiler2TestBase {

	@Test
	public void test0() {
		testResultEqual("idiv", "(II)I", 10, 5);
	}

	@Test
	public void test1() {
		testResultEqual("idiv", "(II)I", 11, 5);
	}

	@Test
	public void test2() {
		testResultEqual("idiv", "(II)I", 5, 5);
	}

	@Test
	public void test3() {
		testResultEqual("idiv", "(II)I", 3, 5);
	}
	
	@Test
	public void test4() {
		testResultEqual("idiv", "(II)I", -10, 5);
	}

	@Test
	public void test5() {
		testResultEqual("idiv", "(II)I", 10, -5);
	}

	@Test
	public void test6() {
		testResultEqual("idiv", "(II)I", -10, -5);
	}

	@Test
	public void test7() {
		testResultEqual("idiv", "(II)I", 0, 5);
	}

	@Ignore("The test framework does not yet pass on exceptions properly")
	@Test
	public void test8() {
		testResultEqual("idiv", "(II)I", 5, 0);
	}

	/**
	 * This is the method under test.
	 */
	static int idiv(int x, int y) {
		return x / y;
	}
}

