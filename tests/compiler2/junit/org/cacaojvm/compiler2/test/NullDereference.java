/** tests/compiler2/junit/NullDereference.java - NullDereference
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

public class NullDereference extends Compiler2TestBase {

	public static interface IFoo {

		void foo();
	}

	public static class Foo implements IFoo {

		public long field;

		public void foo() {}
	}

	// We do this so the class and field are resolved.
	private static Foo randomFoo;
	static {
		randomFoo = new Foo();
		randomFoo.field = 20l;
	}

	static void invokeVirtual() {
		Foo f = null;
		f.foo();
	}

	@Test(expected = NullPointerException.class)
	public void testInvokeVirtual() throws Throwable {
		runCompiler2("invokeVirtual", "()V");
	}

	static void invokeInterface() {
		IFoo f = null;
		f.foo();
	}

	@Test(expected = NullPointerException.class)
	public void testInvokeInterface() throws Throwable {
		runCompiler2("invokeInterface", "()V");
	}

	static long getField() {
		Foo f = null;
		return f.field;
	}

	@Test(expected = NullPointerException.class)
	public void testGetField() throws Throwable {
		runCompiler2("getField", "()J");
	}

	static void putField() {
		Foo f = null;
		f.field = 0l;
	}

	@Test(expected = NullPointerException.class)
	public void testPutField() throws Throwable {
		runCompiler2("putField", "()V");
	}

	static int arrayLength() {
		long a[] = null;
		return a.length;
	}

	@Test(expected = NullPointerException.class)
	public void testArrayLength() throws Throwable {
		runCompiler2("arrayLength", "()I");
	}

	static long arrayLoad() {
		long a[] = null;
		return a[1];
	}

	@Test(expected = NullPointerException.class)
	public void testArrayLoad() throws Throwable {
		runCompiler2("arrayLoad", "()J");
	}

	static void arrayStore() {
		long a[] = null;
		a[1] = 0l;
	}

	@Test(expected = NullPointerException.class)
	public void testArrayStore() throws Throwable {
		runCompiler2("arrayStore", "()V");
	}
}
