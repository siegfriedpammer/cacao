EXTRA_DIST = x.java x.output jctest.java fptest.java

check_DATA = checkall

checkall: x.tst jctest.tst GCBench.tstrun

%.tst:
	../cacao -ieee sun.tools.javac.Main $*.java
	../cacao -ieee $* >$*.thisoutput
	diff --brief $*.thisoutput $*.output
	rm -f $*.thisoutput

%.tstrun:
	../cacao -ieee sun.tools.javac.Main $*.java
	../cacao -ieee $*
