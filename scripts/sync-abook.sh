#!/bin/sh
# (c) 2005 Daniel Kercher 
# Script to sync the ipod with abook

# Usage:
# 
# sync-abook.sh [-i <ipod mountpoint>] [-d <abook database>]
#           ... [-e <encoding ipod>] [-f <encoding abook>]
#
# with the following defaults: 

IPOD_MOUNT='/mnt/ipod'				# mount point of ipod
DATAFILE='/home/daniel/.abook/addressbook'	# the abook db
ENCODING_FROM=UTF-8                             # encoding used by abook
ENCODING=ISO-8859-15                            # encoding used by ipod

# Unless called with "-e=none" this script requires "iconv" available
# from http://www.gnu.org/software/libiconv/

# About the encoding used by the iPod (by Jorg Schuler):
#
# For some reason the encoding used for the contact files and
# calenader files depends on the language you have set your iPod
# to. If you set your iPod to German, iso-8859-15 (or -1?) is
# expected. If you set it to Japanese, SHIFT-JIS is expected. You need
# to reboot the iPod to have the change take effect, however. (I'm
# using firmware version 1.3.)
#
# If you know of more encodings, please let me know, so they can be
# added here:
#
# iPod language      encoding expected
# ----------------------------------------
# German             ISO-8859-15
# Japanese           SHIFT-JIS

# Changelog
# 2005/06/15 (Jorg Schuler <jcsjcs at users dot sourceforge dot net>):
# Received original script from Daniel Kercher and added character
# conversion and command line options.

[ -f $DATAFILE ] || exit 1

# overwrite default settings with optional command line arguments
while getopts i:d:e:f: option; do
    case $option in
        i) IPOD_MOUNT=$OPTARG;;
        d) DATAFILE=$OPTARG;;
        e) ENCODING=$OPTARG;;
	f) ENCODING_FROM=$OPTARG;;
        \?) echo "Usage: `basename $0` [-i <ipod mountpoint>] [-d <abook database>] [-e <encoding ipod>] [-f <encoding abook>]"
	    exit 1;;
    esac
done

# set the RECODE command
if [ $ENCODING = "none" ] || [ $ENCODING = "NONE" ]; then
    RECODE="cat"    # no conversion
else
    RECODE="iconv -f $ENCODING_FROM -t $ENCODING"
fi


# check if DATAFILE exists
if [ ! -f $DATAFILE ]; then
    echo "Error: $DATAFILE does not exist"
    exit 1
fi

echo -n "Syncing abook to iPod... "
abook --convert --infile $DATAFILE --outformat gcrd | sed '/^$/d' | $RECODE > ${IPOD_MOUNT}/Contacts/abook.vcf
echo "done!"
