/* tests/compiler2/junit/Min.java - Min

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
import java.util.Arrays;
import java.util.Collection;

import org.junit.Test;
import org.junit.Ignore;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

@RunWith(Parameterized.class)
public class Min extends Compiler2TestBase {

	private long x;
	private long y;

	public Min(long x, long y) {
		this.x = x;
		this.y = y;
	}

	@Parameters
	public static Collection<Long[]> data() {
		Long[][] list = new Long[][] {
			{  1L,  2L},
			{  2L,  1L},
			{  3L,  2L},
			{-42L,  5L},
			{-21L,-11L},
			{  0L,-11L},
			{ 21L,  0L},
		};
		return Arrays.asList(list);
	}

    @Test
    public void test0() {
		testResultEqual("min", "(JJ)J", x, y);
    }

	/**
	 * This is the method under test.
	 */
	static long min(long a, long b) {
		return (a < b) ? a : b;
	}

}

