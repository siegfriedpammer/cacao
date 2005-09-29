#!/bin/sh

libtoolize --automake
if test `uname` = 'FreeBSD'; then
    aclocal -I m4 -I /usr/local/share/aclocal -I /usr/local/share/aclocal19
else
    aclocal -I m4
fi
autoheader
automake --add-missing
autoconf

cd src/boehm-gc && ./autogen.sh && cd ../..

cd src/libltdl && ./autogen.sh && cd ../..

cd src/libffi && ./autogen.sh && cd ../..
