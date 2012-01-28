#!/bin/sh
# Script that converts a file into an mp4 file
#
# USAGE:
#
# convert-2mp4.sh [options] inputfile
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
extension="mp4"
ENCODER_OPTS="-y -sameq -acodec aac -ab 160k -b 1100k -s 640x480 -aspect 4:3"
ENCODER="ffmpeg"

. ${0%/*}/gtkpod-convert-common.sh

echo "Attempting to convert file $infile" >> "$LOG"

"$encoder" -i "$infile" $ENCODER_OPTS -metadata author="$artist" -metadata title="$title" -metadata album="$album" -metadata year="$year" -metadata track="$track" -metadata genre="$genre" -metadata comment="$comment" "$outfile" >> "$LOG" 2>&1

# Check result
if [ "x$?" != "x0" ]; then
	echo "Failed with status 6" >> "$LOG"
    exit 6
fi

if [ ! -f "$outfile" ]; then
	echo "Failed with status 6" >> "$LOG"
	exit 8
fi

# Seems to be ok: display filename for gtkpod
echo $outfile
exit 0
