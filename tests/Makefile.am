EXTRA_DIST = x.java x.output jctest.java jctest.output

check_DATA = checkall

checkall: x.tst jctest.tst

%.tst:
	../cacao -ieee sun.tools.javac.Main $*.java
	../cacao -ieee $* >$*.thisoutput
	diff --brief $*.thisoutput $*.output
	rm -f $*.thisoutput
