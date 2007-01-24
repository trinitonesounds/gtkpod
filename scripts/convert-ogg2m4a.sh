#!/bin/sh
# Simple script that converts an ogg file into an m4a file
# STDOUT's last line is the converted filename.
# Return Codes:
#   0 ok
#   1 input file not found
#   2 output file cannot be created
#   3 cannot get info
#   4 cannot exec decoding
#   5 cannot exec encoding
#   6 conversion failed

# Get parameter
oggfile=$1

# Build output file
m4afile=`basename "$oggfile"`
m4afile=${m4afile%%.ogg}
m4afile="/tmp/$m4afile.m4a"

# Default values
comment="Encoded for gtkpod with faac"

#echo "Converting \"$oggfile\" into \"$m4afile\""

# Checking input file
if [ "$oggfile" = "" ]; then
    exit 1
fi
if [ ! -f "$oggfile" ]; then
    exit 1
fi

# Checking output file
touch "$m4afile"
if [ "x$?" != "x0" ]; then
    exit 2
fi

# Getting ogg info
album=`ogginfo $oggfile | grep album= | cut -d = -f 2`
title=`ogginfo $oggfile | grep title= | cut -d = -f 2`
artist=`ogginfo $oggfile | grep artist= | cut -d = -f 2`
year=`ogginfo $oggfile | grep date= | cut -d = -f 2`
genre=`ogginfo $oggfile | grep genre= | cut -d = -f 2`
tracknum=`ogginfo $oggfile | grep tracknumber= | cut -d = -f 2`
comment_t=`ogginfo $oggfile | grep comment= | cut -d = -f 2`
comment_a=`ogginfo $oggfile | grep albumcomment= | cut -d = -f 2`
if [ "$comment_t" != "" ]; then
    comment=$comment_t
fi
if [ "$comment_a" != "" ]; then
    comment="$comment_a $comment"
fi

# Launch command
exec oggdec --output - -- "$oggfile" | faac -o "$m4afile" -q 150 -c 22000 -w --artist "$artist" --title "$title" --year "$year" --album "$album" --track "$tracknum" --genre "$genre" --comment "$comment" -
# Check result
if [ "x$?" != "x0" ]; then
    exit 6
fi
# Seems to be ok: display filename for gtkpod
echo $m4afile
exit 0
