# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.3 from Makefile.am

# Copyright (C) 1994, 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# $Id: Makefile 132 1999-09-27 15:54:42Z chris $


SHELL = /bin/sh

srcdir = .
top_srcdir = .
prefix = /usr/local/cacao
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DISTDIR =

pkgdatadir = $(datadir)/cacao
pkglibdir = $(libdir)/cacao
pkgincludedir = $(includedir)/cacao

top_builddir = .

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = alpha-unknown-linux
host_triplet = alpha-unknown-linux-gnu
CC = gcc
COMPILER_OBJECTS = compiler.o
COMPILER_SOURCES = compiler.h compiler.c
GC_OBJ = mm/libmm_new.a
LIBTHREAD = libthreads.a
MAKEINFO = makeinfo
PACKAGE = cacao
RANLIB = ranlib
SYSDEP_DIR = alpha
THREAD_OBJ = threads/libthreads.a
VERSION = 0.40

MAINTAINERCLEANFILES = Makefile.in configure
SUBDIRS = toolbox mm alpha jit comp nat threads mips tst doc narray

EXTRA_DIST = html/cacaoinstall.html html/cacaoman.html html/index.html

CLEANFILES = alpha/asmpart.o \
			 alpha/asmpart.s \
             alpha/offsets.h \
	         nativetable.hh \
	         nativetypes.hh

bin_PROGRAMS = cacao
noinst_PROGRAMS = cacaoh

INCLUDES=-I/usr/include -I$(top_srcdir)/alpha -I$(top_srcdir)/jit -Ialpha -I$(top_srcdir)

cacao_SOURCES = \
	asmpart.h \
	builtin.c \
	builtin.h \
	callargs.h \
	compiler.h compiler.c \
	global.h \
	jit.c \
	jit.h \
	loader.c \
	loader.h \
	main.c \
	native.c \
	native.h \
	tables.c \
	tables.h 

EXTRA_cacao_SOURCES = \
	compiler.c \
	compiler.h 

cacao_LDADD = \
	alpha/asmpart.o \
	compiler.o \
	toolbox/libtoolbox.a \
	mm/libmm_new.a \
    threads/libthreads.a

cacao_DEPENDENCIES = \
	alpha/asmpart.o \
	compiler.o \
	toolbox/libtoolbox.a \
	mm/libmm_new.a \
	threads/libthreads.a

cacaoh_SOURCES = headers.c tables.c loader.c builtin.c
cacaoh_LDADD = toolbox/libtoolbox.a mm/libmm_new.a threads/libthreads.a
cacaoh_DEPENDENCIES = toolbox/libtoolbox.a mm/libmm_new.a threads/libthreads.a
ACLOCAL_M4 = $(top_srcdir)/aclocal.m4
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = config.h
CONFIG_CLEAN_FILES = 
PROGRAMS =  $(bin_PROGRAMS) $(noinst_PROGRAMS)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I.
CPPFLAGS = 
LDFLAGS = 
LIBS = -lm 
cacao_OBJECTS =  builtin.o jit.o loader.o main.o native.o tables.o
cacao_LDFLAGS = 
cacaoh_OBJECTS =  headers.o tables.o loader.o builtin.o
cacaoh_LDFLAGS = 
CFLAGS = -ieee -O2 -g3
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LINK = $(CC) $(CFLAGS) $(LDFLAGS) -o $@
DIST_COMMON =  README AUTHORS COPYING ChangeLog INSTALL Makefile.am \
Makefile.in NEWS acconfig.h aclocal.m4 config.guess config.h.in \
config.sub configure configure.in install-sh missing mkinstalldirs \
stamp-h.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
DEP_FILES =  .deps/builtin.P .deps/compiler.P .deps/headers.P \
.deps/jit.P .deps/loader.P .deps/main.P .deps/native.P .deps/tables.P
SOURCES = $(cacao_SOURCES) $(EXTRA_cacao_SOURCES) $(cacaoh_SOURCES)
OBJECTS = $(cacao_OBJECTS) $(cacaoh_OBJECTS)

