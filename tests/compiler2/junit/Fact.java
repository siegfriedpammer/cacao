/* tests/compiler2/junit/Fact.java - Fact

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

import org.junit.Test;
import org.junit.Ignore;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

public class Fact extends Compiler2TestBase {

	static long fact(long n) {

		long res = 1;
		while (1 < n) {
			res *= n--;
		}
		return res;
	}

    @Test
    public void thisAlwaysPasses() {
		for (long i = 0; i < 10; i++) {
			testResultEqual("fact", "(J)J", i);
		}
    }

}

