/** tests/compiler2/junit/GetInterface.java - GetInterface
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

public class InvokeInterface extends Compiler2TestBase {

	public static interface IFoo {

		int foo();

		int foo(int a, int b);
	}

	public static class FooImpl implements IFoo {

		public int foo() {
			return 1;
		}

		public int foo(int a, int b) {
			return a + b;
		}
	}

	/** This is the method under test. */
	static int invokeInterfaceIntNoParams(IFoo f) {
		return f.foo();
	}

	@Test
	public void testInvokeInterfaceIntNoParams() {
		testResultEqual("invokeInterfaceIntNoParams", "(Lorg/cacaojvm/compiler2/test/InvokeInterface$IFoo;)I", new FooImpl());
	}

	/** This is the method under test. */
	static int invokeInterfaceInt(IFoo f, int a, int b) {
		return f.foo(a, b);
	}

	@Test
	public void testInvokeInterfaceInt() {
		testResultEqual("invokeInterfaceInt", "(Lorg/cacaojvm/compiler2/test/InvokeInterface$IFoo;II)I", new FooImpl(), 1, 2);
		testResultEqual("invokeInterfaceInt", "(Lorg/cacaojvm/compiler2/test/InvokeInterface$IFoo;II)I", new FooImpl(), 2, 3);
	}
//
//	public int foo(int a, int b, int c, int d, int e, int f, int g, int h) {
//		return a + b + c + d + e + f + g + h;
//	}
//
//	/** This is the method under test. */
//	static int invokeInterfaceIntManyParams(InvokeInterface t, int a, int b, int c, int d, int e, int f, int g, int h) {
//		return t.foo(a, b, c, d, e, f, g, h);
//	}
//
//	@Ignore("Stack arguments not yet handled correctly by linear scan register allocator.")
//	@Test
//	public void testInvokeInterfaceIntManyParams() {
//		testResultEqual("invokeInterfaceIntManyParams", "(Lorg/cacaojvm/compiler2/test/InvokeInterface;IIIIIIII)I", this, 1, 2, 3, 4, 5, 6, 7, 8);
//		testResultEqual("invokeInterfaceIntManyParams", "(Lorg/cacaojvm/compiler2/test/InvokeInterface;IIIIIIII)I", this, 2, 3, 4, 5, 6, 7, 8, 9);
//	}
}
