#!/bin/sh

aclocal
autoheader
automake --add-missing
autoconf

cd mm/boehm-gc && ./autogen.sh && cd ../..
cd classpath && ./autogen.sh && cd ../..
