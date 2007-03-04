SUBDIRS = \
    classes1 \
	classes2 \
	classes3

HARNESS_SOURCE_FILES = \
	TestController.java \
	TestLoader.java

HARNESS_CLASS_FILES = \
	TestController.class \
	TestLoader.class

TEST_SOURCE_FILES = \
	test_instance_subtype_violated.java \
	test_param_loading_constraint_violated_derived.java \
	test_param_loading_constraint_violated.java \
	test_param_subtype_violated.java \
	test_retval_loading_constraint_violated.java \
	test_return_subtype_ok.java \
	test_return_subtype_violated.java \
	test_simple_lazy_load.java

TEST_NAMES = \
	test_instance_subtype_violated \
	test_param_loading_constraint_violated_derived \
	test_param_loading_constraint_violated \
	test_param_subtype_violated \
	test_retval_loading_constraint_violated \
	test_return_subtype_ok \
	test_return_subtype_violated \
	test_simple_lazy_load

EXTRA_DIST = $(HARNESS_SOURCE_FILES) $(TEST_SOURCE_FILES)

CLEANFILES = \
	*.class

JAVA      = $(top_builddir)/src/cacao/cacao

if WITH_CLASSPATH_GNU
JAVAFLAGS = -Xbootclasspath:$(top_builddir)/src/lib/classes/:$(CLASSPATH_CLASSES)
else
JAVAFLAGS = -Xbootclasspath:$(CLASSPATH_CLASSES)
endif

check: $(HARNESS_CLASS_FILES)
	for t in $(TEST_NAMES) ; do echo "TEST $$t" ; { $(JAVAC) $$t.java && $(JAVA) $$t ; } || exit 1 ; done

$(HARNESS_CLASS_FILES): $(HARNESS_SOURCE_FILES)
	$(JAVAC) $(HARNESS_SOURCE_FILES)

