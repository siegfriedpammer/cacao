#!/bin/sh

# test for a libtoolize

HAVE_LIBTOOLIZE=false

for LIBTOOLIZE in libtoolize libtoolize15 glibtoolize; do
    if ${LIBTOOLIZE} --version > /dev/null 2>&1; then
        LIBTOOLIZE_VERSION=`${LIBTOOLIZE} --version | sed 's/^.*[^0-9.]\([0-9]\{1,\}\.[0-9.]\{1,\}\).*/\1/'`
#        echo ${LIBTOOLIZE_VERSION}
        case ${LIBTOOLIZE_VERSION} in
            1.5* )
                HAVE_LIBTOOLIZE=true
                break;
                ;;
        esac
    fi
done

if test ${HAVE_LIBTOOLIZE} = false; then
    echo "No proper libtoolize was found."
    echo "You must have libtool 1.5 installed."
    exit 1
fi


# test for a aclocal

HAVE_ACLOCAL=false

for ACLOCAL in aclocal aclocal-1.9 aclocal19; do
    if ${ACLOCAL} --version > /dev/null 2>&1; then
        ACLOCAL_VERSION=`${ACLOCAL} --version | sed 's/^[^0-9]*\([0-9.][0-9.]*\).*/\1/'`
#        echo ${ACLOCAL_VERSION}
        case ${ACLOCAL_VERSION} in
            1.9* )
                HAVE_ACLOCAL=true
                break;
                ;;
        esac
    fi
done

if test ${HAVE_ACLOCAL} = false; then
    echo "No proper aclocal was found."
    echo "You must have automake 1.9 installed."
    exit 1
fi


# test for a autoheader

HAVE_AUTOHEADER=false

for AUTOHEADER in autoheader autoheader259; do
    if ${AUTOHEADER} --version > /dev/null 2>&1; then
        AUTOHEADER_VERSION=`${AUTOHEADER} --version | sed 's/^[^0-9]*\([0-9.][0-9.]*\).*/\1/'`
#        echo ${AUTOHEADER_VERSION}
        case ${AUTOHEADER_VERSION} in
            2.59* )
                HAVE_AUTOHEADER=true
                break;
                ;;
        esac
    fi
done

if test ${HAVE_AUTOHEADER} = false; then
    echo "No proper autoheader was found."
    echo "You must have autoconf 2.59 installed."
    exit 1
fi


# test for a automake

HAVE_AUTOMAKE=false

for AUTOMAKE in automake automake-1.9 automake19; do
    if ${AUTOMAKE} --version > /dev/null 2>&1; then
        AUTOMAKE_VERSION=`${AUTOMAKE} --version | sed 's/^[^0-9]*\([0-9.][0-9.]*\).*/\1/'`
#        echo ${AUTOMAKE_VERSION}
        case ${AUTOMAKE_VERSION} in
            1.9* )
                HAVE_AUTOMAKE=true
                break;
                ;;
        esac
    fi
done

if test ${HAVE_AUTOMAKE} = false; then
    echo "No proper automake was found."
    echo "You must have automake 1.9 installed."
    exit 1
fi


# test for a autoconf

HAVE_AUTOCONF=false

for AUTOCONF in autoconf autoconf259; do
    if ${AUTOCONF} --version > /dev/null 2>&1; then
        AUTOCONF_VERSION=`${AUTOCONF} --version | sed 's/^[^0-9]*\([0-9.][0-9.]*\).*/\1/'`
#        echo ${AUTOCONF_VERSION}
        case ${AUTOCONF_VERSION} in
            2.59* )
                HAVE_AUTOCONF=true
                break;
                ;;
        esac
    fi
done

if test ${HAVE_AUTOCONF} = false; then
    echo "No proper autoconf was found."
    echo "You must have autoconf 2.59 installed."
    exit 1
fi


${LIBTOOLIZE} --automake
if test `uname` = 'FreeBSD'; then
    ${ACLOCAL} -I m4 -I /usr/local/share/aclocal -I /usr/local/share/aclocal19
else
    ${ACLOCAL} -I m4
fi
${AUTOHEADER}
${AUTOMAKE} --add-missing
${AUTOCONF}

export CACAO_LIBTOOLIZE=${LIBTOOLIZE}
export CACAO_ACLOCAL=${ACLOCAL}
export CACAO_AUTOHEADER=${AUTOHEADER}
export CACAO_AUTOMAKE=${AUTOMAKE}
export CACAO_AUTOCONF=${AUTOCONF}

cd src/boehm-gc && ./autogen.sh && cd ../..
