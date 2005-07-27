#!/bin/sh

JAVA=$1
TEST=$2

echo -n "$TEST: "

$JAVA $TEST > $TEST.thisoutput 2>&1

if [ $? -eq "0" ]; then
    # no Error returned
    if [ -f $TEST.2output ]; then
        # Error should have been returned
        echo "OK, but wrong return value: $?"
        head $TEST.output
        exit
    fi
	
    cmp -s $TEST.output $TEST.thisoutput

    if [ $? -eq "0" ]; then
        echo "OK"
    else
        echo "FAILED"
        diff $TEST.output $TEST.thisoutput | head
    fi

else
    # Error returned
    if [ ! -f $TEST.2output ]; then
        # No Error should have been returned
        echo "FAILED, but wrong return value: $?"
        head $TEST.this2output
        exit
    fi

    cmp -s $TEST.2output $TEST.thisoutput

    if [ $? -eq "0" ]; then
        echo "OK"
    else
        echo "FAILED"
        diff $TEST.2output $TEST.thisoutput | head
    fi
fi		

rm -f $TEST.thisoutput
