/** tests/compiler2/junit/Compiler2TestBase.java - CACAO compiler2 unit test base
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
package org.cacaojvm.compiler2.test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

class Compiler2TestBase extends Compiler2Test {

	protected void testResultEqual(Class<?> compileClass, String methodName,
			String methodDesc, Object... args) {
		Object resultBaseline = runBaseline(compileClass, methodName,
				methodDesc, args);
		Object resultCompiler2 = runCompiler2(compileClass, methodName,
				methodDesc, args);
		assertEquals(resultCompiler2, resultBaseline);
	}

	protected void testResultEqual(String methodName, String methodDesc,
			Object... args) {
		testResultEqual(getClass(), methodName, methodDesc, args);
	}

	protected Object runBaseline(String methodName, String methodDesc,
			Object... args) {
		compileBaseline(getClass(), methodName, methodDesc);
		return executeMethod(getClass(), methodName, methodDesc, args);
	}

	protected Object runCompiler2(String methodName, String methodDesc,
			Object... args) {
		compileCompiler2(getClass(), methodName, methodDesc);
		return executeMethod(getClass(), methodName, methodDesc, args);
	}


	protected Object runBaseline(Class<?> compileClass, String methodName, 
			String methodDesc, Object... args) {
		compileBaseline(compileClass, methodName, methodDesc);
		return executeMethod(compileClass, methodName, methodDesc, args);
	}

	protected Object runCompiler2(Class<?> compileClass, String methodName, 
			String methodDesc, Object... args) {
		compileCompiler2(compileClass, methodName, methodDesc);
		return executeMethod(compileClass, methodName, methodDesc, args);
	}

	/**
	 * Not available in JUnit <4.11
	 */
	static public void assertNotEquals(Object first, Object second) {
		if (first == null) {
			if (second == null) {
				fail("Both objects are null");
			}
			return;
		}
		assertTrue("Both objects are equal: " + first, !first.equals(second));
	}
}
