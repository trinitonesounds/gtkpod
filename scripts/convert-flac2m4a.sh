#!/bin/sh
# Simple script that converts a flac file into an m4a file
#
# USAGE:
#
# convert-flac2m4a.sh [options] flacfile
#
# 	-a	Artist tag
#	-A 	Album tag
#	-T	Track tag
#	-t	Title tag
#	-g	Genre tag
#	-y	Year tag
#	-c	Comment tag
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

# Get parameters
while getopts a:A:T:t:g:c:y: opt ; do
	case "$opt" in
		a)	artist="$OPTARG" ;;
		A)	album="$OPTARG" ;;
		T)	track="$OPTARG" ;;
		t)	title="$OPTARG" ;;
		g)	genre="$OPTARG" ;;
		y)	year="$OPTARG" ;;
		c)	comment="$OPTARG" ;;
	esac
done
shift $(($OPTIND - 1))
flacfile="$1"

# Build output file
m4afile=`basename "$flacfile"`
m4afile=${m4afile%%.flac}
m4afile="/tmp/$m4afile.m4a"

# Default values
[ -z "$comment" ] && comment="Encoded for gtkpod with faac"

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

# Check for the existence of flac
flac=`which flac`
if [ -z "$flac" ]; then
    exit 4
fi

# Check for the existence of faac
faac=`which faac`
if [ -z "$faac" ]; then
    exit 5
fi

# Launch command
exec "$flac" -d -c -- "$flacfile" | "$faac" -o "$m4afile" -q 150 -c 22000 -w --artist "$artist" --title "$title" --year "$year" --album "$album" --track "$tracknum" --genre "$genre" --comment "$comment" -

# Check result
if [ "x$?" != "x0" ]; then
    exit 6
fi
# Seems to be ok: display filename for gtkpod
echo $m4afile
exit 0
