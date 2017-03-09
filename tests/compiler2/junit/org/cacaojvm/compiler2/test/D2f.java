/** tests/compiler2/junit/D2F.java
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

public class D2f extends Compiler2TestBase {
	
	@Test
	public void test0() {
		testResultEqual("d2f", "(D)F", 10.0);
	}

	@Test
	public void test1() {
		testResultEqual("d2f", "(D)F", -10.0);
	}

	@Test
	public void test2() {
		testResultEqual("d2f", "(D)F", 10.12345);
	}
	
	@Test
	public void test3() {
		testResultEqual("d2f", "(D)F", -10.12345);
	}

	@Test
	public void test4() {
		testResultEqual("d2f", "(D)F", 4.9E-324);
	}

	@Test
	public void test5() {
		testResultEqual("d2f", "(D)F", 1.7976931348623157E308);
	}	

	@Test
	public void test6() {
		testResultEqual("d2f", "(D)F", -1.7976931348623157E308);
	}	
	
	/**
	 * This is the method under test.
	 */
	static float d2f(double x) {
		return (float)x;
	}
}