#!/bin/sh

aclocal
autoheader
automake --add-missing --include-deps
autoconf

