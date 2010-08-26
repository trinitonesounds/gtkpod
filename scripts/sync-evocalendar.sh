#!/bin/sh
# (c) 2004 Markus Gaugusch <markus@gaugusch.at>
# Script for syncing evolution calendar and task data with iPod

# Usage:
# 
# sync-evocalendar.sh [-i <ipod mountpoint>] [-e <encoding>]
#           ...       [-c <evolution calendar file>] [-t <evolution tasks file>]
#           ...       [-f <ical filter script>]
#
# with the following defaults: 

IPOD_MOUNT=/media/ipod                          # mountpoint of ipod

#the path to a script that will be passed the ical information from STDIN and filter, if needed
#FILTER_SCRIPT=

#get all the local evolution calendars
CALFILES=`find ~/.evolution/calendar/local/ -name "calendar.ics"`

#get all the local evolution tasks
TASKFILES=`find ~/.evolution/tasks/local/ -name "tasks.ics"`

ENCODING=ISO-8859-15                          # encoding used by ipod

# Unless called with "-e none" this script requires "iconv" available
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


# Changelog:
#
# 2006/03/08 (Michele C. Soccio <michele at soccio dot it>:
# Changed to get all the local calendar and task files
#
# Changed to correct the calendar file(s) and task file(s) command line option
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
# Made "iconv" optional (call with "-e none")
#
# Removed "dos2unix" as my iPod (firmware 1.3) happily accepted
# DOS-type vcards as well. Instead changed the "grep" pattern to
# catch \cr\lf as well.
#
# 2004/12/15 (Clinton Gormley <clint at traveljury dot com>):
# adapted sync-korganiser to work with evolution.


# overwrite default settings with optional command line arguments
while getopts i:c:t:e: option; do
    case $option in
	i) IPOD_MOUNT=$OPTARG;;
        c) CALFILES=$OPTARG;;
        t) TASKFILES=$OPTARG;;
        e) ENCODING=$OPTARG;;
        f) FILTER=$OPTARG;;
		\?) echo "Usage: `basename $0` [-i <ipod mountpoint>] [-c <evolution calendar file>] [-t <evolution tasks file>] [-f <filter script>] [-e <encoding>]"
	    exit 1;;
    esac
done


# set the RECODE command
if [ $ENCODING = "none" ] || [ $ENCODING = "NONE" ]; then
    RECODE="cat"    # no conversion
else
    RECODE="iconv -f UTF-8 -t $ENCODING"
fi

if [ $FILTER_SCRIPT ]; then
	FILTER=$FILTER_SCRIPT
else
	FILTER="cat"
fi

# check if CALFILES exist
for i in $CALFILES; do
	if [ ! -f $CALFILE ]; then
	    echo "Error: $i does not exist"
	    exit 1
	fi
done

# check if each TASKFILES exist
for j in $TASKFILES; do
	if [ ! -f $j ]; then
	    echo "Error: $TASKFILE does not exist"
	    exit 1
	fi
done

echo $CALFILES
echo $TASKFILES

# remove all empty lines and recode if necessary
echo -n "Syncing iPod ... [Calendar] "
	cat $CALFILES $TASKFILES | grep -v '^[[:space:]]$\|^$' | $FILTER |  $RECODE > $IPOD_MOUNT/Calendars/evolution
echo "done!"
