/* src/tests/native/checkjni.java - for testing JNI related stuff

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Christian Thalinger

   Changes:

   $Id: checkjni.java 2170 2005-03-31 19:18:38Z twisti $

*/


public class checkjni {
    public native boolean IsAssignableFrom(Class sub, Class sup);
    public native boolean IsInstanceOf(Object obj, Class clazz);

    public static void main(String[] argv) {
        System.loadLibrary("checkjni");

        new checkjni();
    }

    public checkjni() {
        checkIsAssignableFrom();
        checkIsInstanceOf();
    }

    void checkIsAssignableFrom() {
        p("IsAssignableFrom:");

        Class sub = Integer.class;
        Class sup = Object.class;

        equal(IsAssignableFrom(sup, sup), true);
        equal(IsAssignableFrom(sub, sup), true);
        equal(IsAssignableFrom(sup, sub), false);
    }

    void checkIsInstanceOf() {
        p("IsInstanceOf:");

        Object obj = new Object();
        Object obj2 = new Integer(1);
        Class clazz = Object.class;
        Class clazz2 = Integer.class;

        equal(IsInstanceOf(obj, clazz), true);
        equal(IsInstanceOf(obj2, clazz), true);
        equal(IsInstanceOf(obj, clazz2), false);
    }

    void equal(boolean a, boolean b) {
        if (a == b)
            p("PASS");
        else
            p("FAILED");
    }

    void p(String s) {
        System.out.println(s);
    }
}
