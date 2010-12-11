#!/bin/sh
# Script that converts a file into an mp3 or an m4a file
#
# USAGE:
#
# Called by convert-2mp3.sh, convert-2m4a.sh, convert-2mp4.sh.
#
# The following command line arguments are being used
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
#	-q	Quality (overwrites standard encoder options)
#               For mp3 files this could be "--vbr-new -V<number>".
#
# Return Codes:
#   0 ok
#   1 input file not found
#   2 output file cannot be created
#   3 cannot get info
#   4 cannot exec decoding
#   5 cannot exec encoding
#   6 conversion failed
#   7 unknown option

# Get parameters
while getopts q:a:A:T:t:g:c:y:f:x opt ; do
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
		q)	ENCODER_OPTS="$OPTARG" ;;
	        ?)      exit 7 ;;
	esac
done
shift $(($OPTIND - 1))

infile="$1"
infile_extension=${infile##*.}

if [ "$outfile" = "" ]; then
    # Build output file
    outfile=`basename "$infile"`
    outfile=${outfile%.$infile_extension}
    tmp=${TMPDIR-/tmp}
    outfile="$tmp/$outfile.$extension"
fi

# Default values
[ -z "$comment" ] && comment="Encoded for gtkpod with $ENCODER"

# echo "Converting \"$infile\" into \"$outfile\""

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

# Check for the existence of encoder
encoder=`which $ENCODER`
if [ -z "$encoder" ]; then
    exit 5
fi

# Determine decoder

# Convert the source extension to lowercase.
filetype=`echo ${infile_extension}| tr '[:upper:]' '[:lower:]'`
case "$filetype" in
	flac)	decoder="flac" ; options="-d -c --"  ;;
	oga|ogg|ogv|ogx)
	        # Quiet mode is needed to workaround a bug in oggdec
	        # 1.4 which prints the version banner to stdout, which
	        # then corrupts the decoded track.
		decoder="oggdec" ; options="--quiet --output - --" ;;
	m4a)	decoder="faad" ; options="-o -" ;;
	wav)	decoder="" ;;
	*)	decoder="ffmpeg" ;;
esac

# Check for the existence of decoder
if [ $decoder ]; then
    decoder=`which "$decoder"`
    if [ -z "$decoder" ]; then
        exit 4
    fi
fi
