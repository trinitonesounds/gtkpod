#!/bin/bash

if [ $# -eq 0 ]; then
  top_srcdir="."
  distdir="."
elif [ $# -eq 1 ]; then
  top_srcdir=$1
  distdir="."
else
  top_srcdir=$1
  distdir=$2
fi

if [ -f ${distdir}/ChangeLog ]; then
	chmod u+w ${distdir}/ChangeLog
fi

git log --date-order --date=short | \
	sed -e '/^commit.*$/d' | \
	awk '/^Author/ {sub(/\\$/,""); getline t; print $0 t; next}; 1' | \
	sed -e 's/^Author: //g' | \
	sed -e 's/>Date:   \([0-9]*-[0-9]*-[0-9]*\)/>\t\1/g' | \
	sed -e 's/^\(.*\) \(\)\t\(.*\)/\3    \1    \2/g' > ${distdir}/ChangeLog
