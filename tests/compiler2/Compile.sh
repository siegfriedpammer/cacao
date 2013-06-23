#!/bin/bash

set -e
set -u

JAVA=$1
TEST=${2//.o/}
SRCDIR=$3

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

$JAVA -Xbootclasspath/a:. -XX:+DebugCompiler2 -XX:CompileMethod=$METHOD $CLASS
