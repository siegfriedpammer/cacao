/** tests/compiler2/junit/InvokeVirtual.java - InvokeVirtual
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
import org.junit.Ignore;

public class InvokeVirtual extends Compiler2TestBase {

	public int foo() {
		return 1;
	}

	/** This is the method under test. */
	static int invokeVirtualIntNoParams(InvokeVirtual t) {
		return t.foo();
	}

	@Test
	public void testInvokeVirtualIntNoParams() {
		testResultEqual("invokeVirtualIntNoParams", "(Lorg/cacaojvm/compiler2/test/InvokeVirtual;)I", this);
	}

	public int foo(int a, int b) {
		return a + b;
	}

	/** This is the method under test. */
	static int invokeVirtualInt(InvokeVirtual t, int a, int b) {
		return t.foo(a, b);
	}

	@Test
	public void testInvokeVirtualInt() {
		testResultEqual("invokeVirtualInt", "(Lorg/cacaojvm/compiler2/test/InvokeVirtual;II)I", this, 1, 2);
		testResultEqual("invokeVirtualInt", "(Lorg/cacaojvm/compiler2/test/InvokeVirtual;II)I", this, 2, 3);
	}

	public int foo(int a, int b, int c, int d, int e, int f, int g, int h) {
		return a + b + c + d + e + f + g + h;
	}

	/** This is the method under test. */
	static int invokeVirtualIntManyParams(InvokeVirtual t, int a, int b, int c, int d, int e, int f, int g, int h) {
		return t.foo(a, b, c, d, e, f, g, h);
	}

	@Ignore("Stack arguments not yet handled correctly by linear scan register allocator.")
	@Test
	public void testInvokeVirtualIntManyParams() {
		testResultEqual("invokeVirtualIntManyParams", "(Lorg/cacaojvm/compiler2/test/InvokeVirtual;IIIIIIII)I", this, 1, 2, 3, 4, 5, 6, 7, 8);
		testResultEqual("invokeVirtualIntManyParams", "(Lorg/cacaojvm/compiler2/test/InvokeVirtual;IIIIIIII)I", this, 2, 3, 4, 5, 6, 7, 8, 9);
	}
}
