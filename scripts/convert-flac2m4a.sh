#!/bin/sh
# Simple script that converts a flac file into an m4a file
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
flacfile=$1

# Build output file
m4afile=`basename "$flacfile"`
m4afile=${m4afile%%.flac}
m4afile="/tmp/$m4afile.m4a"

# Default values
comment="Encoded for gtkpod with faac"

#echo "Converting \"$flacfile\" into \"$m4afile\""

# Checking input file
if [ "$flacfile" = "" ]; then
    exit 1
fi
if [ ! -f "$flacfile" ]; then
    exit 1
fi

# Checking output file
touch "$m4afile"
if [ "x$?" != "x0" ]; then
    exit 2
fi

# Getting flac info
album=`metaflac --show-tag=album "$1" | cut -d \= -f 2`
title=`metaflac --show-tag=title "$1" | cut -d \= -f 2`
artist=`metaflac --show-tag=artist "$1" | cut -d \= -f 2`
year=`metaflac --show-tag=date "$1" | cut -d \= -f 2`
genre=`metaflac --show-tag=genre "$1" | cut -d \= -f 2`
tracknum=`metaflac --show-tag=tracknumber "$1" | cut -d \= -f 2`
comment_t=`metaflac --show-tag=comment "$1" | cut -d \= -f 2`
comment_a=`metaflac --show-tag=albumcomment "$1" | cut -d \= -f 2`
if [ "$comment_t" != "" ]; then
    comment=$comment_t
fi
if [ "$comment_a" != "" ]; then
    comment="$comment_a $comment"
fi

# Launch command
exec flac -d -c -- "$flacfile" | faac -o "$m4afile" -q 150 -c 22000 -w --artist "$artist" --title "$title" --year "$year" --album "$album" --track "$tracknum" --genre "$genre" --comment "$comment" -
# Check result
if [ "x$?" != "x0" ]; then
    exit 6
fi
# Seems to be ok: display filename for gtkpod
echo $m4afile
exit 0