all: all-recursive-am all-am

.SUFFIXES:
.SUFFIXES: .S .c .o .s
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4)
	cd $(top_srcdir) && $(AUTOMAKE) --gnu Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$@ CONFIG_HEADERS= $(SHELL) ./config.status

$(ACLOCAL_M4):  configure.in 
	cd $(srcdir) && $(ACLOCAL)

config.status: $(srcdir)/configure
	$(SHELL) ./config.status --recheck
$(srcdir)/configure: $(srcdir)/configure.in $(ACLOCAL_M4) $(CONFIGURE_DEPENDENCIES)
	cd $(srcdir) && $(AUTOCONF)

config.h: stamp-h
	@:
stamp-h: $(srcdir)/config.h.in $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES= CONFIG_HEADERS=config.h \
	     $(SHELL) ./config.status
	@echo timestamp > stamp-h
$(srcdir)/config.h.in: $(srcdir)/stamp-h.in
$(srcdir)/stamp-h.in: $(top_srcdir)/configure.in $(ACLOCAL_M4) acconfig.h
	cd $(top_srcdir) && $(AUTOHEADER)
	@echo timestamp > $(srcdir)/stamp-h.in

mostlyclean-hdr:

clean-hdr:

distclean-hdr:
	-rm -f config.h

maintainer-clean-hdr:

mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "  $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`"; \
	     $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(bin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	done

mostlyclean-noinstPROGRAMS:

clean-noinstPROGRAMS:
	-test -z "$(noinst_PROGRAMS)" || rm -f $(noinst_PROGRAMS)

distclean-noinstPROGRAMS:

maintainer-clean-noinstPROGRAMS:

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

cacao: $(cacao_OBJECTS) $(cacao_DEPENDENCIES)
	@rm -f cacao
	$(LINK) $(cacao_LDFLAGS) $(cacao_OBJECTS) $(cacao_LDADD) $(LIBS)

cacaoh: $(cacaoh_OBJECTS) $(cacaoh_DEPENDENCIES)
	@rm -f cacaoh
	$(LINK) $(cacaoh_LDFLAGS) $(cacaoh_OBJECTS) $(cacaoh_LDADD) $(LIBS)

# This directory's subdirectories are mostly independent; you can cd
# into them and run `make' without going through this Makefile.
# To change the values of `make' variables: instead of editing Makefiles,
# (1) if the variable is set in `config.status', edit `config.status'
#     (which will cause the Makefiles to be regenerated when you run `make');
# (2) otherwise, pass the desired values on the `make' command line.



all-recursive install-data-recursive install-exec-recursive \
installdirs-recursive install-recursive uninstall-recursive  \
check-recursive installcheck-recursive info-recursive dvi-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  target=`echo $@ | sed s/-recursive//`; \
	  echo "Making $$target in $$subdir"; \
	  (cd $$subdir && $(MAKE) $$target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

mostlyclean-recursive clean-recursive distclean-recursive \
maintainer-clean-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	rev=''; list='$(SUBDIRS)'; for subdir in $$list; do \
	  rev="$$subdir $$rev"; \
	done; \
	for subdir in $$rev; do \
	  target=`echo $@ | sed s/-recursive//`; \
	  echo "Making $$target in $$subdir"; \
	  (cd $$subdir && $(MAKE) $$target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"
tags-recursive:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) tags); \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $(SOURCES) $(HEADERS) $(LISP)

TAGS: tags-recursive $(HEADERS) $(SOURCES) config.h.in $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  test -f $$subdir/TAGS && tags="$$tags -i $$here/$$subdir/TAGS"; \
	done; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)config.h.in$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags config.h.in $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(PACKAGE)-$(VERSION)
top_distdir = $(distdir)

