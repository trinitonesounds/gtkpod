#!/bin/sh
# Simple script that converts an mp3 file into an m4a file
#
# USAGE:
#
# convert-mp32m4a.sh [options] mp3file
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
while getopts a:A:T:t:g:c: opt ; do
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
mp3file="$1"

# Build output file
m4afile=`basename "$mp3file"`
m4afile=${m4afile%%.mp3}
m4afile="/tmp/$m4afile.m4a"

# Default values
[ -z "$comment"] && comment="Encoded for gtkpod with faac"

#echo "Converting \"$m4afile\" into \"$mp3file\""

# Checking input file
if [ "$mp3file" = "" ]; then
    exit 1
fi
if [ ! -f "$mp3file" ]; then
    exit 1
fi

# Checking output file
touch "$m4afile"
if [ "x$?" != "x0" ]; then
    exit 2
fi

# Check for the existence of lame
lame=`which lame`
if [ -z "$lame" ]; then
    exit 4
fi

# Check for the existence of faac
faac=`which faac`
if [ -z "$faac" ]; then
    exit 5
fi

# Launch command
exec "$lame" --decode "$mp3file" - | "$faac" -o "$m4afile" -q 150 -c 22000 -w --artist "$artist" --title "$title" --year "$year" --album "$album" --track "$tracknum" --genre "$genre" --comment "$comment" -

# Check result
if [ "x$?" != "x0" ]; then
    exit 6
fi
# Seems to be ok: display filename for gtkpod
echo $m4afile
exit 0
