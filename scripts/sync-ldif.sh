#! /bin/bash

# Placed under GPL; to know more, see:
# 			http://www.gnu.org/licenses/licenses.html
#
# if you make any change, I would be happy to be kept advised by
# sending an email at <sberidot at libertysurf dot fr>.
#
# Write "[sync-ldif.sh]" in the subject, otherwise the message is
# bound to be considered as spam... :(
#
#
# RESTRICTION : I haven't managed yet to read correctly data from
# ldif files when accents are used, Thunderbird writes the names
# differently...This is still Chinese for me!! :)

export LDIFAMILYNAME=contactIPOD	# Filenames will look like $LDIFAMILYNAMEXX.vcf, X=[0-9]
export IPOD_MOUNT=/mnt/ipod		# Mount point of the ipod
declare LDIFILE=addressbook.ldif	# default filename 'addressbook.ldif'
declare ENCODING=ISO-8859-15            # To try others encodings : 'iconv --list'
declare DELETE="NO"			# To delete old .vcf files by default? 'NO'!!

# Exiting function
function EXITING {
	export -n LDIFAMILYNAME
	export -n IPOD_MOUNT
}

# Please HELP!!! :)
function DISPLAY_HELP {
	cat <<- EOF
		Beware not to share the same familyname with old .vcf in the ipod directory;
		no one precaution is taken, so files will be overwritten. :(
		For the moment, I did not manage to make accents be displayed properly,
		but still working on it. Enjoy!

		Default options:
		Current Ipod directory is      : $IPOD_MOUNT
		Current Encoding               : $ENCODING
		Current template for filenames : $LDIFAMILYNAME[1-9*].vcf
		$(basename $0) doesn't delete old .vcf files by default, but overwrites.

		Options:
		Syntax : $(basename $0) -f addbook.ldif -m /media/ipod -n contactIPOD -d (-h)
		[-f] addbook.ldif  : contains the .ldif Filename
		[-m] /media/ipod   : contains the ipod Mounted directory
		[-n] contactIPOD   : contains the .vcf family fileName
		[-d]               : contains the option to Delete all old .vcf files
		[-h]               : contains this Help
	EOF
}

# $1 contains the directory to be tested
# Is/Are .vcf file(s) present?
function TST_VCF {
	local j
	for j in "${1%%/}/"* ; do
		if [ "${j##*.}" = "vcf" ] ; then
			return 0
			break
		fi
	done
	return 1
}

# $1 contains the util name
# Is $1 already installed?
function TST_UTIL {
	which "$1" >/dev/null 2>&1
	if [ "$?" != "0" ]; then
		echo "[INSTALL] $1 utility not found, please install $1 package first!"
		EXITING
		exit 3
	fi
}

# $1 contains the .vcf filename
# extracts names from .vcf filename
function EXTRACT_CONTACT_FROM_VCF {
	local NAME="UNKNOWN"
	if [ -e "$1" ] ; then
		NAME=$(grep "fn:" "$1")
		NAME=${NAME:3}
	fi
	echo "$NAME"
}

function DELETE_VCFILE_FROM_IPOD {
	# if no .vcf file's found, exit!
	if ! TST_VCF "$IPOD_MOUNT"/Contacts/ ; then
		echo "[DELETING] no file detected"
		return
	fi
	# begin to delete
	local i
	for i in "$IPOD_MOUNT"/Contacts/*.vcf ; do
		echo "[DELETING] " $(EXTRACT_CONTACT_FROM_VCF "$i") "from ${i##*/}"
		rm -f "$i"
	done
}

# Is Ipod Directory valid?
function IS_IPOD_DIR_VALID {
	# Test of the $IPOD_MOUNT directory
	if [ ! -d "$IPOD_MOUNT/Contacts/" ]; then
		echo "$IPOD_MOUNT/Contacts invalid... Exiting."
		EXITING
		exit 2
	fi
}

# Program's starting here!

# Testing awk and iconv utils...
TST_UTIL "awk"
TST_UTIL "iconv"

# picking up and processing parameters from prompt...
while getopts ":f:m:n:dh:" option ; do
	case $option in
		f )	LDIFILE="$OPTARG";;
		m )     IPOD_MOUNT="${OPTARG%%/}";;
		n )	LDIFAMILYNAME="$OPTARG";;
		d )	DELETE="OK";;
		h | * )	DISPLAY_HELP; EXITING; exit 1;;
	esac
done

# check, if not valid, exit!
IS_IPOD_DIR_VALID

# Is LDIFILE really a ldif file? just testing the extension, and if the file exists...
if [ "${LDIFILE##*.}" == "ldif" ] || [ "${LDIFILE##*.}" == "LDIF" ] && [ -e "$LDIFILE" ] ; then
	# The $IPOD_MOUNT/Contacts/ directory will be emptied if '-d' option!
	if [ "$DELETE" == "OK" ]; then
		echo "Old contacts being deleted from $IPOD_MOUNT. Work in progress..."
		DELETE_VCFILE_FROM_IPOD
		sleep 1
	fi

	echo "New contacts being synchronised from $LDIFILE. Work in progress..."
	# Translation from LDIF into VCF, in order to cut standard output in vcf files...
	# Converting into vcf stream | converting to ENCODING | detection of VCF card
	#							and writing it into the Ipod
	ldif2vcf.sh < "$LDIFILE" | iconv -f UTF-8 -t $ENCODING | awk 'BEGIN{RS="\n"; NAME=""; CARD=""; VCFILE=""; NCARD=1} /^fn:/ {NAME=substr($0,4)} /^$/{next} /^end:vcard/ {VCFILE = ENVIRON["IPOD_MOUNT"] "/Contacts/" ENVIRON["LDIFAMILYNAME"] (NCARD-1) ".vcf"; print "[WRITING] " NAME " in " ENVIRON["LDIFAMILYNAME"] (NCARD-1) ".vcf"; print CARD "end:vcard" > VCFILE; CARD=""; VCFILE=""; NCARD++; next} {CARD=CARD $0 RS} END{print (NCARD-1) " vcards added."}' # >/dev/null 2>&1

	if [ "$?" != "0" ]; then
		echo "[ERROR] An error occured, exiting, sorry for that... Please Report!"
		EXITING
		exit 1
	fi
	echo "complete!"
else
	DISPLAY_HELP
fi

EXITING
exit 0
