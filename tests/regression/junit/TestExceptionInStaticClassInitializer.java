/* tests/regression/bugzilla/TestExceptionInStaticClassInitializer.java

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

public class TestExceptionInStaticClassInitializer extends TestCase {
    public static void main(String[] args) {
        TestRunner.run(suite());
    }

    public static Test suite() {
        return new TestSuite(TestExceptionInStaticClassInitializer.class);
    }

    public void test() {
        try {
            TestExceptionInStaticClassInitializer_x.i = 1;
            fail("Should throw ExceptionInInitializerError");
        }
        catch (ExceptionInInitializerError success) {
            Throwable cause = success.getCause();

            assertTrue("Cause should be RuntimeException but is " + cause.getClass(), cause.getClass() == RuntimeException.class);

            StackTraceElement[] ste = cause.getStackTrace();

            assertTrue("Linenumber should be " + LINE + " but is " + ste[0].getLineNumber(), ste[0].getLineNumber() == LINE);
        }
    }

    // This linenumber must be the one from...
    final static int LINE = 64;
}

class TestExceptionInStaticClassInitializer_x {
    static int i;

    static {
        if (true)
            // ...the following line.
            throw new RuntimeException();
    }
}