/** tests/compiler2/junit/Dadd.java
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

public class Dadd extends Compiler2TestBase {

	@Test
	public void test0() {
		testResultEqual("dadd", "(DD)D", 2.0f, 2.3f);
	}

	@Test
	public void test1() {
		testResultEqual("dadd", "(DD)D", -3.2f, 5.8f);
	}

	@Test
	public void test2() {
		testResultEqual("dadd", "(DD)D", -3.2f, -5.8f);
	}

	@Test
	public void test3() {
		testResultEqual("dadd", "(DD)D", 1.99234f, 1.24234f);
	}

	/**
	 * This is the method under test.
	 */
	static double dadd(double x, double y) {
		return x + y;
	}
}
