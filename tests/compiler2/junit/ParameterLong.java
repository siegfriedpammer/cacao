/** tests/compiler2/junit/ParameterLong.java - ParameterLong
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

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

@RunWith(Parameterized.class)
public class ParameterLong extends Compiler2TestBase {

	private long x0;
	private long x1;
	private long x2;
	private long x3;
	private long x4;
	private long x5;
	private long x6;
	private long x7;
	private long x8;

	public ParameterLong(long x0, long x1, long x2, long x3, long x4, long x5,
			long x6, long x7, long x8) {
		this.x0 = x0;
		this.x1 = x1;
		this.x2 = x2;
		this.x3 = x3;
		this.x4 = x4;
		this.x5 = x5;
		this.x6 = x6;
		this.x7 = x7;
		this.x8 = x8;
	}

	@Parameters
	public static Collection<Long[]> data() {
		List<Long[]> list = new ArrayList<Long[]>();
		list.add(new Long[] { 1L, 2L, 3L, 4L, 5L, 6L, 7L, 8L, 9L });
		list.add(new Long[] { 1L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L });
		list.add(new Long[] { 0L, 1L, 0L, 0L, 0L, 0L, 0L, 0L, 0L });
		list.add(new Long[] { 0L, 0L, 1L, 0L, 0L, 0L, 0L, 0L, 0L });
		list.add(new Long[] { 0L, 0L, 0L, 1L, 0L, 0L, 0L, 0L, 0L });
		list.add(new Long[] { 0L, 0L, 0L, 0L, 1L, 0L, 0L, 0L, 0L });
		list.add(new Long[] { 0L, 0L, 0L, 0L, 0L, 1L, 0L, 0L, 0L });
		list.add(new Long[] { 0L, 0L, 0L, 0L, 0L, 0L, 1L, 0L, 0L });
		list.add(new Long[] { 0L, 0L, 0L, 0L, 0L, 0L, 0L, 1L, 0L });
		list.add(new Long[] { 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 1L });
		return list;
	}

	@Test
	public void testParameterLong1() {
		testResultEqual("parameterLong1", "(JJJJJJJJJ)J", x0, x1, x2, x3, x4,
				x5, x6, x7, x8);
	}

	@Test
	public void testParameterLong2() {
		testResultEqual("parameterLong2", "(JJJJJJJJJ)J", x0, x1, x2, x3, x4,
				x5, x6, x7, x8);
	}

	@Test
	public void testParameterLong3() {
		testResultEqual("parameterLong3", "(JJJJJJJJJ)J", x0, x1, x2, x3, x4,
				x5, x6, x7, x8);
	}

	static long parameterLong1(long a, long b, long c, long d, long e, long f,
			long g, long h, long i) {
		return a + b + c + d + e + f + g + h + i;
	}

	static long parameterLong2(long a, long b, long c, long d, long e, long f,
			long g, long h, long i) {
		return a + c + e + g + i;
	}

	static long parameterLong3(long a, long b, long c, long d, long e, long f,
			long g, long h, long i) {
		return b + d + f + h;
	}
}
