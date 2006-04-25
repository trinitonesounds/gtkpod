#!/bin/sh
#
# Script for syncing thunderbird addressbook data with iPod Nano
#
# It appears as if the old iPod Nano firmware only displayed the first
# entry of a vcf file (the issue doesn't seem to exist with current
# versions of the firmware). This script writes one vcf file per
# address in your address book and could therefore also be useful
# non-Nano users.
#
# (c) 2006 Paul Oremland <paul at oremland dot net>

# Usage:
# 
# sync-thunderbird-nano.sh [-i <ipod mountpoint>] [-e <encoding>]
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

IPOD_MOUNT=/mnt/ipod         # mountpoint of ipod
ENCODING=ISO-8859-15         # encoding used by ipod
NAME=thunderbird             # default file export name
FILE_FLAG=''		     # flag used to determine end of file
let COUNT=0;		     # file counter

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
# 2006/04/10 (Paul Oremland <paul at oremland dot net>):
# break vcf file out into individual vcf files per contact


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


echo "Exporting Contacts:"
SYNC="$(dirname $0)/sync-thunderbird.sh -i ${IPOD_MOUNT} -e ${ENCODING} -d ${THUNPATH} -n ${NAME}"
$SYNC

echo "Breaking apart VCF file into individual contacts"
for line in $(cat $IPOD_MOUNT/Contacts/${NAME}.vcf | sed "s/\ /[54321]/g")
do
	if [ "$FILE_FLAG" = 'END' ]; then
		let COUNT=$COUNT+1;
	fi

	echo $line | sed "s/\[54321\]/\ /g" >> $IPOD_MOUNT/Contacts/${NAME}$COUNT.vcf

	if [ "$line" = 'END:VCARD' ]; then
		echo "Finished Writing ${NAME}$COUNT.vcf"
		FILE_FLAG="END";
	else
		FILE_FLAG='';
	fi
done

echo "Removing ${NAME}.vcf"
rm $IPOD_MOUNT/Contacts/${NAME}.vcf
