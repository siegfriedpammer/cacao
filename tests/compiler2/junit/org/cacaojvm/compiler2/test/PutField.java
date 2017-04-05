/** tests/compiler2/junit/PutField.java - PutField
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

import org.junit.Ignore;
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

	/** This is the method under test. */
	static void setByteConstant(PutField obj) {
		obj.byteField = 111;
	}

	@Test
	public void testByteConstant() {
		runBaseline("setByteConstant", "(Lorg/cacaojvm/compiler2/test/PutField;)V", this);
		byte baselineValue = byteField;

		// Reset the field under test.
		byteField = 0;

		runCompiler2("setByteConstant", "(Lorg/cacaojvm/compiler2/test/PutField;)V", this);
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

	/** This is the method under test. */
	static void setShortConstant(PutField obj) {
		obj.shortField = 11111;
	}

	@Test
	public void testShortConstant() {
		runBaseline("setShortConstant", "(Lorg/cacaojvm/compiler2/test/PutField;)V", this);
		short baselineValue = shortField;

		// Reset the field under test.
		shortField = 0;

		runCompiler2("setShortConstant", "(Lorg/cacaojvm/compiler2/test/PutField;)V", this);
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

	/** This is the method under test. */
	static void setIntConstant(PutField obj) {
		obj.intField = 0xDEADBEEF;
	}

	@Test
	public void testIntConstant() {
		runBaseline("setIntConstant", "(Lorg/cacaojvm/compiler2/test/PutField;)V", this);
		int baselineValue = intField;

		// Reset the field under test.
		intField = 0;

		runCompiler2("setIntConstant", "(Lorg/cacaojvm/compiler2/test/PutField;)V", this);
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
		long value = 0xDEADBEEF12345678l;

		runBaseline("setLong", "(Lorg/cacaojvm/compiler2/test/PutField;J)V", this, value);
		long baselineValue = longField;

		// Reset the field under test.
		longField = 0l;

		runCompiler2("setLong", "(Lorg/cacaojvm/compiler2/test/PutField;J)V", this, value);
		long compiler2Value = longField;

		assertEquals(baselineValue, compiler2Value);
	}

	/** This is the method under test. */
	static void setLongConstant(PutField obj) {
		obj.longField = 0xDEADBEEF12345678l;
	}

	@Test
	public void testLongConstant() {
		runBaseline("setLongConstant", "(Lorg/cacaojvm/compiler2/test/PutField;)V", this);
		long baselineValue = longField;

		// Reset the field under test.
		longField = 0l;

		runCompiler2("setLongConstant", "(Lorg/cacaojvm/compiler2/test/PutField;)V", this);
		long compiler2Value = longField;

		assertEquals(baselineValue, compiler2Value);
	}


	public float floatField;

	/** This is the method under test. */
	static void setFloat(PutField obj, float value) {
		obj.floatField = value;
	}

	@Test
	public void testFloat() {
		float value = 3.40282347e38f;

		runBaseline("setFloat", "(Lorg/cacaojvm/compiler2/test/PutField;F)V", this, value);
		float baselineValue = floatField;

		// Reset the field under test.
		floatField = 0.0f;

		runCompiler2("setFloat", "(Lorg/cacaojvm/compiler2/test/PutField;F)V", this, value);
		float compiler2Value = floatField;

		assertEquals(baselineValue, compiler2Value, 0.0f);
	}

	/** This is the method under test. */
	static void setFloatConstant(PutField obj) {
		obj.floatField = 3.40282347e38f;
	}

	@Test
	public void testFloatConstant() {
		runBaseline("setFloatConstant", "(Lorg/cacaojvm/compiler2/test/PutField;)V", this);
		float baselineValue = floatField;

		// Reset the field under test.
		floatField = 0.0f;

		runCompiler2("setFloatConstant", "(Lorg/cacaojvm/compiler2/test/PutField;)V", this);
		float compiler2Value = floatField;

		assertEquals(baselineValue, compiler2Value, 0.0f);
	}

	public double doubleField;

	/** This is the method under test. */
	static void setDouble(PutField obj, double value) {
		obj.doubleField = value;
	}

	@Test
	public void testDouble() {
		double value = 1.79769313e308d;

		runBaseline("setDouble", "(Lorg/cacaojvm/compiler2/test/PutField;D)V", this, value);
		double baselineValue = doubleField;

		// Reset the field under test.
		doubleField = 0.0d;

		runCompiler2("setDouble", "(Lorg/cacaojvm/compiler2/test/PutField;D)V", this, value);
		double compiler2Value = doubleField;

		assertEquals(baselineValue, compiler2Value, 0.0d);
	}

	/** This is the method under test. */
	static void setDoubleConstant(PutField obj) {
		obj.doubleField = 1.79769313e308d;
	}

	@Test
	public void testDoubleConstant() {
		runBaseline("setDoubleConstant", "(Lorg/cacaojvm/compiler2/test/PutField;)V", this);
		double baselineValue = doubleField;

		// Reset the field under test.
		doubleField = 0.0d;

		runCompiler2("setDoubleConstant", "(Lorg/cacaojvm/compiler2/test/PutField;)V", this);
		double compiler2Value = doubleField;

		assertEquals(baselineValue, compiler2Value, 0.0d);
	}
}
