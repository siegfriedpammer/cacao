/** tests/compiler2/junit/o/c/c/t/VerboseTextListener.java - Verbose JUnit Listener
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

import java.io.PrintStream;
import java.text.NumberFormat;
import java.util.List;

import org.junit.internal.JUnitSystem;
import org.junit.runner.Description;
import org.junit.runner.Result;
import org.junit.runner.notification.Failure;

public class VerboseTextListener implements CacaoRunListener {
	private static final String INDENT = "  ";
	private final PrintStream fWriter;

	public VerboseTextListener(JUnitSystem system) {
		this(system.out());
	}

	public VerboseTextListener(PrintStream writer) {
		this.fWriter = writer;
	}

	public void testRunFinished(Result result) {
		printHeader(result.getRunTime());
		printFailures(result);
		printFooter(result);
	}

	public void testSuccess(Description description) {
		printResult("Finished");
	}

	public void testIgnored(Description description) {
		printResult("Ignored");
	}

	public void testFailure(Failure failure) {
		printResult("Error");
	}

	public void testClassStarted(Description description) {
		getWriter().append("Test ").append(description.getClassName())
				.println(":");
	}

	public void testStarted(Description description) {
		getWriter().append(INDENT).append(description.getMethodName())
				.append(": ");
	}

	protected void printResult(String result) {
		getWriter().println(result);
	}

	/*
	 * Internal methods
	 */

	private PrintStream getWriter() {
		return fWriter;
	}

	protected void printHeader(long runTime) {
		getWriter().println();
		getWriter().println("Time: " + elapsedTimeAsString(runTime));
	}

	protected void printFailures(Result result) {
		List<Failure> failures = result.getFailures();
		if (failures.size() == 0) {
			return;
		}
		if (failures.size() == 1) {
			getWriter().println("There was " + failures.size() + " failure:");
		} else {
			getWriter().println("There were " + failures.size() + " failures:");
		}
		int i = 1;
		for (Failure each : failures) {
			printFailure(each, "" + i++);
		}
	}

	protected void printFailure(Failure each, String prefix) {
		getWriter().println(prefix + ") " + each.getTestHeader());
		getWriter().print(each.getTrace());
	}

	protected void printFooter(Result result) {
		if (result.wasSuccessful()) {
			getWriter().println();
			getWriter().print("OK");
			getWriter().println(
					" (" + result.getRunCount() + " test"
							+ (result.getRunCount() == 1 ? "" : "s") + ")");

		} else {
			getWriter().println();
			getWriter().println("FAILURES!!!");
			getWriter().println(
					"Tests run: " + result.getRunCount() + ",  Failures: "
							+ result.getFailureCount());
		}
		getWriter().println();
	}

	/**
	 * Returns the formatted string of the elapsed time. Duplicated from
	 * BaseTestRunner. Fix it.
	 */
	protected String elapsedTimeAsString(long runTime) {
		return NumberFormat.getInstance().format((double) runTime / 1000);
	}
}
