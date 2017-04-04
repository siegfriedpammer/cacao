/** tests/compiler2/junit/ArrayLoad.java - ArrayLoad
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

public class ArrayLoad extends Compiler2TestBase {

	@Test
	public void test0() {
		final int n = 10;
		long[] array = new long[n];
		for (int i = 0; i < array.length; i++) {
			array[i] = i;
		}
		for (int i = 0; i < array.length; i++) {
			testResultEqual("testArrayLoad", "([JI)J", array, i);
		}
	}

	/**
	 * This is the method under test.
	 */
	static long testArrayLoad(long test[], int index) {
		return test[index];
	}
}
