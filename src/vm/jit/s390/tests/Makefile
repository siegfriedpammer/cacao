all: tests.class

tests.java: tests.java.pp
	cpp tests.java.pp | grep -v '^#' > tests.java

tests.class: tests.java
	ecj -bootclasspath /home/peter/cacao-dev/cldc-classes $<
