#!/bin/sh
# Script for syncing thunderbird addressbook data with iPod
# (c) 2004 Clinton Gormley <clint at traveljury dot com>
#

# Usage:
# 
# sync-thunderbird.sh [-i <ipod mountpoint>] [-e <encoding>]
#                      [-d <path to thunderbird address book>]
#                      [-n <name of exported file>]
#
# specify '-d' if your thunderbird address book is not in
# ~/.thunderbird/
#
# specify '-n' if you want to export more than one address book
# (otherwise the second call to this script will overwrite the output
# of the first call)

# with the following defaults: 

IPOD_MOUNT=/media/ipod         # mountpoint of ipod
ENCODING=ISO-8859-15         # encoding used by ipod
NAME=thunderbird             # default file export name

# Unless called with "-e=none" this script requires "recode" available
# from ftp://ftp.iro.umontreal.ca/pub/recode/recode-3.6.tar.gz

# About the encoding used by the iPod (by Jorg Schuler):
#
# For some reason the encoding used for the contact files and
# calender files depends on the language you have set your iPod
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


# Changelog:
#
# 2005/05/18 (Clinton Gormley <clint at traveljury dot com>):
# adapted sync-kaddressbook to work with thunderbird.
#
# 2004/12/15 (Jorg Schuler <jcsjcs at users dot sourceforge dot net>):
# Split evolution support into a new file.
#
# 2005/06/23 (Jorg Schuler <jcsjcs at users dot sourceforge dot net>):
# use 'iconv' instead of 'recode'


# overwrite default settings with optional command line arguments
while getopts i:d:e:n: option; do
    case $option in
        i) IPOD_MOUNT=$OPTARG;;
        d) THUNPATH=$OPTARG;;
        e) ENCODING=$OPTARG;;
	n) NAME=$OPTARG;;
        
        \?) echo "Usage: `basename $0 ` [-i <ipod mountpoint>] [-e <encoding>] [-d <path to thunderbird address book>] [-n <name of exported file>]"
	    exit 1;;
    esac
done


# set the RECODE command
if [ $ENCODING = "none" ] || [ $ENCODING = "NONE" ]; then
    RECODE="cat"    # no conversion
else
    RECODE="iconv -f UTF-8 -t $ENCODING"
fi


echo "Trying to export from thunderbird:"

MAB2VCARD=`dirname $0`/mab2vcard

# remove all empty lines and recode if necessary
echo -n "Syncing iPod ... [Contacts] "
$MAB2VCARD $THUNPATH | grep -v '^[[:space:]]$\|^$' | $RECODE > $IPOD_MOUNT/Contacts/${NAME}.vcf
echo "done!"
