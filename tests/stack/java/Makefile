all: testlib1 testlib2

testlib1:
	javac stackTraceTest1.java
	gcc -shared -o libstackTraceTest1.so stackTraceTest1.c

testlib2:
	javac stackTraceTest2.java
	gcc -shared -o libstackTraceTest2.so stackTraceTest2.c
clean:
	rm -f libstackTraceTest1.so stackTraceTest1.class
	rm -f libstackTraceTest2.so stackTraceTest2.class
test:
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd` java stackTraceTest1
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd` java stackTraceTest2

