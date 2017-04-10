/** tests/compiler2/junit/SampleTest.java - SampleTest
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

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

@RunWith(Parameterized.class)
public class Ishl extends Compiler2TestBase {

	private int x;
	private int y;

	public Ishl(int x, int y) {
		this.x = x;
		this.y = y;
	}

	/**
	 * This is the method under test.
	 */
	static int ishl(int x, int y) {
		return x << y;
	}

	@Parameters
	public static Collection<Integer[]> data() {
		List<Integer[]> list = new ArrayList<Integer[]>();
		list.add(new Integer[] {  10,  5 });
		list.add(new Integer[] {  11,  5 });
		list.add(new Integer[] {   5,  5 });
		list.add(new Integer[] {   3,  5 });
		list.add(new Integer[] { -10,  5 });
		list.add(new Integer[] {  10, -5 });
		list.add(new Integer[] { -10, -5 });
		list.add(new Integer[] {   5,  0 });
		return list;
	}

	@Test
	public void testIshl() {
		testResultEqual("ishl", "(II)I", x, y);
	}
}
