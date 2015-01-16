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

import org.junit.Test;
import org.junit.runner.Description;
import org.junit.runner.Result;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunListener;

/**
 * This is a replacement for the {@link RunListener} provided by JUnit4.
 *
 * The reason for this to exist is that the methods from {@link RunListener} are
 * called in a weird way. For example {@link RunListener#testFinished} is called
 * whether or not there was an error or that {@link RunListener#testStarted} is
 * not called for {@linkplain RunListener#testIgnored ignored} tests.
 */
public interface CacaoRunListener {

	/**
	 * Called for every new test class (i.e. a class containing methods
	 * annotated with {@link Test}).
	 */
	void testClassStarted(Description description);

	/**
	 * Called for every test before it is executed or ignored.
	 */
	void testStarted(Description description);

	/**
	 * Called if the test succeeded.
	 */
	void testSuccess(Description description);

	/**
	 * Called if the test has been ignored.
	 */
	void testIgnored(Description description);

	/**
	 * Called if there was an error while executing the test.
	 */
	void testFailure(Failure failure);

	/**
	 * Called after all tests are finished.
	 */
	void testRunFinished(Result result);
}
