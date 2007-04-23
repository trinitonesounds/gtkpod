#!/bin/sh
# Simple script that converts a ogg file into an mp3 file
#
# USAGE:
#
# convert-ogg2mp3.sh [options] oggfile
#
#       -x      Print converted filename extension and exit
#       -f      Set converted filename
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
#   7 unknown option

# Constants
extension="mp3"

# Get parameters
while getopts a:A:T:t:g:c:y:f:x opt ; do
	case "$opt" in
	        x)      echo "$extension"; exit 0 ;;
	        f)      outfile="$OPTARG" ;;
		a)	artist="$OPTARG" ;;
		A)	album="$OPTARG" ;;
		T)	track="$OPTARG" ;;
		t)	title="$OPTARG" ;;
		g)	genre="$OPTARG" ;;
		y)	year="$OPTARG" ;;
		c)	comment="$OPTARG" ;;
	        ?)      exit 7 ;;
	esac
done
shift $(($OPTIND - 1))
infile="$1"

if [ "$outfile" = "" ]; then
    # Build output file
    outfile=`basename "$infile"`
    outfile=${outfile%%.flac}
    outfile="/tmp/$outfile.$extension"
fi

# Default values
[ -z "$comment" ] && comment="Encoded for gtkpod with lame"

#echo "Converting \"$infile\" into \"$outfile\""

# Checking input file
if [ "$infile" = "" ]; then
    exit 1
fi
if [ ! -f "$infile" ]; then
    exit 1
fi

# Checking output file
touch "$outfile"
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
exec "$oggdec" --output - -- "$infile" | "$lame" --preset standard --add-id3v2 --tt "$title" --ta "$artist" --tl "$album" --ty "$year" --tc "$comment" --tn "$tracknum" $genreopt - "$outfile"

# Check result
if [ "x$?" != "x0" ]; then
    exit 6
fi

# Try to set the genre with id3v2 if needed.  This may fail as older versions of
# id3v2 did not support genre strings in place of numeric genre id's
if [ -n "$useid3v2" ]; then
    $id3v2 -g "$genre" "$outfile" || :
fi

# Seems to be ok: display filename for gtkpod
echo $outfile
exit 0
