#!/bin/sh
# Script for syncing evolution addressbook data with iPod
# (c) 2004 Clinton Gormley <clint at traveljury dot com>
#

# Usage:
# 
# sync-addressbooks.sh [-i <ipod mountpoint>] [-e <encoding>]
#                      [-d <path to evolution-addressbook-export>]
#
# (specify '-d' if evolution-addressbook-export is not in your default
# path)
#
# with the following defaults: 

IPOD_MOUNT=/mnt/ipod         # mountpoint of ipod
EVOPATH='/opt/gnome/libexec/evolution/2.0:/usr/lib/evolution/2.0:/opt/gnome/bin'                        # additional path
ENCODING=ISO-8859-15         # encoding used by ipod

# Unless called with "-e=none" this script requires "iconv" available
# from http://www.gnu.org/software/libiconv/

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
# 2004/12/07 (Clinton Gormley <clint at traveljury dot com>):
# adapted sync-kaddressbook to work with evolution.
#
# 2004/12/15 (Jorg Schuler <jcsjcs at users dot sourceforge dot net>):
# Split evolution support into a new file.


# overwrite default settings with optional command line arguments
while getopts i:d:e: option; do
    case $option in
        i) IPOD_MOUNT=$OPTARG;;
        d) EVOPATH=$OPTARG;;
        e) ENCODING=$OPTARG;;
        
        \?) echo "Usage: `basename $0 ` [-i <ipod mountpoint>] [-e <encoding>] [-d <path to evolution-addressbook-export>]"
	    exit 1;;
    esac
done


# set the RECODE command
if [ $ENCODING = "none" ] || [ $ENCODING = "NONE" ]; then
    RECODE="cat"    # no conversion
else
    RECODE="iconv -f UTF-8 -t $ENCODING"
fi


echo "Trying to export from evolution:"

# adjust path to find evolution-addressbook-export
PATH=$EVOPATH:$PATH

# check if evolution-addressbook-export is in PATH
evolution-addressbook-export --help  >/dev/null 2>&1

if [ ! $? == 0 ]; then
	echo "** Error: Couldn't find evolution-addressbook-export in your PATH:"
	echo $PATH
	echo ""
	echo "Please specify \"-d <path to evolution-addressbook-export>\" when you call this script"
    exit 1
fi


# remove all empty lines and recode if necessary
echo -n "Syncing iPod ... [Contacts] "
# Redirect STDERR to avoid the "FIXME : wait for completion unimplemented" warning
evolution-addressbook-export 2>&1 | grep -v '^[[:space:]]$\|^$' | $RECODE > $IPOD_MOUNT/Contacts/evolution
echo "done!"
