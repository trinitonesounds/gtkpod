#!/bin/sh
# Script that converts a file into an mp3 file
#
# USAGE:
#
# convert-2mp3.sh [options] inputfile
#
# For a list of allowed options please consult gtkpod-convert-common.sh
#
# STDOUT's last line is the converted filename.
# Return Codes:
#   0 ok
#   1 input file not found
#   2 output file cannot be created
#   3 cannot get info
#   4 cannot exec decoding
#   5 cannot exec encoding
#   6 conversion failed
#   7 unknown option
#

# Constants
extension="mp3"
ENCODER_OPTS="--preset standard"
ENCODER="lame"

. ${0%/*}/gtkpod-convert-common.sh

# Check if the genre is one which lame supports
if [ -n "$genre" ] && `"$encoder" --genre-list | grep -qi "\b$genre\b"`; then
    usegenre=$genre
else
    usegenre=""
    # check for id3v2
    id3v2=`which id3v2`
    if [ -n "$id3v2" ]; then
        useid3v2=1
    fi
fi

if [ $filetype = "wav" ]; then
    "$encoder" $ENCODER_OPTS --add-id3v2 --tt "$title" --ta "$artist" --tl "$album" --ty "$year" --tc "$comment" --tn "$track" --tg "$usegenre" "$infile" "$outfile"
else
    "$decoder" $options "$infile" | "$encoder" $ENCODER_OPTS --add-id3v2 --tt "$title" --ta "$artist" --tl "$album" --ty "$year" --tc "$comment" --tn "$track" --tg "$usegenre" - "$outfile"
fi
# Check result
if [ "x$?" != "x0" ]; then
    exit 6
fi

# Try to set the genre with id3v2 if needed.  This may fail as older versions of
# id3v2 did not support genre strings in place of numeric genre id's
if [ -n "$useid3v2" ]; then
    $id3v2 -g "$genre" "$mp3file" || :
fi

# Seems to be ok: display filename for gtkpod
echo $outfile
exit 0
