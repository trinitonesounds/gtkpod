#!/bin/sh
# Simple script that converts a flac file into an mp3 file
#
# USAGE:
#
# convert-flac2mp3.sh [options] flacfile
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
mp3file=`basename "$flacfile"`
mp3file=${mp3file%%.flac}
mp3file="/tmp/$mp3file.mp3"

# Default values
[ -z "$comment" ] && comment="Encoded for gtkpod with lame"

#echo "Converting \"$flacfile\" into \"$mp3file\""

# Checking input file
if [ "$flacfile" = "" ]; then
    exit 1
fi
if [ ! -f "$flacfile" ]; then
    exit 1
fi

# Checking output file
touch "$mp3file"
if [ "x$?" != "x0" ]; then
    exit 2
fi

# Check for the existence of flac
flac=`which flac`
if [ -z "$flac" ]; then
    exit 4
fi

# Check for the existence of lame
lame=`which lame`
if [ -z "$lame" ]; then
    exit 5
fi

# Launch command
exec "$flac" -d -c -- "$flacfile" | "$lame" --preset standard --add-id3v2 --tt "$title" --ta "$artist" --tl "$album" --ty "$year" --tc "$comment" --tn "$tracknum" --tg "$genre" - "$mp3file"

# Check result
if [ "x$?" != "x0" ]; then
    exit 6
fi
# Seems to be ok: display filename for gtkpod
echo $mp3file
exit 0
