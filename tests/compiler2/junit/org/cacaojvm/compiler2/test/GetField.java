/** tests/compiler2/junit/GetField.java - GetField
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

public class GetField extends Compiler2TestBase {

	public byte byteField;

	/** This is the method under test. */
	static byte getByte(GetField obj) {
		return obj.byteField;
	}

	@Test
	public void testByte() {
		byteField = 111;
		testResultEqual("getByte", "(Lorg/cacaojvm/compiler2/test/GetField;)B", this);
	}

	public short shortField;

	/** This is the method under test. */
	static short getShort(GetField obj) {
		return obj.shortField;
	}

	@Test
	public void testShort() {
		shortField = 11111;
		testResultEqual("getShort", "(Lorg/cacaojvm/compiler2/test/GetField;)S", this);
	}

	public int intField;

	/** This is the method under test. */
	static int getInt(GetField obj) {
		return obj.intField;
	}

	@Test
	public void testInt() {
		intField = 0xDEADBEEF;
		testResultEqual("getInt", "(Lorg/cacaojvm/compiler2/test/GetField;)I", this);
	}

	public long longField;

	/** This is the method under test. */
	static long getLong(GetField obj) {
		return obj.longField;
	}

	@Test
	public void testLong() {
		longField = 0xDEADBEEF12345678L;
		testResultEqual("getLong", "(Lorg/cacaojvm/compiler2/test/GetField;)J", this);
	}

	public float floatField;

	/** This is the method under test. */
	static float getFloat(GetField obj) {
		return obj.floatField;
	}

	@Test
	public void testFloat() {
		floatField = 3.40282347e38F;
		testResultEqual("getFloat", "(Lorg/cacaojvm/compiler2/test/GetField;)F", this);
	}

	public double doubleField;

	/** This is the method under test. */
	static double getDouble(GetField obj) {
		return obj.doubleField;
	}

	@Test
	public void testDouble() {
		doubleField = 1.79769313e308D;
		testResultEqual("getDouble", "(Lorg/cacaojvm/compiler2/test/GetField;)D", this);
	}
}
