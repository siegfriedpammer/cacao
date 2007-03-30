CC=gcc
JAVAC=javac

all:
	$(JAVAC) NativeGlobalRef.java
	$(CC) -shared -O2 native.c -o libnative.so -fPIC

clean:
	rm -rf NativeGlobalRef.class libnative.so
