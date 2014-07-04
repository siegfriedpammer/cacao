/** tests/compiler2/junit/Array2dimLoad.java - Array2dimLoad
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

public class Array2dimLoad extends Compiler2TestBase {

	@Test
	public void test0() {
		int[][] a = new int[10][10];
		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 10; j++) {
				a[i][j] = i * j;
			}
		}
		for (int i = 1; i < 10; i += 2) {
			for (int j = 2; j < 10; j += 3) {
				testResultEqual("testArray2dimLoad", "([[III)I", a, i, j);
			}
		}
	}

	static int testArray2dimLoad(int test[][], int i, int j) {
		return test[i][j];
	}
}
