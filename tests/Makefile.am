SUBDIRS = kaffe

EXTRA_DIST = x.java x.output \
	jctest.java jctest.output \
	fp.java fp.output \
	fptest.java fptest.output \
	GCBench.java

check_DATA = checkall

checkall: x.tst jctest.tst fp.tst fptest.tst TestArrayClasses.tst GCBench.tstrun

%.tst:
	../cacao sun.tools.javac.Main $*.java
	../cacao $* >$*.thisoutput
	diff --brief $*.output $*.thisoutput
	rm -f $*.thisoutput

%.tstrun:
	../cacao sun.tools.javac.Main $*.java
	../cacao $*
