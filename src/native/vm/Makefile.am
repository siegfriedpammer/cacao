## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1645 2004-12-01 14:06:44Z twisti $

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/@ARCH_DIR@

CLEANFILES = implementednatives.data

noinst_LIBRARIES = libnativevm.a

libnativevm_a_SOURCES = \
	Constructor.c \
	Field.c \
	Method.c \
	Proxy.c \
	VMClass.c \
	VMClassLoader.c \
	VMObject.c \
	VMRuntime.c \
	VMSecurityManager.c \
	VMString.c \
	VMSystem.c \
	VMThread.c \
	VMThrowable.c

all: libnativevm.a implementednatives.data

implementednatives.data: $(libnativevm_a_SOURCES)
	echo "" > implementednatives.data
	grep "JNICALL Java_" *.c | perl -p -i -e "s/.*JNICALL\s//" | perl -p -i -e"s/\(.*//"  | ( \
		while read line; do \
			grep $$line $(top_srcdir)/src/native/include/*.h > /dev/null; \
			if test $$? == 0; then \
				echo $$line >> implementednatives.data; \
			fi ; \
		done )

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
