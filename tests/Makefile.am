SUBDIRS = kaffe

EXTRA_DIST = x.java x.output \
	jctest.java jctest.output \
	fp.java fp.output \
	fptest.java fptest.output \
	TestBase.java TestArrayClasses.java TestArrayClasses.output \
	BasicToStrings.java BasicToStrings.output \
	GCBench.java

check_DATA = checkall

checkall: x.tst jctest.tst TestArrayClasses.tst BasicToStrings.tst fp.tst fptest.tst GCBench.tstrun

%.tst:
	../cacao -log /dev/null sun.tools.javac.Main $*.java
	../cacao -log /dev/null $* >$*.thisoutput
	diff --brief $*.output $*.thisoutput
	rm -f $*.thisoutput

%.tstrun:
	../cacao -log /dev/null sun.tools.javac.Main $*.java
	../cacao -log /dev/null $*
