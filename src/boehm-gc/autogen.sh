#!/bin/sh

LIBTOOLIZE=${CACAO_LIBTOOLIZE}
ACLOCAL=${CACAO_ACLOCAL}
AUTOHEADER=${CACAO_AUTOHEADER}
AUTOMAKE=${CACAO_AUTOMAKE}
AUTOCONF=${CACAO_AUTOCONF}

${LIBTOOLIZE} --automake
if test `uname` = 'FreeBSD'; then
    ${ACLOCAL} -I . -I /usr/local/share/aclocal -I /usr/local/share/aclocal19
else
    ${ACLOCAL}
fi
${AUTOHEADER}
${AUTOMAKE} --add-missing
${AUTOCONF}
