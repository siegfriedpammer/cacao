## tests/regression/assertion/Makefile.am
##
## Copyright (C) 1996-2005, 2006, 2007, 2008
## CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
##
## This file is part of CACAO.
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as
## published by the Free Software Foundation; either version 2, or (at
## your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
## 02110-1301, USA.


JAVA	  = $(top_builddir)/src/cacao/cacao

if WITH_CLASSPATH_GNU
JAVAFLAGS = -Xbootclasspath:$(top_builddir)/src/lib/classes/:$(CLASSPATH_CLASSES)
else
JAVAFLAGS = -Xbootclasspath:$(CLASSPATH_CLASSES)
endif

SOURCE_FILES = \
	$(srcdir)/testassertions.java \
	$(srcdir)/packagetest/testassertions.java

EXTRA_DIST = \
	$(SOURCE_FILES) \
	Test.sh \
	\
	enabled.output \
	disabled.output

CLEANFILES = \
	*.class \
	*.thisoutput \
	packagetest/*.class
	
ASSERTION_TESTS = \
	EA_TEST1 \
	EA_TEST2 \
	EA_PACKAGE_TEST1 \
	EA_PACKAGE_TEST2 \
	EA_PACKAGE_TEST3 \
	EA_PACKAGE_TEST4 \
	EA_PACKAGE_TEST5 \
	DA_PACKAGE_TEST1 \
	DA_PACKAGE_TEST2 \
	DA_PACKAGE_TEST3 \
	DA_PACKAGE_TEST4 \
	DA_PACKAGE_TEST5 \
	EA_CLASS_TEST1 \
	EA_CLASS_TEST2 \
	EA_CLASS_TEST3 \
	EA_CLASS_TEST4 \
	EA_CLASS_TEST5 \
	DA_CLASS_TEST1 \
	DA_CLASS_TEST2 \
	DA_CLASS_TEST3 \
	DA_CLASS_TEST4 \
	DA_CLASS_TEST5 \
	ESA_TEST1 \
	ESA_TEST2
	

check: build $(ASSERTION_TESTS)

build:
	@$(JAVAC) -d . $(SOURCE_FILES)

EA_TEST1:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -ea $(JAVAFLAGS)" testassertions eatest1 enabled $(srcdir)

EA_TEST2:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -enableassertions $(JAVAFLAGS)" testassertions eatest2 enabled $(srcdir)

EA_PACKAGE_TEST1:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -da -ea:packagetest... $(JAVAFLAGS)" packagetest.testassertions eapackagetest1 enabled $(srcdir)

EA_PACKAGE_TEST2:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -disableassertions -ea:packagetest... $(JAVAFLAGS)" packagetest.testassertions eapackagetest2 enabled $(srcdir)

EA_PACKAGE_TEST3:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -disableassertions -enableassertions:packagetest... $(JAVAFLAGS)" packagetest.testassertions eapackagetest3 enabled $(srcdir)

EA_PACKAGE_TEST4:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -enableassertions:packagetest... -disableassertions  $(JAVAFLAGS)" packagetest.testassertions eapackagetest4 enabled $(srcdir)

EA_PACKAGE_TEST5:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -da -enableassertions:packagetest... $(JAVAFLAGS)" packagetest.testassertions eapackagetest5 enabled $(srcdir)

EA_CLASS_TEST1:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -da -ea:testassertions $(JAVAFLAGS)" testassertions eaclasstest1 enabled $(srcdir)

EA_CLASS_TEST2:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -disableassertions -ea:testassertions $(JAVAFLAGS)" testassertions eaclasstest2 enabled $(srcdir)

EA_CLASS_TEST3:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -disableassertions -enableassertions:testassertions $(JAVAFLAGS)" testassertions eaclasstest3 enabled $(srcdir)

EA_CLASS_TEST4:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -enableassertions:testassertions -disableassertions  $(JAVAFLAGS)" testassertions eaclasstest4 enabled $(srcdir)

EA_CLASS_TEST5:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -da -enableassertions:testassertions $(JAVAFLAGS)" testassertions eaclasstest5 enabled $(srcdir)

DA_PACKAGE_TEST1:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -da:packagetest... -ea $(JAVAFLAGS)" packagetest.testassertions dapackagetest1 disabled $(srcdir)

DA_PACKAGE_TEST2:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -disableassertions:packagetest... -ea $(JAVAFLAGS)" packagetest.testassertions dapackagetest2 disabled $(srcdir)

DA_PACKAGE_TEST3:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -disableassertions:packagetest... -enableassertions $(JAVAFLAGS)" packagetest.testassertions dapackagetest3 disabled $(srcdir)

DA_PACKAGE_TEST4:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -enableassertions -disableassertions:packagetest...  $(JAVAFLAGS)" packagetest.testassertions dapackagetest4 disabled $(srcdir)

DA_PACKAGE_TEST5:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -da:packagetest... -enableassertions $(JAVAFLAGS)" packagetest.testassertions dapackagetest5 disabled $(srcdir)

DA_CLASS_TEST1:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -ea -da:testassertions $(JAVAFLAGS)" testassertions daclasstest1 disabled $(srcdir)

DA_CLASS_TEST2:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -enableassertions -da:testassertions $(JAVAFLAGS)" testassertions daclasstest2 disabled $(srcdir)

DA_CLASS_TEST3:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -enableassertions -disableassertions:testassertions $(JAVAFLAGS)" testassertions daclasstest3 disabled $(srcdir)

DA_CLASS_TEST4:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -disableassertions:testassertions -enableassertions  $(JAVAFLAGS)" testassertions daclasstest4 disabled $(srcdir)

DA_CLASS_TEST5:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -ea -disableassertions:testassertions $(JAVAFLAGS)" testassertions daclasstest5 disabled $(srcdir)
	
ESA_TEST1:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -esa $(JAVAFLAGS)" testassertions esatest1 disabled $(srcdir)

ESA_TEST2:
	@LD_LIBRARY_PATH=$(top_builddir)/src/cacao/.libs $(SHELL) $(srcdir)/Test.sh "$(JAVA) -enablesystemassertions $(JAVAFLAGS)" testassertions esatest2 disabled $(srcdir)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
