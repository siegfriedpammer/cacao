## tests/kaffe/Makefile.am
##
## Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
## R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
## C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
## TU Wien
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
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
## 02111-1307, USA.
##
## Contact: cacao@complang.tuwien.ac.at
##
## Authors: Christian Thalinger
##
## Changes:
##
## $Id: Makefile.am 1676 2004-12-03 16:52:15Z twisti $

## Process this file with automake to produce Makefile.in

TESTS_ENVIRONMENT = $(SHELL) TestScript

ALWAYS_TESTS = HelloWorldApp.class.save \
	HelloWorldApp.java \
	TestIntLong.java \
	TestFloatDouble.java \
	DoubleCvt.java \
	DoubleNeg.java \
	DoubleConst.java \
	DoublePrint.java \
	DoubleComp.java \
	ModuloTest.java \
	LongNeg.java \
	FPUStack.java \
	divtest.java \
	Str.java \
	Str2.java \
	InternHog.java \
	IndexTest.java \
	StackDump.java \
	tname.java \
	ttest.java \
	ThreadState.java \
	UncaughtException.java \
	ReflectInterfaces.java \
	InnerTest.java \
	SerialUID.java \
	TestSerializable.java \
	TestSerializable2.java \
	TestCasts.java \
	Alias.java \
	NullPointerTest.java \
	NullInvoke.java \
	TableSwitch.java \
	LostFrame.java \
	ConstructorTest.java \
	burford.java \
	IllegalInterface.java \
	GetInterfaces.java \
	IntfTest.java \
	SignedShort.java \
	CharCvt.java \
	BadFloatTest.java \
	ProcessTest.java \
	UDPTest.java \
	SoTimeout.java \
	wc.java \
	FileTest.java \
	FileChecks.java \
	finalexc.java \
	finaltest.java \
	finaltest2.java \
	forNameTest.java \
	LoaderTest.java \
	ArrayForName.java \
	KaffeVerifyBug.java \
	Schtum.java \
	Reflect.java \
	MethodBug.java \
	Bean.java \
	SortTest.java \
	HashTest.java \
	MapTest.java \
	URLTest.java \
	PropertiesTest.java \
	ReaderTest.java \
	CharArrayReaderTest.java \
	LineNumberReaderTest.java \
	BufferedReaderTest.java \
	ReaderReadVoidTest.java \
	PipeTest.java \
	GetField.java \
	ReflectInvoke.java \
	InvTarExcTest.java \
	SystemLoaderTest.java \
	CLTest.java \
	CLTestConc.java \
	CLTestJLock.java \
	CLTestLie.java \
	CLTestFindLoaded.java \
	ExceptionTest.java \
	ExceptionTestClassLoader.java \
	ExceptionTestClassLoader2.java \
	TestClassRef.java \
	ClassGC.java \
	GCTest.java \
	ThreadLocalTest.java \
	BeanBug.java \
	BitSetTest.java \
	ExceptionInInitializerTest.java \
	InitTest.java \
	ProcessClassTest.java \
	ProcessClassInst.java \
	ProcessClassStop.java \
	FindSystemClass.java \
	ProcessClassLockTest.java \
	DoubleBug.java \
	TestUnlock.java \
	MarkResetTest.java \
	HashtableTest1.java \
	ReflectMultiArray.java \
	ExecTest.java \
	ThreadStop.java \
	DeadThread.java \
	tthrd1.java \
	SoInterrupt.java \
	sysdepCallMethod.java \
	TestNative.java

XFAIL_TESTS =

TESTS = $(ALWAYS_TESTS)

EXTRA_DIST = TestScript $(ALWAYS_TESTS) \
	CLTestConc_run.java \
	ZipVerifyUtils.java \
	ClassGCTest.java \
	ClassGCTestLater.java \
	LoaderTest_Class0.java \
	LoaderTest_Class1.java \
	LoaderTest_Class1BeanInfo.java \
	LoaderTest_Class2.java \
	LoaderTest_Class3.java \
	LoaderTest_Loader1.java \
	LoaderTest_Loader2.java

CLEANFILES = *.class *.fail *.out core *.core ExceptionTest ziptest1.zip \
	ziptest2.zip frozen_serial *.tmp


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
