#!/bin/sh

# autogen.sh from Valgrind

run ()
{
    echo "running: $*"
    eval $*

    if test $? != 0 ; then
	echo "error: while running '$*'"
	exit 1
    fi
}

run aclocal
run autoheader
run automake --add-missing --gnu
run intltoolize --force
run autoconf
