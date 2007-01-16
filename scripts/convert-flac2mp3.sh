#!/bin/sh
# Simple script that converts an ogg file into an mp3 file
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
mp3file=`basename "$flacfile"`
mp3file=${mp3file%%.flac}
mp3file="/tmp/$mp3file.mp3"

# Default values
comment="Encoded for gtkpod with lame"

#echo "Converting \"$flacfile\" into \"$mp3file\""

# Checking input file
if [ "$flacfile" == "" ]; then 
    exit 1
fi
if [ ! -f "$flacfile" ]; then
    exit 1
fi

# Checking output file
touch $mp3file
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
if [ "$comment_t" != "" ]; then
    comment=$comment_t
fi

#Fix unrecognised genre for lame
if [ "$genre" != "" ]; then
    check_genre=`lame --genre-list | cut -c5- | grep -i $genre`
    if [ "x$?" != "x0" ]; then
        echo "WARNING: Unrecognised genre \"$genre\". Genre cleaned."
        genre=""
    fi
fi

# Launch command
exec flac -d -c "$flacfile" | lame -h --add-id3v2 --tt "$title" --ta "$artist" --tl "$album" --ty "$year" --tc "$comment" --tn "$tracknum" --tg "$genre" - "$mp3file"
# Check result
if [ "x$?" != "x0" ]; then
    exit 6
fi
# Seems to be ok: display filename for gtkpod
echo $mp3file
exit 0
