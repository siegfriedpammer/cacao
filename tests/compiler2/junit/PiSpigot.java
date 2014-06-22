/* tests/compiler2/junit/PiSpigot.java - PiSpigot

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
import java.util.List;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;
import org.junit.Ignore;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

@RunWith(Parameterized.class)
public class PiSpigot extends Compiler2TestBase {

	private long x;

	public PiSpigot(long x) {
		this.x = x;
	}

	@Parameters
	public static Collection<Long[]> data() {
		List<Long[]> list = new ArrayList<Long[]>();
		for (long i = 0; i < 15 ; i++) {
			list.add(new Long[]{i});
		}
		return list;
	}

	@Test
	public void test0() {
		testResultEqual("piSpigot", "(J)D", x);
	}

	/**
	 * This is the method under test.
	 */
	static double piSpigot(long num_digits) {
		double pi = 0;
		for (long i = 0; i < num_digits; ++i) {
			// fake power
			long p =1;
			for (long j = 0; j < i; ++j) {
				p *= 16;
			}
			long d1 = 8*i + 1;
			long d2 = 8*i + 4;
			long d3 = 8*i + 5;
			long d4 = 8*i + 6;

			pi += 1/(double)p * ( 4/(double)d1 - 2/(double)d2
				- 1/(double)d3 - 1/(double)d4 );
		}
		return pi;
	}

}

