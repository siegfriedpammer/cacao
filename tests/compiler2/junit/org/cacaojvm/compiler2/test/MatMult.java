/** tests/compiler2/junit/MatMult.java - MatMult
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
import org.junit.Ignore;

public class MatMult extends Compiler2TestBase {

	@Test
	public void test_matMult() throws Throwable {

		// @formatter:off
		int[][] a = new int[][] {
				{ 1, 2 },
				{ 3, 4 },
				{ 5, 6 },
				{ 7, 8 }
			};
		int[][] b = new int[][] {
				{ 3, 6, 9 },
				{ 4, 8, 12 }
			};
		// @formatter:on

		int[][] c = new int[a.length][b[0].length];

		TimingResults tr = new TimingResults();

		int[][] aBaseline = a.clone();
		int[][] bBaseline = b.clone();
		int[][] cBaseline = c.clone();
		runBaseline("matMult", "([[I[[I[[I)V", tr.baseline, aBaseline, bBaseline, cBaseline);

		int[][] aCompiler2 = a.clone();
		int[][] bCompiler2 = b.clone();
		int[][] cCompiler2 = c.clone();
		runCompiler2("matMult", "([[I[[I[[I)V", tr.compiler2, aCompiler2, bCompiler2,
				cCompiler2);

		tr.report();

		assertArrayEquals(aBaseline, aCompiler2);
		assertArrayEquals(bBaseline, bCompiler2);
		assertArrayEquals(cBaseline, cCompiler2);

		assertNotEquals(aBaseline, aCompiler2);
		assertNotEquals(bBaseline, bCompiler2);
		assertNotEquals(cBaseline, cCompiler2);
	}

	/**
	 * Matrix multiplication.
	 *
	 * @param A
	 *            n*m matrix (input)
	 * @param B
	 *            m*p matrix (input)
	 * @param C
	 *            n*p matrix (output, preallocated)
	 */
	static void matMult(int A[][], int B[][], int AB[][]) {
		// sanity checks
		int n = A.length;
		int m = B.length;
		if (n == 0 || m == 0)
			return;
		if (A[0].length != m)
			return;
		int p = B[0].length;
		if (AB.length != n)
			return;
		if (AB[0].length != p)
			return;

		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < p; ++j) {
				int sum = 0;
				for (int k = 0; k < m; ++k) {
					sum += A[i][k] * B[k][j];
				}
				AB[i][j] = sum;
			}
		}
	}
}
