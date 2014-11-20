/** tests/compiler2/junit/Permut.java - Permut
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

import static org.junit.Assert.assertArrayEquals;
import org.junit.Test;

public class Permut extends Compiler2TestBase {

	private static int fact(int n) {
		if (n <= 1) {
			return 1;
		}
		return n * fact(n - 1);
	}

	@Test
	public void test0() {
		int n = 5;
		int fn = fact(n);

		int a[] = new int[n];
		int p[] = new int[n];
		int result[][] = new int[fn][n];

		for (int i = 0; i < n; ++i) {
			a[i] = i;
			p[i] = 0;
		}

		int[] aBaseline = a.clone();
		int[] pBaseline = p.clone();
		int[][] resultBaseline = result.clone();
		runBaseline("permut", "([I[I[[I)V", aBaseline, pBaseline,
				resultBaseline);

		int[] aCompiler2 = a.clone();
		int[] pCompiler2 = p.clone();
		int[][] resultCompiler2 = result.clone();
		runCompiler2("permut", "([I[I[[I)V", aCompiler2, pCompiler2,
				resultCompiler2);

		assertArrayEquals(aBaseline, aCompiler2);
		assertArrayEquals(pBaseline, pCompiler2);
		assertArrayEquals(resultBaseline, resultCompiler2);

		assertNotEquals(aBaseline, aCompiler2);
		assertNotEquals(pBaseline, pCompiler2);
		assertNotEquals(resultBaseline, resultCompiler2);
	}

	/**
	 * This is the method under test.
	 * 
	 * @param list
	 *            a [n]
	 * @param list
	 *            p [n]
	 * @param result
	 *            result [n!][n]
	 */
	static void permut(int a[], int p[], int result[][]) {
		int n = a.length;
		int i = 1;
		int c = 0;
		while (i < n) {
			if (p[i] < i) {
				int j = ((i & 1) == 0) ? 0 : p[i];
				int tmp = a[i];
				a[i] = a[j];
				a[j] = tmp;
				// result
				for (int k = 0; k < n; k++) {
					result[c][k] = a[k];
				}
				c++;

				p[i]++;
				i = 1;
			} else {
				p[i] = 0;
				i++;
			}
		}
	}
}
