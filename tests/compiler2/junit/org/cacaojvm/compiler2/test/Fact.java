/** tests/compiler2/junit/Fact.java - Fact
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

import java.util.Arrays;
import java.util.Collection;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

@RunWith(Parameterized.class)
public class Fact extends Compiler2TestBase {

	private long value;

	public Fact(long value) {
		this.value = value;
	}

	@Parameters
	public static Collection<Long[]> data() {
		Long[][] list = new Long[10][1];
		for (int i = 0; i < 10; i++) {
			list[i][0] = (long) i;
		}
		return Arrays.asList(list);
	}

	@Test
	public void test0() {
		TimingResults tr = new TimingResults();
		testResultEqualWithTiming("fact", "(J)J", tr, value);
		tr.report();
	}

	/**
	 * This is the method under test.
	 */
	static long fact(long n) {

		long res = 1;
		while (1 < n) {
			res *= n--;
		}
		return res;
	}
}
