/* tests/regression/junit/All.java - runs all CACAO regression unit tests

   Copyright (C) 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


import junit.framework.*;
import junit.textui.*;

public class All extends TestCase {
    /**
     * Runs all CACAO regression unit tests using
     * junit.textui.TestRunner
     */
    public static void main(String[] args) {
        Test s = suite();
        TestRunner.run(s);
    }

    /**
     * Collects all CACAO regression unit tests as one suite.
     */
    public static Test suite() {
        TestSuite suite = new TestSuite("CACAO Regression Unit Tests");

        // Add your test here.

        suite.addTest(new TestSuite(TestPatcher.class));
        suite.addTest(new TestSuite(TestExceptionInStaticClassInitializer.class));

        return suite;
    }
}
