#!/bin/bash

set -e
set -u

JAVA=$1
TEST=${2//.o/}
SRCDIR=$3
OPT_CACAO=""

if [[ $# == 4 ]] ; then
	OPT_CACAO+="-XX:-DebugCompiler2"
else
	OPT_CACAO+="-XX:+DebugCompiler2"
fi

CLASS=''
METHOD=''
for token in $(echo $TEST | tr '.' ' ') ; do
	if [[ "$CLASS"x == ""x ]] ; then
		CLASS=$METHOD
	else
		CLASS="$CLASS.$METHOD"
	fi
	METHOD=$token
done

#echo $JAVA -Xbootclasspath/a:. $OPT_CACAO -XX:CompileMethod=$METHOD $CLASS
$JAVA -Xbootclasspath/a:. $OPT_CACAO -XX:CompileMethod=$METHOD $CLASS
