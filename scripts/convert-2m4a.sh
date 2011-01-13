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
# use the following for better quality (25% increase in file size) or simply
# specify your options with "-q ..."
#ENCODER_OPTS="-q 256 -c 44100"
ENCODER="faac"

. ${0%/*}/gtkpod-convert-common.sh

LOG=`dirname "$outfile"`
LOG=${LOG%.log}

echo "Attempting to convert file" >> $LOG

if [ $filetype = "wav" ]; then
    "$encoder" -o "$outfile" $ENCODER_OPTS -w --artist "$artist" --title "$title" --year "$year" --album "$album" --track "$track" --genre "$genre" --comment "$comment" "$infile" >> $LOG 2>&1
else
    "$decoder" $options "$infile" | "$encoder" -o "$outfile" $ENCODER_OPTS -w --artist "$artist" --title "$title" --year "$year" --album "$album" --track "$track" --genre "$genre" --comment "$comment" - >> $LOG 2>&1
fi
# Check result
if [ "x$?" != "x0" ]; then
    exit 6
fi

# Seems to be ok: display filename for gtkpod
echo $outfile
exit 0
