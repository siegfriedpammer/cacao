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
	public void testByte() throws Throwable {
		byte value = 111;

		runBaseline("setByte", "(B)V", value);
		byte baselineValue = byteField;

		// Reset the field under test.
		byteField = 0;

		runCompiler2("setByte", "(B)V", value);
		byte compiler2Value = byteField;

		assertEquals(baselineValue, compiler2Value);
	}

	/** This is the method under test. */
	static void setByteConstant() {
		byteField = 111;
	}

	@Test
	public void testByteConstant() throws Throwable {
		runBaseline("setByteConstant", "()V");
		byte baselineValue = byteField;

		// Reset the field under test.
		byteField = 0;

		runCompiler2("setByteConstant", "()V");
		byte compiler2Value = byteField;

		assertEquals(baselineValue, compiler2Value);
	}

	static short shortField;

	/** This is the method under test. */
	static void setShort(short value) {
		shortField = value;
	}

	@Test
	public void testShort() throws Throwable {
		short value = 11111;

		runBaseline("setShort", "(S)V", value);
		short baselineValue = shortField;

		// Reset the field under test.
		shortField = 0;

		runCompiler2("setShort", "(S)V", value);
		short compiler2Value = shortField;

		assertEquals(baselineValue, compiler2Value);
	}

	/** This is the method under test. */
	static void setShortConstant() {
		shortField = 11111;
	}

	@Test
	public void testShortConstant() throws Throwable {
		runBaseline("setShortConstant", "()V");
		short baselineValue = shortField;

		// Reset the field under test.
		shortField = 0;

		runCompiler2("setShortConstant", "()V");
		short compiler2Value = shortField;

		assertEquals(baselineValue, compiler2Value);
	}

	static int intField;

	/** This is the method under test. */
	static void setInt(int value) {
		intField = value;
	}

	@Test
	public void testInt() throws Throwable {
		int value = 0xDEADBEEF;

		runBaseline("setInt", "(I)V", value);
		int baselineValue = intField;

		// Reset the field under test.
		intField = 0;

		runCompiler2("setInt", "(I)V", value);
		int compiler2Value = intField;

		assertEquals(baselineValue, compiler2Value);
	}

	/** This is the method under test. */
	static void setIntConstant() {
		intField = 0xDEADBEEF;
	}

	@Test
	public void testIntConstant() throws Throwable {
		runBaseline("setIntConstant", "()V");
		int baselineValue = intField;

		// Reset the field under test.
		intField = 0;

		runCompiler2("setIntConstant", "()V");
		int compiler2Value = intField;

		assertEquals(baselineValue, compiler2Value);
	}

	static long longField;

	/** This is the method under test. */
	static void setLong(long value) {
		longField = value;
	}

	@Test
	public void testLong() throws Throwable {
		long value = 0xDEADBEEF12345678l;

		runBaseline("setLong", "(J)V", value);
		long baselineValue = longField;

		// Reset the field under test.
		longField = 0l;

		runCompiler2("setLong", "(J)V", value);
		long compiler2Value = longField;

		assertEquals(baselineValue, compiler2Value);
	}

	/** This is the method under test. */
	static void setLongConstant() {
		longField = 0xDEADBEEF12345678l;
	}

	@Test
	public void testLongConstant() throws Throwable {
		runBaseline("setLongConstant", "()V");
		long baselineValue = longField;

		// Reset the field under test.
		longField = 0l;

		runCompiler2("setLongConstant", "()V");
		long compiler2Value = longField;

		assertEquals(baselineValue, compiler2Value);
	}

	static float floatField;

	/** This is the method under test. */
	static void setFloat(float value) {
		floatField = value;
	}

	@Test
	public void testFloat() throws Throwable {
		float value = 3.40282347e38f;

		runBaseline("setFloat", "(F)V", value);
		float baselineValue = floatField;

		// Reset the field under test.
		floatField = 0.0f;

		runCompiler2("setFloat", "(F)V", value);
		float compiler2Value = floatField;

		assertEquals(baselineValue, compiler2Value, 0.0f);
	}

	/** This is the method under test. */
	static void setFloatConstant() {
		floatField = 3.40282347e38f;
	}

	@Test
	public void testFloatConstant() throws Throwable {
		runBaseline("setFloatConstant", "()V");
		float baselineValue = floatField;

		// Reset the field under test.
		floatField = 0.0f;

		runCompiler2("setFloatConstant", "()V");
		float compiler2Value = floatField;

		assertEquals(baselineValue, compiler2Value, 0.0f);
	}

	static double doubleField;

	/** This is the method under test. */
	static void setDouble(double value) {
		doubleField = value;
	}

	@Test
	public void testDouble() throws Throwable {
		double value = 1.79769313e308d;

		runBaseline("setDouble", "(D)V", value);
		double baselineValue = doubleField;

		// Reset the field under test.
		doubleField = 0.0d;

		runCompiler2("setDouble", "(D)V", value);
		double compiler2Value = doubleField;

		assertEquals(baselineValue, compiler2Value, 0.0d);
	}

	/** This is the method under test. */
	static void setDoubleConstant() {
		doubleField = 1.79769313e308d;
	}

	@Test
	public void testDoubleConstant() throws Throwable {
		runBaseline("setDoubleConstant", "()V");
		double baselineValue = doubleField;

		// Reset the field under test.
		doubleField = 0.0d;

		runCompiler2("setDoubleConstant", "()V");
		double compiler2Value = doubleField;

		assertEquals(baselineValue, compiler2Value, 0.0d);
	}
}
