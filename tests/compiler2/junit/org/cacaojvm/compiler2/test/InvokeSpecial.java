/** tests/compiler2/junit/InvokeSpecial.java - InvokeSpecial
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

public class InvokeSpecial extends Compiler2TestBase {

	private final int foo() {
		return 1;
	}

	/** This is the method under test. */
	static int invokeSpecialNoArgs(InvokeSpecial t) {
		return t.foo();
	}

	@Test
	public void testInvokeSpecialNoArgs() {
		testResultEqual("invokeSpecialNoArgs", "(Lorg/cacaojvm/compiler2/test/InvokeSpecial;)I", this);
	}
}
