#!/bin/sh
# (c) 2005 Daniel Kercher
# sync ipod with webcalendar

# Usage:
# 
# sync-webcalendar.sh [-i <ipod mountpoint>] [-d <webcalendar uri>]
#
# with the following defaults: 

IPOD_MOUNT='/mnt/ipod'				# mount point of ipod
DATAFILE='https://native.intern.net/projekte/webcalendar/publish.php/daniel.ics'	# uri for webcalendar

WGET_OPTIONS='--no-check-certificate'							# special options for wget

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
# Received original script from Daniel Kercher and added command line
# options
# FIXME: some way to convert the character set

# overwrite default settings with optional command line arguments
while getopts i:d: option; do
    case $option in
        i) IPOD_MOUNT=$OPTARG;;
        d) DATAFILE=$OPTARG;;
        \?) echo "Usage: `basename $0` [-i <ipod mountpoint>] [-d <webcalendar uri>]"
	    exit 1;;
    esac
done

echo -n "Syncing webcalendar to iPod... "
wget -q $WGET_OPTIONS -O ${IPOD_MOUNT}/Calendars/webcalendar.ics $DATAFILE
echo "done!"
