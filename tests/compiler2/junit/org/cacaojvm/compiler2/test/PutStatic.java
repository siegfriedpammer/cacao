/** tests/compiler2/junit/PutStatic.java - PutStatic
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

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class PutStatic extends Compiler2TestBase {

	static byte byteField;

	/** This is the method under test. */
	static void setByte(byte value) {
		byteField = value;
	}

	@Test
	public void testByte() {
		byte value = 111;

		runBaseline("setByte", "(B)V", value);
		byte baselineValue = byteField;

		// Reset the field under test.
		byteField = 0;

		runCompiler2("setByte", "(B)V", value);
		byte compiler2Value = byteField;

		assertEquals(baselineValue, compiler2Value);
	}

	static short shortField;

	/** This is the method under test. */
	static void setShort(short value) {
		shortField = value;
	}

	@Test
	public void testShort() {
		short value = 11111;

		runBaseline("setShort", "(S)V", value);
		short baselineValue = shortField;

		// Reset the field under test.
		shortField = 0;

		runCompiler2("setShort", "(S)V", value);
		short compiler2Value = shortField;

		assertEquals(baselineValue, compiler2Value);
	}

	static int intField;

	/** This is the method under test. */
	static void setInt(int value) {
		intField = value;
	}

	@Test
	public void testInt() {
		int value = 0xDEADBEEF;

		runBaseline("setInt", "(I)V", value);
		int baselineValue = intField;

		// Reset the field under test.
		intField = 0;

		runCompiler2("setInt", "(I)V", value);
		int compiler2Value = intField;

		assertEquals(baselineValue, compiler2Value);
	}

	static long longField;

	/** This is the method under test. */
	static void setLong(long value) {
		longField = value;
	}

	@Test
	public void testLong() {
		long value = 0xDEADBEEF12345678L;

		runBaseline("setLong", "(J)V", value);
		long baselineValue = longField;

		// Reset the field under test.
		intField = 0;

		runCompiler2("setLong", "(J)V", value);
		long compiler2Value = longField;

		assertEquals(baselineValue, compiler2Value);
	}
}
