EXTRA_DIST = check.diff compress.diff compress.output db.diff db.output javac.diff javac.output mpegaudio.diff mpegaudio.output raytrace.diff raytrace.output

JAVA = ../../cacao
JAVAC = $(JAVA) at.dms.kjc.Main

checkjvm: compress db javac mpegaudio raytrace 

check compress db javac mpegaudio raytrace: force
	rm -rf $@
	cp -r ../jvm98/spec/benchmarks/_*_$@ $@
	patch -p0 < $@.diff
	(cd $@; \
	 chmod 644 *; \
	 chmod 755 input; \
	 JAVA="$(JAVA)" JAVAC="$(JAVAC)" sh setup.sh || true; \
	 $(JAVAC) -d . *.java && \
	 $(JAVA) Main > $@.output; \
	 JAVA="$(JAVA)" JAVAC="$(JAVAC)" sh postoutput.sh || true; \
	)
	diff --brief $@/$@.output $@.output
	rm -rf $@

force:
