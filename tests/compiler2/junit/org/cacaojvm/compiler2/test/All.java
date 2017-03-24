/** tests/compiler2/junit/All.java - runs all CACAO compiler2 unit tests
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

import org.junit.runner.RunWith;
import org.junit.runners.Suite;

@RunWith(Suite.class)
@Suite.SuiteClasses({
	// @formatter:off
	Fact.class,
	Sqrt.class,
	Power.class,
	Min.class,
	Pi.class,
	PiSpigot.class,
	Overflow.class,
	ParameterLong.class,
	ParameterDouble.class,
	Ineg.class,
	Lneg.class,
	Fneg.class,
	Dneg.class,
	Fcmp.class,
	Dcmp.class,
	BoyerMoore.class,
	ArrayLength.class,
	Permut.class,
	ArrayLoad.class,
	ArrayStore.class,
	ArrayLoadFloat.class,
	DoubleArrayStore.class,
	MatAdd.class,
	MatMult.class,
	GetStatic.class,
	Array2dimLoad.class,
	Array2dimStore.class,
	LookupSwitch.class,
	
	D2f.class,
	Dadd.class,
	Dsub.class,
	Dmul.class,
	Ddiv.class,
	
	F2d.class,
	Fadd.class,
	Fsub.class,
	Fmul.class,
	Fdiv.class,
	
	// I2b.class,
	// I2c.class,
	// I2s.class,
	// I2f.class,
	// I2d.class,
	// Idiv.class,
	// Irem.class,

	InvokeStatic.class,
	InvokeSpecial.class,
	InvokeVirtual.class,

	MatcherBenchmarks.class

	// SampleTest.class
	// @formatter:on
})
public class All {
}
