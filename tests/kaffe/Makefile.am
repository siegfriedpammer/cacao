TESTS_ENVIRONMENT = $(SHELL) TestScript

ALWAYS_TESTS = \
## Simple health check
	HelloWorldApp.class.save \
## Now test the compiler
	HelloWorldApp.java \
## Test ints, longs, floats and doubles
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
## Test strings
	Str.java \
	Str2.java \
	InternHog.java \
## Test exceptions
	IndexTest.java \
	StackDump.java \
## Test threads
	tname.java \
	ttest.java \
	ThreadInterrupt.java \
	ThreadState.java \
	UncaughtException.java \
## Misc tests
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
## simple test for Method.invoke
	ReflectInvoke.java \
## simple test for InvocationTargetExceptions
	InvTarExcTest.java \
## tests for ClassLoader
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
## Test various ways of stopping threads
	ThreadStop.java \
	DeadThread.java \
	tthrd1.java \
## Test interrupting I/O
## This is kind of tricky, and Sun doesn't support it.
	SoInterrupt.java \
	sysdepCallMethod.java \
## This one is skipped unless you --enable-debug.
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
