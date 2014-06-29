/* tests/compiler2/junit/ArrayLoadFloat.java - ArrayLoadFloat

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

public class ArrayLoadFloat extends Compiler2TestBase {

	@Test
	public void test0() {
		final int n = 10;
		float[] array = new float[n];
		for (int i = 0; i < array.length; i++) {
			array[i] = (float)Math.PI / i;
		}
		for (int i = 0; i < array.length; i++) {
			testResultEqual("testArrayLoadFloat", "([FI)F", array, i);
		}
	}

	/**
	 * This is the method under test.
	 */
	static float testArrayLoadFloat(float test[], int index) {
		return test[index];
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
