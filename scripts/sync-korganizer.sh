#!/bin/bash
# (c) 2004 Markus Gaugusch <markus@gaugusch.at>
# Script for syncing korganizer data with iPod
#
# Changelog:
#
# 2004/06/27:
# Changed to accept ipod-mountpath and encoding as command line
# option
#
# Split into two files -- one for contacts, one for calendar.
#
# Placed under GPL in agreement with Markus Gaugusch.
#
# Added note about encoding used by the iPod.
#
# -- Jorg Schuler <jcsjcs at users dot sourceforge dot net>.


# About the encoding used by the iPod:
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


ENCODING=ISO-8859-15                    # encoding used by ipod
CF=~/.kde/share/apps/korganizer/std.ics # calendar file
IPOD_MOUNT=/mnt/ipod                    # mountpoint of ipod

# overwrite default settings with optional command line arguments
while getopts i:d:e: option; do
    case $option in
        i) IPOD_MOUNT=$OPTARG;;
        d) CF=$OPTARG;;
        e) ENCODING=$OPTARG;;
        \?) echo "usage: [-i <ipod mountpoint>] [-d <korganizer data file>] [-e <encoding>]";;
    esac
done


echo -n "Syncing iPod ... [Calendar] "
cat $CF |dos2unix | grep -v '^$' | recode UTF8..$ENCODING > $IPOD_MOUNT/Calendars/`basename $CF`
echo "done!"
