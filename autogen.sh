#!/bin/sh
#
aclocal
automake --foreign
autoconf

./configure $*
