#!/usr/bin/python
# Script for syncing Palm addressbook data with iPod via Jppy
# (c) 2005 Nick Piper <nick at nickpiper dot co dot uk>
#

# Usage:
# 
# sync-jppy.py [-i <ipod mountpoint>] [-e <encoding>]
#
#
# with the following defaults: 

IPOD_MOUNT="/media/ipod"         # mountpoint of ipod
ENCODING="ISO-8859-15"         # encoding used by ipod

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


from optparse import OptionParser
import os
import jppy
import glob

parser = OptionParser()
parser.add_option("-i", "--ipod", dest="mountpoint",default=IPOD_MOUNT,
                  help="use iPod mounted at DIR", metavar="DIR")
parser.add_option("-e", "--encoding",
                  dest="encoding", default=ENCODING,
                  help="encode to ENCODING")
parser.add_option("-q", "--quiet",
                  action="store_false", dest="verbose", default=True,
                  help="don't print status messages to stdout")

(options, args) = parser.parse_args()

if options.verbose:
    print "Trying to export from Jppy:"

dir = os.path.join(options.mountpoint,"Contacts")
if not os.path.exists(dir):
    parser.error("Are you sure the iPod is at %s?" % options.mountpoint)

ab=jppy.addressBook()

n=0
for r in ab.records():
    n=n+1
    card=r.vcard().decode('palmos').encode(options.encoding)
    open(os.path.join(dir,"jppy-%d.vcf" % n),"w").write(card)

r=0
dirnamelen=len(os.path.join(os.path.join(dir,"jppy-")))
for fn in glob.glob(os.path.join(dir,"jppy-*.vcf")):
    if int(fn[dirnamelen:-4]) > n:
        r=r+1
        os.unlink(fn)
        
if options.verbose:
    print "Saved %d records, removed %d records." % (n,r)
