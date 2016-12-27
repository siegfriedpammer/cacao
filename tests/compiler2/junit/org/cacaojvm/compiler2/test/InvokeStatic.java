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

}
