#!/bin/sh

libtoolize --automake
aclocal
autoheader
automake --add-missing
autoconf
