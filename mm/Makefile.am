noinst_LIBRARIES = libmm_new.a libmm_old.a

libmm_new_a_SOURCES = allocator2.c allocator.h \
		      bitmap2.c bitmap2.h \
		      heap2.c \
		      lifespan.c lifespan.h \
		      mm.h
libmm_old_a_SOURCES = heap.old.c

INCLUDES=-I$(top_srcdir) -I$(top_srcdir)/@SYSDEP_DIR@
