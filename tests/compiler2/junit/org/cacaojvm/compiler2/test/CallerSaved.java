/** tests/compiler2/junit/CallerSaved.java - CallerSaved
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

public class CallerSaved extends Compiler2TestBase {

	static long add(long a, long b) {
		// Use up all caller-saved registers
		long c = 5;
		long d = 6;
		long e = 7;
		long f = 8;
		long g = 9;
		long h = 10;
		long i = 11;
		long j = 12;
		long k = 13;
		return a + b + c + d + e + f + g + h + i + j + k;
	}

	/** This is the method under test. */
	static long invokeStaticLong(long value) {
		add(10, 20);
		
		return value + 5;
	}

	// @Test
	public void testInvokeStaticLong() {
		testResultEqual("invokeStaticLong", "(J)J", 623);
	}

	static class Foo {
		private long value = 0;

		public void increment() {
			value++;
		}

		public long get() {
			return value;
		}

		public void add(long val) {
			value += val;
		}
	}

	static long invokeObject() {
		Foo foo = new Foo();

		foo.increment();

		long a = 20 + foo.get();
		foo.add(a);
		foo.increment();

		return foo.get();
	}

	@Test
	public void testMultipleInvokeVirtual() {
		testResultEqual("invokeObject", "()J");
	}
}
