/** tests/compiler2/junit/InliningTests.java - InliningTests
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


public class InliningTests extends Compiler2TestBase {
	private static class TestObject {
		public long m_value;

		public TestObject (long value){
			m_value = value;
		}

		public long getValueVirtual(){
			return m_value;
		}

		public final long getValueSpecial(){
			return m_value;
		}
	}

    private long m_value = 10;

    private long const_val(){
        return 1;
    }

    private long field(){
        return m_value;
    }

    public long fieldVirtual(){
        return m_value;
    }

	/** This is the method under test. */
	static long singleStatic() {
		return Math.max(12, 43);
	}

	//@Test
	public void testStaticInliningWithSingleCalls() {
		testResultEqual("singleStatic", "()J");
	}

	/** This is the method under test. */
	static long mutipleStatic() {
		long a = Math.max(111l,112l);
		long b = Math.max(149l,15l);
		return Math.max(a,b);
	}

	//@Test
	public void testStaticInliningWithMultipleCalls() {
		testResultEqual("mutipleStatic", "()J");
	}

    static long inlinePrivateInvoke(InliningTests testObj){
        return testObj.const_val();
    }

	//@Test
	public void testinlinePrivateInvoke() {
		testResultEqual("inlinePrivateInvoke", "(Lorg/cacaojvm/compiler2/test/InliningTests;)J", this);
	}

	// //@Test NOTE: also fails without inlining pass 
	public void testinlinePrivateInvokeWithNull() {
		testResultEqual("inlinePrivateInvoke", "(Lorg/cacaojvm/compiler2/test/InliningTests;)J");
	}

    static long inlineFieldAccess(InliningTests testObj){
        return testObj.field();
    }

    static long inlineFinalGetterWithCtor(){
        return new TestObject(20).getValueSpecial();
    }

	//@Test // TODO inlining: scheduling edges are not set.
	public void testInlineFinalGetterWithCtor() {
		testResultEqual("inlineFinalGetterWithCtor", "()J");
	}

    static long inlineVirtualGetterWithCtor(){
        return new TestObject(20).getValueVirtual();
    }

	//@Test
	public void testInlineVirtualGetterWithCtor() {
		testResultEqual("inlineVirtualGetterWithCtor", "()J");
	}

    static long dontInlineRecursive(long n){
        if(n == 0) return 0;
        return n + dontInlineRecursive(n - 1);
    }

	//@Test
	public void testDontInlineRecursive() {
		testResultEqual("dontInlineRecursive", "(J)J", 5);
	}

    static long guardedExample(InliningTests obj){
		return obj.fieldVirtual();
    }

	@Test
	public void testGuardedExample() {
		testResultEqual("guardedExample", "(Lorg/cacaojvm/compiler2/test/InliningTests;)J", this);
	}
}
