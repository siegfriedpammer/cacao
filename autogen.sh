#!/bin/sh

libtoolize --automake
aclocal
autoheader
automake --add-missing
autoconf

#echo "boehm-gc"
cd src/boehm-gc && ./autogen.sh && cd ../..

#echo "classpath"
cd src/classpath && ./autogen.sh && cd ../..
