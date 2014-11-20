/** tests/compiler2/junit/Sqrt.java - Sqrt
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

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

@RunWith(Parameterized.class)
public class Sqrt extends Compiler2TestBase {

	private long value;

	public Sqrt(long value) {
		this.value = value;
	}

	@Parameters
	public static Collection<Long[]> data() {
		List<Long[]> list = new ArrayList<Long[]>();
		list.add(new Long[] { 0L });
		list.add(new Long[] { 4L });
		list.add(new Long[] { 15L });
		list.add(new Long[] { 123L });
		list.add(new Long[] { 284L });
		list.add(new Long[] { 513L });
		list.add(new Long[] { 777L });
		list.add(new Long[] { 845L });
		list.add(new Long[] { 1000L });
		return list;
	}

	@Test
	public void test0() {
		testResultEqual("sqrt", "(J)J", value);
	}

	/**
	 * This is the method under test.
	 */
	static long sqrt(long x) {
		long y = 0;
		long z = x + 1;
		while ((y + 1) != z) {
			long t = (y + z) / 2;
			if (t * t <= x) {
				y = t;
			} else {
				z = t;
			}
		}
		return y;
	}

}
