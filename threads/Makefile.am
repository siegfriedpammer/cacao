noinst_LIBRARIES = @LIBTHREAD@
EXTRA_LIBRARIES = libthreads.a

libthreads_a_SOURCES = locks.c locks.h \
		       thread.c thread.h \
		       threadio.c threadio.h

INCLUDES=-I$(top_srcdir) -I$(top_srcdir)/@SYSDEP_DIR@
