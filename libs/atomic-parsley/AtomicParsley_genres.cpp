//==================================================================//
/*
 AtomicParsley - AtomicParsley_genres.cpp

 AtomicParsley is GPL software; you can freely distribute,
 redistribute, modify & use under the terms of the GNU General
 Public License; either version 2 or its successor.

 AtomicParsley is distributed under the GPL "AS IS", without
 any warranty; without the implied warranty of merchantability
 or fitness for either an expressed or implied particular purpose.

 Please see the included GNU General Public License (GPL) for
 your rights and further details; see the file COPYING. If you
 cannot, write to the Free Software Foundation, 59 Temple Place
 Suite 330, Boston, MA 02111-1307, USA.  Or www.fsf.org

 Copyright �2005-2006 puck_lock

 ----------------------
 Code Contributions by:

 * Mellow_Flow - fix genre matching/verify genre limits
 */
//==================================================================//
//#include <sys/types.h>
//#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "AP_commons.h"     //just so win32/msvc can get uint8_t defined
#include "AtomicParsley.h"  //for stiks & sfIDs
//////////////

static const char* ID3v1GenreList[] =
    {
        "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age",
        "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae", "Rock", "Techno", "Industrial", "Alternative", "Ska",
        "Death Metal", "Pranks", "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk", "Fusion",
        "Trance", "Classical", "Instrumental", "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise", "AlternRock",
        "Bass", "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic",
        "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy",
        "Cult", "Gangsta", "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret", "New Wave",
        "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro",
        "Musical", "Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing", "Fast Fusion", "Bebob",
        "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock", "Progressive Rock", "Psychedelic Rock",
        "Symphonic Rock", "Slow Rock", "Big Band", "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech",
        "Chanson", "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus", "Porn Groove", "Satire",
        "Slow Jam", "Club", "Tango", "Samba", "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle",
        "Duet", "Punk Rock", "Drum Solo", "A Capella", "Euro-House", "Dance Hall" };
/*
 "Goa", "Drum & Bass", "Club House", "Hardcore",
 "Terror", "Indie", "BritPop", "NegerPunk", "Polsk Punk",
 "Beat", "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover",
 "Contemporary C", "Christian Rock", "Merengue", "Salsa", "Thrash Metal",
 "Anime", "JPop", "SynthPop",
 }; */ //apparently the other winamp id3v1 extensions aren't valid
stiks stikArray[] =
    {
        { "Movie", 0 },
        { "Normal", 1 },
        { "Audiobook", 2 },
        { "Whacked Bookmark", 5 },
        { "Music Video", 6 },
        { "Short Film", 9 },
        { "TV Show", 10 },
        { "Booklet", 11 } };

// from William Herrera: http://search.cpan.org/src/BILLH/LWP-UserAgent-iTMS_Client-0.16/lib/LWP/UserAgent/iTMS_Client.pm
sfIDs storefronts[] =
    {
        { "United States", 143441 },
        { "France", 143442 },
        { "Germany", 143443 },
        { "United Kingdom", 143444 },
        { "Austria", 143445 },
        { "Belgium", 143446 },
        { "Finland", 143447 },
        { "Greece", 143448 },
        { "Ireland", 143449 },
        { "Italy", 143450 },
        { "Luxembourg", 143451 },
        { "Netherlands", 143452 },
        { "Portugal", 143453 },
        { "Spain", 143454 },
        { "Canada", 143455 },
        { "Sweden", 143456 },
        { "Norway", 143457 },
        { "Denmark", 143458 },
        { "Switzerland", 143459 },
        { "Australia", 143460 },
        { "New Zealand", 143461 },
        { "Japan", 143462 } };

char* GenreIntToString(int genre) {
    char* return_string = NULL;
    if (genre > 0 && genre <= (int) (sizeof(ID3v1GenreList) / sizeof(*ID3v1GenreList))) {
        return_string = (char*) ID3v1GenreList[genre - 1];
    }
    return return_string;
}

uint8_t StringGenreToInt(const char* genre_string) {
    uint8_t return_genre = 0;
    uint8_t total_genres = (uint8_t) (sizeof(ID3v1GenreList) / sizeof(*ID3v1GenreList));
    uint8_t genre_length = strlen(genre_string) + 1;

    for (uint8_t i = 0; i < total_genres; i++) {
        if (memcmp(genre_string, ID3v1GenreList[i],
                strlen(ID3v1GenreList[i]) + 1 > genre_length ? strlen(ID3v1GenreList[i]) + 1 : genre_length) == 0) {
            return_genre = i + 1; //the list starts at 0; the embedded genres start at 1
            break;
        }
    }
    if (return_genre > total_genres) {
        return_genre = 0;
    }
    return return_genre;
}

void ListGenresValues() {
    uint8_t total_genres = (uint8_t) (sizeof(ID3v1GenreList) / sizeof(*ID3v1GenreList));
    fprintf(stdout, "\tAvailable standard genres - case sensitive.\n");

    for (uint8_t i = 0; i < total_genres; i++) {
        fprintf(stdout, "(%i.)  %s\n", i + 1, ID3v1GenreList[i]);
    }
    return;
}

stiks* MatchStikString(const char* in_stik_string) {
    stiks* matching_stik = NULL;
    uint8_t total_known_stiks = (uint32_t) (sizeof(stikArray) / sizeof(*stikArray));
    uint8_t stik_str_length = strlen(in_stik_string) + 1;

    for (uint8_t i = 0; i < total_known_stiks; i++) {
        if (memcmp(in_stik_string, stikArray[i].stik_string,
                strlen(stikArray[i].stik_string) + 1 > stik_str_length ? strlen(stikArray[i].stik_string) + 1 : stik_str_length)
                == 0) {
            matching_stik = &stikArray[i];
            break;
        }
    }
    return matching_stik;
}

stiks* MatchStikNumber(uint8_t in_stik_num) {
    stiks* matching_stik = NULL;
    uint8_t total_known_stiks = (uint32_t) (sizeof(stikArray) / sizeof(*stikArray));

    for (uint8_t i = 0; i < total_known_stiks; i++) {
        if (stikArray[i].stik_number == in_stik_num) {
            matching_stik = &stikArray[i];
            break;
        }
    }
    return matching_stik;
}

void ListStikValues() {
    uint8_t total_known_stiks = (uint32_t) (sizeof(stikArray) / sizeof(*stikArray));
    fprintf(stdout, "\tAvailable stik settings - case sensitive  (number in parens shows the stik value).\n");

    for (uint8_t i = 0; i < total_known_stiks; i++) {
        fprintf(stdout, "(%u)  %s\n", stikArray[i].stik_number, stikArray[i].stik_string);
    }
    return;
}

sfIDs* MatchStoreFrontNumber(uint32_t storefrontnum) {
    sfIDs* matching_sfID = NULL;
    uint8_t total_known_sfs = (uint32_t) (sizeof(storefronts) / sizeof(*storefronts));

    for (uint8_t i = 0; i < total_known_sfs; i++) {
        if (storefronts[i].storefront_number == storefrontnum) {
            matching_sfID = &storefronts[i];
            break;
        }
    }
    return matching_sfID;
}
