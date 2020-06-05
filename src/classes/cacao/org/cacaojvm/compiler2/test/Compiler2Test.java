/** classes/cacao/org/cacaojvm/compiler2/test/Compiler2Test - cacao compiler2 test driver
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

class Compiler2Test {
	private static native void compileMethod(boolean baseline,
			Class<?> compileClass, String methodName, String methodDesc);
	
	protected static native Object executeMethod(Class<?> compileClass, 
			String methodName, String methodDesc, Object[] args) throws Throwable;

	/**
	 * Returns the statistics value for the specified statistics entry.
	 * If the the name of the entry is not unique the first found entry  will be returned.
	 *
	 * @param statisticsEntry name of the statistics entry
	 * @returns the current value of the statistics entry if the entry exists
	 * @throws UnsatisfiedLinkError when cacao wasn't compiled with ENABLE_STATISTICS.
	 *         If the statisticsEntry is null or not a existing name then an IllegalArgumentException is thrown.
	 */
	protected static native long getStatistics(String statisticsEntry);

	protected static void compileBaseline(Class<?> compileClass,
			String methodName, String methodDesc) {
		compileMethod(true, compileClass, methodName, methodDesc);
	}

	protected static void compileCompiler2(Class<?> compileClass,
			String methodName, String methodDesc) {
		compileMethod(false, compileClass, methodName, methodDesc);
	}

}