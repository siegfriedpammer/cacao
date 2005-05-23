#!/bin/sh

libtoolize --automake
if test `uname` = 'FreeBSD'; then
    echo "FreeBSD -- only tested with 5.3-RELEASE though"
    aclocal -I . -I src/classpath/m4 -I /usr/local/share/aclocal -I /usr/local/share/aclocal19
else
    aclocal -I . -I src/classpath/m4
fi
autoheader
automake --add-missing
autoconf

#echo "boehm-gc"
cd src/boehm-gc && ./autogen.sh && cd ../..

#echo "classpath"
cd src/classpath && ./autogen.sh && cd ../..
