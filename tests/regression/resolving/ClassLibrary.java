/* regression/resolving/ClassLibrary.java - check current class library

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

import java.util.*;

public enum ClassLibrary {
	GNU_CLASSPATH,
	OPEN_JDK,
	UNKNOWN;

	static ClassLibrary getCurrent() {
		String vendor       = System.getProperty("java.vendor").intern();
		String runtime_name = System.getProperty("java.runtime.name").intern();
		String spec_version = System.getProperty("java.specification.version").intern();

		if (vendor == "GNU Classpath")
			return GNU_CLASSPATH;

		if (runtime_name == "IcedTea6 Runtime Environment")
			return OPEN_JDK;

		if (runtime_name == "IcedTea7 Runtime Environment")
			return OPEN_JDK;

		if (runtime_name == "OpenJDK Runtime Environment")
			return OPEN_JDK;

		System.out.println("WARNING: Unknown class library (" + vendor       + 
														  "/" + runtime_name + 
														  "/" + spec_version + 
														  ")");
		return UNKNOWN;
	}

	public static void main(String[] args) {
		System.out.println("Current class library: " + getCurrent());

		Enumeration<?> props = System.getProperties().propertyNames();

		while (props.hasMoreElements()) {
			String prop = String.valueOf(props.nextElement());
			String str  = prop;

			str = "'" + str + "'";
			while (str.length() < 40) str = str + " ";


			System.out.printf("%s : '%s'\n", str, System.getProperty(prop));
		}
	}
}

/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: java
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */