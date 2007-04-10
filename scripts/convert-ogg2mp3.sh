#!/bin/sh
# Simple script that converts a ogg file into an mp3 file
#
# USAGE:
#
# convert-ogg2mp3.sh [options] oggfile
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
oggfile="$1"

# Build output file
mp3file=`basename "$oggfile"`
mp3file=${mp3file%%.ogg}
mp3file="/tmp/$mp3file.mp3"

# Default values
[ -z "$comment" ] && comment="Encoded for gtkpod with lame"

#echo "Converting \"$oggfile\" into \"$mp3file\""

# Checking input file
if [ "$oggfile" = "" ]; then
    exit 1
fi
if [ ! -f "$oggfile" ]; then
    exit 1
fi

# Checking output file
touch "$mp3file"
if [ "x$?" != "x0" ]; then
    exit 2
fi

# Check for the existence of oggdec
oggdec=`which oggdec`
if [ -z "$oggdec" ]; then
    exit 4
fi

# Check for the existence of lame
lame=`which lame`
if [ -z "$lame" ]; then
    exit 5
fi

# Check if the genre is one which lame supports
if [ -n "$genre" ] && `$lame --genre-list | grep -qi "\b$genre\b"`; then
    genreopt="--tg \"$genre\""
else
    # check for id3v2
    id3v2=`which id3v2`
    if [ -n "$id3v2" ]; then
        useid3v2=1
    fi
fi

# Launch command
exec "$oggdec" --output - -- "$oggfile" | "$lame" --preset standard --add-id3v2 --tt "$title" --ta "$artist" --tl "$album" --ty "$year" --tc "$comment" --tn "$tracknum" $genreopt - "$mp3file"

# Try to set the genre with id3v2 if needed.  This may fail as older versions of
# id3v2 did not support genre strings in place of numberic genre id's
if [ -n "$useid3v2" ]; then
    $id3v2 -g "$genre" "$mp3file" || :
fi

# Check result
if [ "x$?" != "x0" ]; then
    exit 6
fi
# Seems to be ok: display filename for gtkpod
echo $mp3file
exit 0
