/** tests/compiler2/junit/DoubleArrayStore.java - DoubleArrayStore
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

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class DoubleArrayStore extends Compiler2TestBase {

	private static final double DELTA = 1e-15;

	@Test
	public void test0() {
		final int n = 10;
		double[] arrayBaseline = new double[n];
		double[] arrayCompiler2 = new double[n];

		for (int i = 0; i < n; i++) {
			runBaseline("testDoubleArrayStore", "([D)V", arrayBaseline);
			runCompiler2("testDoubleArrayStore", "([D)V", arrayCompiler2);
		}

		assertEquals(arrayBaseline.length, arrayCompiler2.length);

		for (int i = 0; i < n; i++) {
			assertEquals(arrayBaseline[i], arrayCompiler2[i], DELTA);
		}
	}

	/**
	 * This is the method under test.
	 */
	static void testDoubleArrayStore(double test[]) {
		for (int i = 0; i < test.length; i++) {
			test[i] = (long) i;
		}
	}
}