# This target untars the dist file and tries a VPATH configuration.  Then
# it guarantees that the distribution is self-contained by making another
# tarfile.
distcheck: dist
	-rm -rf $(distdir)
	GZIP=$(GZIP) $(TAR) zxf $(distdir).tar.gz
	mkdir $(distdir)/=build
	mkdir $(distdir)/=inst
	dc_install_base=`cd $(distdir)/=inst && pwd`; \
	cd $(distdir)/=build \
	  && ../configure --srcdir=.. --prefix=$$dc_install_base \
	  && $(MAKE) \
	  && $(MAKE) dvi \
	  && $(MAKE) check \
	  && $(MAKE) install \
	  && $(MAKE) installcheck \
	  && $(MAKE) dist
	-rm -rf $(distdir)
	@echo "========================"; \
	echo "$(distdir).tar.gz is ready for distribution"; \
	echo "========================"
dist: distdir
	-chmod -R a+r $(distdir)
	GZIP=$(GZIP) $(TAR) chozf $(distdir).tar.gz $(distdir)
	-rm -rf $(distdir)
dist-all: distdir
	-chmod -R a+r $(distdir)
	GZIP=$(GZIP) $(TAR) chozf $(distdir).tar.gz $(distdir)
	-rm -rf $(distdir)
distdir: $(DISTFILES)
	-rm -rf $(distdir)
	mkdir $(distdir)
	-chmod 777 $(distdir)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(distdir) && pwd`; \
	distdir=`cd $(distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --gnu Makefile
	$(mkinstalldirs) $(distdir)/html
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done
	for subdir in $(SUBDIRS); do \
	  test -d $(distdir)/$$subdir \
	  || mkdir $(distdir)/$$subdir \
	  || exit 1; \
	  chmod 777 $(distdir)/$$subdir; \
	  (cd $$subdir && $(MAKE) top_distdir=../$(distdir) distdir=../$(distdir)/$$subdir distdir) \
	    || exit 1; \
	done

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)

-include $(DEP_FILES)

mostlyclean-depend:

clean-depend:

distclean-depend:

maintainer-clean-depend:
	-rm -rf .deps

%.o: %.c
	@echo '$(COMPILE) -c $<'; \
	$(COMPILE) -Wp,-MD,.deps/$(*F).P -c $<

%.lo: %.c
	@echo '$(LTCOMPILE) -c $<'; \
	$(LTCOMPILE) -Wp,-MD,.deps/$(*F).p -c $<
	@-sed -e 's/^\([^:]*\)\.o:/\1.lo \1.o:/' \
	  < .deps/$(*F).p > .deps/$(*F).P
	@-rm -f .deps/$(*F).p
info: info-recursive
dvi: dvi-recursive
check: all-am
	$(MAKE) check-recursive
installcheck: installcheck-recursive
all-recursive-am: config.h
	$(MAKE) all-recursive

all-am: Makefile $(PROGRAMS) config.h

install-exec-am: install-binPROGRAMS

uninstall-am: uninstall-binPROGRAMS

install-exec: install-exec-recursive install-exec-am
	@$(NORMAL_INSTALL)

install-data: install-data-recursive
	@$(NORMAL_INSTALL)

install: install-recursive install-exec-am
	@:

uninstall: uninstall-recursive uninstall-am

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' INSTALL_SCRIPT='$(INSTALL_PROGRAM)' install
installdirs: installdirs-recursive
	$(mkinstalldirs)  $(DATADIR)$(bindir)


mostlyclean-generic:
	-test -z "$(MOSTLYCLEANFILES)" || rm -f $(MOSTLYCLEANFILES)

clean-generic:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	-rm -f Makefile $(DISTCLEANFILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)

maintainer-clean-generic:
	-test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
	-test -z "$(BUILT_SOURCES)" || rm -f $(BUILT_SOURCES)
mostlyclean-am:  mostlyclean-hdr mostlyclean-binPROGRAMS \
		mostlyclean-noinstPROGRAMS mostlyclean-compile \
		mostlyclean-tags mostlyclean-depend mostlyclean-generic

clean-am:  clean-hdr clean-binPROGRAMS clean-noinstPROGRAMS \
		clean-compile clean-tags clean-depend clean-generic \
		mostlyclean-am

distclean-am:  distclean-hdr distclean-binPROGRAMS \
		distclean-noinstPROGRAMS distclean-compile \
		distclean-tags distclean-depend distclean-generic \
		clean-am

