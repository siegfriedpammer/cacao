/* tests/compiler2/junit/MatMult.java - MatMult

   Copyright (C) 1996-2014
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

import static org.junit.Assert.*;

import java.util.List;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;
import org.junit.Ignore;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

public class MatMult extends Compiler2TestBase {


    @Test
    public void test0() {
		final int n = 4;
		final int m = 2;
		final int p = 3;

		int[][] a = new int[/*n*/][/*m*/] {
			{ 1, 2},
			{ 3, 4},
			{ 5, 6},
			{ 7, 8}
		};
		int[][] b = new int[/*m*/][/*p*/] {
			{3, 6, 9},
			{4, 8,12}
		};

		int[][] c = new int[n][p];

		int[][] aBaseline = a.clone();
		int[][] bBaseline = b.clone();
		int[][] cBaseline = c.clone();
		runBaseline("matMult", "([[I[[I[[I)V", aBaseline, bBaseline, cBaseline);

		int[][] aCompiler2 = a.clone();
		int[][] bCompiler2 = b.clone();
		int[][] cCompiler2 = c.clone();
		runCompiler2("matMult", "([[I[[I[[I)V", aCompiler2, bCompiler2, cCompiler2);

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
	 * @param A n*m matrix (input)
	 * @param B m*p matrix (input)
	 * @param C n*p matrix (output, preallocated)
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
				int sum=0;
				for (int k = 0; k < m; ++k) {
				sum += A[i][k] * B[k][j];
				}
				AB[i][j] = sum;
			}
		}
	}
}

/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: java
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
