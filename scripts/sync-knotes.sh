#!/bin/bash
# (c) 2005 Sebastian Scherer <basti _at_ andrew.cmu.edu>
# Script for syncing knotes with iPod.
# Recode and options adapted from sync-notes.sh
#
# sync-notes.sh [-i <ipod mountpoint>] [-e <encoding>]
#
# defaults:
IPOD_MOUNT=/media/iPod
ENCODING=ISO-8859-15                          # encoding used by ipod

#First start knotes if it is not started already:
if dcop knotes 2>/dev/null 1>/dev/null
then
   :
else
    knotes 2>/dev/null 1>/dev/null
fi


#Set options if other parameters given.
while getopts i:e: option; do
    case $option in
        i) IPOD_MOUNT=$OPTARG;;
        e) ENCODING=$OPTARG;;
        \?) echo "Usage: `basename $0` [-i <ipod mountpoint>] [-e <encoding>]"
	    exit 1;;
    esac
done

# set the RECODE command
if [ $ENCODING = "none" ] || [ $ENCODING = "NONE" ]; then
    RECODE="cat"    # no conversion
else
    which iconv >/dev/null 2>&1
    if [ "$?" != "0" ]; then
      echo "iconv utility not found. please install 'iconv'."
      exit
    fi
    RECODE="iconv -f UTF-8 -t $ENCODING"
fi

# check if iPod mountpoint exists
if [ ! -d $IPOD_MOUNT/Notes ]; then
    echo "Error: Cannot find iPod at $IPOD_MOUNT"
    exit 1
fi

echo -n "Syncing iPod ... [Notes] "
#Remove old notes
mkdir -p $IPOD_MOUNT/Notes
rm -rf $IPOD_MOUNT/Notes/*

#Add new notes
INDICES=`dcop knotes KNotesIface notes| awk --field-separator '->' '{print $1}'`
TITLES=`dcop knotes KNotesIface notes | awk --field-separator '->' '{print $2}'`
COUNT=1
for INDEX in $INDICES
do
  TITLE=`echo "$TITLES" | sed "$COUNT!d"`
#  echo "$COUNT Index: $INDEX, Title: $TITLE"
  TEXT=`dcop knotes KNotesIface text $INDEX`
  #echo "Text: $TEXT"
  TITLE=`echo "$TITLE" | sed 's/\W/./g'`
  echo "$TEXT" | $RECODE > "$IPOD_MOUNT/Notes/${COUNT}-${TITLE}"
  COUNT=`expr $COUNT + 1`
done

echo "done!"
