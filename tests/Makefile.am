EXTRA_DIST = x.java x.output

check: 
	../cacao -ieee sun.tools.javac.Main x.java
	../cacao x >x.thisoutput
	@if test "`diff --brief x.thisoutput x.output`" = ""; then \
		echo "*** Test completed successfully"; \
	else \
		echo "*** Test failed"; fi;
	@rm -f x.thisoutput
