## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1622 2004-11-30 13:18:52Z twisti $

if USE_THREADS
if NATIVE_THREADS
THREADS = native
else
THREADS = green
endif
else
THREADS =
LIBRARY =
endif

SUBDIRS = $(THREADS)


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
