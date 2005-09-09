#! /usr/bin/awk -f

function INIT_VAR() {
	FIRSTNAME="FIRST_NAME"
	FAMILYNAME="FAMILY_NAME"
	NICKNAME=""
	MAIL=""
	SECONDMAIL=""
	WORKPHONE=""
	HOMEPHONE=""
	CELLPHONE=""
	ADDRESS=""
	CITY=""
	ZIPCODE=""
	MISC=""
	MISC1=""
	MISC2=""
	MISC3=""
	MISC4=""
	URLSITE=""

	URL=""
	DISPLAY=""}

BEGIN {	INIT_VAR()}

/^givenName:/,/^$/ { DISPLAY="OK" }

/^givenName:/ { FIRSTNAME = substr($0, 12)}

/^sn:/ { FAMILYNAME = substr($0, 5)}

/^xmozillanickname:/ { NICKNAME = substr($0, 19)}

/^mail:/ { MAIL = substr($0, 7)}

/^mozillaSecondEmail:/ { SECONDMAIL = substr($0, 21)}

/^homePhone:/ { HOMEPHONE = substr($0, 12)}

/^mobile:/ { CELLPHONE = substr($0, 9)}

/^telephoneNumber:/ { WORKPHONE = substr($0, 18) }

/^homePostalAddress:/ { ADDRESS = substr($0, 20)}

/^mozillaHomeLocalityName:/ { CITY = substr($0, 26)}

/^mozillaHomePostalCode:/ { ZIPCODE = substr($0, 24)}

/^homeurl:/ { URLSITE = substr($0, 10)}

/^custom1:/ { MISC1 = substr($0, 10)}

/^custom2:/ { MISC2 = substr($0, 10)}

/^custom3:/ { MISC3 = substr($0, 10)}

/^custom4:/ { MISC4 = substr($0, 10)}

/^$/ && DISPLAY == "OK" {
	# making up the Vcard
	print "begin:vcard"
	print "version:3.0"
	print "n:" FAMILYNAME ";" FIRSTNAME ";;;"
	print "fn:" FIRSTNAME " " FAMILYNAME
	if ( NICKNAME ) {
		print "nickname:" NICKNAME }

	if ( MAIL || URLSITE || SECONDMAIL ) {
		MAIL?URL="mailto:" MAIL:URL
		if ( SECONDMAIL ) {
			URL?URL=URL "\\n(mailto:" SECONDMAIL ")":URL=URL "(mailto:" SECONDMAIL ")" }
		if ( URLSITE ) {
			URL?URL=URL "\\n(url:" URLSITE ")":URL=URL "(url:" URLSITE ")" }

		print "url;type=work:" URL }

	if ( HOMEPHONE ) {
		print "tel;type=home:" HOMEPHONE }

	if ( WORKPHONE ) {
		print "tel;type=work:" WORKPHONE }

	if ( CELLPHONE ) {
		print "tel;type=cell:" CELLPHONE }

	if ( ADDRESS && ZIPCODE && CITY ) {
		print "adr;type=home:;;" ADDRESS ";" ZIPCODE ";" CITY ";;;"
		print "label;type=home:" ADDRESS "\\n" ZIPCODE " " CITY }

	if ( MISC1 || MISC2 || MISC3 || MISC4 ) {
		MISC1?MISC=MISC1:MISC
		if ( MISC2 ) {
			MISC?MISC=MISC "\\n" MISC2:MISC=MISC MISC2 }
		if ( MISC3 ) {
			MISC?MISC=MISC "\\n" MISC3:MISC=MISC MISC3 }
		if ( MISC4 ) {
			MISC?MISC=MISC "\\n" MISC4:MISC=MISC MISC4 }
		
		print "note:" MISC }

	print "end:vcard"

	INIT_VAR() }

END {exit 0}
