/** tests/compiler2/junit/Throw.java - Throw
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

import java.lang.reflect.InvocationTargetException;

import static org.hamcrest.core.IsInstanceOf.instanceOf;
import static org.hamcrest.MatcherAssert.assertThat;

import org.junit.Test;
import org.junit.Ignore;

public class Throw extends Compiler2TestBase {

	public static class FooException extends Exception {
	}

	static void throwException(FooException e) throws FooException {
		throw e;
	}

	@Test(expected = FooException.class)
	public void testThrowException() throws Throwable {
		runCompiler2("throwException", "(Lorg/cacaojvm/compiler2/test/Throw$FooException;)V", new FooException());
	}

	static void throwNewException() throws FooException {
		throw new FooException();
	}

	@Test(expected = FooException.class)
	public void testThrowNewException() throws Throwable {
		runCompiler2("throwException", "(Lorg/cacaojvm/compiler2/test/Throw$FooException;)V", new FooException());
	}
}