maintainer-clean-am:  maintainer-clean-hdr maintainer-clean-binPROGRAMS \
		maintainer-clean-noinstPROGRAMS \
		maintainer-clean-compile maintainer-clean-tags \
		maintainer-clean-depend maintainer-clean-generic \
		distclean-am

mostlyclean:  mostlyclean-recursive mostlyclean-am

clean:  clean-recursive clean-am

distclean:  distclean-recursive distclean-am
	-rm -f config.status

maintainer-clean:  maintainer-clean-recursive maintainer-clean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	-rm -f config.status

.PHONY: mostlyclean-hdr distclean-hdr clean-hdr maintainer-clean-hdr \
mostlyclean-binPROGRAMS distclean-binPROGRAMS clean-binPROGRAMS \
maintainer-clean-binPROGRAMS uninstall-binPROGRAMS install-binPROGRAMS \
mostlyclean-noinstPROGRAMS distclean-noinstPROGRAMS \
clean-noinstPROGRAMS maintainer-clean-noinstPROGRAMS \
mostlyclean-compile distclean-compile clean-compile \
maintainer-clean-compile install-data-recursive \
uninstall-data-recursive install-exec-recursive \
uninstall-exec-recursive installdirs-recursive uninstalldirs-recursive \
all-recursive check-recursive installcheck-recursive info-recursive \
dvi-recursive mostlyclean-recursive distclean-recursive clean-recursive \
maintainer-clean-recursive tags tags-recursive mostlyclean-tags \
distclean-tags clean-tags maintainer-clean-tags distdir \
mostlyclean-depend distclean-depend clean-depend \
maintainer-clean-depend info dvi installcheck all-recursive-am all-am \
install-exec-am uninstall-am install-exec install-data install \
uninstall all installdirs mostlyclean-generic distclean-generic \
clean-generic maintainer-clean-generic clean mostlyclean distclean \
maintainer-clean


native.c: nativetypes.hh alpha/offsets.h nativetable.hh

nativetypes.hh alpha/offsets.h nativetable.hh: cacaoh
	./cacaoh \
		java.lang.Object \
		java.lang.String \
		java.lang.Class \
		java.lang.ClassLoader \
		java.lang.Compiler \
		java.lang.Double \
		java.lang.Float \
		java.lang.Math \
		java.lang.Runtime \
		java.lang.SecurityManager \
		java.lang.System \
		java.lang.Thread \
		java.lang.ThreadGroup \
		java.lang.Throwable \
		java.io.File \
		java.io.FileDescriptor \
		java.io.FileInputStream \
		java.io.FileOutputStream \
		java.io.PrintStream \
		java.io.RandomAccessFile \
		java.util.Properties \
		java.util.Date

alpha/asmpart.o: $(top_srcdir)/alpha/asmpart.c alpha/offsets.h
	rm -f alpha/asmpart.s
	gcc -E $(INCLUDES) $(top_srcdir)/alpha/asmpart.c \
			> alpha/asmpart.s
	gcc $(CFLAGS) $(INCLUDES) -c -o alpha/asmpart.o \
			alpha/asmpart.s
	rm -f asmpart.s

compiler.o: $(top_srcdir)/builtin.h $(top_srcdir)/compiler.h \
		    $(top_srcdir)/global.h $(top_srcdir)/loader.h \
	        $(top_srcdir)/tables.h $(top_srcdir)/native.h \
            $(top_srcdir)/asmpart.h $(top_srcdir)/compiler.c $(top_srcdir)/comp/*.c \
            $(top_srcdir)/alpha/gen.c $(top_srcdir)/alpha/disass.c
	gcc $(CFLAGS) -I. $(INCLUDES) -c $(top_srcdir)/compiler.c

jit.o: jit.c \
	jit/mcode.c \
	jit/parse.c \
	jit/reg.c \
	jit/stack.c \
	jit/jitdef.h \
	narray/graph.c \
	narray/loop.c \
	narray/analyze.c \
	narray/tracing.c \
	narray/loop.h

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
