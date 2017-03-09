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

import org.junit.runner.Description;
import org.junit.runner.Result;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunListener;

/**
 * Wraps a {@link CacaoRunListener} into a {@link RunListener}.
 *
 * @see CacaoRunListener
 */
final class CacaoRunListenerWrapper extends RunListener {
	private Class<?> currentClass;
	private boolean error;
	private final CacaoRunListener listener;

	public CacaoRunListenerWrapper(CacaoRunListener listener) {
		this.listener = listener;
	}

	@Override
	public void testRunFinished(Result result) {
		listener.testRunFinished(result);
	}

	@Override
	public void testStarted(Description description) {
		testClassStartedIfNew(description);
		listener.testStarted(description);
		error = false;
	}

	@Override
	public void testFailure(Failure failure) {
		error = true;
		listener.testFailure(failure);
	}

	@Override
	public void testIgnored(Description description) {
		testClassStartedIfNew(description);
		listener.testStarted(description);
		listener.testIgnored(description);
	}

	@Override
	public void testFinished(Description description) {
		if (!error) {
			listener.testSuccess(description);
		}
	}

	private void testClassStartedIfNew(Description description) {
		Class<?> testClass = description.getTestClass();
		if (!testClass.equals(currentClass)) {
			listener.testClassStarted(description);
			currentClass = testClass;
		}
	}

}
