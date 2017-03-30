/** tests/compiler2/junit/GetStatic.java - GetStatic
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

public class PutField extends Compiler2TestBase {

	public byte byteField;

	/** This is the method under test. */
	static void setByte(PutField obj, byte value) {
		obj.byteField = value;
	}

	@Test
	public void testByte() {
		byte value = 111;

		runBaseline("setByte", "(Lorg/cacaojvm/compiler2/test/PutField;B)V", this, value);
		byte baselineValue = byteField;

		// Reset the field under test.
		byteField = 0;

		runCompiler2("setByte", "(Lorg/cacaojvm/compiler2/test/PutField;B)V", this, value);
		byte compiler2Value = byteField;

		assertEquals(baselineValue, compiler2Value);
	}

	public short shortField;

	/** This is the method under test. */
	static void setShort(PutField obj, short value) {
		obj.shortField = value;
	}

	@Test
	public void testShort() {
		short value = 11111;

		runBaseline("setShort", "(Lorg/cacaojvm/compiler2/test/PutField;S)V", this, value);
		short baselineValue = shortField;

		// Reset the field under test.
		shortField = 0;

		runCompiler2("setShort", "(Lorg/cacaojvm/compiler2/test/PutField;S)V", this, value);
		short compiler2Value = shortField;

		assertEquals(baselineValue, compiler2Value);
	}

	public int intField;

	/** This is the method under test. */
	static void setInt(PutField obj, int value) {
		obj.intField = value;
	}

	@Test
	public void testInt() {
		int value = 0xDEADBEEF;

		runBaseline("setInt", "(Lorg/cacaojvm/compiler2/test/PutField;I)V", this, value);
		int baselineValue = intField;

		// Reset the field under test.
		intField = 0;

		runCompiler2("setInt", "(Lorg/cacaojvm/compiler2/test/PutField;I)V", this, value);
		int compiler2Value = intField;

		assertEquals(baselineValue, compiler2Value);
	}

	public long longField;

	/** This is the method under test. */
	static void setLong(PutField obj, long value) {
		obj.longField = value;
	}

	@Test
	public void testLong() {
		long value = 0xDEADBEEF12345678L;

		runBaseline("setLong", "(Lorg/cacaojvm/compiler2/test/PutField;J)V", this, value);
		long baselineValue = longField;

		// Reset the field under test.
		intField = 0;

		runCompiler2("setLong", "(Lorg/cacaojvm/compiler2/test/PutField;J)V", this, value);
		long compiler2Value = longField;

		assertEquals(baselineValue, compiler2Value);
	}
}
