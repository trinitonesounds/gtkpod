#!/bin/sh
# (c) 2004 Markus Gaugusch <markus@gaugusch.at>
# Script for syncing evolution calendar and task data with iPod

# Usage:
# 
# sync-evocalendar.sh [-i <ipod mountpoint>] [-e <encoding>]
#           ...       [-c <evolution calendar file>] [-t <evolution tasks file>]
#
# with the following defaults: 

IPOD_MOUNT=/mnt/ipod                          # mountpoint of ipod
CALFILE=~/.evolution/calendar/local/system/calendar.ics # calendar data file
TASKFILE=~/.evolution/tasks/local/system/tasks.ics # task data file
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
# adapted sync-korganiser to work with evolution.


# overwrite default settings with optional command line arguments
while getopts i:d:e: option; do
    case $option in
        i) IPOD_MOUNT=$OPTARG;;
        c) CALFILE=$OPTARG;;
        t) TASKFILE=$OPTARG;;
        e) ENCODING=$OPTARG;;
        \?) echo "Usage: `basename $0` [-i <ipod mountpoint>] [-c <evolution calendar file>] [-t <evolution tasks file>] [-e <encoding>]"
	    exit 1;;
    esac
done


# set the RECODE command
if [ $ENCODING = "none" ] || [ $ENCODING = "NONE" ]; then
    RECODE="cat"    # no conversion
else
    RECODE="recode UTF8..$ENCODING"
fi


# check if CALFILE exists
if [ ! -f $CALFILE ]; then
    echo "Error: $CALFILE does not exist"
    exit 1
fi

# check if TASKFILE exists
if [ ! -f $TASKFILE ]; then
    echo "Error: $TASKFILE does not exist"
    exit 1
fi


# remove all empty lines and recode if necessary
echo -n "Syncing iPod ... [Calendar] "
cat $CALFILE $TASKFILE | grep -v '^[[:space:]]$\|^$' | $RECODE > $IPOD_MOUNT/Calendars/evolution
echo "done!"
