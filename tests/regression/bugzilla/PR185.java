/* tests/regression/bugzilla/PR185.java

   Copyright (C) 1996-2013
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

import org.junit.Test;
import static org.junit.Assert.*;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;

public class PR185 {
    static class Runner {
        static class StringReader implements Runnable {
            public StringReader(String prefix, InputStream ins) {
                this.prefix = prefix;
                this.ins = ins;
            }
            InputStream ins;
            String prefix;
            public String str = null;
            public void run() {
                try {
                    BufferedReader r = new BufferedReader(new InputStreamReader(ins));
                    String line;
                    StringBuilder sb = new StringBuilder();
                    while ((line = r.readLine()) != null) {
                        sb.append(prefix + line);
                        sb.append('\n');
                    }
                    str = sb.toString();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        void run() {
            String thecmd = System.getProperty("cacao.test.javacmd");
            try {
                Process p = Runtime.getRuntime().exec(thecmd + " finalizer_exceptions");
                StringReader rerr = new StringReader("err:", p.getErrorStream());
                StringReader rout = new StringReader("out:", p.getInputStream());
                Thread terr = new Thread(rerr);
                Thread tout = new Thread(rout);
                terr.start();
                tout.start();
                int rc = p.waitFor();
                terr.join();
                tout.join();
                assertEquals(rc, 0);
                assertEquals(rout.str, "out:Success!\n");
                assertTrue(rerr.str.isEmpty());
            } catch (Exception e) {
                fail(e.toString());
            }
        }
    }

    @Test
    public void test() {
        new Runner().run();
    }
}
