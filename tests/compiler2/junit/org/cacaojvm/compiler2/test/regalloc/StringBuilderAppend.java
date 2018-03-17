/** tests/compiler2/junit/regalloc/StringBuilderAppend.java
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

package org.cacaojvm.compiler2.test.regalloc;

import org.cacaojvm.compiler2.test.*;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class StringBuilderAppend extends Compiler2TestBase {

	@Test
	public void testAppendSingleCharacter() throws Throwable {
		StringBuilder builderBaseline = new StringBuilder();
		StringBuilder builderCompiler2 = new StringBuilder();

		runBaseline(StringBuilder.class, "append", "(C)Ljava/lang/StringBuilder;", builderBaseline, 'a');
		runCompiler2(StringBuilder.class, "append", "(C)Ljava/lang/StringBuilder;", builderCompiler2, 'a');

		assertEquals(builderCompiler2.toString(), builderBaseline.toString());
	}

	@Test
	public void testAppendLongValue() throws Throwable {
		StringBuilder builderBaseline = new StringBuilder();
		StringBuilder builderCompiler2 = new StringBuilder();

		runBaseline(StringBuilder.class, "append", "(J)Ljava/lang/StringBuilder;", builderBaseline, Long.MAX_VALUE);
		runCompiler2(StringBuilder.class, "append", "(J)Ljava/lang/StringBuilder;", builderCompiler2, Long.MAX_VALUE);

		assertEquals(builderCompiler2.toString(), builderBaseline.toString());
	}

	@Test
	public void testStringValueOf() {
		testResultEqual(String.class, "valueOf", "(I)Ljava/lang/String;", 5);
	}
}