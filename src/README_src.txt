Time-stamp: <2003-09-21 14:15:31 jcs>

This document is intended to give a short overview of the source
tree and make it easier for individuals to contribute to the project
without first having to work out the program concepts from the source
files.

Please feel free to add information as you please.


Data Handling
-------------

The iPod's iTunesDB is read in its entirety at the beginning of a
session. There are two data structures to keep the information:

  typedef strcut {...} Playlist;   /* in playlist.h */
  typedef struct {...} Song;       /* in song.h */

Since not all the information used by gtkpod can be stored into the
iTunesDB, a second file called "iTunesDB.ext" is used to store charset
information, MD5 checksums, original filename etc.

All songs on the iPod are kept in a doubly-linked list with
pointers to the Song structures defined above:

  GList *songs;  /* in song.c */

All playlists on the iPod are kept in a doubly-linked list with
pointers to the Playlist structures defined above:

  GList *playlists; /* in playlist.c */

Each Playlist structrue contains a double-linked list (GList *members)
with pointers to the Song structres defined above.

In accordance to the structre of iPod's iTunesDB, first all songs are
added to the songs list using functions in song.c. Only after songs
have been added, can they be linked to by the Playlist
structures. Functions in playlist.c are used to add songs to
particular playlists. Those functions also take care to propagate the
information to the display models.

GUI
---

Most of the GUI is set up using glade-2. Don't change interface.c
directly. All changes will be lost.

