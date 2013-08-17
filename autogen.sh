#!/bin/sh

set -x
aclocal
libtoolize
autoconf
autoheader
automake --add-missing --foreign
./configure $*
