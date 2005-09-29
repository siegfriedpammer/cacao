INCLUDES = -I$(top_builddir)

noinst_LTLIBRARIES = libfdlibm.la 

libfdlibm_la_SOURCES =  \
			e_fmod.c \
			fdlibm.h \
			ieeefp.h \
			java-assert.h \
			mprec.h \
			s_copysign.c \
			s_finite.c \
			w_fmod.c

# We just want the standard flags for fdlibm since it is an upstream lib
# and our normal -pedantic -Wall -Werror breaks this lib. So no AM_CFLAGS.
# We also don't need extra includes, so no AM_CPPFLAGS either.
