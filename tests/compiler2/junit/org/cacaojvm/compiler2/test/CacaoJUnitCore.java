/** tests/compiler2/junit/o/c/c/t/CacaoJUnitCore.java - CACAO JUnit Core
 *
 * Copyright (C) 1996-2015
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
import java.util.List;

import junit.runner.Version;

import org.junit.internal.JUnitSystem;
import org.junit.internal.RealSystem;
import org.junit.runner.Description;
import org.junit.runner.JUnitCore;
import org.junit.runner.Result;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunListener;

public class CacaoJUnitCore {
	/**
	 * Run the tests contained in the classes named in the <code>args</code>. If
	 * all tests run successfully, exit with a status of 0. Otherwise exit with
	 * a status of 1. Write feedback while tests are running and write stack
	 * traces for all failed tests after the tests all complete.
	 *
	 * @param args
	 *            names of classes in which to find tests to run
	 * @see JUnitCore
	 */
	public static void main(String... args) {
		new CacaoJUnitCore().runMain(args);
	}

	/**
	 * @param args
	 *            from main()
	 */
	private void runMain(String... args) {
		JUnitSystem system = new RealSystem();
		JUnitCore junitCore = new JUnitCore();
		system.out().println("CacaoJUnitCore");
		system.out().println("JUnit version " + Version.id());
		List<Class<?>> classes = new ArrayList<Class<?>>();
		List<Failure> missingClasses = new ArrayList<Failure>();
		for (String each : args) {
			try {
				classes.add(Class.forName(each));
			} catch (ClassNotFoundException e) {
				system.out().println("Could not find class: " + each);
				Description description = Description
						.createSuiteDescription(each);
				Failure failure = new Failure(description, e);
				missingClasses.add(failure);
			}
		}
		RunListener listener = new CacaoRunListenerWrapper(new VerboseTextListener(system));
		junitCore.addListener(listener);
		Result result = junitCore.run(classes.toArray(new Class[0]));
		for (Failure each : missingClasses) {
			result.getFailures().add(each);
		}
		System.exit(result.wasSuccessful() ? 0 : 1);
    }
}
