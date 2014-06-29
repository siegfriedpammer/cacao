/* tests/compiler2/junit/Array2dimStore.java - Array2dimStore

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

import static org.junit.Assert.*;

import java.util.List;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;
import org.junit.Ignore;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

public class Array2dimStore extends Compiler2TestBase {


    @Test
    public void test0() {
		int[][] a = new int[10][10];

		int[][] aBaseline = a.clone();
		runBaseline("testArray2dimStore", "([[I)V", new Object[]{aBaseline});

		int[][] aCompiler2 = a.clone();
		runCompiler2("testArray2dimStore", "([[I)V", new Object[]{aCompiler2});

		assertArrayEquals(aBaseline, aCompiler2);
		assertNotEquals(aBaseline, aCompiler2);
	}

	static void testArray2dimStore(int test[][]) {
		/*
		for (int i = 0; i < test.length; i++) {
			for (int j = 0; j < test[i].length; j++) {
				test[i][j] = i * j;
			}
		}
		*/
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
