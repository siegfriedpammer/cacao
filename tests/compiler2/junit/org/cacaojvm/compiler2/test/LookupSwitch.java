/** tests/compiler2/junit/LookupSwitch.java - LookupSwitch
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

public class LookupSwitch extends Compiler2TestBase {

	@Test
	public void test0() {
		testResultEqual("loopupswitch", "(I)I", 0);
	}

	@Test
	public void test1() {
		testResultEqual("loopupswitch", "(I)I", 42);
	}

	@Test
	public void test2() {
		testResultEqual("loopupswitch", "(I)I", -42);
	}

	@Test
	public void test3() {
		testResultEqual("loopupswitch", "(I)I", -1);
	}

	@Test
	public void test4() {
		testResultEqual("loopupswitch", "(I)I", Short.MAX_VALUE);
	}

	@Test
	public void test5() {
		testResultEqual("loopupswitch", "(I)I", Integer.MAX_VALUE);
	}

	/**
	 * This is the method under test.
	 */
	static int loopupswitch(int x) {
		switch (x) {
		case -1: return 17;
		case  0: return 31;
		case 42: return 19;
		case Short.MAX_VALUE: return 51;
		case Integer.MAX_VALUE: return 47;
		}
		return -1;
	}
}
