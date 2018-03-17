/** tests/compiler2/junit/InvokeStatic.java - InvokeStatic
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

public class InvokeStatic extends Compiler2TestBase {

	/** This is the method under test. */
	static int invokeStaticInt() {
		return Math.max(111,112);
	}

	@Test
	public void testInvokeStaticInt() {
		testResultEqual("invokeStaticInt", "()I");
	}

	/** This is the method under test. */
	static long invokeStaticLong() {
		return Math.max(111l,112l);
	}

	@Test
	public void testInvokeStaticLong() {
		testResultEqual("invokeStaticLong", "()J");
	}

	/** This is the method under test. */
	static float invokeStaticFloat() {
		return Math.max(111.4f,112.5f);
	}

	@Test
	public void testInvokeStaticFloat() {
		testResultEqual("invokeStaticFloat", "()F");
	}

	/** This is the method under test. */
	static double invokeStaticDouble() {
		return Math.max(111.4d,112.5d);
	}

	@Test
	public void testInvokeStaticDouble() {
		testResultEqual("invokeStaticDouble", "()D");
	}

	public static Object identity(Object o) {
		return o;
	}

	/** This is the method under test. */
	static Object invokeStaticObject(Object o) {
		return identity(o);
	}

	@Test
	public void testInvokeStaticObject() {
		testResultEqual("invokeStaticObject", "(Ljava/lang/Object;)Ljava/lang/Object;", "42");
	}

	/*
	 * The following tests are all similar and intended to test various configurations of stack
	 * parameters/alignments etc
	 */
	static boolean callStackArgument7(long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
		return a1 == 1 && a2 == 2 && a3 == 3 && a4 == 4 && a5 == 5 && a6 == 6 && a7 == 7;
	}

	static boolean invokeStaticStackArgument7(long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
		return callStackArgument7(a1,a2,a3,a4,a5,a6,a7);
	}

	@Test
	public void testInvokeStaticStackArgument7() {
		testResultEqual("invokeStaticStackArgument7", "(JJJJJJJ)Z", 1, 2, 3, 4, 5, 6, 7);
	}

	static boolean callStackArgument8(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8) {
		return a1 == 1 && a2 == 2 && a3 == 3 && a4 == 4 && a5 == 5 && a6 == 6 && a7 == 7 && a8 == 8;
	}

	static boolean invokeStaticStackArgument8(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8) {
		return callStackArgument8(a1,a2,a3,a4,a5,a6,a7,a8);
	}

	@Test
	public void testInvokeStaticStackArgument8() {
		testResultEqual("invokeStaticStackArgument8", "(JJJJJJJJ)Z", 1, 2, 3, 4, 5, 6, 7, 8);
	}

	static boolean callStackArgument9(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9) {
		return a1 == 1 && a2 == 2 && a3 == 3 && a4 == 4 && a5 == 5 && a6 == 6 && a7 == 7 && a8 == 8 && a9 == 9;
	}

	static boolean invokeStaticStackArgument9(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9) {
		return callStackArgument9(a1,a2,a3,a4,a5,a6,a7,a8,a9);
	}

	@Test
	public void testInvokeStaticStackArgument9() {
		testResultEqual("invokeStaticStackArgument9", "(JJJJJJJJJ)Z", 1, 2, 3, 4, 5, 6, 7, 8, 9);
	}

	static boolean callStackArgument10(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10) {
		return a1 == 1 && a2 == 2 && a3 == 3 && a4 == 4 && a5 == 5 && a6 == 6 && a7 == 7 && a8 == 8 && a9 == 9 && a10 == 10;
	}

	static boolean invokeStaticStackArgument10(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10) {
		return callStackArgument10(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10);
	}

	@Test
	public void testInvokeStaticStackArgument10() {
		testResultEqual("invokeStaticStackArgument10", "(JJJJJJJJJJ)Z", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
	}

	static boolean callStackArgument11(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11) {
		return a1 == 1 && a2 == 2 && a3 == 3 && a4 == 4 && a5 == 5 && a6 == 6 && a7 == 7 && a8 == 8 && a9 == 9 && a10 == 10 && a11 == 11;
	}

	static boolean invokeStaticStackArgument11(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11) {
		return callStackArgument11(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11);
	}

	@Test
	public void testInvokeStaticStackArgument11() {
		testResultEqual("invokeStaticStackArgument11", "(JJJJJJJJJJJ)Z", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
	}
}
