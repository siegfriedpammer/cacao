/* tests/compiler2/junit/BoyerMoore.java - BoyerMoore

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

public class BoyerMoore extends Compiler2TestBase {


    @Test
    public void test0() {
		String strPattern = "ana";
		String strText = "bananas";

		int[] pattern = new int[strPattern.length()];
		int[] text = new int[strText.length()];
		int[] matches = new int[text.length];
		int[] rightMostIndexes = new int[256];

		int i = 0;
		for (byte c : strPattern.getBytes()) {
			pattern[i++] = (int) c;
		}
		i = 0;
		for (byte c : strText.getBytes()) {
			text[i++] = (int) c;
		}

		int[] patternBaseline = pattern.clone();
		int[] textBaseline = text.clone();
		int[] matchesBaseline = matches.clone();
		int[] rightMostIndexesBaseline = rightMostIndexes.clone();
		Object resultBaseline = runBaseline("BoyerMoore", "([I[I[I[I)I", patternBaseline, textBaseline, matchesBaseline, rightMostIndexesBaseline);

		int[] patternCompiler2 = pattern.clone();
		int[] textCompiler2 = text.clone();
		int[] matchesCompiler2 = matches.clone();
		int[] rightMostIndexesCompiler2 = rightMostIndexes.clone();
		Object resultCompiler2 = runCompiler2("BoyerMoore", "([I[I[I[I)I", patternCompiler2, textCompiler2, matchesCompiler2, rightMostIndexesCompiler2);

		assertEquals(resultBaseline,resultCompiler2);

		assertArrayEquals(patternBaseline,patternCompiler2);
		assertArrayEquals(textBaseline,textCompiler2);
		assertArrayEquals(matchesBaseline,matchesCompiler2);
		assertArrayEquals(rightMostIndexesBaseline,rightMostIndexesCompiler2);

		assertNotEquals(patternBaseline,patternCompiler2);
		assertNotEquals(textBaseline,textCompiler2);
		assertNotEquals(matchesBaseline,matchesCompiler2);
		assertNotEquals(rightMostIndexesBaseline,rightMostIndexesCompiler2);
    }

	/**
	 * This is the method under test.
	 */
	static int BoyerMoore(int pattern[], int text[], int matches[], int rightMostIndexes[]) {
		int m = text.length;
		int n = pattern.length;
		int res_index = 0;

		for (int i = 0; i < rightMostIndexes.length; i ++) {
			rightMostIndexes[i] = -1;
		}
		for (int i = pattern.length - 1; i >= 0; i--) {
			int c = pattern[i];
			if (rightMostIndexes[c] == -1)
				rightMostIndexes[c] = i;
			}

			int alignedAt = 0;
			while (alignedAt + (n - 1) < m) {
				for (int indexInPattern = n - 1; indexInPattern >= 0; indexInPattern--) {
					int indexInText = alignedAt + indexInPattern;
					int x = text[indexInText];
					int y = pattern[indexInPattern];
					if (indexInText >= m)
						break;
					if (x != y) {
						int r = rightMostIndexes[x];
						if (r == -1) {
							alignedAt = indexInText + 1;
						}
						else {
							int shift = indexInText - (alignedAt + r);
							alignedAt += shift > 0 ? shift : alignedAt + 1;
						}
						break;
					}
					else if (indexInPattern == 0) {
						matches[res_index++]= alignedAt;
						alignedAt++;
					}
				}
			}
		return res_index;
	}
}

