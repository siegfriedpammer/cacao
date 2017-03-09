/** tests/compiler2/junit/Overflow.java - Overflow
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

public class Overflow extends Compiler2TestBase {

	@Test
	public void testOverflowLong0() {
		testResultEqual("overflowLong", "(J)J", 0);
	}

	@Test
	public void testOverflowLong1() {
		testResultEqual("overflowLong", "(J)J", 42);
	}

	@Test
	public void testOverflowLong2() {
		testResultEqual("overflowLong", "(J)J", -17);
	}

	@Test
	public void testOverflowLongIntLong0() {
		testResultEqual("overflowLongIntLong", "(J)J", 0);
	}

	@Test
	public void testOverflowLongIntLong1() {
		testResultEqual("overflowLongIntLong", "(J)J", 42);
	}

	@Test
	public void testOverflowLongIntLong2() {
		testResultEqual("overflowLongIntLong", "(J)J", -11);
	}

	@Test
	public void testOverflowInt0() {
		testResultEqual("overflowInt", "(I)I", 0);
	}

	@Test
	public void testOverflowInt1() {
		testResultEqual("overflowInt", "(I)I", 42);
	}

	@Test
	public void testOverflowInt2() {
		testResultEqual("overflowInt", "(I)I", -11);
	}

	/**
	 * This is the method under test.
	 */
	static long overflowLong(long n) {
		return n + Long.MAX_VALUE;
	}

	/**
	 * This is the method under test.
	 */
	static long overflowLongIntLong(long n) {
		int y = 1;
		y += Integer.MAX_VALUE;
		y -= Integer.MIN_VALUE;
		return n + y;
	}

	/**
	 * This is the method under test.
	 */
	static int overflowInt(int n) {
		return n + Integer.MAX_VALUE;
	}

}
