## Process this file with automake to produce Makefile.in

# $Id: Makefile.am 1429 2004-11-02 08:58:26Z jowenn $


INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/jit/@ARCH_DIR@

EXTRA_DIST =

noinst_LIBRARIES = libnat.a

libnat_a_SOURCES = \
	Constructor.c \
	Field.c \
	Method.c \
	Proxy.c \
	Runtime.c \
	Thread.c \
	VMClass.c \
	VMClassLoader.c \
	VMObject.c \
	VMSecurityManager.c \
	VMString.c \
	VMSystem.c \
	VMThrowable.c \
	JOWENNTest1.c \
	classcontextnativeTest.c

#libgtk_a_SOURCES = \
	GdkGraphics.c \
	GdkPixbufDecoder.c \
	GtkComponentPeer.c \
	GtkFileDialogPeer.c \
	GtkLabelPeer.c \
	GtkScrollPanePeer.c

implementedNatives:
	echo "">implementednatives.data
	grep "JNICALL Java_" *.c | perl -p -i -e "s/.*JNICALL\s//" | perl -p -i -e"s/\(.*//"  | (\
		while read line; do \
			grep $$line *.h >/dev/null ; \
			if test $$? == 0; then \
				echo $$line >>implementednatives.data ;\
			fi ;\
		done )

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
