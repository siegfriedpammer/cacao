EXTRA_DIST = check.diff compress.diff compress.output db.diff db.output javac.diff javac.output mpegaudio.diff mpegaudio.output raytrace.diff raytrace.output jess.output jack.output

checkjvm: compress db javac mpegaudio raytrace jess jack

compress db javac mpegaudio raytrace jess jack: force
	rm -rf $@
	cp -r ../jvm98/spec/benchmarks/_*_$@ $@
	patch -p0 < $@.diff
	(cd $@; \
	 chmod 644 *; \
	 chmod 755 input; \
	 sh setup.sh || true; \
	 ../../cacao -ieee sun.tools.javac.Main -d . *.java  && \
	 ../../cacao -rt -ieee Main > $@.output; \
	 sh postoutput.sh || true; \
	)
	diff --brief $@/$@.output $@.output
	rm -rf $@
	
force:
