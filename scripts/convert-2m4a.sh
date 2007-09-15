#!/bin/sh
# Script that converts a file into an m4a file
#
# USAGE:
#
# convert-2m4a.sh [options] inputfile
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
extension="m4a"
ENCODER_OPTS="-q 150 -c 22000"
ENCODER="faac"

. ${0%/*}/gtkpod-convert-common.sh

if [ $filetype = "wav" ]; then
    "$encoder" -o "$outfile" $ENCODER_OPTS -w --artist "$artist" --title "$title" --year "$year" --album "$album" --track "$track" --genre "$genre" --comment "$comment" "$infile"
else
    "$decoder" $options "$infile" | "$encoder" -o "$outfile" $ENCODER_OPTS -w --artist "$artist" --title "$title" --year "$year" --album "$album" --track "$track" --genre "$genre" --comment "$comment" -
fi
# Check result
if [ "x$?" != "x0" ]; then
    exit 6
fi

# Seems to be ok: display filename for gtkpod
echo $outfile
exit 0
