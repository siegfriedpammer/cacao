#!/bin/sh

aclocal
autoheader
automake --add-missing --include-deps
autoconf

cd mm/boehm-gc && ./autogen.sh && cd ../..
cd classpath && rm -f configure configure_int &&./autogen.sh && mv configure configure_int && cp cacaoconfigure configure && cd ../..

