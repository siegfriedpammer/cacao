SUBDIRS = kaffe

EXTRA_DIST = \
	x.java x.output \
	jctest.java jctest.output \
	fp.java fp.output \
	fptest.java fptest.output \
	TestBase.java TestArrayClasses.java TestArrayClasses.output \
	BasicToStrings.java BasicToStrings.output \
	extest.java extest.output \
	GCBench.java

check_DATA = checkall

checkall: x.tst jctest.tst TestArrayClasses.tst BasicToStrings.tst fp.tst fptest.tst extest.tst GCBench.tstrun

%.tst:
	../cacao -classpath . -log /dev/null sun.tools.javac.Main $*.java
	../cacao -classpath . -log /dev/null $* >$*.thisoutput
	diff --brief $*.output $*.thisoutput
	rm -f $*.thisoutput

%.tstrun:
	../cacao -classpath . -log /dev/null sun.tools.javac.Main $*.java
	../cacao -classpath . -log /dev/null $*
