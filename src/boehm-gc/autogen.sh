#!/bin/sh

libtoolize --automake
if test `uname` == 'FreeBSD'; then
	echo "FreeBSD (only tested with 5.3-RELEASE though"
	aclocal  -I . -I /usr/local/share/aclocal -I /usr/local/share/aclocal19
else
	aclocal
fi
autoheader
automake --add-missing
autoconf
