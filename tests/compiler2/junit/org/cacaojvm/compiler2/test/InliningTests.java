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
	private static interface TestInterface {
		public long getValueInterface();
	}

	private static class TestObject implements TestInterface {
		public long m_value;

		public TestObject(){
			this(0);
		}

		public TestObject (long value){
			m_value = value;
		}

		public long getValueVirtual(){
			return m_value;
		}

		public final long getValueSpecial(){
			return m_value;
		}
		
		public long getValueInterface(){
			return m_value;
		}
	}

	private static class TestObjectDerived extends TestObject {
		public long getValueVirtual(){
			return 123;
		}
	}

	private static abstract class AbstractTestObject{
		public abstract long getValue();
	}

	private static class DerivedFromAbstractTestObject extends AbstractTestObject {
		public long getValue(){
			return 1234;
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
		// TODO inlining: Assert with statistics
	}

	/** This is the method under test. */
	static long multipleStatic() {
		long a = Math.max(142l, 9l);
		long b = Math.max(a, 43l);
		long c = Math.max(b, 43l);
		long d = Math.max(c, 1000l);
		long e = Math.max(d, 823l);
		return e;
	}

	//@Test
	public void testStaticInliningWithMultipleCalls() {
		testResultEqual("multipleStatic", "()J");
		// TODO inlining: Assert with statistics
	}

	static long smallerOrTen(int a) {
		long ten = 10;
		return a > 0 ? Math.max(a, ten) : a;
	}

	//@Test
	public void testStaticInliningInControlFlow() {
		testResultEqual("smallerOrTen", "(I)J", 11);
		// TODO inlining: Assert with statistics
	}

    static long inlinePrivateInvoke(InliningTests testObj){
        return testObj.const_val();
    }

	//@Test
	public void testinlinePrivateInvoke() {
		testResultEqual("inlinePrivateInvoke", "(Lorg/cacaojvm/compiler2/test/InliningTests;)J", this);
		// TODO inlining: Assert with statistics
	}

	////@Test NOTE: also fails without inlining pass 
	public void testinlinePrivateInvokeWithNull() {
		testResultEqual("inlinePrivateInvoke", "(Lorg/cacaojvm/compiler2/test/InliningTests;)J");
		// TODO inlining: Assert with statistics
	}

    static long inlineFieldAccess(InliningTests testObj){
        return testObj.field();
    }

    static long inlineFinalGetterWithCtor(){
        return new TestObject(20).getValueSpecial();
    }

	//@Test
	public void testInlineFinalGetterWithCtor() {
		testResultEqual("inlineFinalGetterWithCtor", "()J");
		// TODO inlining: Assert with statistics
	}

    static long inlineVirtualGetter(TestObject obj){
        return obj.getValueVirtual();
    }

	@Test
	public void testInlineVirtualGetter() {
		testResultEqual("inlineVirtualGetter", "(Lorg/cacaojvm/compiler2/test/InliningTests$TestObject;)J", new TestObject(20));
		// TODO inlining: Assert with statistics
	}

    static long inlineDerivedVirtualGetter(TestObject obj){
        return obj.getValueVirtual();
    }

	//@Test
	public void testInlineDerivedVirtualGetter() {
		testResultEqual("inlineDerivedVirtualGetter", "(Lorg/cacaojvm/compiler2/test/InliningTests$TestObject;)J", new TestObjectDerived());
		// TODO inlining: Assert with statistics
	}

    static long inlineInterfaceGetterWithCtor(TestInterface obj){
        return obj.getValueInterface();
    }

	//@Test
	public void testInlineDerivedAbstractGetter() {
		testResultEqual("inlineDerivedAbstractGetter", "(Lorg/cacaojvm/compiler2/test/InliningTests$AbstractTestObject;)J", new DerivedFromAbstractTestObject());
		// TODO inlining: Assert with statistics
	}

    static long inlineDerivedAbstractGetter(AbstractTestObject obj){
        return obj.getValue();
    }

	//@Test
	public void testInlineInterfaceGetterWithCtor() {
		testResultEqual("inlineInterfaceGetterWithCtor", "(Lorg/cacaojvm/compiler2/test/InliningTests$TestInterface;)J", new TestObject(20));
		// TODO inlining: Assert with statistics
	}

    static long dontInlineRecursive(long n){
        if(n == 0) return 0;
        return n + dontInlineRecursive(n - 1);
    }

	//@Test
	public void testDontInlineRecursive() {
		testResultEqual("dontInlineRecursive", "(J)J", 5);
		// TODO inlining: Assert with statistics
	}
}
