/* tests/compiler2/junit/Fcmp.java - Fcmp

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
import java.lang.Math.*;

import org.junit.Test;
import org.junit.Ignore;
import org.junit.runners.JUnit4;

public class Fcmp extends Compiler2TestBase {

    @Test
    public void test0() {
		testResultEqual("fcmp", "(FF)I", -1.0F, 1.0F);
    }

    @Test
    public void test1() {
		testResultEqual("fcmp", "(FF)I", 1.0F, -1.0F);
    }

    @Test
    public void test2() {
		testResultEqual("fcmp", "(FF)I", 0.0F, 0.0F);
    }

    @Test
    public void test3() {
		testResultEqual("fcmp", "(FF)I", Float.MIN_VALUE, Float.MAX_VALUE);
    }

    @Test
    public void test4() {
		testResultEqual("fcmp", "(FF)I", Float.MAX_VALUE, Float.MIN_VALUE);
    }

    @Test
    public void test5() {
		testResultEqual("fcmp", "(FF)I", 0.0F/0.0F, 0.0F);
    }

    @Test
    public void test6() {
		testResultEqual("fcmp", "(FF)I", 0.0F, 0.0F/0.0F);
    }

    @Test
    public void test7() {
		testResultEqual("fcmp", "(FF)I", 0.0F/0.0F, 0.0F/0.0F);
    }

	/**
	 * This is the method under test.
	 */
	static int fcmp(float a, float b) {
		if (a < b)
			return -1;
		return 1;
	}
}

