/** tests/compiler2/junit/Fneg.java - Fneg
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

import org.junit.Test;

public class Fneg extends Compiler2TestBase {

	@Test
	public void test0() {
		testResultEqual("fneg", "(F)F", 0.0F);
	}

	@Test
	public void test1() {
		testResultEqual("fneg", "(F)F", 42.0F);
	}

	@Test
	public void test2() {
		testResultEqual("fneg", "(F)F", -42.0F);
	}

	@Test
	public void test3() {
		testResultEqual("fneg", "(F)F", Float.MIN_VALUE);
	}

	@Test
	public void test4() {
		testResultEqual("fneg", "(F)F", Float.MAX_VALUE);
	}

	@Test
	public void test5() {
		testResultEqual("fneg", "(F)F", 0.0F / 0.0F);
	}

	/**
	 * This is the method under test.
	 */
	static float fneg(float x) {
		return -x;
	}
}
