#!/bin/sh
# (c) 2004 Markus Gaugusch <markus@gaugusch.at>
# Script for syncing notes with iPod

# Usage:
# 
# sync-notes.sh [-i <ipod mountpoint>] [-p <path to folder containing notes (~/ipod_notes by default)>]
#           ...        [-e <encoding>]
#
# with the following defaults: 

IPOD_MOUNT=/media/PERLIPOD                          # mountpoint of ipod
NOTESPATH=~/Desktop/Notizen		  # path to folder containing notes
ENCODING=ISO-8859-15                          # encoding used by ipod

# Unless called with "-e=none" this script requires "recode" available
# from ftp://ftp.iro.umontreal.ca/pub/recode/recode-3.6.tar.gz

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


# Changelog:
#
# 2004/06/27 (Jorg Schuler <jcsjcs at users dot sourceforge dot net>):
# Changed to accept ipod-mountpath and encoding as command line
# option
#
# Split into two files -- one for contacts, one for calendar.
#
# Placed under GPL in agreement with Markus Gaugusch.
#
# Added note about encoding used by the iPod.
#
# 2004/07/02 (Jorg Schuler <jcsjcs at users dot sourceforge dot net>):
# Added Usage-line, added check for vcard file, rearranged source
#
# 2004/07/03 (Jorg Schuler <jcsjcs at users dot sourceforge dot net>):
# Made "recode" optional (call with -e="none")
#
# Removed "dos2unix" as my iPod (firmware 1.3) happily accepted
# DOS-type vcards as well. Instead changed the "grep" pattern to
# catch \cr\lf as well.
#
# 2004/12/15 (Clinton Gormley <clint at traveljury dot com>):
# adapted sync-korganiser to synchronise a folder containing notes
#
# 2005/04/02 (Thomas Perl <thp at perli dot net>):
# * added features to remove gedit backup files (*~) from source 
#   folder before syncing, better would be to modify the find 
#   command to exclude such backup files instead of deleting them
# * added support for directories inside of notes, syncing only 
#   directories containing files
# * added check to see if recode is installed

# overwrite default settings with optional command line arguments
while getopts i:d:e: option; do
    case $option in
        i) IPOD_MOUNT=$OPTARG;;
        d) NOTESPATH=$OPTARG;;
        e) ENCODING=$OPTARG;;
        \?) echo "Usage: `basename $0` [-i <ipod mountpoint>] [-p <path to folder containing notes (~/ipod_notes by default)>] [-e <encoding>]"
	    exit 1;;
    esac
done


# set the RECODE command
if [ $ENCODING = "none" ] || [ $ENCODING = "NONE" ]; then
    RECODE="cat"    # no conversion
else
    which recode >/dev/null 2>&1
    if [ "$?" != "0" ]; then
      echo "recode utility not found. please install 'recode'."
      exit
    fi
    RECODE="recode UTF8..$ENCODING"
fi


# check if NOTESPATH exists
if [ ! -d $NOTESPATH ]; then
    echo "Error: folder $NOTESPATH does not exist"
    exit 1
fi

# check if iPod mountpoint exists
if [ ! -d $IPOD_MOUNT/Notes ]; then
    echo "Error: Cannot find iPod at $IPOD_MOUNT"
    exit 1
fi


# remove existing files on iPod and transfer (recode if necessary) all current files to iPod
# Seeing all notes should be under 4K, easier to delete and recopy rather than checking all files sizes

echo -n "Syncing iPod ... [Notes] "
rm -rf $IPOD_MOUNT/Notes/*
rm -f $NOTESPATH/*~
cd $NOTESPATH

I=0

find -type f |
(
  read FILE
  while [ "$?" == "0" ]; do
	((++I))
	mkdir -p "$IPOD_MOUNT/Notes/`dirname "$FILE"`/"
	cat "$FILE" | $RECODE > "$IPOD_MOUNT/Notes/$FILE"
	read FILE
  done
  echo  
  case $I in 
	0) echo "No notes found to copy";;
	1) echo "1 note copied";;
	*) echo "$I notes copied";;
  esac
)

echo "done!"
