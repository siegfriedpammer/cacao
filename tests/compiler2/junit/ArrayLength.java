/* tests/compiler2/junit/ArrayLength.java - ArrayLength

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

public class ArrayLength extends Compiler2TestBase {

	@Test
	public void test0() {
		long[] array = new long[0];
		testResultEqual("testArrayLength", "([J)I", array);
	}

	@Test
	public void test1() {
		long[] array = new long[1];
		testResultEqual("testArrayLength", "([J)I", array);
	}

	@Test
	public void test2() {
		long[] array = new long[10];
		testResultEqual("testArrayLength", "([J)I", array);
	}

	@Test
	public void test3() {
		long[] array = new long[42];
		testResultEqual("testArrayLength", "([J)I", array);
	}

	@Test
	@Ignore
	public void test4() {
		testResultEqual("testArrayLength", "([J)I", null);
	}

	/**
	 * This is the method under test.
	 */
	static int testArrayLength(long test[]) {
		return test.length;
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
