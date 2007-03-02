SOURCE_FILES = \
	BarPassFoo.java \
	BarUseFoo.java \
	DerivedFoo.java \
	Foo.java

CLASS_FILES = \
	BarPassFoo.class \
	BarUseFoo.class \
	DerivedFoo.class \
	Foo.class

EXTRA_DIST = $(SOURCE_FILES)

CLEANFILES = \
	*.class

check: $(CLASS_FILES)

$(CLASS_FILES): $(SOURCE_FILES)
	$(JAVAC) $(SOURCE_FILES)


