#!/bin/bash
#
############################################################################
#                                                                          #
# this script will export the contacts stored and maintained by the KDE    #
# contact application kaddressbook. The file used by kaddressbook to store #
# its contacts is a sinlge vcard file, usually                             #
# "$KDEHOME/share/apps/kabc/std.vcf".                                      #
# This script will take this or any other file being used by kaddressbook  #
# and split the single vcard file into a single file for each contact.     #
#                                                                          #
# written by Juergen Helmers <helmerj@openoffice.org>  (c) GNU GPL         #
#                                                                          #
############################################################################

# Changelog:
#
# Changed to accept ipod-mountpath as command line option: Jorg Schuler
# <jcsjcs at users dot sourceforge dot net>



# default kaddressbook data file
datafile=$HOME/.kde/share/apps/kabc/std.vcf
# default ipod mountpoint
ipod_mount=/mnt/ipod

# overwrite default settings with optional command line arguments
while getopts i:d: option; do
    case $option in
	i) ipod_mount=$OPTARG;;
	d) datafile=$OPTARG;;
        \?) echo "usage: [-i <ipod mountpoint>] [-d <kaddress data file>]";;
    esac
done

# target directory on mounted iPod
target=$ipod_mount/Contacts

#determine the number of lines in the datafile
max=`wc $datafile|awk '{printf $1}'`

#set counters and the numbering prefix
counter=1
vcard_num=1
prefix=00000
tmp=$prefix$vcard_num
vcard=${tmp:(-6)}


#loop over the datafile line by line
while [ $counter -le $max ]
do
line="`head -$counter $datafile | tail -1`"

#check if we have a new contact
if [ "`echo $line|cut -d: -f1`" = "BEGIN" ] ; then
	echo $line > $target/vcard$vcard.vcf
	counter=`expr $counter + 1`

#check if we have the end of a contact
elif [ "`echo $line|cut -d: -f1`" = "END" ] ; then
	echo $line >> $target/vcard$vcard.vcf
	counter=`expr $counter + 1`
	vcard_num=`expr $vcard_num + 1`
	tmp=$prefix$vcard_num
	vcard=${tmp:(-6)}
	echo "exporting Contact number $vcard"

#check if we have an empty line
elif [ "$line" = "" ] ; then
	counter=`expr $counter + 1`

#default, write the line to the vCard file
else
	echo $line >> $target/vcard$vcard.vcf
	counter=`expr $counter + 1`
fi

done
