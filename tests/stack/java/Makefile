all:
	javac stackTraceTest1.java
	gcc -shared -o libstackTraceTest1.so stackTraceTest1.c
clean:
	rm -f libstackTraceTest1.so stackTraceTest1.class
test:
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd` java stackTraceTest1
