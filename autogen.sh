#!/bin/sh

aclocal
autoheader
automake --add-missing --include-deps
autoconf

cd mm/boehm-gc && ./autogen.sh && cd ../..
cd gnuclasspathnat && ./autogen.sh && cd ../..

