//==================================================================//
/*
 AtomicParsley - AtomicParsley.cpp

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

 * Mike Brancato - Debian patches & build support
 * Lowell Stewart - null-termination bugfix for Apple compliance
 * Brian Story - native Win32 patches; memset/framing/leaks fixes
 */
//==================================================================//
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <wchar.h>

#include "AtomicParsley.h"
#include "AP_AtomDefinitions.h"
#include "AP_iconv.h"
#include "AtomicParsley_genres.h"
#include "APar_uuid.h"

#if defined (DARWIN_PLATFORM)
#include "AP_NSImage.h"
#include "AP_NSFile_utils.h"
#endif

///////////////////////////////////////////////////////////////////////////////////////
//                               Global Variables                                    //
///////////////////////////////////////////////////////////////////////////////////////

static bool inited = false;

int metadata_style;
uint32_t mdatData;
AtomicInfo parsedAtoms[MAX_ATOMS];

static bool modified_atoms;
static bool alter_original;

static bool svn_build; //controls which type of versioning - release number

static FILE* source_file;
static uint32_t file_size;

static short atom_number;
static uint8_t generalAtomicLevel;

static bool file_opened;
static bool parsedfile;
static bool move_moov_atom;
static bool moov_atom_was_mooved;
static AtomicInfo* hdlrAtom;
static AtomicInfo* udtaAtom;
static bool complete_free_space_erasure;
static bool initial_optimize_pass;
static bool psp_brand;
static bool prevent_update_using_padding;
static bool tree_display_only;

static uint32_t max_buffer;

static uint32_t bytes_before_mdat;
static uint32_t bytes_into_mdat;
static uint64_t mdat_supplemental_offset;
static uint32_t removed_bytes_tally;
static uint32_t new_file_size; //used for the progressbar
static uint32_t brand;

static uint32_t gapless_void_padding; //possibly used in the context of gapless playback support by Apple

static struct udta_stats udta_dynamics;
static struct padding_preferences pad_prefs;

static bool contains_unsupported_64_bit_atom; //reminder that there are some 64-bit files that aren't yet supported (and where that limit is set)

static short max_display_width;
static char* file_progress_buffer;

static struct PicPrefs myPicturePrefs;
static bool parsed_prefs;
static char* twenty_byte_buffer;

static EmployedCodecs track_codecs;

static uint8_t UnicodeOutputStatus; //on windows, controls whether input/output strings are utf16 or raw utf8; reset in wmain()

static uint8_t forced_suffix_type;

///////////////////////////////////////////////////////////////////////////////////////
//                                Versioning                                         //
///////////////////////////////////////////////////////////////////////////////////////

void ShowVersionInfo() {

#if defined (_MSC_VER)
    char unicode_enabled[12];
    memset(unicode_enabled, 0, 12);
    if (UnicodeOutputStatus == WIN32_UTF16) {
        memcpy(unicode_enabled, "(utf16)", 7);

//its utf16 in the sense that any text entering on a modern Win32 system enters as utf16le - but gets converted immediately after AP.exe starts to utf8
//all arguments, strings, filenames, options are sent around as utf8. For modern Win32 systems, filenames get converted to utf16 for output as needed.
//Any strings to be set as utf16 in 3gp assets are converted to utf16be as needed (true for all OS implementations).
//Printing out to the console is a mixed bag of vanilla ascii & utf16le. Redirected output should be utf8. TODO: Win32 output should be uniformly utf16le.
    }
    else if (UnicodeOutputStatus == UNIVERSAL_UTF8) {
        memcpy(unicode_enabled, "(raw utf8)", 10);

//utf8 in the sense that any text entered had its utf16 upper byte stripped and reduced to (unchecked) raw utf8 for utilities that work in utf8. Any
//unicode (utf16) filenames were clobbered in that processes are invalid now. Any intermediate folder with unicode in it will now likely cause an error of
//some sort.
    }

#else
#define unicode_enabled	"(utf8)"
#endif

    if (svn_build) { //below is the versioning from svn if used; remember to switch to AtomicParsley_version for a release

        fprintf(stdout, "AtomicParsley from svn built on %s %s\n", __DATE__, unicode_enabled);

    }
    else { //below is the release versioning

        fprintf(stdout, "AtomicParsley version: %s %s\n", AtomicParsley_version, unicode_enabled); //release version
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                               Generic Functions                                   //
///////////////////////////////////////////////////////////////////////////////////////

// http://www.flipcode.com/articles/article_advstrings01.shtml
bool IsUnicodeWinOS() {
#if defined (_MSC_VER)
    OSVERSIONINFOW os;
    memset(&os, 0, sizeof(OSVERSIONINFOW));
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
    return (GetVersionExW(&os) != 0);
#else
    return false;
#endif
}

/*----------------------
 findFileSize
 utf8_filepath - a pointer to a string (possibly utf8) of the full path to the file

 take an ascii/utf8 filepath (which if under a unicode enabled Win32 OS was already converted from utf16le to utf8 at program start) and test if
 AP is running on a unicode enabled Win32 OS. If it is and converted to utf8 (rather than just stripped), convert the utf8 filepath to a utf16
 (native-endian) filepath & pass that to a wide stat. Or stat it with a utf8 filepath on Unixen & win32 (stripped utf8).
 ----------------------*/
off_t findFileSize(const char *utf8_filepath) {
    if (IsUnicodeWinOS() && UnicodeOutputStatus == WIN32_UTF16) {
#if defined (_MSC_VER)
        wchar_t* utf16_filepath = Convert_multibyteUTF8_to_wchar(utf8_filepath);

        struct _stat fileStats;
        _wstat(utf16_filepath, &fileStats);

        free(utf16_filepath);
        utf16_filepath = NULL;
        return fileStats.st_size;
#endif
    }
    else {
        struct stat fileStats;
        stat(utf8_filepath, &fileStats);
        return fileStats.st_size;
    }
    return 0; //won't ever get here.... unless this is win32, set to utf8 and the folder/file had unicode.... TODO (? use isUTF8() for high ascii?)
}

static void APar_Init() {
    if (inited)
        return;

    // Init only need to be called once
    inited = true;

    modified_atoms = false;
    alter_original = false;

    svn_build = false;

    source_file = NULL;

    atom_number = 0;
    generalAtomicLevel = 1;

    file_opened = false;
    parsedfile = false;
    move_moov_atom = true;
    moov_atom_was_mooved = false;
    hdlrAtom = NULL;
    udtaAtom = NULL;
    complete_free_space_erasure = false;
    initial_optimize_pass = true;
    psp_brand = false;
    prevent_update_using_padding = false;
    metadata_style = UNDEFINED_STYLE;
    tree_display_only = false;

    max_buffer = 4096 * 125; // increased to 512KB

    bytes_before_mdat = 0;
    bytes_into_mdat = 0;
    mdat_supplemental_offset = 0;
    removed_bytes_tally = 0;
    new_file_size = 0;
    brand = 0;
    mdatData = 0;

    gapless_void_padding = 0;

    contains_unsupported_64_bit_atom = false;

#if defined (WIN32) || defined (__CYGWIN__)
    max_display_width = 45;
#else
    max_display_width = 75;
#endif

    file_progress_buffer = (char*) calloc(1, sizeof(char) * (max_display_width + 50));

    parsed_prefs = false;
    twenty_byte_buffer = (char *) malloc(sizeof(char) * 20);

    track_codecs = EmployedCodecs();

    UnicodeOutputStatus = UNIVERSAL_UTF8;

    forced_suffix_type = NO_TYPE_FORCING;
}

/*----------------------
 APar_OpenFile
 utf8_filepath - a pointer to a string (possibly utf8) of the full path to the file
 file_flags - 3 bytes max for the flags to open the file with (read, write, binary mode....)

 take an ascii/utf8 filepath (which if under a unicode enabled Win32 OS was already converted from utf16le to utf8 at program start) and test if
 AP is running on a unicode enabled Win32 OS. If it is, convert the utf8 filepath to a utf16 (native-endian) filepath & pass that to a wide fopen
 with the 8-bit file flags changed to 16-bit file flags. Or open a utf8 file with vanilla fopen on Unixen.
 ----------------------*/
FILE* APar_OpenFile(const char* utf8_filepath, const char* file_flags) {
    FILE* aFile = NULL;
    if (IsUnicodeWinOS() && UnicodeOutputStatus == WIN32_UTF16) {
#if defined (_MSC_VER)
        wchar_t* Lfile_flags = (wchar_t *)malloc(sizeof(wchar_t)*4);
        memset(Lfile_flags, 0, sizeof(wchar_t)*4);
        mbstowcs(Lfile_flags, file_flags, strlen(file_flags) );

        wchar_t* utf16_filepath = Convert_multibyteUTF8_to_wchar(utf8_filepath);

        aFile = _wfopen(utf16_filepath, Lfile_flags);

        free(utf16_filepath);
        utf16_filepath = NULL;
#endif
    }
    else {
        aFile = fopen(utf8_filepath, file_flags);
    }

    if (!aFile) {
        fprintf(stdout, "AP error trying to fopen: %s\n", strerror(errno));
    }
    return aFile;
}

/*----------------------
 openSomeFile
 utf8_filepath - a pointer to a string (possibly utf8) of the full path to the file
 open - flag to either open or close (function does both)

 take an ascii/utf8 filepath and either open or close it; used for the main ISO Base Media File; store the resulting FILE* in a global source_file
 ----------------------*/
FILE* openSomeFile(const char* utf8file, bool open) {
    APar_Init();

    if (open && !file_opened) {
        source_file = APar_OpenFile(utf8file, "rb");
        if (source_file != NULL) {
            file_opened = true;
        }
    }
    else {
        fclose(source_file);
        file_opened = false;
    }
    return source_file;
}

void TestFileExistence(const char *filePath, bool errorOut) {
    FILE *a_file = NULL;
    a_file = APar_OpenFile(filePath, "rb");
    if ((a_file == NULL) && errorOut) {
        fprintf(stderr, "AtomicParsley error: can't open %s for reading: %s\n", filePath, strerror(errno));
    }
    else {
        fclose(a_file);
    }
}

uint32_t APar_ReadFile(char* destination_buffer, FILE* a_file, uint32_t bytes_to_read) {
    uint32_t bytes_read = 0;
    if (destination_buffer != NULL) {
        fseeko(a_file, 0, SEEK_SET); // not that 2gb support is required - malloc would probably have a few issues
        bytes_read = (uint32_t) fread(destination_buffer, 1, (size_t) bytes_to_read, a_file);
        file_size += bytes_read; //accommodate huge files embedded within small files for APar_Validate
    }
    return bytes_read;
}

int APar_TestArtworkBinaryData(const char* artworkPath) {
    int artwork_dataType = 0;
    FILE *artfile = APar_OpenFile(artworkPath, "rb");
    if (artfile != NULL) {
        char pic_data[10];
        memset(pic_data, 0, 10);

        fread(&pic_data, 1, 8, artfile);
        if (strncmp(&*pic_data, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8) == 0) {
            artwork_dataType = AtomFlags_Data_PNGBinary;
        }
        else if (strncmp(&*pic_data, "\xFF\xD8\xFF\xE0", 4) == 0 || memcmp(&*pic_data, "\xFF\xD8\xFF\xE1", 4) == 0) {
            artwork_dataType = AtomFlags_Data_JPEGBinary;
        }
        else {
            fprintf(stdout, "AtomicParsley error: %s\n\t image file is not jpg/png and cannot be embedded.\n", artworkPath);
            artwork_dataType = -1;
        }
        fclose(artfile);

    }
    else {
        fprintf(stdout, "AtomicParsley error: %s\n\t image file could not be opened.\n", artworkPath);
        artwork_dataType = -1;
    }
    return artwork_dataType;
}

#if defined (DARWIN_PLATFORM)
// enables writing out the contents of a single memory-resident atom out to a text file; for in-house testing purposes only - and unused in some time
void APar_AtomicWriteTest(short AtomicNumber, bool binary) {
    AtomicInfo anAtom = parsedAtoms[AtomicNumber];

    char* indy_atom_path = (char *)malloc(sizeof(char)*MAXPATHLEN); //this malloc can escape memset because its only for in-house testing
    strcat(indy_atom_path, "/Users/");
    strcat(indy_atom_path, getenv("USER") );
    strcat(indy_atom_path, "/Desktop/singleton_atom.txt");

    FILE* single_atom_file;
    single_atom_file = fopen(indy_atom_path, "wb");
    if (single_atom_file != NULL) {

        if (binary) {
            fwrite(anAtom.AtomicData, (size_t)(anAtom.AtomicLength - 12), 1, single_atom_file);
        }
        else {
            char* data = (char*)malloc(sizeof(char)*4);
            char4TOuint32(anAtom.AtomicLength, data);

            fwrite(data, 4, 1, single_atom_file);
            fwrite(anAtom.AtomicName, 4, 1, single_atom_file);

            char4TOuint32((uint32_t)anAtom.AtomicVerFlags, data);
            fwrite(data, 4, 1, single_atom_file);

            fwrite(anAtom.AtomicData, strlen(anAtom.AtomicData)+1, 1, single_atom_file);
            free(data);
        }
    }
    fclose(single_atom_file);
    free(indy_atom_path);
    return;
}
#endif

char* extractAtomName(char *fileData, int name_position) {
//name_position = 1 for normal atoms and needs to be done first; 2 for uuid atoms (which can only occur after we first find the atomName == "uuid")
    memset(twenty_byte_buffer, 0, sizeof(char) * 20);
    memcpy(twenty_byte_buffer, fileData + name_position * 4, 4);

    return twenty_byte_buffer;
}

void APar_FreeMemory() {
    for (int iter = 0; iter < atom_number; iter++) {
        if (parsedAtoms[iter].AtomicData != NULL) {
            free(parsedAtoms[iter].AtomicData);
            parsedAtoms[iter].AtomicData = NULL;
        }
        if (parsedAtoms[iter].ReverseDNSname != NULL) {
            free(parsedAtoms[iter].ReverseDNSname);
            parsedAtoms[iter].ReverseDNSname = NULL;
        }
        if (parsedAtoms[iter].uuid_ap_atomname != NULL) {
            free(parsedAtoms[iter].uuid_ap_atomname);
            parsedAtoms[iter].uuid_ap_atomname = NULL;
        }
    }
    free(twenty_byte_buffer);
    twenty_byte_buffer = NULL;
    free(file_progress_buffer);
    file_progress_buffer = NULL;

    if (source_file && file_opened) {
        fclose(source_file);
        file_opened = false;
    }

    inited = false;

    return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                        Picture Preferences Functions                              //
///////////////////////////////////////////////////////////////////////////////////////

PicPrefs APar_ExtractPicPrefs(char* env_PicOptions) {
    if (!parsed_prefs) {

        parsed_prefs = true; //only set default values & parse once

        myPicturePrefs.max_dimension = 0; //dimensions won't be used to alter image
        myPicturePrefs.dpi = 72;
        myPicturePrefs.max_Kbytes = 0; //no target size to shoot for
        myPicturePrefs.allJPEG = false;
        myPicturePrefs.allPNG = false;
        myPicturePrefs.addBOTHpix = false;
        myPicturePrefs.force_dimensions = false;
        myPicturePrefs.force_height = 0;
        myPicturePrefs.force_width = 0;
        myPicturePrefs.removeTempPix = true; //we'll just make this the default

        char* unparsed_opts = env_PicOptions;
        if (env_PicOptions == NULL)
            return myPicturePrefs;

        while (unparsed_opts[0] != 0) {
            if (memcmp(unparsed_opts, "MaxDimensions=", 14) == 0) {
                unparsed_opts += 14;
                myPicturePrefs.max_dimension = (int) strtol(unparsed_opts, NULL, 10);

            }
            else if (memcmp(unparsed_opts, "DPI=", 4) == 0) {
                unparsed_opts += 4;
                myPicturePrefs.dpi = (int) strtol(unparsed_opts, NULL, 10);

            }
            else if (memcmp(unparsed_opts, "MaxKBytes=", 10) == 0) {
                unparsed_opts += 10;
                myPicturePrefs.max_Kbytes = (int) strtol(unparsed_opts, NULL, 10) * 1024;

            }
            else if (memcmp(unparsed_opts, "AllPixJPEG=", 11) == 0) {
                unparsed_opts += 11;
                if (memcmp(unparsed_opts, "true", 4) == 0) {
                    myPicturePrefs.allJPEG = true;
                }

            }
            else if (memcmp(unparsed_opts, "AllPixPNG=", 10) == 0) {
                unparsed_opts += 10;
                if (memcmp(unparsed_opts, "true", 4) == 0) {
                    myPicturePrefs.allPNG = true;
                }

            }
            else if (memcmp(unparsed_opts, "AddBothPix=", 11) == 0) {
                unparsed_opts += 11;
                if (memcmp(unparsed_opts, "true", 4) == 0) {
                    myPicturePrefs.addBOTHpix = true;
                }

            }
            else if (memcmp(unparsed_opts, "SquareUp", 7) == 0) {
                unparsed_opts += 7;
                myPicturePrefs.squareUp = true;

            }
            else if (strncmp(unparsed_opts, "removeTempPix", 13) == 0) {
                unparsed_opts += 13;
                myPicturePrefs.removeTempPix = true;

            }
            else if (memcmp(unparsed_opts, "keepTempPix", 11) == 0) { //NEW
                unparsed_opts += 11;
                myPicturePrefs.removeTempPix = false;

            }
            else if (memcmp(unparsed_opts, "ForceHeight=", 12) == 0) {
                unparsed_opts += 12;
                myPicturePrefs.force_height = strtol(unparsed_opts, NULL, 10);

            }
            else if (memcmp(unparsed_opts, "ForceWidth=", 11) == 0) {
                unparsed_opts += 11;
                myPicturePrefs.force_width = strtol(unparsed_opts, NULL, 10);

            }
            else {
                unparsed_opts++;
            }
        }
    }

    if (myPicturePrefs.force_height > 0 && myPicturePrefs.force_width > 0)
        myPicturePrefs.force_dimensions = true;
    return myPicturePrefs;
}

///////////////////////////////////////////////////////////////////////////////////////
//                            Track Level Atom Info                                  //
///////////////////////////////////////////////////////////////////////////////////////

void APar_TrackInfo(uint8_t &total_tracks, uint8_t &track_num, short &codec_atom) { //returns the codec used for each 'trak' atom; used under Mac OS X only
    uint8_t track_tally = 0;
    short iter = 0;

    while (parsedAtoms[iter].NextAtomNumber != 0) {

        if (memcmp(parsedAtoms[iter].AtomicName, "trak", 4) == 0 && parsedAtoms[iter].AtomicLevel == 2) {
            track_tally += 1;
            if (track_num == 0) {
                total_tracks += 1;

            }
            else if (track_num == track_tally) {
                //drill down into stsd
                short next_atom = parsedAtoms[iter].NextAtomNumber;
                while (parsedAtoms[next_atom].AtomicLevel > parsedAtoms[iter].AtomicLevel) {

                    if (strncmp(parsedAtoms[next_atom].AtomicName, "stsd", 4) == 0) {

                        codec_atom = parsedAtoms[next_atom].AtomicNumber;
                        //return with the stsd atom - its stsd_codec uint32_t holds the 4CC name of the codec for the trak
                        //(mp4v, avc1, drmi, mp4a, drms, alac, mp4s, text, tx3g or jpeg)
                        return;
                    }
                    else {
                        next_atom = parsedAtoms[next_atom].NextAtomNumber;
                    }
                }
            }
        }
        iter = parsedAtoms[iter].NextAtomNumber;
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                            Locating/Finding Atoms                                 //
///////////////////////////////////////////////////////////////////////////////////////

uint32_t APar_ProvideTallyForAtom(const char* atom_name) {
    uint32_t tally_for_atom = 0;
    short iter = parsedAtoms[0].NextAtomNumber;
    while (true) {
        if (memcmp(parsedAtoms[iter].AtomicName, atom_name, 4) == 0) {
            if (parsedAtoms[iter].AtomicLength == 0) {
                tally_for_atom += (uint32_t) file_size - parsedAtoms[iter].AtomicStart;
            }
            else if (parsedAtoms[iter].AtomicLength == 1) {
                tally_for_atom += (uint32_t) parsedAtoms[iter].AtomicLengthExtended;
            }
            else {
                tally_for_atom += parsedAtoms[iter].AtomicLength;
            }
        }
        if (iter == 0) {
            break;
        }
        else {
            iter = parsedAtoms[iter].NextAtomNumber;
        }
    }
    return tally_for_atom;
}

short APar_FindPrecedingAtom(short an_atom_num) {
    short precedingAtom = 0;
    short iter = 0;
    while (parsedAtoms[iter].NextAtomNumber != 0) {
        if (parsedAtoms[iter].NextAtomNumber == parsedAtoms[an_atom_num].NextAtomNumber) {
            break;
        }
        else {
            precedingAtom = iter;
            iter = parsedAtoms[iter].NextAtomNumber;
        }
    }
    return precedingAtom;
}

short APar_FindParentAtom(int order_in_tree, uint8_t this_atom_level) {
    short thisAtom = 0;
    short iter = order_in_tree;
    while (parsedAtoms[iter].AtomicNumber != 0) {
        iter = APar_FindPrecedingAtom(iter);
        if (parsedAtoms[iter].AtomicLevel == this_atom_level - 1) {
            thisAtom = iter;
            break;
        }
    }
    return thisAtom;
}

/*----------------------
 APar_ProvideAtomPath
 this_atom - index into array of parsedAtoms for the wanted path of an atom
 atom_path - string into which the path will be placed (working backwards)
 fromFile - controls the manner of extracting parents (atom sizes from file, or a simpler atomic level if from memory)

 First, determine exactly how many atoms will constitute the full path and calculate where into the string to first start placing atom names. Start by
 working off the current atom. Using fromFile, either use a more stringent atom start/length from a file, or a more relaxed atom level if from memory.
 The array in memory won't have proper atom sizes except for the last child atom typically ('data' will have a proper size, but its parent and all
 other parents will not have sizing automatically updated - which happens only at writeout time).
 ----------------------*/
void APar_ProvideAtomPath(short this_atom, char* &atom_path, bool fromFile) {
    short preceding_atom = this_atom;
    uint8_t current_atomic_level = parsedAtoms[this_atom].AtomicLevel;
    int str_offset = (parsedAtoms[this_atom].AtomicLevel - 1) * 5; //5 = 'atom" + '.'
    if (parsedAtoms[this_atom].AtomicClassification == EXTENDED_ATOM) {
        str_offset += 5; //include a "uuid=" string;
    }

    memcpy(atom_path + str_offset, parsedAtoms[preceding_atom].AtomicName, 4);
    str_offset -= 5;
    if (parsedAtoms[preceding_atom].AtomicLevel != 1) {
        memcpy(atom_path + str_offset + 4, ".", 1);
    }
    if (parsedAtoms[this_atom].AtomicClassification == EXTENDED_ATOM) {
        memcpy(atom_path + str_offset, "uuid=", 5);
        str_offset -= 5;
    }

    while (parsedAtoms[preceding_atom].AtomicNumber != 0) {

        if (fromFile) {
            if (parsedAtoms[preceding_atom].AtomicStart < parsedAtoms[this_atom].AtomicStart
                    && parsedAtoms[preceding_atom].AtomicLength > parsedAtoms[this_atom].AtomicLength
                    && parsedAtoms[preceding_atom].AtomicStart + parsedAtoms[preceding_atom].AtomicLength
                            >= parsedAtoms[this_atom].AtomicStart + parsedAtoms[this_atom].AtomicLength
                    && parsedAtoms[preceding_atom].AtomicContainerState <= DUAL_STATE_ATOM) {
                memcpy(atom_path + str_offset, parsedAtoms[preceding_atom].AtomicName, 4);
                str_offset -= 5;
                if (str_offset >= 0) {
                    memcpy(atom_path + str_offset + 4, ".", 1);
                }

                preceding_atom = APar_FindPrecedingAtom(preceding_atom); //preceding_atom--;

            }
            else {
                preceding_atom = APar_FindPrecedingAtom(preceding_atom); //preceding_atom--;
            }
        }
        else {
            if (parsedAtoms[preceding_atom].AtomicLevel < current_atomic_level) {
                memcpy(atom_path + str_offset, parsedAtoms[preceding_atom].AtomicName, 4);
                str_offset -= 5;
                if (str_offset >= 0) {
                    memcpy(atom_path + str_offset + 4, ".", 1);
                }

                current_atomic_level = parsedAtoms[preceding_atom].AtomicLevel;
                preceding_atom = APar_FindPrecedingAtom(preceding_atom); //preceding_atom--;
            }
            else {
                preceding_atom = APar_FindPrecedingAtom(preceding_atom); //preceding_atom--;
            }
        }
        if (preceding_atom == 0 || str_offset < 0) {
            break;
        }
    }

    return;
}

bool APar_Eval_ChunkOffsetImpact(short an_atom_num) {
    bool impact_calculations_directly = false;
    short iter = 0;
    uint8_t found_desired_atom = 0;

    while (true) {
        if (strncmp(parsedAtoms[iter].AtomicName, "mdat", 4) == 0) {
            if (found_desired_atom) {
                impact_calculations_directly = true;
            }
            break;
        }
        else {
            iter = parsedAtoms[iter].NextAtomNumber;
        }
        if (iter == 0) {
            break;
        }
        if (iter == an_atom_num) {
            found_desired_atom = 1;
        }
    }
    return impact_calculations_directly;
}

short APar_FindLastAtom() {
    short this_atom_num = 0; //start our search with the first atom
    while (parsedAtoms[this_atom_num].NextAtomNumber != 0) {
        this_atom_num = parsedAtoms[this_atom_num].NextAtomNumber;
    }
    return this_atom_num;
}

short APar_FindEndingAtom() {
    short end_atom_num = 0; //start our search with the first atom
    while (true) {
        if ((parsedAtoms[end_atom_num].NextAtomNumber == 0) || (end_atom_num == atom_number - 1)) {
            break;
        }
        else {
            end_atom_num = parsedAtoms[end_atom_num].NextAtomNumber;
        }
    }
    return end_atom_num;
}

short APar_FindLastChild_of_ParentAtom(short thisAtom) {
    short child_atom = parsedAtoms[thisAtom].NextAtomNumber;
    short last_atom = thisAtom; //if there are no children, this will be the first and last atom in the hiearchy
    while (true) {
        if (parsedAtoms[child_atom].AtomicLevel > parsedAtoms[thisAtom].AtomicLevel) {
            last_atom = child_atom;
        }
        child_atom = parsedAtoms[child_atom].NextAtomNumber;
        if (child_atom == 0 || parsedAtoms[child_atom].AtomicLevel <= parsedAtoms[thisAtom].AtomicLevel) {
            break;
        }
    }
    return last_atom;
}

/*----------------------
 APar_ReturnChildrenAtoms
 this_atom - the parent atom that contains any number of children atoms (that are currenly unknown)
 atom_index - the index of the desired child. Passing 0 will return a count of the *total* number of children atoms under this_atom

 Working off of AtomicLevel, test the atoms that follow this_atom to see if they are immediately below this_atom. Increment total_children if is - if
 total_children should match our index, return that desired child at index atom.
 ----------------------*/
short APar_ReturnChildrenAtoms(short this_atom, uint8_t atom_index) {
    short child_atom = 0;
    uint8_t total_children = 0;
    short iter = parsedAtoms[this_atom].NextAtomNumber;

    while (true) {
        if ((parsedAtoms[iter].AtomicLevel == parsedAtoms[this_atom].AtomicLevel + 1 && this_atom > 0)
                || (this_atom == 0 && parsedAtoms[iter].AtomicLevel == 1)) {
            total_children++;

            if (atom_index == total_children) {
                child_atom = iter;
                break;
            }
        }
        if (parsedAtoms[iter].AtomicLevel <= parsedAtoms[this_atom].AtomicLevel && this_atom != 0) {
            break;
        }
        else {
            iter = parsedAtoms[iter].NextAtomNumber;
        }
        if (iter == 0) {
            break;
        }
    }
    if (atom_index == 0) {
        child_atom = (short) total_children;
    }
    return child_atom;
}

/*----------------------
 APar_AtomicComparison
 proto_atom - the temporary atom structure to run the tests on
 test_atom - the exising atom to compare the proto_atom against
 match_full_uuids - selects whether to match by atom names (4 bytes) or uuids(16 bytes) which are stored on AtomicName

 Test if proto_atom matches a single atom (test_atom) by name, level & classification (packed_lang_atom, extended atom...); for certain types of data
 (like packed_lang & reverseDNS 'moov.udta.meta.ilst.----.name:[iTunNORM] atoms currently) add finer grained tests. The return result will be NULL
 if not matched, or returns the atom it matches.
 ----------------------*/
AtomicInfo* APar_AtomicComparison(AtomicInfo* proto_atom, short test_atom, bool match_full_uuids) {
    AtomicInfo* return_atom = NULL;
    size_t ATOM_TEST_LEN = (match_full_uuids ? 16 : 4);

    if (parsedAtoms[test_atom].AtomicClassification == EXTENDED_ATOM
            && parsedAtoms[test_atom].uuid_style == UUID_DEPRECATED_FORM) { //accommodate deprecated form
        if (memcmp(parsedAtoms[test_atom].uuid_ap_atomname, proto_atom->AtomicName, 4) == 0) {
            return &parsedAtoms[test_atom];
        }
    }

    //can't do AtomicVerFlags because lots of utilities don't write the proper iTunes flags for iTunes metadata
    if (memcmp(proto_atom->AtomicName, parsedAtoms[test_atom].AtomicName, ATOM_TEST_LEN) == 0
            && proto_atom->AtomicLevel == parsedAtoms[test_atom].AtomicLevel
            && (proto_atom->AtomicClassification == parsedAtoms[test_atom].AtomicClassification
                    || proto_atom->AtomicClassification == UNKNOWN_ATOM)) {

        if (proto_atom->AtomicClassification == PACKED_LANG_ATOM) {
            //0x05D9 = 'any' and will be used (internally) to match on name,class,container state alone, disregarding AtomicLanguage
            if (proto_atom->AtomicLanguage == parsedAtoms[test_atom].AtomicLanguage
                    || proto_atom->AtomicLanguage == 0x05D9) {
                return_atom = &parsedAtoms[test_atom];
            }

        }
        else if (proto_atom->ReverseDNSname != NULL && parsedAtoms[test_atom].ReverseDNSname != NULL) {
            //match on moov.udta.meta.ilst.----.name:[something] (reverse DNS atom)
            size_t proto_rdns_len = strlen(proto_atom->ReverseDNSname) + 1;
            size_t test_rdns_len = strlen(parsedAtoms[test_atom].ReverseDNSname) + 1;
            size_t rdns_strlen = (proto_rdns_len > test_rdns_len ? proto_rdns_len : test_rdns_len);
            if (memcmp(proto_atom->ReverseDNSname, parsedAtoms[test_atom].ReverseDNSname, rdns_strlen) == 0) {
                return_atom = &parsedAtoms[test_atom];
            }
        }
        else {
            return_atom = &parsedAtoms[test_atom];
        }
    }
    return return_atom;
}

/*----------------------
 APar_FindLastLikeNamedAtom
 atom_name - the name of the atom to search for; the string itself may have more than 4 bytes
 containing_hierarchy - the parent hierarchy that is expected to carry multiply named atoms differing (in language for example)

 Follow through the atom tree; if a test atom is matches by name, and is a child to the container atom, remember that atom. If nothing matches, the index
 of the container atom is returned; otherwise the last like named atom is returned.
 ----------------------*/
short APar_FindLastLikeNamedAtom(char* atom_name, short containing_hierarchy) {
    short last_identically_named_atom = APar_FindLastChild_of_ParentAtom(containing_hierarchy); //default returns the last atom in the parent, not the parent
    short eval_atom = parsedAtoms[containing_hierarchy].NextAtomNumber;

    while (true) {
        if (parsedAtoms[eval_atom].AtomicLevel < parsedAtoms[containing_hierarchy].AtomicLevel + 1 || eval_atom == 0) {
            break;
        }
        else {
            if (memcmp(parsedAtoms[eval_atom].AtomicName, atom_name, 4) == 0
                    && parsedAtoms[eval_atom].AtomicLevel == parsedAtoms[containing_hierarchy].AtomicLevel + 1) {
                last_identically_named_atom = eval_atom;
            }
            eval_atom = parsedAtoms[eval_atom].NextAtomNumber;
        }
    }
    return last_identically_named_atom;
}

void APar_FreeSurrogateAtom(AtomicInfo* surrogate_atom) {
    if (surrogate_atom->ReverseDNSname != NULL) {
        free(surrogate_atom->ReverseDNSname);
        surrogate_atom->ReverseDNSname = NULL;
    }
    return;
}

/*----------------------
 APar_CreateSurrogateAtom

 Make a temporary AtomicInfo structure to run comparisons against; currently comparisons are done on name, level, classification (versioned...), langauge
 (3gp assets), and iTunes-style reverse dns 'name' carrying a string describing the purpose of the data (iTunNORM). This atom exists outside of a file's
 atom hieararchy that resides in the parsedAtoms[] array.
 ----------------------*/
void APar_CreateSurrogateAtom(AtomicInfo* surrogate_atom, const char* atom_name, uint8_t atom_level, uint8_t atom_class, uint16_t atom_lang, char* revdns_name, uint8_t revdns_name_len) {
    surrogate_atom->AtomicName = (char*) atom_name;
    surrogate_atom->AtomicLevel = atom_level;

    if (revdns_name != NULL && revdns_name_len) {
        surrogate_atom->ReverseDNSname = (char *) malloc(sizeof(char) * revdns_name_len > 8 ? revdns_name_len + 1 : 9);
        memset(surrogate_atom->ReverseDNSname, 0, sizeof(char) * revdns_name_len > 8 ? revdns_name_len + 1 : 9);
        memcpy(surrogate_atom->ReverseDNSname, revdns_name, revdns_name_len);

    }
    else {
        APar_FreeSurrogateAtom(surrogate_atom);
    }
    surrogate_atom->AtomicClassification = atom_class;
    surrogate_atom->AtomicLanguage = atom_lang;
    return;
}

/*----------------------
 APar_FindAtom
 atom_name - the full path describing the hiearchy the desired atom can be found in
 createMissing - either create the missing interim atoms as required, or return a NULL if not found
 atom_type - the classification of the last atom (packed language, uuid extended atom...)
 atom_lang - the language of the 3gp asset used when atom_type is packed language type
 match_full_uuids - match 16byte full uuids (typically removing ( possibly non-AP) uuids via --manualAtomRemoval; AP uuids (the new ones) still work on 4bytes**

 Follow through the atom tree starting with the atom following 'ftyp'. Testing occurs on an atom level basis; a stand-in temporary skeletal atom
 is created to evaluate. If they atoms are deemed matching, atom_name is advanced forward (it still contains the full path, but only 4bytes are
 typically used at a time) and testing occurs until either the desired atom is found, or the last containing hiearchy with an exising atom is
 exhausted without making new atoms.

 NOTE: atom_name can come in these forms:
 classic/vanilla/ordinary atoms:		moov.udta.meta.ilst.cprt.data
 iTunes reverseDNS atoms:          moov.udta.meta.ilst.----.name:[iTunNORM]
 uuid user-extension atoms:        moov.udta.meta.uuid=tdtg (the deprecated form)
 uuid user-extension atoms:        moov.udta.meta.uuid=ba45fcaa-7ef5-5201-8a63-78886495ab1f
 index-based atoms:                moov.trak[2].mdia.minf

 NOTE: On my computer it takes about .04 second to scan the file, .1 second to add about 2 dozen tags, and 1.0 second to copy a file. Updating a file
 from start to finish takes 0.21 seconds. As many loops as this new APar_FindAtom eliminates, it is only marginally faster than the old code.

 ** the reason why the old deprecated uuid form & the new uuid full 16byte form work off of a 4byte value (the atom name) is that because we are using a version 5
 sha1 hashed uuid of a name in a given namespace, the identical name in the identical namespace will yield identical an identical uuid (if corrected for endianness).
 This means that that matching by 4 bytes of atom name is the functional equvalent of matching by 16byte uuids.
 ----------------------*/
AtomicInfo* APar_FindAtom(const char* atom_name, bool createMissing, uint8_t atom_type, uint16_t atom_lang, bool match_full_uuids) {
    AtomicInfo* thisAtom = NULL;
    char* search_atom_name = (char*) atom_name;
    char* reverse_dns_name = NULL;
    uint8_t revdns_name_len = 0;
    uint8_t atom_index = 0; // if there are atoms mutliple identically named at the same level, this is where to store the count as it occurs
    uint8_t desired_index = 1;
    uint8_t search_atom_type = UNKNOWN_ATOM;
    int known_atom = -1;
    short search_atom_start_num = parsedAtoms[0].NextAtomNumber; //don't test 'ftyp'; its atom_number[0] & will be used to know when we have hit the end of the tree; can't hardcode it to '1' because ftyp's following atom can change; only ftype as parsedAtoms[0] is guaranteed.
    uint8_t present_atomic_level = 1;
    AtomicInfo* last_known_present_parent = NULL;
    AtomicInfo atom_surrogate =
        { 0 };

#if defined(DEBUG_V)
    fprintf(stdout, "debug: AP_FindAtom entry trying to find '%s'; create missing: %u\n", atom_name, createMissing);
#endif

    while (search_atom_name != NULL) {
        desired_index = 1; //reset the index

        if (atom_type == EXTENDED_ATOM && memcmp(search_atom_name, "uuid=", 5) == 0) {
            search_atom_name += 5;
            search_atom_type = atom_type;
        }

#if defined(DEBUG_V)
        fprintf(stdout, "debug: AP_FindAtom   loop evaluate test  %s (index=%u)\n", search_atom_name, atom_index);
#endif

        size_t portion_len = strlen(search_atom_name);
        if (memcmp(search_atom_name + 4, ":[", 2) == 0 && memcmp(search_atom_name + portion_len - 1, "]", 1) == 0) {
            reverse_dns_name = search_atom_name + 4 + 2; //4bytes atom name 2bytes ":["
            revdns_name_len = portion_len - 7; //4bytes atom name, 2 bytes ":[", 1 byte "]"
            search_atom_type = atom_type;

        }
        else if (memcmp(search_atom_name + 4, "[", 1) == 0) {
            sscanf(search_atom_name + 5, "%hhu", &desired_index);

#if defined(DEBUG_V)
            fprintf(stdout, "debug: AP_FindAtom     >#<indexed atom>#< '%s' at index=%u\n", search_atom_name, desired_index);
#endif
        }

        if (strlen(search_atom_name) == 4 || !strchr(search_atom_name, '.')) {
            if (atom_type == UNKNOWN_ATOM) {
                known_atom =
                        APar_MatchToKnownAtom(search_atom_name, last_known_present_parent->AtomicName, false, atom_name);
                search_atom_type = KnownAtoms[known_atom].box_type;
            }
            else {
                search_atom_type = atom_type;
            }
        }

        APar_CreateSurrogateAtom(&atom_surrogate, search_atom_name, present_atomic_level, search_atom_type, atom_lang, reverse_dns_name, revdns_name_len);
        atom_index = 0;

        short iter = search_atom_start_num;
        while (true) {
            AtomicInfo* result = NULL;

            //if iter == 0, that means test against 'ftyp' - and since its always 0, don't test it; its to know that the end of the tree is reached
            if (iter != 0 && (parsedAtoms[iter].AtomicLevel == present_atomic_level || reverse_dns_name != NULL)) {
                result = APar_AtomicComparison(&atom_surrogate, iter, (
                        search_atom_type == EXTENDED_ATOM ? match_full_uuids : false));
#if defined(DEBUG_V)
                fprintf(stdout, "debug: AP_FindAtom     compare  %s(%u)  against %s (wanted index=%u)\n", search_atom_name, atom_index, parsedAtoms[iter].AtomicName, desired_index);
            }
            else {
                fprintf(stdout, "debug: AP_FindAtom       %s  rejected against %s\n", search_atom_name, parsedAtoms[iter].AtomicName);
#endif
            }
            if (result != NULL) { //something matched
                atom_index++;
#if defined(DEBUG_V)
                fprintf(stdout, "debug: AP_FindAtom       ***matched*** current index=%u (want %u)\n", atom_index, desired_index);
#endif
                if (search_atom_type != UNKNOWN_ATOM || (search_atom_type == UNKNOWN_ATOM && known_atom != -1)) {
                    thisAtom = result;
#if defined(DEBUG_V)
                    fprintf(stdout, "debug: AP_FindAtom         perfect match: %s(%u) == existing %s(%u)\n", search_atom_name, desired_index, parsedAtoms[iter].AtomicName, atom_index);
#endif
                }
                else {
                    last_known_present_parent = result; //if not, then it isn't the last atom, and must be some form of parent
                }
                if (desired_index == atom_index) {
                    search_atom_start_num = parsedAtoms[iter].NextAtomNumber;
                    break;
                }
            }

            if (parsedAtoms[iter].AtomicLevel < present_atomic_level && reverse_dns_name == NULL) {
                iter = 0; //force the ending determination of whether to make new atoms or not;
            }

            if (iter == 0 && createMissing) {
                //create that atom
                if (last_known_present_parent != NULL) {
                    short last_hierarchical_atom = 0;
#if defined(DEBUG_V)
                    fprintf(stdout, "debug: AP_FindAtom-------missing atom, need to create '%s'\n", search_atom_name);
#endif
                    if (search_atom_type == PACKED_LANG_ATOM) {
                        last_hierarchical_atom =
                                APar_FindLastLikeNamedAtom(atom_surrogate.AtomicName, last_known_present_parent->AtomicNumber);
                    }
                    else {
                        last_hierarchical_atom =
                                APar_FindLastChild_of_ParentAtom(last_known_present_parent->AtomicNumber);
                    }
                    thisAtom =
                            APar_CreateSparseAtom(&atom_surrogate, last_known_present_parent, last_hierarchical_atom);
                    search_atom_start_num = thisAtom->AtomicNumber;
                    if (strlen(search_atom_name) >= 4) {
                        last_known_present_parent = thisAtom;
                    }
                }
                break;

            }
            else if (iter == 0 && !createMissing) {
                search_atom_name = NULL; //force the break;
                break;
            }
            iter = parsedAtoms[iter].NextAtomNumber;
        }

        if (iter == 0 && search_atom_name == NULL) {
            break;
        }
        else {
            while (true) { // search_atom_name = strsep(&atom_name,".") equivalent
                if (search_atom_name[0] == 0) {
                    search_atom_name = NULL;
                    break;
                }
                else if (memcmp(search_atom_name, ".", 1) == 0) {
                    search_atom_name++;
                    break;
                }
                else {
                    search_atom_name++;
                }
            }
            present_atomic_level++;
        }
    }
    //APar_PrintAtomicTree(); //because PrintAtomicTree calls DetermineDynamicUpdate (which calls this FindAtom function) to print out padding space, an infinite loop occurs
    APar_FreeSurrogateAtom(&atom_surrogate);
    return thisAtom;
}

///////////////////////////////////////////////////////////////////////////////////////
//                   'data'/'stco'/3GP asset Atom extraction                         //
///////////////////////////////////////////////////////////////////////////////////////

void APar_AtomicRead(short this_atom_number) {
    parsedAtoms[this_atom_number].AtomicData = (char*) malloc(sizeof(char)
            * (size_t) (parsedAtoms[this_atom_number].AtomicLength));
    memset(parsedAtoms[this_atom_number].AtomicData, 0, sizeof(char)
            * (size_t) (parsedAtoms[this_atom_number].AtomicLength));

    fseeko(source_file, parsedAtoms[this_atom_number].AtomicStart + 12, SEEK_SET);
    fread(parsedAtoms[this_atom_number].AtomicData, 1, parsedAtoms[this_atom_number].AtomicLength - 12, source_file);
    return;
}

#if defined (_MSC_VER)
void APar_unicode_win32Printout(wchar_t* unicode_out) { //based on http://blogs.msdn.com/junfeng/archive/2004/02/25/79621.aspx
    //its possible that this isn't even available on windows95
    DWORD dwBytesWritten;
    DWORD fdwMode;
    HANDLE outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    // ThreadLocale adjustment, resource loading, etc. is skipped
    if ( (GetFileType(outHandle) & FILE_TYPE_CHAR) && GetConsoleMode( outHandle, &fdwMode) ) {
        if ( wmemcmp(unicode_out, L"\xEF\xBB\xBF", 3) != 0 ) { //skip BOM when writing directly to the console
            WriteConsoleW( outHandle, unicode_out, wcslen(unicode_out), &dwBytesWritten, 0);
        }
    }
    else {
        //http://www.ansi.edu.pk/library/WSLP/book4/html/ch08e.htm
        //WriteConsoleW above works best, but doesn't redirect well out to a file (AP.exe foo.mp4 -t > E:\tags.txt); here is where any redirection gets shunted
        //any utf8 that gets redirected here first gets read from the mpeg-4 file, converted to utf16 (carried on unsigned chars), put into wchar_t, and soon be converted back into utf8 for output to file - some round trip!

        int found_codepage = GetConsoleOutputCP();
        int set_codepage = SetConsoleCP(CP_UTF8);
        int charCount = WideCharToMultiByte(CP_UTF8, 0, unicode_out, -1, 0, 0, 0, 0);
        char* szaStr = (char*) malloc(charCount);
        WideCharToMultiByte( CP_UTF8, 0, unicode_out, -1, szaStr, charCount, 0, 0);
        WriteFile(outHandle, szaStr, charCount-1, &dwBytesWritten, 0);
        set_codepage = SetConsoleCP(found_codepage);//restore it back to what it was
        free(szaStr);
    }
    return;
}
#endif

void APar_fprintf_UTF8_data(char* utf8_encoded_data) {
#if defined (_MSC_VER)
    if (GetVersion() & 0x80000000 || UnicodeOutputStatus == UNIVERSAL_UTF8) {
        fprintf(stdout, "%s", utf8_encoded_data); //just printout the raw utf8 bytes (not characters) under pre-NT windows
    }
    else {
        wchar_t* utf16_data = Convert_multibyteUTF8_to_wchar(utf8_encoded_data);
        fflush(stdout);

        APar_unicode_win32Printout(utf16_data);

        fflush(stdout);
        free(utf16_data);
        utf16_data = NULL;
    }
#else
    fprintf(stdout, "%s", utf8_encoded_data);
#endif
    return;
}

void APar_PrintUnicodeAssest(char* unicode_string, int asset_length) { //3gp files
    if (memcmp(unicode_string, "\xFE\xFF", 2) == 0) { //utf16
        fprintf(stdout, " (utf16)] : ");

#if defined (_MSC_VER)
        if (GetVersion() & 0x80000000 || UnicodeOutputStatus == UNIVERSAL_UTF8) { //pre-NT or AP-utf8.exe (pish, thats my win98se, and without unicows support convert utf16toutf8 and output raw bytes)
            unsigned char* utf8_data = Convert_multibyteUTF16_to_UTF8(unicode_string, (asset_length -13) * 6, asset_length-14);
            fprintf(stdout, "%s", utf8_data);

            free(utf8_data);
            utf8_data = NULL;

        }
        else {
            wchar_t* utf16_data = Convert_multibyteUTF16_to_wchar(unicode_string, (asset_length - 16) / 2, true);
            //wchar_t* utf16_data = Convert_multibyteUTF16_to_wchar(unicode_string, ((asset_length - 16) / 2) + 1, true);
            APar_unicode_win32Printout(utf16_data);

            free(utf16_data);
            utf16_data = NULL;
        }
#else
        unsigned char* utf8_data = Convert_multibyteUTF16_to_UTF8(unicode_string, (asset_length - 13) * 6, asset_length
                - 14);
        fprintf(stdout, "%s", utf8_data);

        free(utf8_data);
        utf8_data = NULL;
#endif

    }
    else { //utf8
        fprintf(stdout, " (utf8)] : ");

        APar_fprintf_UTF8_data(unicode_string);

    }
    return;
}

//the difference between APar_PrintUnicodeAssest above and APar_SimplePrintUnicodeAssest below is:
//APar_PrintUnicodeAssest contains the entire contents of the atom, NULL bytes and all
//APar_SimplePrintUnicodeAssest contains a purely unicode string (either utf8 or utf16 with BOM)
//and slight output formatting differences

void APar_SimplePrintUnicodeAssest(char* unicode_string, int asset_length, bool print_encoding) { //3gp files
    if (memcmp(unicode_string, "\xFE\xFF", 2) == 0) { //utf16
        if (print_encoding) {
            fprintf(stdout, " (utf16): ");
        }

#if defined (_MSC_VER)
        if (GetVersion() & 0x80000000 || UnicodeOutputStatus == UNIVERSAL_UTF8) { //pre-NT or AP-utf8.exe (pish, thats my win98se, and without unicows support convert utf16toutf8 and output raw bytes)
            unsigned char* utf8_data = Convert_multibyteUTF16_to_UTF8(unicode_string, asset_length * 6, asset_length-14);
            fprintf(stdout, "%s", utf8_data);

            free(utf8_data);
            utf8_data = NULL;

        }
        else {
            wchar_t* utf16_data = Convert_multibyteUTF16_to_wchar(unicode_string, asset_length / 2, true);
            //wchar_t* utf16_data = Convert_multibyteUTF16_to_wchar(unicode_string, (asset_length / 2) + 1, true);
            APar_unicode_win32Printout(utf16_data);

            free(utf16_data);
            utf16_data = NULL;
        }
#else
        unsigned char* utf8_data = Convert_multibyteUTF16_to_UTF8(unicode_string, asset_length * 6, asset_length);
        fprintf(stdout, "%s", utf8_data);

        free(utf8_data);
        utf8_data = NULL;
#endif

    }
    else { //utf8
        if (print_encoding) {
            fprintf(stdout, " (utf8): ");
        }

        APar_fprintf_UTF8_data(unicode_string);

    }
    return;
}

/*----------------------
 APar_print_ISO_UserData_per_track

 This will only show what is under moov.trak.udta atoms (not moov.udta). Get the total number of tracks; construct the moov.trak[index].udta path to find,
 then if the atom after udta is of a greater level, read in from the file & print out what it contains.
 ----------------------*/
void APar_print_ISO_UserData_per_track() {
    uint8_t total_tracks = 0;
    uint8_t a_track = 0; //unused
    short an_atom = 0; //unused
    short a_trak_atom = 0;
    char iso_atom_path[400];
    AtomicInfo* trak_udtaAtom = NULL;

    APar_TrackInfo(total_tracks, a_track, an_atom); //With track_num set to 0, it will return the total trak atom into total_tracks here.

    for (uint8_t i = 1; i <= total_tracks; i++) {
        memset(&iso_atom_path, 0, 400);
        sprintf(iso_atom_path, "moov.trak[%u].udta", i);

        trak_udtaAtom = APar_FindAtom(iso_atom_path, false, SIMPLE_ATOM, 0);

        fprintf(stdout, "Track %i:\n", i);

        if (trak_udtaAtom != NULL
                && parsedAtoms[trak_udtaAtom->NextAtomNumber].AtomicLevel == trak_udtaAtom->AtomicLevel + 1) {
            a_trak_atom = trak_udtaAtom->NextAtomNumber;
            while (parsedAtoms[a_trak_atom].AtomicLevel == trak_udtaAtom->AtomicLevel + 1) { //only work on moov.trak[i].udta's child atoms

                char bitpacked_lang[3];
                memset(bitpacked_lang, 0, 3);
                unsigned char unpacked_lang[3];

                uint32_t box_length = parsedAtoms[a_trak_atom].AtomicLength;
                char* box_data = (char*) malloc(sizeof(char) * box_length);
                memset(box_data, 0, sizeof(char) * box_length);

                if (memcmp(parsedAtoms[a_trak_atom].AtomicName, "cprt", 4) == 0) {
                    fprintf(stdout, " Copyright ");
                }
                else {
                    fprintf(stdout, " Atom \"%s\" ", parsedAtoms[a_trak_atom].AtomicName);
                }

                uint16_t packed_lang = APar_read16(bitpacked_lang, source_file, parsedAtoms[a_trak_atom].AtomicStart
                        + 12);
                APar_UnpackLanguage(unpacked_lang, packed_lang);

                APar_readX(box_data, source_file, parsedAtoms[a_trak_atom].AtomicStart + 14, box_length - 14); //4bytes length, 4 bytes name, 4 bytes flags, 2 bytes lang
                fprintf(stdout, "[lang=%s", unpacked_lang);
                APar_PrintUnicodeAssest(box_data, box_length);
                fprintf(stdout, "\n");

                free(box_data);
                box_data = NULL;

                a_trak_atom = parsedAtoms[a_trak_atom].NextAtomNumber;
            }
        }
        else {
            fprintf(stdout, " No user data for this track.\n");
        }
    }

    return;
}

void APar_PrintUserDataAssests() { //3gp files

#if defined (UTF8_ENABLED)
    fprintf(stdout, "\xEF\xBB\xBF"); //Default to output of a UTF-8 BOM (except under win32's WriteConsoleW where it gets transparently eliminated)
#elif defined (_MSC_VER)
    if (UnicodeOutputStatus == WIN32_UTF16) {
        APar_unicode_win32Printout(L"\xEF\xBB\xBF"); //TODO: is this necessary? was it ever???
    }
#endif

    AtomicInfo* udtaAtom = APar_FindAtom("moov.udta", false, SIMPLE_ATOM, 0);

    if (udtaAtom == NULL)
        return;

    for (int i = udtaAtom->NextAtomNumber; i < atom_number; i++) {
        if (parsedAtoms[i].AtomicLevel <= udtaAtom->AtomicLevel) { //we've gone too far
            break;
        }
        if (parsedAtoms[i].AtomicLevel == udtaAtom->AtomicLevel + 1) {

            uint32_t box = UInt32FromBigEndian(parsedAtoms[i].AtomicName);

            char bitpacked_lang[3];
            memset(bitpacked_lang, 0, 3);
            unsigned char unpacked_lang[3];

            uint32_t box_length = parsedAtoms[i].AtomicLength;
            char* box_data = (char*) malloc(sizeof(char) * box_length);
            memset(box_data, 0, sizeof(char) * box_length);

            switch (box) {
            case 0x7469746C: //'titl'
            case 0x64736370: //'dscp'
            case 0x63707274: //'cprt'
            case 0x70657266: //'perf'
            case 0x61757468: //'auth'
            case 0x676E7265: //'gnre'
            case 0x616C626D: //'albm'
            {
                fprintf(stdout, "User data \"%s\" ", parsedAtoms[i].AtomicName);

                uint16_t packed_lang = APar_read16(bitpacked_lang, source_file, parsedAtoms[i].AtomicStart + 12);
                APar_UnpackLanguage(unpacked_lang, packed_lang);

                APar_readX(box_data, source_file, parsedAtoms[i].AtomicStart + 14, box_length - 14); //4bytes length, 4 bytes name, 4 bytes flags, 2 bytes lang

                //get tracknumber *after* we read the whole tag; if we have a utf16 tag, it will have a BOM, indicating if we have to search for 2 NULLs or a utf8 single NULL, then the ****optional**** tracknumber
                uint16_t track_num = 1000; //tracknum is a uint8_t, so setting it > 256 means a number wasn't found
                if (box == 0x616C626D) { //'albm' has an *optional* uint8_t at the end for tracknumber; if the last byte in the tag is not 0, then it must be the optional tracknum (or a non-compliant, non-NULL-terminated string). This byte is the length - (14 bytes +1tracknum) or -15
                    if (box_data[box_length - 15] != 0) {
                        track_num = (uint16_t) box_data[box_length - 15];
                        box_data[box_length - 15] = 0; //NULL out the last byte if found to be not 0 - it will impact unicode conversion if it remains
                    }
                }

                fprintf(stdout, "[lang=%s", unpacked_lang);

                APar_PrintUnicodeAssest(box_data, box_length);

                if (box == 0x616C626D && track_num != 1000) {
                    fprintf(stdout, "  |  Track: %u", track_num);
                }
                fprintf(stdout, "\n");
                break;
            }

            case 0x72746E67: //'rtng'
            {
                fprintf(stdout, "User data \"%s\" ", parsedAtoms[i].AtomicName);

                APar_readX(box_data, source_file, parsedAtoms[i].AtomicStart + 12, 4);

                fprintf(stdout, "[Rating Entity=%s", box_data);
                memset(box_data, 0, box_length);
                APar_readX(box_data, source_file, parsedAtoms[i].AtomicStart + 16, 4);
                fprintf(stdout, " | Criteria=%s", box_data);

                uint16_t packed_lang = APar_read16(bitpacked_lang, source_file, parsedAtoms[i].AtomicStart + 20);
                APar_UnpackLanguage(unpacked_lang, packed_lang);
                fprintf(stdout, " lang=%s", unpacked_lang);

                memset(box_data, 0, box_length);
                APar_readX(box_data, source_file, parsedAtoms[i].AtomicStart + 22, box_length - 8);

                APar_PrintUnicodeAssest(box_data, box_length - 8);
                fprintf(stdout, "\n");
                break;
            }

            case 0x636C7366: //'clsf'
            {
                fprintf(stdout, "User data \"%s\" ", parsedAtoms[i].AtomicName);

                APar_readX(box_data, source_file, parsedAtoms[i].AtomicStart + 12, box_length - 12); //4bytes length, 4 bytes name, 4 bytes flags, 2 bytes lang

                fprintf(stdout, "[Classification Entity=%s", box_data);
                fprintf(stdout, " | Index=%u", UInt16FromBigEndian(box_data + 4));

                uint16_t packed_lang = APar_read16(bitpacked_lang, source_file, parsedAtoms[i].AtomicStart + 18);
                APar_UnpackLanguage(unpacked_lang, packed_lang);
                fprintf(stdout, " lang=%s", unpacked_lang);

                APar_PrintUnicodeAssest(box_data + 8, box_length - 8);
                fprintf(stdout, "\n");
                break;
            }

            case 0x6B797764: //'kywd'
            {
                fprintf(stdout, "User data \"%s\" ", parsedAtoms[i].AtomicName);

                uint32_t box_offset = 12;

                uint16_t packed_lang = APar_read16(bitpacked_lang, source_file, parsedAtoms[i].AtomicStart
                        + box_offset);
                box_offset += 2;

                APar_UnpackLanguage(unpacked_lang, packed_lang);

                uint8_t keyword_count = APar_read8(source_file, parsedAtoms[i].AtomicStart + box_offset);
                box_offset++;
                fprintf(stdout, "[Keyword count=%u", keyword_count);
                fprintf(stdout, " lang=%s]", unpacked_lang);

                char* keyword_data = (char*) malloc(sizeof(char) * box_length * 2);

                for (uint8_t x = 1; x <= keyword_count; x++) {
                    memset(keyword_data, 0, box_length * 2);
                    uint8_t keyword_length = APar_read8(source_file, parsedAtoms[i].AtomicStart + box_offset);
                    box_offset++;

                    APar_readX(keyword_data, source_file, parsedAtoms[i].AtomicStart + box_offset, (uint32_t) keyword_length);
                    box_offset += keyword_length;
                    APar_SimplePrintUnicodeAssest(keyword_data, keyword_length, true);
                }
                free(keyword_data);
                keyword_data = NULL;

                fprintf(stdout, "\n");
                break;
            }

            case 0x6C6F6369: //'loci' aka The Most Heinous Metadata Atom Every Invented - decimal meters? fictional location? Astromical Body? Say I shoot it on the International Space Station? That isn't a Astronimical Body. And 16.16 alt only goes up to 20.3 miles (because of negatives, its really 15.15) & the ISS is at 230 miles. Oh, pish.... what ever shall I do? I fear I am on the horns of a dilema.
            {
                fprintf(stdout, "User data \"%s\" ", parsedAtoms[i].AtomicName);

                uint32_t box_offset = 12;
                uint16_t packed_lang = APar_read16(bitpacked_lang, source_file, parsedAtoms[i].AtomicStart
                        + box_offset);
                box_offset += 2;

                APar_UnpackLanguage(unpacked_lang, packed_lang);

                APar_readX(box_data, source_file, parsedAtoms[i].AtomicStart + box_offset, box_length);
                fprintf(stdout, "[lang=%s] ", unpacked_lang);

                //the length of the location string is unknown (max is box lenth), but the long/lat/alt/body/notes needs to be retrieved.
                //test if the location string is utf16; if so search for 0x0000 (or if utf8, find the first NULL).
                if (memcmp(box_data, "\xFE\xFF", 2) == 0) {
                    box_offset += 2 * widechar_len(box_data, box_length) + 2; //*2 for utf16 (double-byte); +2 for the terminating NULL
                    fprintf(stdout, "(utf16) ");
                }
                else {
                    fprintf(stdout, "(utf8) ");
                    box_offset += strlen(box_data) + 1; //+1 for the terminating NULL
                }
                fprintf(stdout, "Location: ");
                APar_SimplePrintUnicodeAssest(box_data, box_length, false);

                uint8_t location_role = APar_read8(source_file, parsedAtoms[i].AtomicStart + box_offset);
                box_offset++;
                switch (location_role) {
                case 0: {
                    fprintf(stdout, " (Role: shooting location) ");
                    break;
                }
                case 1: {
                    fprintf(stdout, " (Role: real location) ");
                    break;
                }
                case 2: {
                    fprintf(stdout, " (Role: fictional location) ");
                    break;
                }
                default: {
                    fprintf(stdout, " (Role: [reserved]) ");
                    break;
                }
                }

                char* float_buffer = (char*) malloc(sizeof(char) * 5);
                memset(float_buffer, 0, 5);

                fprintf(stdout, "[Long %lf", fixed_point_16x16bit_to_double(APar_read32(float_buffer, source_file, parsedAtoms[i].AtomicStart
                        + box_offset)));
                box_offset += 4;
                fprintf(stdout, " Lat %lf", fixed_point_16x16bit_to_double(APar_read32(float_buffer, source_file, parsedAtoms[i].AtomicStart
                        + box_offset)));
                box_offset += 4;
                fprintf(stdout, " Alt %lf ", fixed_point_16x16bit_to_double(APar_read32(float_buffer, source_file, parsedAtoms[i].AtomicStart
                        + box_offset)));
                box_offset += 4;
                free(float_buffer);
                float_buffer = NULL;

                if (box_offset < box_length) {
                    fprintf(stdout, " Body: ");
                    APar_SimplePrintUnicodeAssest(box_data + box_offset - 14, box_length - box_offset, false);
                    if (memcmp(box_data + box_offset - 14, "\xFE\xFF", 2) == 0) {
                        box_offset += 2 * widechar_len(box_data + box_offset - 14, box_length - box_offset) + 2; //*2 for utf16 (double-byte); +2 for the terminating NULL
                    }
                    else {
                        box_offset += strlen(box_data + box_offset - 14) + 1; //+1 for the terminating NULL
                    }
                }
                fprintf(stdout, "]");

                if (box_offset < box_length) {
                    fprintf(stdout, " Notes: ");
                    APar_SimplePrintUnicodeAssest(box_data + box_offset - 14, box_length - box_offset, false);
                }

                fprintf(stdout, "\n");
                break;
            }

            case 0x79727263: //'yrrc'
            {
                fprintf(stdout, "User data \"%s\" ", parsedAtoms[i].AtomicName);

                uint16_t recording_year = APar_read16(bitpacked_lang, source_file, parsedAtoms[i].AtomicStart + 12);
                fprintf(stdout, ": %u\n", recording_year);
                break;
            }

            default: {
                break;
            }
            }
            free(box_data);
            box_data = NULL;
        }
    }
    return;
}

/*----------------------
 APar_Extract_uuid_binary_file
 uuid_atom - pointer to the struct holding the information describing the target atom
 originating_file - the full file path string to the parsed file
 output_path - a (possibly null) string where the embedded file will be extracted to

 If the output path is a null pointer, create a new path derived from originating file name & path - strip off the extension and use that as the base. Read into
 memory the contents of that particular uuid atom. Glob onto the base path the atom name and then the suffix that was embedded along with the file. Write out
 the file to the now fully formed uuid_outfile path.
 ----------------------*/
void APar_Extract_uuid_binary_file(AtomicInfo* uuid_atom, const char* originating_file, char* output_path) {
    uint32_t path_len = 0;
    uint32_t atom_offsets = 0;
    char* uuid_outfile = (char*) calloc(1, sizeof(char) * MAXPATHLEN + 1); //malloc a new string because it may be a cli arg for a specific output path
    if (output_path == NULL) {
        const char* orig_suffix = strrchr(originating_file, '.');
        if (orig_suffix == NULL) {
            fprintf(stdout, "AP warning: a file extension for the input file was not found.\n\tGlobbing onto original filename...\n");
            path_len = strlen(originating_file);
            memcpy(uuid_outfile, originating_file, path_len);
        }
        else {
            path_len = orig_suffix - originating_file;
            memcpy(uuid_outfile, originating_file, path_len);
        }

    }
    else {
        path_len = strlen(output_path);
        memcpy(uuid_outfile, output_path, path_len);
    }
    char* uuid_payload = (char*) calloc(1, sizeof(char) * (uuid_atom->AtomicLength - 36 + 1));

    fseeko(source_file, uuid_atom->AtomicStart + 36, SEEK_SET);
    fread(uuid_payload, 1, uuid_atom->AtomicLength - 36, source_file);

    uint32_t descrip_len = UInt32FromBigEndian(uuid_payload);
    atom_offsets += 4 + descrip_len;
    uint8_t suffix_len = (uint8_t) uuid_payload[atom_offsets];

    char* file_suffix = (char*) calloc(1, sizeof(char) * suffix_len + 16);
    memcpy(file_suffix, uuid_payload + atom_offsets + 1, suffix_len);
    atom_offsets += 1 + suffix_len;

    uint8_t mime_len = (uint8_t) uuid_payload[atom_offsets];
    uint32_t mimetype_string = atom_offsets + 1;
    atom_offsets += 1 + mime_len;
    uint32_t bin_len = UInt32FromBigEndian(uuid_payload + atom_offsets);
    atom_offsets += 4;

    sprintf(uuid_outfile + path_len, "-%s-uuid%s", uuid_atom->uuid_ap_atomname, file_suffix);

    FILE *outfile = APar_OpenFile(uuid_outfile, "wb");
    if (outfile != NULL) {
        fwrite(uuid_payload + atom_offsets, (size_t) bin_len, 1, outfile);
        fclose(outfile);
        fprintf(stdout, "Extracted uuid=%s attachment (mime-type=%s) to file: ", uuid_atom->uuid_ap_atomname, uuid_payload
                + mimetype_string);
        APar_fprintf_UTF8_data(uuid_outfile);
        fprintf(stdout, "\n");
    }

    free(uuid_payload);
    uuid_payload = NULL;
    free(uuid_outfile);
    uuid_outfile = NULL;
    free(file_suffix);
    file_suffix = NULL;
    return;
}

/**
 * Extract the artwork from the given atom to a file using the
 * pic_output_prefix as the basis of the returned artwork file.
 *
 * Returns the final path of the resulting artwork file. This path
 * must be freed.
 */
char* APar_ExtractAAC_Artwork(short this_atom_num, char* pic_output_prefix, short artwork_count) {
    char *base_outpath = (char *) malloc(sizeof(char) * MAXPATHLEN + 1);
    memset(base_outpath, 0, MAXPATHLEN + 1);

    strcpy(base_outpath, pic_output_prefix);
    strcat(base_outpath, "_artwork");
    sprintf(base_outpath, "%s_%d", base_outpath, artwork_count);

    char* art_payload = (char*) malloc(sizeof(char) * (parsedAtoms[this_atom_num].AtomicLength - 16) + 1);
    memset(art_payload, 0, (parsedAtoms[this_atom_num].AtomicLength - 16) + 1);

    fseeko(source_file, parsedAtoms[this_atom_num].AtomicStart + 16, SEEK_SET);
    fread(art_payload, 1, parsedAtoms[this_atom_num].AtomicLength - 16, source_file);

    const char* suffix = (char *) malloc(sizeof(char) * 5);
    if (memcmp(art_payload, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8) == 0) {
        suffix = ".png";
    }
    else if (memcmp(art_payload, "\xFF\xD8\xFF\xE0", 4) == 0 || memcmp(art_payload, "\xFF\xD8\xFF\xE1", 4) == 0) {
        suffix = ".jpg";
    }

    strcat(base_outpath, suffix);

    FILE *outfile = APar_OpenFile(base_outpath, "wb");
    if (outfile != NULL) {
        fwrite(art_payload, (size_t) (parsedAtoms[this_atom_num].AtomicLength - 16), 1, outfile);
        fclose(outfile);
        fprintf(stdout, "Extracted artwork to file: ");
        APar_fprintf_UTF8_data(base_outpath);
        fprintf(stdout, "\n");
    }
    free(art_payload);

    return base_outpath;
}

char* APar_ExtractDataAtom(int this_atom_number) {
    if (!source_file)
        return NULL;

    AtomicInfo* thisAtom = &parsedAtoms[this_atom_number];
    char* parent_atom_name;

    AtomicInfo parent_atom_stats = parsedAtoms[this_atom_number - 1];
    parent_atom_name = parent_atom_stats.AtomicName;

    uint32_t min_atom_datasize = 12;
    uint32_t atom_header_size = 16;

    if (thisAtom->AtomicClassification == EXTENDED_ATOM) {
        if (thisAtom->uuid_style == UUID_DEPRECATED_FORM) {
            min_atom_datasize += 4;
            atom_header_size += 4;
        }
        else {
            min_atom_datasize = 36;
            atom_header_size = 36;
        }
    }

    if (thisAtom->AtomicLength <= min_atom_datasize)
        return NULL;

    char* data_payload = (char*) malloc(sizeof(char) * (thisAtom->AtomicLength - atom_header_size + 1));
    memset(data_payload, 0, sizeof(char) * (thisAtom->AtomicLength - atom_header_size + 1));

    fseeko(source_file, thisAtom->AtomicStart + atom_header_size, SEEK_SET);
    fread(data_payload, 1, thisAtom->AtomicLength - atom_header_size, source_file);

    if (thisAtom->AtomicVerFlags == (uint32_t) AtomFlags_Data_Text) {
        if (thisAtom->AtomicLength < (atom_header_size + 4)) {
            //tvnn was showing up with 4 chars instead of 3; easier to null it out for now
            data_payload[thisAtom->AtomicLength - atom_header_size] = '\00';
        }

        return data_payload;

    }
    else {
        if ((memcmp(parent_atom_name, "trkn", 4) == 0) || (memcmp(parent_atom_name, "disk", 4) == 0)) {
            char* result = (char*) malloc(sizeof(char) * 40);

            if (UInt16FromBigEndian(data_payload + 4) != 0) {
                sprintf(result, "%hu of %hu\n", UInt16FromBigEndian(data_payload + 2), UInt16FromBigEndian(data_payload
                        + 4));
            }
            else {
                sprintf(result, "%hu\n", UInt16FromBigEndian(data_payload + 2));
            }

            return result;

        }
        else if (strncmp(parent_atom_name, "gnre", 4) == 0) {
            if (thisAtom->AtomicLength - atom_header_size < 3) { //oh, a 1byte int for genre number
                char* genre_string = GenreIntToString(UInt16FromBigEndian(data_payload));
                if (genre_string != NULL) {
                    return genre_string;
                }
                else {
                    char* result = (char*) malloc(sizeof(char) * 40);
                    sprintf(result, " out of bound value - %hu\n", UInt16FromBigEndian(data_payload));
                    return result;
                }
            }
            else {
                char* result = (char*) malloc(sizeof(char) * 40);
                sprintf(result, " out of bound value - %hu\n", UInt16FromBigEndian(data_payload));
                return result;
            }

        }
        else if ((strncmp(parent_atom_name, "purl", 4) == 0) || (strncmp(parent_atom_name, "egid", 4) == 0)) {
            return data_payload;

        }
        else {
            if (thisAtom->AtomicVerFlags == AtomFlags_Data_UInt
                    && (thisAtom->AtomicLength <= 20 || thisAtom->AtomicLength == 24)) {
                uint8_t bytes_rep = thisAtom->AtomicLength - atom_header_size;

                switch (bytes_rep) {
                case 1: {
                    if ((memcmp(parent_atom_name, "cpil", 4) == 0) || (memcmp(parent_atom_name, "pcst", 4) == 0)) {
                        char* result = (char*) malloc(sizeof(char) * 6);
                        if (data_payload[0] == 1) {
                            sprintf(result, "true");
                        }
                        else {
                            sprintf(result, "false");
                        }

                        return result;

                    }
                    else if (strncmp(parent_atom_name, "stik", 4) == 0) {
                        stiks* returned_stik = MatchStikNumber((uint8_t) data_payload[0]);
                        if (returned_stik != NULL) {
                            return strdup(returned_stik->stik_string);
                        }
                        else {
                            char* result = (char*) malloc(sizeof(char) * 40);
                            sprintf(result, "Unknown value: %hhu\n", (uint8_t) data_payload[0]);
                            return result;
                        }

                    }
                    else if (strncmp(parent_atom_name, "rtng", 4) == 0) { //okay, this is definitely an 8-bit number
                        char* result = (char*) malloc(sizeof(char) * 20);

                        if (data_payload[0] == 2) {
                            sprintf(result, "Clean Content");
                        }
                        else if (data_payload[0] != 0) {
                            sprintf(result, "Explicit Content");
                        }
                        else {
                            sprintf(result, "Inoffensive");
                        }

                        return result;

                    }
                    else {
                        char* result = (char*) malloc(sizeof(char) * 4);
                        sprintf(result, "%hhu\n", data_payload[0]);
                        return result;
                    }
                }
                case 2: { //tmpo
                    char* result = (char*) malloc(sizeof(char) * 7); // 5 for tempo value, up to 65535, 2 for \n\0
                    sprintf(result, "%hu\n", UInt16FromBigEndian(data_payload));
                    return result;
                }
                case 4: { //tves, tvsn
                    if (memcmp(parent_atom_name, "sfID", 4) == 0) {
                        sfIDs* this_store = MatchStoreFrontNumber(UInt32FromBigEndian(data_payload));
                        if (this_store != NULL) {
                            char* result = (char*) malloc(sizeof(char) * (strlen(this_store->storefront_string) + 4));
                            sprintf(result, "%s (%u)\n", this_store->storefront_string, this_store->storefront_number);
                            return result;
                        }
                        else {
                            char* result = (char*) malloc(sizeof(char) * 12);
                            sprintf(result, "Unknown (%u)\n", UInt32FromBigEndian(data_payload));
                            return result;
                        }

                    }
                    else {
                        char* result = (char*) malloc(sizeof(char) * 4);
                        sprintf(result, "%u\n", UInt32FromBigEndian(data_payload));
                        return result;
                    }
                }
                case 8: {
                    char* result = (char*) malloc(sizeof(char) * 4);
                    sprintf(result, "%llu\n", UInt64FromBigEndian(data_payload));
                    return result;
                }
                }

            }
            else if (thisAtom->AtomicClassification == EXTENDED_ATOM
                    && thisAtom->AtomicVerFlags == AtomFlags_Data_uuid_binary
                    && thisAtom->uuid_style == UUID_AP_SHA1_NAMESPACE) {
                uint32_t offset_into_uuiddata = 0;
                uint32_t descrip_len = UInt32FromBigEndian(data_payload);
                offset_into_uuiddata += 4;

                char* uuid_description = (char*) calloc(1, sizeof(char) * descrip_len + 16); //char uuid_description[descrip_len+1];
                memcpy(uuid_description, data_payload + offset_into_uuiddata, descrip_len);
                offset_into_uuiddata += descrip_len;

                uint8_t suffix_len = (uint8_t) data_payload[offset_into_uuiddata];
                offset_into_uuiddata += 1;

                char* file_suffix = (char*) calloc(1, sizeof(char) * suffix_len + 16); //char file_suffix[suffix_len+1];
                memcpy(file_suffix, data_payload + offset_into_uuiddata, suffix_len);
                offset_into_uuiddata += suffix_len;

                uint8_t mime_len = (uint8_t) data_payload[offset_into_uuiddata];
                offset_into_uuiddata += 1;

                char* uuid_mimetype = (char*) calloc(1, sizeof(char) * mime_len + 16); //char uuid_mimetype[mime_len+1];
                memcpy(uuid_mimetype, data_payload + offset_into_uuiddata, mime_len);

                char* result = (char*) malloc(sizeof(char)
                        * (35 + strlen(file_suffix) + strlen(uuid_mimetype) + strlen(uuid_description)));
                sprintf(result, "FILE%s; mime-type=%s; description=%s\n", file_suffix, uuid_mimetype, uuid_description);

                free(uuid_description);
                uuid_description = NULL;
                free(file_suffix);
                file_suffix = NULL;
                free(uuid_mimetype);
                uuid_description = NULL;
                return result;

            }
            else { //purl & egid would end up here too, but Apple switched it to a text string (0x00), so gets taken care above explicitly
                char* result = (char*) malloc(sizeof(char) * 7);
                sprintf(result, "hex 0x");

                for (int hexx = 1; hexx <= (int) (thisAtom->AtomicLength - atom_header_size); ++hexx) {

                    char result2[5];
                    sprintf(result2, "%02X", (uint8_t) data_payload[hexx - 1]);
                    strcat(result, result2);

                    if ((hexx % 4) == 0 && hexx >= 4) {
                        strcat(result, " ");
                    }
                    if ((hexx % 16) == 0 && hexx > 16) {
                        strcat(result, "\n\t\t\t");
                    }
                    if (hexx == (int) (thisAtom->AtomicLength - atom_header_size)) {
                        strcat(result, "\n");
                    }
                }
            } //end if AtomFlags_Data_UInt
        }
    }

    return NULL;
}

void APar_PrintDataAtoms(const char *path, char* output_path, uint8_t supplemental_info, uint8_t target_information) {

#if defined (UTF8_ENABLED)
    fprintf(stdout, "\xEF\xBB\xBF"); //Default to output of a UTF-8 BOM (except under win32's WriteConsoleW where it gets transparently eliminated)
#elif defined (_MSC_VER)
    if (UnicodeOutputStatus == WIN32_UTF16) {
        APar_unicode_win32Printout(L"\xEF\xBB\xBF"); //TODO: is this necessary? was it ever???
    }
#endif

    short artwork_count = 0;

    for (int i = 0; i < atom_number; i++) {
        AtomicInfo* thisAtom = &parsedAtoms[i];

        if (strncmp(thisAtom->AtomicName, "data", 4) == 0) { //thisAtom->AtomicClassification == VERSIONED_ATOM) {

            AtomicInfo* parent = &parsedAtoms[APar_FindParentAtom(i, thisAtom->AtomicLevel)];

            if ((thisAtom->AtomicVerFlags == (uint32_t) AtomFlags_Data_Binary
                    || thisAtom->AtomicVerFlags == (uint32_t) AtomFlags_Data_Text
                    || thisAtom->AtomicVerFlags == (uint32_t) AtomFlags_Data_UInt)
                    && target_information == PRINT_DATA) {
                if (strncmp(parent->AtomicName, "----", 4) == 0) {
                    if (memcmp(parsedAtoms[i - 1].AtomicName, "name", 4) == 0) {
                        fprintf(stdout, "Atom \"%s\" [%s] contains: ", parent->AtomicName, parsedAtoms[i - 1].ReverseDNSname);
                        APar_ExtractDataAtom(i);
                    }

                }
                else if (memcmp(parent->AtomicName, "covr", 4) == 0) { //libmp4v2 doesn't properly set artwork with the right flags (its all 0x00)
                    artwork_count++;

                }
                else {
                    //converts iso8859 � in '�ART' to a 2byte utf8 � glyph; replaces libiconv conversion
                    memset(twenty_byte_buffer, 0, sizeof(char) * 20);
                    isolat1ToUTF8((unsigned char*) twenty_byte_buffer, 10, (unsigned char*) parent->AtomicName, 4);

                    if (UnicodeOutputStatus == WIN32_UTF16) {
                        fprintf(stdout, "Atom \"");
                        APar_fprintf_UTF8_data(twenty_byte_buffer);
                        fprintf(stdout, "\" contains: ");
                    }
                    else {
                        fprintf(stdout, "Atom \"%s\" contains: ", twenty_byte_buffer);
                    }

                    APar_ExtractDataAtom(i);
                }

            }
            else if (memcmp(parent->AtomicName, "covr", 4) == 0) {
                artwork_count++;
                if (target_information == EXTRACT_ARTWORK) {
                    char* path = APar_ExtractAAC_Artwork(thisAtom->AtomicNumber, output_path, artwork_count);
                    free(path);
                }
            }
            if (thisAtom->AtomicLength <= 12) {
                fprintf(stdout, "\n"); // (corrupted atom); libmp4v2 touching a file with copyright
            }
        }
        else if (thisAtom->AtomicClassification == EXTENDED_ATOM && thisAtom->uuid_style == UUID_DEPRECATED_FORM) {
            memset(twenty_byte_buffer, 0, sizeof(char) * 20);
            //converts iso8859 � in '�foo' to a 2byte utf8 � glyph; replaces libiconv conversion
            isolat1ToUTF8((unsigned char*) twenty_byte_buffer, 10, (unsigned char*) thisAtom->AtomicName, 4);

            if (thisAtom->AtomicVerFlags == (uint32_t) AtomFlags_Data_Text && target_information == PRINT_DATA) {

                if (UnicodeOutputStatus == WIN32_UTF16) {
                    fprintf(stdout, "Atom uuid=\"");
                    APar_fprintf_UTF8_data(twenty_byte_buffer);
                    fprintf(stdout, "\" contains: ");
                }
                else {
                    fprintf(stdout, "Atom uuid=\"%s\" contains: ", twenty_byte_buffer);
                }

                APar_ExtractDataAtom(i);
            }
        }
        else if (thisAtom->AtomicClassification == EXTENDED_ATOM) {
            if (thisAtom->uuid_style == UUID_AP_SHA1_NAMESPACE) {
                memset(twenty_byte_buffer, 0, sizeof(char) * 20);
                if (target_information == PRINT_DATA) {
                    isolat1ToUTF8((unsigned char*) twenty_byte_buffer, 10, (unsigned char*) thisAtom->uuid_ap_atomname, 4);

                    fprintf(stdout, "Atom uuid=");
                    APar_print_uuid((ap_uuid_t*) thisAtom->AtomicName, false);
                    fprintf(stdout, " (AP uuid for \"");
                    APar_fprintf_UTF8_data(twenty_byte_buffer);
                    fprintf(stdout, "\") contains: ");

                    APar_ExtractDataAtom(i);
                }
                else {
                    if (target_information == EXTRACT_ALL_UUID_BINARYS
                            && thisAtom->AtomicVerFlags == AtomFlags_Data_uuid_binary) {
                        APar_Extract_uuid_binary_file(thisAtom, path, output_path);
                    }
                }
            }
        }
    }

    if (artwork_count != 0 && target_information == PRINT_DATA) {
        if (artwork_count == 1) {
            fprintf(stdout, "Atom \"covr\" contains: %i piece of artwork\n", artwork_count);
        }
        else {
            fprintf(stdout, "Atom \"covr\" contains: %i pieces of artwork\n", artwork_count);
        }
    }

    if (supplemental_info) {
        fprintf(stdout, "---------------------------\n");
        udta_dynamics.dynamic_updating = false;
        APar_DetermineDynamicUpdate(true); //gets the size of the padding
        APar_Optimize(true); //just to know if 'free' atoms can be considered padding, or (in the case of say a faac file) it's *just* 'free'

        if (supplemental_info && 0x02) { //PRINT_FREE_SPACE
            fprintf(stdout, "free atom space: %u\n", APar_ProvideTallyForAtom("free"));
        }
        if (supplemental_info && 0x04) { //PRINT_PADDING_SPACE
            if (!moov_atom_was_mooved) {
                fprintf(stdout, "padding available: %u bytes\n", udta_dynamics.max_usable_free_space);
            }
            else {
                fprintf(stdout, "padding available: 0 (reorg)\n");
            }
        }
        if (supplemental_info && 0x08 && udtaAtom != NULL) { //PRINT_USER_DATA_SPACE
            fprintf(stdout, "user data space: %u\n", udtaAtom->AtomicLength);
        }
        if (supplemental_info && 0x10) { //PRINT_USER_DATA_SPACE
            fprintf(stdout, "media data space: %u\n", APar_ProvideTallyForAtom("mdat"));
        }
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                                 Atom Tree                                         //
///////////////////////////////////////////////////////////////////////////////////////

//this function reflects the atom tree as it stands in memory accurately (so I hope).
void APar_PrintAtomicTree() {
    bool unknown_atom = false;

#if defined (UTF8_ENABLED)
    fprintf(stdout, "\xEF\xBB\xBF"); //Default to output of a UTF-8 BOM (except under win32's WriteConsoleW where it gets transparently eliminated)
#elif defined (_MSC_VER)
    if (UnicodeOutputStatus == WIN32_UTF16) {
        APar_unicode_win32Printout(L"\xEF\xBB\xBF"); //TODO: is this necessary? was it ever???
    }
#endif

    char* tree_padding = (char*) malloc(sizeof(char) * 126); //for a 25-deep atom tree (4 spaces per atom)+single space+term.
    uint32_t freeSpace = 0;
    short thisAtomNumber = 0;

    //loop through each atom in the struct array (which holds the offset info/data)
    while (true) {
        AtomicInfo* thisAtom = &parsedAtoms[thisAtomNumber];
        memset(tree_padding, 0, sizeof(char) * 126);
        memset(twenty_byte_buffer, 0, sizeof(char) * 20);

        if (thisAtom->uuid_ap_atomname != NULL) {
            isolat1ToUTF8((unsigned char*) twenty_byte_buffer, 10, (unsigned char*) thisAtom->uuid_ap_atomname, 4); //converts iso8859 � in '�ART' to a 2byte utf8 � glyph
        }
        else {
            isolat1ToUTF8((unsigned char*) twenty_byte_buffer, 10, (unsigned char*) thisAtom->AtomicName, 4); //converts iso8859 � in '�ART' to a 2byte utf8 � glyph
        }

        strcpy(tree_padding, "");
        if (thisAtom->AtomicLevel != 1) {
            for (uint8_t pad = 1; pad < thisAtom->AtomicLevel; pad++) {
                strcat(tree_padding, "    "); // if the atom depth is over 1, then add spaces before text starts to form the tree
            }
            strcat(tree_padding, " "); // add a single space
        }

        if (thisAtom->AtomicLength == 0) {
            fprintf(stdout, "%sAtom %s @ %u of size: %u (%u*), ends @ %u\n", tree_padding, twenty_byte_buffer, thisAtom->AtomicStart, ((uint32_t) file_size
                    - thisAtom->AtomicStart), thisAtom->AtomicLength, (uint32_t) file_size);
            fprintf(stdout, "\t\t\t (*)denotes length of atom goes to End-of-File\n");

        }
        else if (thisAtom->AtomicLength == 1) {
            fprintf(stdout, "%sAtom %s @ %u of size: %llu (^), ends @ %llu\n", tree_padding, twenty_byte_buffer, thisAtom->AtomicStart, thisAtom->AtomicLengthExtended, (thisAtom->AtomicStart
                    + thisAtom->AtomicLengthExtended));
            fprintf(stdout, "\t\t\t (^)denotes a 64-bit atom length\n");

            //uuid atoms of any sort
        }
        else if (thisAtom->AtomicClassification == EXTENDED_ATOM && thisAtom->uuid_style == UUID_DEPRECATED_FORM) {

            if (UnicodeOutputStatus == WIN32_UTF16) {
                fprintf(stdout, "%sAtom uuid=", tree_padding);
                APar_fprintf_UTF8_data(twenty_byte_buffer);
                fprintf(stdout, " @ %u of size: %u, ends @ %u\n", thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart
                        + thisAtom->AtomicLength));
            }
            else {
                fprintf(stdout, "%sAtom uuid=%s @ %u of size: %u, ends @ %u\n", tree_padding, twenty_byte_buffer, thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart
                        + thisAtom->AtomicLength));
            }

        }
        else if (thisAtom->AtomicClassification == EXTENDED_ATOM && thisAtom->uuid_style != UUID_DEPRECATED_FORM) {
            if (thisAtom->uuid_style == UUID_AP_SHA1_NAMESPACE) {
                fprintf(stdout, "%sAtom uuid=", tree_padding);
                APar_print_uuid((ap_uuid_t*) thisAtom->AtomicName, false);
                fprintf(stdout, "(APuuid=%s) @ %u of size: %u, ends @ %u\n", twenty_byte_buffer, thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart
                        + thisAtom->AtomicLength));
            }
            else {
                fprintf(stdout, "%sAtom uuid=", tree_padding);
                APar_print_uuid((ap_uuid_t*) thisAtom->AtomicName, false);
                fprintf(stdout, " @ %u of size: %u, ends @ %u\n", thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart
                        + thisAtom->AtomicLength));
            }

            //3gp assets (most of them anyway)
        }
        else if (thisAtom->AtomicClassification == PACKED_LANG_ATOM) {
            unsigned char unpacked_lang[3];
            APar_UnpackLanguage(unpacked_lang, thisAtom->AtomicLanguage);

            if (UnicodeOutputStatus == WIN32_UTF16) {
                fprintf(stdout, "%sAtom ", tree_padding);
                APar_fprintf_UTF8_data(twenty_byte_buffer);
                fprintf(stdout, " [%s] @ %u of size: %u, ends @ %u\n", unpacked_lang, thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart
                        + thisAtom->AtomicLength));
            }
            else {
                fprintf(stdout, "%sAtom %s [%s] @ %u of size: %u, ends @ %u\n", tree_padding, twenty_byte_buffer, unpacked_lang, thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart
                        + thisAtom->AtomicLength));
            }

            //all other atoms (the bulk of them will fall here)
        }
        else {

            if (UnicodeOutputStatus == WIN32_UTF16) {
                fprintf(stdout, "%sAtom ", tree_padding);
                APar_fprintf_UTF8_data(twenty_byte_buffer);
                fprintf(stdout, " @ %u of size: %u, ends @ %u", thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart
                        + thisAtom->AtomicLength));
            }
            else {
                fprintf(stdout, "%sAtom %s @ %u of size: %u, ends @ %u", tree_padding, twenty_byte_buffer, thisAtom->AtomicStart, thisAtom->AtomicLength, (thisAtom->AtomicStart
                        + thisAtom->AtomicLength));
            }

            if (thisAtom->AtomicContainerState == UNKNOWN_ATOM_TYPE) {
                for (uint8_t i = 0; i < (5 - thisAtom->AtomicLevel); i++) {
                    fprintf(stdout, "\t");
                }
                fprintf(stdout, "\t\t\t ~\n");
                unknown_atom = true;
            }
            else {
                fprintf(stdout, "\n");
            }
        }

        //simple tally & percentage of free space info
        if (memcmp(thisAtom->AtomicName, "free", 4) == 0) {
            freeSpace = freeSpace + thisAtom->AtomicLength;
        }
        //this is where the *raw* audio/video file is, the rest is container-related fluff.
        if ((memcmp(thisAtom->AtomicName, "mdat", 4) == 0) && (thisAtom->AtomicLength > 100)) {
            mdatData += thisAtom->AtomicLength;
        }
        else if (memcmp(thisAtom->AtomicName, "mdat", 4) == 0 && thisAtom->AtomicLength == 0) { //mdat.length = 0 = ends at EOF
            mdatData = (uint32_t) file_size - thisAtom->AtomicStart;
        }
        else if (memcmp(thisAtom->AtomicName, "mdat", 4) == 0 && thisAtom->AtomicLengthExtended != 0) {
            mdatData += thisAtom->AtomicLengthExtended; //this is still adding a (limited) uint64_t into a uint32_t
        }

        if (parsedAtoms[thisAtomNumber].NextAtomNumber == 0) {
            break;
        }
        else {
            thisAtomNumber = parsedAtoms[thisAtomNumber].NextAtomNumber;
        }
    }

    if (unknown_atom) {
        fprintf(stdout, "\n ~ denotes an unknown atom\n");
    }

    fprintf(stdout, "------------------------------------------------------\n");
    fprintf(stdout, "Total size: %llu bytes; ", (uint64_t) file_size);
    fprintf(stdout, "%i atoms total. ", atom_number - 1);
    ShowVersionInfo();
    fprintf(stdout, "Media data: %u bytes; %u bytes all other atoms (%2.3lf%% atom overhead).\n", mdatData, (uint32_t) (file_size
            - mdatData), (double) (file_size - mdatData) / (double) file_size * 100.0);
    fprintf(stdout, "Total free atom space: %u bytes; %2.3lf%% waste.", freeSpace, (double) freeSpace
            / (double) file_size * 100.0);
    if (freeSpace) {
        udta_dynamics.dynamic_updating = false;
        APar_DetermineDynamicUpdate(true); //gets the size of the padding
        APar_Optimize(true); //just to know if 'free' atoms can be considered padding, or (in the case of say a faac file) it's *just* 'free'
        if (!moov_atom_was_mooved) {
            fprintf(stdout, " Padding available: %u bytes.", udta_dynamics.max_usable_free_space);
        }
    }
    if (gapless_void_padding > 0) {
        fprintf(stdout, "\nGapless playback null space at end of file: %u bytes.", gapless_void_padding);
    }
    fprintf(stdout, "\n------------------------------------------------------\n");

    free(tree_padding);
    tree_padding = NULL;

    return;
}

void APar_SimpleAtomPrintout() { //loop through each atom in the struct array (which holds the offset info/data)

#if defined (UTF8_ENABLED)
    fprintf(stdout, "\xEF\xBB\xBF"); //Default to output of a UTF-8 BOM (except under win32's WriteConsoleW where it gets transparently eliminated)
#elif defined (_MSC_VER)
    if (UnicodeOutputStatus == WIN32_UTF16) {
        APar_unicode_win32Printout(L"\xEF\xBB\xBF"); //TODO: is this necessary? was it ever???
    }
#endif

    for (int i = 0; i < atom_number; i++) {
        AtomicInfo* thisAtom = &parsedAtoms[i];

        fprintf(stdout, "%i  -  Atom \"%s\" (level %u) has next atom at #%i\n", i, thisAtom->AtomicName, thisAtom->AtomicLevel, thisAtom->NextAtomNumber);
    }
    fprintf(stdout, "Total of %i atoms.\n", atom_number - 1);
}

///////////////////////////////////////////////////////////////////////////////////////
//                      File scanning & atom parsing                                 //
///////////////////////////////////////////////////////////////////////////////////////

void APar_AtomizeFileInfo(uint32_t Astart, uint32_t Alength, uint64_t Aextendedlength, char* Astring, uint8_t Alevel, uint8_t Acon_state, uint8_t Aclass, uint32_t Averflags, uint16_t Alang, uuid_vitals* uuid_info) {
    static bool passed_mdat = false;
    AtomicInfo* thisAtom = &parsedAtoms[atom_number];

    thisAtom->AtomicStart = Astart;
    thisAtom->AtomicLength = Alength;
    thisAtom->AtomicLengthExtended = Aextendedlength;
    thisAtom->AtomicNumber = atom_number;
    thisAtom->AtomicLevel = Alevel;
    thisAtom->AtomicContainerState = Acon_state;
    thisAtom->AtomicClassification = Aclass;

    thisAtom->AtomicName = (char*) malloc(sizeof(char) * 20);
    memset(thisAtom->AtomicName, 0, sizeof(char) * 20);

    if (Aclass == EXTENDED_ATOM) {
        thisAtom->uuid_style = uuid_info->uuid_form;
        if (uuid_info->uuid_form == UUID_DEPRECATED_FORM) {
            memcpy(thisAtom->AtomicName, Astring, 4);
            thisAtom->uuid_ap_atomname = (char*) calloc(1, sizeof(char) * 16);
            memcpy(thisAtom->uuid_ap_atomname, Astring, 4);
        }
        else {
            memcpy(thisAtom->AtomicName, uuid_info->binary_uuid, 16);
            if (uuid_info->uuid_form == UUID_AP_SHA1_NAMESPACE) {
                thisAtom->uuid_ap_atomname = (char*) calloc(1, sizeof(char) * 16);
                memcpy(thisAtom->uuid_ap_atomname, uuid_info->uuid_AP_atom_name, 4);
            }
        }
    }
    else {
        memcpy(thisAtom->AtomicName, Astring, 4);
    }

    thisAtom->AtomicVerFlags = Averflags;
    thisAtom->AtomicLanguage = Alang;

    thisAtom->stsd_codec = 0;

    //set the next atom number of the PREVIOUS atom (we didn't know there would be one until now); this is our default normal mode
    parsedAtoms[atom_number - 1].NextAtomNumber = atom_number;
    thisAtom->NextAtomNumber = 0; //this could be the end... (we just can't quite say until we find another atom)

    if (strncmp(Astring, "mdat", 4) == 0) {
        passed_mdat = true;
    }

    if (!passed_mdat && Alevel == 1) {
        bytes_before_mdat += Alength; //this value gets used during FreeFree (for removed_bytes_tally) & chunk offset calculations
    }

    atom_number++; //increment to the next AtomicInfo array

    return;
}

uint8_t APar_GetCurrentAtomDepth(uint32_t atom_start, uint32_t atom_length) {
    short level = 1;
    for (int i = 0; i < atom_number; i++) {
        AtomicInfo* thisAtom = &parsedAtoms[i];
        if (atom_start == (thisAtom->AtomicStart + thisAtom->AtomicLength)) {
            return thisAtom->AtomicLevel;
        }
        else {
            if ((atom_start < thisAtom->AtomicStart + thisAtom->AtomicLength) && (atom_start > thisAtom->AtomicStart)) {
                level++;
            }
        }
    }
    return level;
}

void APar_IdentifyBrand(char* file_brand) {
    brand = UInt32FromBigEndian(file_brand);
    switch (brand) {
    //what ISN'T supported
    case 0x71742020: //'qt  '  --this is listed at mp4ra, but there are features of the file that aren't supported (like the 4 NULL bytes after the last udta child atom
        fprintf(stdout, "AtomicParsley error: Quicktime movie files are not supported.\n");
        break;

        //3gp-brands are listed in 3GPP/3GPP2 specification documents, not all are listed at mp4ra

    case 0x33673261: //'3g2a' 3GPP2 release 0
    case 0x33673262: //'3g2b' 3GPP2 release A
    case 0x6B646469: //'kddi' 3GPP2 EZmovie (optionally restricted) media
        metadata_style = THIRD_GEN_PARTNER_VER2;
        break;

    case 0x33677034: //'3gp4'
    case 0x33677035: //'3gp5' //'albm' album tag was added in Release6, so it shouldn't be added to a 3gp5 or 3gp4 branded file.
    case 0x6D6D7034: //'mmp4' probably does not support album; minor brands only go up to 3gp5, so.... I'm guessing not

        metadata_style = THIRD_GEN_PARTNER;
        break;

    case 0x33677036: //'3gp6'

    case 0x33677236: //'3gr6' progressive
    case 0x33677336: //'3gs6' streaming
    case 0x33676536: //'3ge6' extended presentations (jpeg images)
    case 0x33676736: //'3gg6' general (not yet suitable; superset)

        metadata_style = THIRD_GEN_PARTNER_VER1_REL6;
        break;

        //what IS supported for iTunes-style metadata
    case 0x4D534E56: //'MSNV'  (PSP) - this isn't actually listed at mp4ra, but since they are popular...
        metadata_style = ITUNES_STYLE;
        psp_brand = true;
        break;
    case 0x4D344120: //'M4A '  -- these are all listed at http://www.mp4ra.org/filetype.html as registered brands
    case 0x4D344220: //'M4B '
    case 0x4D345020: //'M4P '
    case 0x4D345620: //'M4V '
    case 0x6D703432: //'mp42'
    case 0x6D703431: //'mp41'
    case 0x69736F6D: //'isom'
    case 0x69736F32: //'iso2'
    case 0x61766331: //'avc1'
        metadata_style = ITUNES_STYLE;
        break;

        //other lesser unsupported brands; http://www.mp4ra.org/filetype.html like dv, mjpeg200, mp21 & ... whatever mpeg7 brand is
    default:
        fprintf(stdout, "AtomicParsley error: unsupported MPEG-4 file brand found '%s'\n", file_brand);
        break;

    }

    return;
}

void APar_Extract_stsd_codec(FILE* file, uint32_t midJump) {
    char *codec_data = (char *) malloc(12 + 1);
    memset(codec_data, 0, 12 + 1);
    fseeko(file, midJump, SEEK_SET);
    fread(codec_data, 1, 12, file);
    parsedAtoms[atom_number - 1].stsd_codec = UInt32FromBigEndian(extractAtomName(codec_data, 1));

    free(codec_data);
    codec_data = NULL;
    return;
}

uint64_t APar_64bitAtomRead(FILE *file, uint32_t jump_point) {
    uint64_t extended_dataSize = 0;
    char *sixtyfour_bit_data = (char *) malloc(8 + 1);
    memset(sixtyfour_bit_data, 0, 8 + 1);

    fseeko(file, jump_point + 8, SEEK_SET); //this will seek back to the last jump point and...
    fread(sixtyfour_bit_data, 1, 8, file); //read in 16 bytes so we can do a valid extraction and do a nice atom data printout

    extended_dataSize = UInt64FromBigEndian(sixtyfour_bit_data);

    //here's the problem: there can be some HUGE MPEG-4 files. They get specified by a 64-bit value. The specification says only use them when necessary - I've seen them on files about 700MB. So this will artificially place a limit on the maximum file size that would be supported under a 32-bit only AtomicParsley (but could still see & use smaller 64-bit values). For my 700MB file, moov was (rounded up) 4MB. So say 4MB x 6 +1MB give or take, and the biggest moov atom I would support is.... a heart stopping 30MB (rounded up). GADZOOKS!!! So, since I have no need to go greater than that EVER, I'm going to stik with uin32_t for filesizes and offsets & that sort. But a smaller 64-bit mdat (essentially a pseudo 32-bit traditional mdat) can be supported as long as its less than UINT32_T_MAX (minus our big fat moov allowance).

    if (extended_dataSize > 4294967295UL - 30000000) {
        contains_unsupported_64_bit_atom = true;
        fprintf(stdout, "You must be off your block thinking I'm going to tag a file that is at LEAST %llu bytes long.\n", extended_dataSize);
        fprintf(stdout, "AtomicParsley doesn't have full 64-bit support");
    }
    return extended_dataSize;
}

/*----------------------
 APar_MatchToKnownAtom
 atom_name - the name of our newly found atom
 atom_container - the name of the parent container atom
 fromFile - controls the manner of extracting parents (passed thu to another function)

 Using the atom_name of this new atom, search through KnownAtoms, testing that the names match. If they do, move onto a finer grained sieve.
 If the parent can be at any level (like "free"), just let it through; if the parent is "ilst" (iTunes-style metadata), or a uuid, return a generic match
 The final test is the one most atoms will go through. Some atoms can have different parents - up to 5 different parents are allowed by this version of AP
 Iterate through the known parents, and test it against atom_container. If they match, return the properties of the known atom
 ----------------------*/
int APar_MatchToKnownAtom(const char* atom_name, const char* atom_container, bool fromFile, const char* find_atom_path) {
    uint32_t total_known_atoms = (uint32_t) (sizeof(KnownAtoms) / sizeof(*KnownAtoms));
    uint32_t return_known_atom = 0;

    //if this atom is contained by 'ilst', then it is *highly* likely an iTunes-style metadata parent atom
    if (memcmp(atom_container, "ilst", 4) == 0 && memcmp(atom_name, "uuid", 4) != 0) {
        return_known_atom = total_known_atoms - 2; //2nd to last KnowAtoms is a generic placeholder iTunes-parent atom

        //if this atom is "data" get the full path to it; we will take any atom under 'ilst' and consider it an iTunes metadata parent atom
    }
    else if (memcmp(atom_name, "data", 4) == 0 && find_atom_path != NULL) {
        if (memcmp(find_atom_path, "moov.udta.meta.ilst.", 20) == 0) {
            return_known_atom = total_known_atoms - 1; //last KnowAtoms is a generic placeholder iTunes-data atom
        }

    }
    else if (memcmp(atom_name, "data", 4) == 0) {
        char* fullpath = (char *) malloc(sizeof(char) * 200);
        memset(fullpath, 0, sizeof(char) * 200);

        if (fromFile) {
            APar_ProvideAtomPath(parsedAtoms[atom_number - 1].AtomicNumber, fullpath, fromFile);
        }
        else { //find_atom_path only is NULL in APar_ScanAtoms (where fromFile is true) and in APar_CreateSparseAtom, where atom_number was just filled
            APar_ProvideAtomPath(parsedAtoms[atom_number].AtomicNumber, fullpath, fromFile);
        }

        if (memcmp(fullpath, "moov.udta.meta.ilst.", 20) == 0) {
            return_known_atom = total_known_atoms - 1; //last KnowAtoms is a generic placeholder iTunes-data atom
        }
        free(fullpath);
        fullpath = NULL;

        //if this atom is "esds" get the full path to it; take any atom under 'stsd' as a parent to esds (that parent would be a 4CC codec; not all do have 'esds'...)
    }
    else if (memcmp(atom_name, "esds", 4) == 0) {
        char* fullpath = (char *) malloc(sizeof(char) * 300);
        memset(fullpath, 0, sizeof(char) * 200);

        APar_ProvideAtomPath(parsedAtoms[atom_number - 1].AtomicNumber, fullpath, fromFile);

        if (memcmp(fullpath, "moov.trak.mdia.minf.stbl.stsd.", 30) == 0) {
            return_known_atom = total_known_atoms - 3; //manually return the esds atom
        }
        free(fullpath);
        fullpath = NULL;

    }
    else {
        //try matching the name of the atom
        for (uint32_t i = 1; i < total_known_atoms; i++) {
            if (memcmp(atom_name, KnownAtoms[i].known_atom_name, 4) == 0) {
                //name matches, now see if the container atom matches any known container for that atom
                if (memcmp(KnownAtoms[i].known_parent_atoms[0], "_ANY_LEVEL", 10) == 0) {
                    return_known_atom = i; //the list starts at 0; the unknown atom is at 0; first known atom (ftyp) is at 1
                    break;

                }
                else {
                    uint8_t total_known_containers = (uint8_t) (sizeof(KnownAtoms[i].known_parent_atoms)
                            / sizeof(*KnownAtoms[i].known_parent_atoms)); //always 5
                    for (uint8_t iii = 0; iii < total_known_containers; iii++) {
                        if (KnownAtoms[i].known_parent_atoms[iii] != NULL) {
                            if (memcmp(atom_container, KnownAtoms[i].known_parent_atoms[iii], strlen(atom_container))
                                    == 0) { //strlen(atom_container)
                                return_known_atom = i; //the list starts at 0; the unknown atom is at 0; first known atom (ftyp) is at 1
                                break;
                            }
                        }
                    }
                }
                if (return_known_atom) {
                    break;
                }
            }
        }
    }
    if (return_known_atom > total_known_atoms) {
        return_known_atom = 0;
    }
    return return_known_atom;
}

/*----------------------
 APar_Manually_Determine_Parent
 atom_start - the place in the file where the atom begins
 atom_length - length of the eval atom
 container - a string of the last known container atom (or FILE_LEVEL)

 given the next atom (unknown string at this point), run some tests using its starting point and length. Iterate backwards through the already parsed
 atoms, and for each test if it could contain this atom. Tests include if the container starts before ours (which it would to contain the new atom),
 that its length is longer than our length (any parent would need to be longer than us if even by 8 bytes), the start + length sum of the parent atom
 (where it ends), needs to be greater than or equal to where this new atom ends, and finally, that the eval containing atom be some form of parent as
 defined in KnownAtoms
 ----------------------*/
void APar_Manually_Determine_Parent(uint32_t atom_start, uint32_t atom_length, char* container) {
    short preceding_atom = atom_number - 1;
    while (parsedAtoms[preceding_atom].AtomicNumber != 0) {

        if (parsedAtoms[preceding_atom].AtomicStart < atom_start
                && parsedAtoms[preceding_atom].AtomicLength > atom_length
                && parsedAtoms[preceding_atom].AtomicStart + parsedAtoms[preceding_atom].AtomicLength
                        >= atom_start + atom_length
                && parsedAtoms[preceding_atom].AtomicContainerState <= DUAL_STATE_ATOM) {
            memcpy(container, parsedAtoms[preceding_atom].AtomicName, 5);
            break;

        }
        else {
            preceding_atom--;
        }
        if (preceding_atom == 0) {
            memcpy(container, "FILE_LEVEL", 11);
        }
    }
    return;
}

/*----------------------
 APar_ScanAtoms
 path - the complete path to the originating file to be tested
 scan_for_tree_ONLY - controls whether we go into 'stsd' or just a superficial scan

 if the file has not yet been scanned (this gets called by nearly every cli option), then open the file and start scanning. Read in the first 12 bytes
 and see if bytes 4-8 are 'ftyp' as any modern MPEG-4 file will have 'ftyp' first. If it does, start scanning the rest of the file. An MPEG-4 file is
 logically organized into discrete hierarchies called "atoms" or "boxes". Each atom is at minimum 8 bytes long. Bytes 1-4 make an unsigned 32-bit integer
 that denotes how long this atom is (ie: 8 would mean this atom is 8 bytes long). The next 4 bytes (bytes 5-8) make the atom name. If the atom presents
 longer than 8 bytes, then that supplemental data would be what the atom carries. Atoms are broadly separated into 2 categories: parents & children
 (or container & leaf). Typically, a parent can hold other atoms, but not data; a child can hold data but not other atoms. This 'rule' is broken sometimes
 (the atoms listed as DUAL_STATE_ATOM), but largely holds.

 Each atom is read in as 12 bytes (to accommodate flags & versioning). The atom name is extracted, and using the last known container (either FILE_LEVEL
 or an actual atom name), the new atom's hierarchy is found based on its length & position. Using its containing atom, the KnownAtoms table is searched to
 locate the properties of that atom (parent/child, versioned/simple), and jumping around in the file is based off that known atom's type. Atoms that
 fall into a hybrid category (DUAL_STATE_ATOMs) are explicitly handled. If an atom is listed has having a language attribute, it is read to support
 multiple langauges (as most 3GP assets do).
 ----------------------*/
void APar_ScanAtoms(const char *path, bool scan_for_tree_ONLY) {

    APar_Init();

    if (!parsedfile) {
        file_size = findFileSize(path);

        FILE *file = APar_OpenFile(path, "rb");
        if (file != NULL) {
            char *data = (char *) malloc(12 + 1);
            memset(data, 0, 12 + 1);
            char *container = (char *) malloc(20);
            memset(container, 0, 20);
            memcpy(container, "FILE_LEVEL", 10);
            bool corrupted_data_atom = false;

            uuid_vitals uuid_info =
                { 0 };
            uuid_info.binary_uuid = (char*) malloc(sizeof(char) * 16 + 1); //this will hold any potential 16byte uuids
            uuid_info.uuid_AP_atom_name = (char*) malloc(sizeof(char) * 5); //this will hold any atom name that is written after the uuid written by AP

            if (data == NULL)
                return;
            uint32_t dataSize = 0;
            uint32_t jump = 0;

            fread(data, 1, 12, file);
            char *atom = extractAtomName(data, 1);

            if (memcmp(atom, "ftyp", 4) == 0) { //jpeg2000 files will have a beginning 'jp  ' atom that won't get through here

                APar_IdentifyBrand(data + 8);

                dataSize = UInt32FromBigEndian(data);
                jump = dataSize;

                APar_AtomizeFileInfo(0, jump, 0, atom, generalAtomicLevel, CHILD_ATOM, SIMPLE_ATOM, 0, 0, &uuid_info);

                fseek(file, jump, SEEK_SET);

                while (jump < (uint32_t) file_size) {
                    uuid_info.uuid_form = UUID_DEPRECATED_FORM; //start with the assumption that any found atom is in the depracted uuid form

                    fread(data, 1, 12, file);
                    char *atom = extractAtomName(data, 1);
                    dataSize = UInt32FromBigEndian(data);

                    if (dataSize > (uint64_t) file_size) {
                        dataSize = (uint32_t) (file_size - jump);
                    }

                    if (dataSize == 0 && (atom[0] == 0 && atom[1] == 0 && atom[2] == 0 && atom[3] == 0)) {
                        gapless_void_padding = file_size - jump; //Apple has decided to add around 2k of NULL space outside of any atom structure starting with iTunes 7.0.0
                        break; //its possible this is part of gapless playback - but then why would it come after the 'free' at the end of a file like gpac writes?
                    } //after actual tested its elimination, it doesn't seem to be required for gapless playback

                    //diagnose damage to 'cprt' by libmp4v2 in 1.4.1 & 1.5.0.1
                    //typically, the length of this atom (dataSize) will exceeed it parent (which is reported as 17)
                    //true length ot this data will be 9 - impossible for iTunes-style 'data' atom.
                    if (memcmp(atom, "data", 4) == 0
                            && parsedAtoms[atom_number - 1].AtomicContainerState == PARENT_ATOM) {
                        if (dataSize > parsedAtoms[atom_number - 1].AtomicLength) {
                            dataSize = parsedAtoms[atom_number - 1].AtomicLength - 8; //force its length to its true length
                            fprintf(stdout, "AtomicParsley warning: the 'data' child of the '%s' atom seems to be corrupted.\n", parsedAtoms[atom_number
                                    - 1].AtomicName);
                            corrupted_data_atom = true;
                        }
                    }
                    //end diagnosis; APar_Manually_Determine_Parent will still determine it to be a versioned atom (it tests by names), but at file write out,
                    //it will write with a length of 9 bytes

                    APar_Manually_Determine_Parent(jump, dataSize, container);
                    int filtered_known_atom = APar_MatchToKnownAtom(atom, container, true, NULL);

                    uint32_t atom_verflags = 0;
                    uint16_t atom_language = 0;

                    if (memcmp(atom, "uuid", 4) == 0) {
                        memset(uuid_info.binary_uuid, 0, 20);

                        APar_readX(uuid_info.binary_uuid, file, jump + 8, 16);

                        if (UInt32FromBigEndian(uuid_info.binary_uuid + 8) == 0) { //the deperacted uuid form
                            atom = extractAtomName(data, 2);
                            atom_verflags = APar_read32(uuid_info.binary_uuid, file, jump + 12);
                            if (atom_verflags > (uint32_t) AtomFlags_Data_UInt) {
                                atom_verflags = 0;
                            }
                        }
                        else {
                            uint8_t uuid_version = APar_extract_uuid_version(NULL, uuid_info.binary_uuid);
                            APar_endian_uuid_bin_str_conversion(uuid_info.binary_uuid);
                            if (uuid_version == 5) {
                                uuid_info.uuid_form = UUID_SHA1_NAMESPACE;
                                //read in what AP would set the atom name to. The new uuid form is:
                                // 4bytes atom length, 4 bytes 'uuid', 16bytes uuidv5, 4bytes name of uuid in AP namespace, 4bytes versioning, 4bytes NULL, Xbytes data
                                APar_readX(uuid_info.uuid_AP_atom_name, file, jump + 24, 4);

                                char uuid_of_foundname_in_AP_namesapce[20];
                                APar_generate_uuid_from_atomname(uuid_info.uuid_AP_atom_name, uuid_of_foundname_in_AP_namesapce);
                                if (memcmp(uuid_info.binary_uuid, uuid_of_foundname_in_AP_namesapce, 16) == 0) {
                                    uuid_info.uuid_form = UUID_AP_SHA1_NAMESPACE; //our own uuid ver5 atoms in the AtomicParsley.sf.net namespace
                                    atom_verflags = APar_read32(twenty_byte_buffer, file, jump + 28);
                                }
                            }
                            else {
                                uuid_info.uuid_form = UUID_OTHER;
                            }
                        }
                    }

                    if (KnownAtoms[filtered_known_atom].box_type == VERSIONED_ATOM && !corrupted_data_atom) {
                        atom_verflags = UInt32FromBigEndian(data + 8); //flags & versioning were already read in with the original 12 bytes
                    }

                    if (KnownAtoms[filtered_known_atom].box_type == PACKED_LANG_ATOM) {
                        //the problem with storing the language is that the uint16_t 2 bytes that carry the actual language are in different places on different atoms
                        //some atoms have it right after the atom version/flags; some like rating/classification have it 8 bytes later; yrrc doesn't have it at all
                        char bitpacked_lang[4];
                        memset(bitpacked_lang, 0, 4);

                        uint32_t userdata_box = UInt32FromBigEndian(atom);

                        switch (userdata_box) {
                        case 0x7469746C: //'titl'
                        case 0x64736370: //'dscp'
                        case 0x63707274: //'cprt'
                        case 0x70657266: //'perf'
                        case 0x61757468: //'auth'
                        case 0x676E7265: //'gnre'
                        case 0x616C626D: //'albm'
                        case 0x6B797764: //'kywd'
                        case 0x6C6F6369: //'loci'
                        {
                            atom_language = APar_read16(bitpacked_lang, file, jump + 12);
                            break;
                        }
                        case 0x636C7366: //'clsf'
                        {
                            atom_language = APar_read16(bitpacked_lang, file, jump + 18);
                            break;
                        }
                        case 0x72746E67: //'rtng'
                        {
                            atom_language = APar_read16(bitpacked_lang, file, jump + 20);
                            break;
                        }
                            //case 0x79727263 : //'yrrc' is the only 3gp tag that doesn't support multiple languages; won't even get here because != PACKED_LANG_ATOM
                        default: {
                            //TODO: a future ID32 atom will fall here; the offset to the uint16 language will need to be listed at that point
                            break;//which means that any new/unknown packed language atoms will have their language of 0; AP will only support 1 of this atom name then
                        }
                        }
                    }

                    //mdat.length=1; and ONLY supported for mdat atoms - no idea if the spec says "only mdat", but that's what I'm doing for now
                    if ((strncmp(atom, "mdat", 4) == 0) && (generalAtomicLevel == 1) && (dataSize == 1)) {
                        uint64_t extended_dataSize = APar_64bitAtomRead(file, jump);
                        APar_AtomizeFileInfo(jump, 1, extended_dataSize, atom, generalAtomicLevel, KnownAtoms[filtered_known_atom].container_state, KnownAtoms[filtered_known_atom].box_type, atom_verflags, atom_language, &uuid_info);

                    }
                    else {
                        APar_AtomizeFileInfo(jump, dataSize, 0, atom, generalAtomicLevel, KnownAtoms[filtered_known_atom].container_state,
                                corrupted_data_atom ? SIMPLE_ATOM : KnownAtoms[filtered_known_atom].box_type, atom_verflags, atom_language, &uuid_info);
                    }
                    corrupted_data_atom = false;

                    //read in the name of an iTunes-style internal reverseDNS directly into parsedAtoms
                    if (memcmp(atom, "name", 4) == 0 && memcmp(parsedAtoms[atom_number - 2].AtomicName, "mean", 4) == 0
                            && memcmp(parsedAtoms[atom_number - 3].AtomicName, "----", 4) == 0) {

                        parsedAtoms[atom_number - 1].ReverseDNSname = (char *) malloc(sizeof(char) * dataSize);
                        memset(parsedAtoms[atom_number - 1].ReverseDNSname, 0, sizeof(char) * dataSize);

                        fseeko(file, jump + 12, SEEK_SET); //'name' atom is the 2nd child
                        fread(parsedAtoms[atom_number - 1].ReverseDNSname, 1, dataSize - 12, file);
                    }

                    if (dataSize == 0) { // length = 0 means it reaches to EOF
                        break;
                    }

                    switch (KnownAtoms[filtered_known_atom].container_state) {
                    case PARENT_ATOM: {
                        jump += 8;
                        if (memcmp(atom, "udta", 4) == 0) {
                            if (memcmp(container, "moov", 4) == 0) {
                                udta_dynamics.original_udta_size = parsedAtoms[atom_number - 1].AtomicLength;
                            }
                        }
                        break;
                    }
                    case CHILD_ATOM: {
                        if ((generalAtomicLevel == 1) && (dataSize == 1)) { //mdat.length =1 64-bit length that is more of a cludge.
                            jump += parsedAtoms[atom_number - 1].AtomicLengthExtended;
                        }
                        else {
                            jump += dataSize;
                        }
                        break;
                    }
                    case DUAL_STATE_ATOM: {
                        if (memcmp(atom, "meta", 4) == 0) {
                            jump += 12;

                        }
                        else if (memcmp(atom, "stsd", 4) == 0) {
                            if (scan_for_tree_ONLY) {
                                //for a tree ONLY, we go all the way, parsing everything; for any other option, we leave this atom as a monolithic entity
                                jump += 16;
                            }
                            else {
                                APar_Extract_stsd_codec(file, jump + 16); //just get the codec used for this track
                                jump += dataSize;
                            }

                        }
                        else if (memcmp(atom, "schi", 4) == 0) {
                            if (memcmp(container, "sinf", 4) == 0) { //seems for iTMS drm files, schi is a simple parent atom, and 'user' comes right after it
                                jump += 8;
                            }
                            else {
                                jump += dataSize; //no idea what it would be under srpp, so just skip over it
                            }

                        }
                        else if (memcmp(container, "stsd", 4) == 0) {
                            //each one is different, so list its size manually
                            //the beauty of this is that even if there is an error here or a new codec shows up, it only affects SHOWING the tree.
                            //Getting a tree for display ONLY purposes is different from when setting a tag - display ONLY goes into stsd; tagging makes 'stsd' monolithic.
                            //so setting metadata on unknown or improperly enumerated codecs (which might have different lengths) don't affect tagging.
                            uint32_t named_atom = UInt32FromBigEndian(atom);
                            switch (named_atom) {
                            case 0x6D703473: { //mp4s
                                jump += 16;
                                break;
                            }
                            case 0x73727470: //srtp
                            case 0x72747020: { //'rtp '
                                jump += 24;
                                break;
                            }
                            case 0x616C6163: //alac
                            case 0x6D703461: //mp4a
                            case 0x73616D72: //samr
                            case 0x73617762: //sawb
                            case 0x73617770: //sawp
                            case 0x73657663: //sevc
                            case 0x73716370: //sqcp
                            case 0x73736D76: //ssmv
                            case 0x64726D73: { //drms
                                jump += 36;
                                break;
                            }
                            case 0x74783367: { //tx3g
                                jump += 46;
                                break;
                            }
                            case 0x6D703476: //mp4v
                            case 0x61766331: //avc1
                            case 0x6A706567: //jpeg
                            case 0x73323633: //s263
                            case 0x64726D69: { //drmi
                                jump += 86;
                                break;
                            }
                            default: { //anything else that isn't covered here will just jump past any child atoms (avcp, text, enc*)
                                jump += dataSize;
                            }
                            }
                        }
                        break;
                    }
                    case UNKNOWN_ATOM_TYPE: {
                        short parent_atom = APar_FindParentAtom(atom_number - 1, generalAtomicLevel);
                        //to accommodate the retarted utility that keeps putting in 'prjp' atoms in mpeg-4 files written QTstyle
                        if (parsedAtoms[parent_atom].AtomicContainerState == DUAL_STATE_ATOM) {
                            jump = parsedAtoms[parent_atom].AtomicStart + parsedAtoms[parent_atom].AtomicLength;
                        }
                        else {
                            jump += dataSize;
                        }
                        break;
                    }
                    } //end swtich

                    generalAtomicLevel = APar_GetCurrentAtomDepth(jump, dataSize);

                    if ((jump > 8 ? jump : 8) >= (uint32_t) file_size) { //prevents jumping past EOF for the smallest of atoms
                        break;
                    }

                    fseeko(file, jump, SEEK_SET);
                }

            }
            else {
                fprintf(stderr, "\nAtomicParsley error: bad mpeg4 file (ftyp atom missing or alignment error).\n\n");
            }

            if (data) {
                free(data);
                data = NULL;
            }

            if (container) {
                free(container);
                container = NULL;
            }

            if (uuid_info.binary_uuid)
                free(uuid_info.binary_uuid);

            if (uuid_info.uuid_AP_atom_name)
                free(uuid_info.uuid_AP_atom_name);

            fclose(file);
        }
        parsedfile = true;
    }

    if (!tree_display_only && !parsedfile && APar_FindAtom("moov", false, SIMPLE_ATOM, 0) == NULL) {
        fprintf(stderr, "\nAtomicParsley error: bad mpeg4 file (no 'moov' atom).\n\n");
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          Atom Removal Functions                                   //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
 APar_EliminateAtom
 this_atom_number - the index into parsedAtoms[] of the atom to be erased
 resume_atom_number - the point in parsedAtoms[] where the tree will be picked up

 This manually removes the atoms from being used. The atom is still in parsedAtoms[] at the same location it was previously, but because parsedAtoms is used
 as a linked list & followed by NextAtomNumber, effectively, this atom (and atoms leading to resume_atom_number) are no longer considered part of the tree.
 ----------------------*/
void APar_EliminateAtom(short this_atom_number, int resume_atom_number) {
    if (this_atom_number > 0 && this_atom_number < atom_number && resume_atom_number >= 0
            && resume_atom_number < atom_number) {
        short preceding_atom_pos = APar_FindPrecedingAtom(this_atom_number);
        if (APar_Eval_ChunkOffsetImpact(this_atom_number)) {
            removed_bytes_tally += parsedAtoms[this_atom_number].AtomicLength; //used in validation routine
        }
        parsedAtoms[preceding_atom_pos].NextAtomNumber = resume_atom_number;

        memset(parsedAtoms[this_atom_number].AtomicName, 0, 4); //blank out the name of the parent atom name
        parsedAtoms[this_atom_number].AtomicNumber = -1;
        parsedAtoms[this_atom_number].NextAtomNumber = -1;
    }
    return;
}

/*----------------------
 APar_RemoveAtom
 atom_path - the "peri.od_d.elim.inat.ed__.atom.path" string that represents the target atom
 atom_type - the type of atom to be eliminated (packed language, extended...) of the target atom
 UD_lang - the language code for a packed language atom (ignored for non-packed language atoms)

 APar_RemoveAtom tries to find the atom in the string. If it exists, then depending on its atom_type, it or its last child will get passed along for elimination.
 TODO: the last child part could use some more intelligence at some point; its relatively hardcoded.
 ----------------------*/
void APar_RemoveAtom(const char* atom_path, uint8_t atom_type, uint16_t UD_lang) {

    AtomicInfo* desiredAtom = APar_FindAtom(atom_path, false, atom_type, UD_lang, (
            atom_type == EXTENDED_ATOM ? true : false));

    if (desiredAtom == NULL)
        return; //the atom didn't exist or wasn't found
    if (desiredAtom->AtomicNumber == 0)
        return; //we got the default atom, ftyp - and since that can't be removed, it must not exist (or it was missed)

    modified_atoms = true;
    if (atom_type != EXTENDED_ATOM) {
        if (atom_type == PACKED_LANG_ATOM || desiredAtom->AtomicClassification == UNKNOWN_ATOM) {
            APar_EliminateAtom(desiredAtom->AtomicNumber, desiredAtom->NextAtomNumber);

            //reverseDNS atom
        }
        else if (desiredAtom->ReverseDNSname != NULL) {
            short parent_atom = APar_FindParentAtom(desiredAtom->AtomicNumber, desiredAtom->AtomicLevel);
            short last_elim_atom = APar_FindLastChild_of_ParentAtom(parent_atom);
            APar_EliminateAtom(parent_atom, parsedAtoms[last_elim_atom].NextAtomNumber);

        }
        else if (memcmp(desiredAtom->AtomicName, "data", 4) == 0 && desiredAtom->AtomicLevel == 6) {
            short parent_atom = APar_FindParentAtom(desiredAtom->AtomicNumber, desiredAtom->AtomicLevel);
            short last_elim_atom = APar_FindLastChild_of_ParentAtom(parent_atom);
            APar_EliminateAtom(parent_atom, parsedAtoms[last_elim_atom].NextAtomNumber);

        }
        else if (desiredAtom->AtomicContainerState <= DUAL_STATE_ATOM) {
            short last_elim_atom = APar_FindLastChild_of_ParentAtom(desiredAtom->AtomicNumber);
            APar_EliminateAtom(desiredAtom->AtomicNumber, parsedAtoms[last_elim_atom].NextAtomNumber);

        }
        else if (UD_lang == 1) { //yrrc
            APar_EliminateAtom(desiredAtom->AtomicNumber, desiredAtom->NextAtomNumber);

        }
        else {
            short last_elim_atom = APar_FindLastChild_of_ParentAtom(desiredAtom->AtomicNumber);
            APar_EliminateAtom(desiredAtom->AtomicNumber, last_elim_atom);
        }

        //this will only work for AtomicParsley created uuid atoms that don't have children, but since all uuid atoms are treaded as non-parent atoms... no problems
    }
    else if (atom_type == EXTENDED_ATOM) {
        APar_EliminateAtom(desiredAtom->AtomicNumber, desiredAtom->NextAtomNumber);
    }
    return;
}

/*----------------------
 APar_freefree
 purge_level - an integer ranging from -1 to some positive number of the level a 'free' atom must be on for it to be erased.

 Some tagging utilities (things based on libmp4v2 & faac irrespective of which tagging system used) have a dirty little secret. They way the work is to copy the
 'moov' atom - in its entirety - to the end of the file, and make the changes there. The original 'moov' file is nulled out, but the file only increases in size.
 Even if you eliminate the tag, the file grows. Only when the 'moov' atom is last do these taggers work efficiently - and they are blazingly fast, no doubt about
 that - but they are incredibly wasteful. It is possible to switch between using AP and mp4tags and build a file with dozens of megabytes wasted just be changing
 a single letter.
 This function can be used to iterate through the atoms in the file, and selectively eliminate 'free' atoms. Pass a -1 and every last 'free' atom that exists will
 be eliminated (but another will appear later as padding - to completely eliminate any resulting 'free' atoms, the environmental variable "AP_PADDING" needs to be
 set with MID_PAD to 0). Pass a 0, and all 'free' atoms preceding 'moov' or after 'mdat' (including gpac's pesky "Place Your Ad Here" free-at-the-end) will be
 removed. A value of >= 1 will eliminate 'free' atoms between those levels and level 1 (or file level).
 ----------------------*/
void APar_freefree(int purge_level) {
    modified_atoms = true;
    short eval_atom = 0;
    short moov_atom = 0; //a moov atom has yet to be seen
    short mdat_atom = 0; //any ol' mdat

    if (purge_level == -1) {
        complete_free_space_erasure = true; //prevent any in situ dynamic updating when trying to remove all free atoms. Also triggers a more efficient means of forcing padding
    }

    while (true) {
        eval_atom = parsedAtoms[eval_atom].NextAtomNumber;
        if (eval_atom == 0) { //we've hit the last atom
            break;
        }

        if (memcmp(parsedAtoms[eval_atom].AtomicName, "free", 4) == 0
                || memcmp(parsedAtoms[eval_atom].AtomicName, "skip", 4) == 0) {
            if (purge_level == -1 || purge_level >= parsedAtoms[eval_atom].AtomicLevel
                    || (purge_level == 0 && parsedAtoms[eval_atom].AtomicLevel == 1
                            && (moov_atom == 0 || mdat_atom != 0))) {
                short prev_atom = APar_FindPrecedingAtom(eval_atom);
                if (parsedAtoms[eval_atom].NextAtomNumber == 0) { //we've hit the last atom
                    APar_EliminateAtom(eval_atom, parsedAtoms[eval_atom].NextAtomNumber);
                    parsedAtoms[prev_atom].NextAtomNumber = 0;
                }
                else {
                    APar_EliminateAtom(eval_atom, parsedAtoms[eval_atom].NextAtomNumber);
                }
                eval_atom = prev_atom; //go back to the previous atom and continue the search
            }
        }
        if (memcmp(parsedAtoms[eval_atom].AtomicName, "moov", 4) == 0) {
            moov_atom = eval_atom;
        }
        if (memcmp(parsedAtoms[eval_atom].AtomicName, "mdat", 4) == 0) {
            mdat_atom = eval_atom;
        }
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          Atom Moving Functions                                    //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
 APar_MoveAtom
 this_atom_number - the atom that will follow the new_position atom
 new_position - the atom that the atoms at that level will precede, followed by this_atom_number (including its hierarchy of child atoms)

 first find which atoms lead to both atoms (precedingAtom flows to this_atom_number, lastStationaryAtom flows to new_position). Depending on whether there
 are children, find the last atom in the atoms that will be moving. Shunt as required; reordering occurs by following NextAtomNumber (linked list)
 ----------------------*/
void APar_MoveAtom(short this_atom_number, short new_position) {
    short precedingAtom = 0;
    short lastStationaryAtom = 0;
    short iter = 0;

    //look for the preceding atom (either directly before of the same level, or moov's last nth level child
    while (parsedAtoms[iter].NextAtomNumber != 0) {
        if (parsedAtoms[iter].NextAtomNumber == this_atom_number) {
            precedingAtom = iter;
            break;
        }
        else {
            if (parsedAtoms[iter].NextAtomNumber == 0) { //we found the last atom (which we end our search on)
                break;
            }
        }
        iter = parsedAtoms[iter].NextAtomNumber;
    }

    iter = 0;

    //search where to insert our new atom
    while (parsedAtoms[iter].NextAtomNumber != 0) {
        if (parsedAtoms[iter].NextAtomNumber == new_position) {
            lastStationaryAtom = iter;
            break;
        }
        iter = parsedAtoms[iter].NextAtomNumber;
        if (parsedAtoms[iter].NextAtomNumber == 0) { //we found the last atom
            lastStationaryAtom = iter;
            break;
        }

    }

    if (parsedAtoms[this_atom_number].AtomicContainerState <= DUAL_STATE_ATOM) {
        if (parsedAtoms[new_position].AtomicContainerState <= DUAL_STATE_ATOM) {
            short last_SwapChild = APar_FindLastChild_of_ParentAtom(this_atom_number);
            short last_WiredChild = APar_FindLastChild_of_ParentAtom(new_position);

            short swap_resume = parsedAtoms[last_SwapChild].NextAtomNumber;
            short wired_resume = parsedAtoms[last_WiredChild].NextAtomNumber;

            parsedAtoms[precedingAtom].NextAtomNumber = swap_resume; //shunt the main tree (over the [this_atom_number] atom to be move) to other tween atoms,
            parsedAtoms[lastStationaryAtom].NextAtomNumber = new_position; //pick up with the 2nd to last hierarchy
            parsedAtoms[last_WiredChild].NextAtomNumber = this_atom_number; //and route the 2nd to last hierarchy to wrap around to the this_atom_number atom
            parsedAtoms[last_SwapChild].NextAtomNumber = wired_resume; //and continue with whatever was after the [new_position] atom

        }
        else {
            short last_child = APar_FindLastChild_of_ParentAtom(this_atom_number);
            parsedAtoms[lastStationaryAtom].NextAtomNumber = this_atom_number;
            parsedAtoms[precedingAtom].NextAtomNumber = parsedAtoms[last_child].NextAtomNumber;
            parsedAtoms[last_child].NextAtomNumber = new_position;
        }

    }
    else {
        parsedAtoms[lastStationaryAtom].NextAtomNumber = this_atom_number;
        parsedAtoms[precedingAtom].NextAtomNumber = parsedAtoms[this_atom_number].NextAtomNumber;
        parsedAtoms[this_atom_number].NextAtomNumber = new_position;
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          Atom Creation Functions                                  //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
 APar_InterjectNewAtom
 atom_name - the 4 character name of the atom
 cntr_state - the type of container it will be (child, parent, dual state)
 atom_class - the atom type it will be (simple, versioned, uuid, versioned with packed language)
 atom_length - the forced length of this atom (undefined beyond intrinsic length of the container type except for 'free' atoms)
 atom_verflags - the 1 byte atom version & 3 bytes atom flags for the atom if versioned
 packed_lang - the 2 byte packed language for the atom if versioned with packed language type
 atom_level - the level of this atom (1 denotes file level, anything else denotes a child of the last preceding parent or dual state atom)
 preceding_atom - the atom that precedes this newly created interjected atom

 Creates a single new atom (carrying NULLed data) inserted after preceding_atom
 ----------------------*/
short APar_InterjectNewAtom(const char* atom_name, uint8_t cntr_state, uint8_t atom_class, uint32_t atom_length, uint32_t atom_verflags, uint16_t packed_lang, uint8_t atom_level, short preceding_atom) {

    if (tree_display_only) {
        return 0;
    }

    AtomicInfo* new_atom = &parsedAtoms[atom_number];
    new_atom->AtomicNumber = atom_number;
    new_atom->AtomicName = (char*) malloc(sizeof(char) * 6);
    memset(new_atom->AtomicName, 0, sizeof(char) * 6);
    memcpy(new_atom->AtomicName, atom_name, 4);

    new_atom->AtomicContainerState = cntr_state;
    new_atom->AtomicClassification = atom_class;
    new_atom->AtomicVerFlags = atom_verflags;
    new_atom->AtomicLevel = atom_level;
    new_atom->AtomicLength = atom_length;
    new_atom->AtomicLanguage = packed_lang;

    new_atom->AtomicData = (char*) malloc(sizeof(char) * atom_length); //puts a hard limit on the length of strings (the spec doesn't)
    memset(new_atom->AtomicData, 0, sizeof(char) * atom_length);

    new_atom->NextAtomNumber = parsedAtoms[preceding_atom].NextAtomNumber;
    parsedAtoms[preceding_atom].NextAtomNumber = atom_number;

    atom_number++;
    return new_atom->AtomicNumber;
}

/*----------------------
 APar_CreateSparseAtom

 surrogate_atom - an skeletal template of the atom to be created; currently name, level, lang, (if uuid/extended: container & class) are copied; other
 stats should be filled in routines that called for their creation and know things like flags & if is to carry an data (this doesn't malloc)
 parent_atom - the stats for the parent atom (used to match through the KnownAtoms array and get things like container & class (for most atoms)
 preceding_atom - the new atom will follow this atom

 Create a single new atom (not carrying any data) copied from a template to follow preceding_atom
 ----------------------*/
AtomicInfo* APar_CreateSparseAtom(AtomicInfo* surrogate_atom, AtomicInfo* parent_atom, short preceding_atom) {
    AtomicInfo* new_atom = &parsedAtoms[atom_number];
    new_atom->AtomicNumber = atom_number;
    new_atom->AtomicStart = 0;
    int known_atom = 0;

    new_atom->AtomicName = (char*) malloc(sizeof(char) * 20);
    memset(new_atom->AtomicName, 0, sizeof(char) * 20);
    size_t copy_bytes_len = (surrogate_atom->AtomicClassification == EXTENDED_ATOM ? 16 : 4);
    memcpy(new_atom->AtomicName, surrogate_atom->AtomicName, copy_bytes_len);
    new_atom->AtomicLevel = surrogate_atom->AtomicLevel;
    new_atom->AtomicLanguage = surrogate_atom->AtomicLanguage;

    //this is almost assuredly wrong for everything except a simple parent atom & needs to be handled properly afterwards; Note the use of 'Sparse'
    new_atom->AtomicVerFlags = 0;
    new_atom->AtomicLength = 8;

    new_atom->NextAtomNumber = parsedAtoms[preceding_atom].NextAtomNumber;
    parsedAtoms[preceding_atom].NextAtomNumber = atom_number;

    //if 'uuid' atom, copy the info directly, otherwise use KnownAtoms to get the info
    if (surrogate_atom->AtomicClassification == EXTENDED_ATOM) {
        new_atom->AtomicContainerState = CHILD_ATOM;
        new_atom->AtomicClassification = surrogate_atom->AtomicClassification;

        new_atom->uuid_style = UUID_AP_SHA1_NAMESPACE;

    }
    else {
        //determine the type of atom from our array of KnownAtoms; this is a worst case scenario; it should be handled properly afterwards
        //make sure the level & the atom gets integrated into NextAtomNumber before APar_MatchToKnownAtom because getting the fullpath will rely on that
        known_atom = APar_MatchToKnownAtom(surrogate_atom->AtomicName, parent_atom->AtomicName, false, NULL);

        new_atom->AtomicContainerState = KnownAtoms[known_atom].container_state;
        new_atom->AtomicClassification = KnownAtoms[known_atom].box_type;
    }

    atom_number++;

    return new_atom;
}

/*----------------------
 APar_Unified_atom_Put
 atom_num - the index into the parsedAtoms array for the atom we are setting (aka AtomicNumber)
 unicode_data - a pointer to a string (possibly utf8 already); may go onto conversion to utf16 prior to the put
 text_tag_style - flag to denote that unicode_data is to become utf-16, or stay the flavors of utf8 (iTunes style, 3gp style...)
 ancillary_data - a (possibly cast) 32-bit number of any type of supplemental data to be set
 anc_bit_width - controls the number of bytes to set for ancillary data [0 to skip, 8 (1byte) - 32 (4 bytes)]

 take any variety of data & tack it onto the malloced AtomicData at the next available spot (determined by its length)
 priority is given to the numerical ancillary_data so that language can be set prior to setting whatever unicode data. Finally, advance
 the length of the atom so that we can tack onto the end repeated times (up to the max malloced amount - which isn't checked [blush])
 if unicode_data is NULL itself, then only ancillary_data will be set - which is endian safe cuz o' bitshifting (or set 1 byte at a time)

 works on iTunes-style & 3GP asset style but NOT binary safe (use APar_atom_Binary_Put)
 TODO: work past the max malloced amount onto a new larger array
 ----------------------*/
void APar_Unified_atom_Put(short atom_num, const char* unicode_data, uint8_t text_tag_style, uint32_t ancillary_data, uint8_t anc_bit_width) {
    if (atom_num <= 0) {
        return; //although it should error out, because we aren't setting anything on ftyp; APar_MetaData_atom_Init on a 3gp file (for iTunes-style metadata) will give an atom_num=0, thus preventing the setting of non-compliant metadata onto 3gp files but will allow setting of data onto a non-0 atom
    }
    uint32_t atom_data_pos = parsedAtoms[atom_num].AtomicLength
            - (parsedAtoms[atom_num].AtomicClassification == EXTENDED_ATOM ? 32 : 12);
    switch (anc_bit_width) {
    case 0: { //aye, 'twas a false alarm; arg (I'm a pirate), we just wanted to set a text string
        break;
    }

    case 8: { //compilation, podcast flag, advisory
        parsedAtoms[atom_num].AtomicData[atom_data_pos] = (uint8_t) ancillary_data;
        parsedAtoms[atom_num].AtomicLength++;
        atom_data_pos++;
        break;
    }

    case 16: { //lang & its ilk
        parsedAtoms[atom_num].AtomicData[atom_data_pos] = (ancillary_data & 0xff00) >> 8;
        parsedAtoms[atom_num].AtomicData[atom_data_pos + 1] = (ancillary_data & 0xff) << 0;
        parsedAtoms[atom_num].AtomicLength += 2;
        atom_data_pos += 2;
        break;
    }

    case 32: { //things like coordinates and.... stuff (ah, the prose)
        parsedAtoms[atom_num].AtomicData[atom_data_pos] = (ancillary_data & 0xff000000) >> 24;
        parsedAtoms[atom_num].AtomicData[atom_data_pos + 1] = (ancillary_data & 0xff0000) >> 16;
        parsedAtoms[atom_num].AtomicData[atom_data_pos + 2] = (ancillary_data & 0xff00) >> 8;
        parsedAtoms[atom_num].AtomicData[atom_data_pos + 3] = (ancillary_data & 0xff) << 0;
        parsedAtoms[atom_num].AtomicLength += 4;
        atom_data_pos += 4;
        break;
    }

    default: {
        break;
    }
    }

    if (unicode_data != NULL) {
        if (text_tag_style == UTF16_3GP_Style) {
            uint32_t string_length = strlen(unicode_data) + 1;
            uint32_t glyphs_req_bytes = mbstowcs(NULL, unicode_data, string_length) * 2; //passing NULL pre-calculates the size of wchar_t needed;

            unsigned char* utf16_conversion = (unsigned char*) calloc(1, sizeof(unsigned char) * string_length * 2);

            UTF8ToUTF16BE(utf16_conversion, glyphs_req_bytes, (unsigned char*) unicode_data, string_length);

            parsedAtoms[atom_num].AtomicData[atom_data_pos] = 0xFE; //BOM
            parsedAtoms[atom_num].AtomicData[atom_data_pos + 1] = 0xFF; //BOM
            atom_data_pos += 2; //BOM

            /* copy the string directly onto AtomicData at the address of the start of AtomicData + the current length in atom_data_pos */
            /* in marked contrast to iTunes-style metadata where a string is a single string, 3gp tags like keyword & classification are more complex */
            /* directly putting the text into memory and being able to tack on more becomes a necessary accommodation */
            memcpy(parsedAtoms[atom_num].AtomicData + atom_data_pos, utf16_conversion, glyphs_req_bytes);
            parsedAtoms[atom_num].AtomicLength += glyphs_req_bytes;

            //double check terminating NULL (don't want to double add them - blush.... or have them missing - blushing on the.... other side)
            if (parsedAtoms[atom_num].AtomicData[atom_data_pos + (glyphs_req_bytes - 1)]
                    + parsedAtoms[atom_num].AtomicData[atom_data_pos + glyphs_req_bytes] != 0) {
                parsedAtoms[atom_num].AtomicLength += 4; //+4 because add 2 bytes for the character we just found + 2bytes for the req. NULL
            }
            free(utf16_conversion);
            utf16_conversion = NULL;

        }
        else if (text_tag_style == UTF8_iTunesStyle_Binary) { //because this will be 'binary' data (a misnomer for purl & egid), memcpy 4 bytes into AtomicData, not at the start of it
            uint32_t binary_bytes = strlen(unicode_data);
            memcpy(parsedAtoms[atom_num].AtomicData + atom_data_pos, unicode_data, binary_bytes + 1);
            parsedAtoms[atom_num].AtomicLength += binary_bytes;

        }
        else {
            uint32_t total_bytes = 0;

            if (text_tag_style == UTF8_3GP_Style) {
                total_bytes = strlen(unicode_data);
                total_bytes++; //include the terminating NULL

            }
            else if (text_tag_style == UTF8_iTunesStyle_256glyphLimited) {

                uint32_t raw_bytes = strlen(unicode_data);
                total_bytes = utf8_length(unicode_data, 255); //counts the number of characters, not bytes

                if (raw_bytes > total_bytes && total_bytes > 255) {

                    fprintf(stdout, "AtomicParsley warning: %s was trimmed to 255 characters (%u characters over)\n", parsedAtoms[APar_FindParentAtom(atom_num, parsedAtoms[atom_num].AtomicLevel)].AtomicName, utf8_length(unicode_data
                            + total_bytes, 0));
                }
                else {
                    total_bytes = raw_bytes;
                }

            }
            else if (text_tag_style == UTF8_iTunesStyle_Unlimited) {
                total_bytes = strlen(unicode_data);

                if (total_bytes > MAXDATA_PAYLOAD) {
                    free(parsedAtoms[atom_num].AtomicData);
                    parsedAtoms[atom_num].AtomicData = NULL;

                    parsedAtoms[atom_num].AtomicData = (char*) malloc(sizeof(char) * (total_bytes + 1));
                    memset(parsedAtoms[atom_num].AtomicData + atom_data_pos, 0, total_bytes + 1);

                }
            }

            //if we are setting iTunes-style metadata, add 0 to the pointer; for 3gp user data atoms - add in the (length-default bare atom lenth): account for language uint16_t (plus any other crap we will set); unicodeWin32 with wchar_t was converted right after program started, so do a direct copy

            memcpy(parsedAtoms[atom_num].AtomicData + atom_data_pos, unicode_data, total_bytes + 1);
            parsedAtoms[atom_num].AtomicLength += total_bytes;
        }
    }
    return;
}

/*----------------------
 APar_atom_Binary_Put
 atom_num - the index into the parsedAtoms array for the atom we are setting (aka AtomicNumber)
 binary_data - a pointer to a string of binary data
 bytecount - number of bytes to copy
 atomic_data_offset - place binary data some bytes offset from the start of AtomicData

 Simple placement of binary data (perhaps containing NULLs) onto AtomicData.
 TODO: if over MAXDATA_PAYLOAD malloc a new char string
 ----------------------*/
void APar_atom_Binary_Put(short atom_num, const char* binary_data, uint32_t bytecount, uint32_t atomic_data_offset) {
    if (atom_num <= 0) {
        return;
    }
    if (atomic_data_offset + bytecount + parsedAtoms[atom_num].AtomicLength <= MAXDATA_PAYLOAD) {
        memcpy(parsedAtoms[atom_num].AtomicData + atomic_data_offset, binary_data, bytecount);
        parsedAtoms[atom_num].AtomicLength += bytecount;
    }
    else {
        fprintf(stdout, "AtomicParsley warning: some data was longer than the allotted space and was skipped\n");
    }
    return;
}

/*----------------------
 APar_Verify__udta_meta_hdlr__atom

 only test if the atom is present for now, it will be created just before writeout time - to insure it only happens once.
 ----------------------*/
void APar_Verify__udta_meta_hdlr__atom() {
    bool Create__udta_meta_hdlr__atom = false;

    if (metadata_style == ITUNES_STYLE && hdlrAtom == NULL) {
        hdlrAtom = APar_FindAtom("moov.udta.meta.hdlr", false, VERSIONED_ATOM, 0);
        if (hdlrAtom == NULL) {
            Create__udta_meta_hdlr__atom = true;
        }
    }
    if (Create__udta_meta_hdlr__atom) {

        //if Quicktime (Player at the least) is used to create any type of mp4 file, the entire udta hierarchy is missing. If iTunes doesn't find
        //this "moov.udta.meta.hdlr" atom (and its data), it refuses to let any information be changed & the dreaded "Album Artwork Not Modifiable"
        //shows up. It's because this atom is missing. Oddly, QT Player can see the info, but this only works for mp4/m4a files.

        hdlrAtom = APar_FindAtom("moov.udta.meta.hdlr", true, VERSIONED_ATOM, 0);

        APar_MetaData_atom_QuickInit(hdlrAtom->AtomicNumber, 0, 0);
        APar_Unified_atom_Put(hdlrAtom->AtomicNumber, NULL, UTF8_iTunesStyle_256glyphLimited, 0x6D646972, 32); //'mdir'
        APar_Unified_atom_Put(hdlrAtom->AtomicNumber, NULL, UTF8_iTunesStyle_256glyphLimited, 0x6170706C, 32); //'appl'
        APar_Unified_atom_Put(hdlrAtom->AtomicNumber, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 32);
        APar_Unified_atom_Put(hdlrAtom->AtomicNumber, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 32);
        APar_Unified_atom_Put(hdlrAtom->AtomicNumber, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 16);
    }
    return;
}

/*----------------------
 APar_MetaData_atomGenre_Set
 atomPayload - the desired string value of the genre

 genre is special in that it gets carried on 2 atoms. A standard genre (as listed in ID3v1GenreList) is represented as a number on a 'gnre' atom
 any value other than those, and the genre is placed as a string onto a '�gen' atom. Only one or the other can be present. So if atomPayload is a
 non-NULL value, first try and match the genre into the ID3v1GenreList standard genres. Try to remove the other type of genre atom, then find or
 create the new genre atom and put the data manually onto the atom.
 ----------------------*/
void APar_MetaData_atomGenre_Set(const char* atomPayload) {
    if (metadata_style == ITUNES_STYLE) {
        const char* standard_genre_atom = "moov.udta.meta.ilst.gnre";
        const char* std_genre_data_atom = "moov.udta.meta.ilst.gnre.data";
        const char* custom_genre_atom = "moov.udta.meta.ilst.�gen";
        const char* cstm_genre_data_atom = "moov.udta.meta.ilst.�gen.data";

        if (strlen(atomPayload) == 0) {
            APar_RemoveAtom(std_genre_data_atom, VERSIONED_ATOM, 0); //find the atom; don't create if it's "" to remove
            APar_RemoveAtom(cstm_genre_data_atom, VERSIONED_ATOM, 0); //find the atom; don't create if it's "" to remove
        }
        else {

            uint8_t genre_number = StringGenreToInt(atomPayload);
            AtomicInfo* genreAtom;

            APar_Verify__udta_meta_hdlr__atom();
            modified_atoms = true;

            if (genre_number != 0) {
                //first find if a custom genre atom ("�gen") exists; erase the custom-string genre atom in favor of the standard genre atom

                AtomicInfo* verboten_genre_atom = APar_FindAtom(custom_genre_atom, false, SIMPLE_ATOM, 0);

                if (verboten_genre_atom != NULL) {
                    if (strlen(verboten_genre_atom->AtomicName) > 0) {
                        if (strncmp(verboten_genre_atom->AtomicName, "�gen", 4) == 0) {
                            APar_RemoveAtom(cstm_genre_data_atom, VERSIONED_ATOM, 0);
                        }
                    }
                }

                genreAtom = APar_FindAtom(std_genre_data_atom, true, VERSIONED_ATOM, 0);
                APar_MetaData_atom_QuickInit(genreAtom->AtomicNumber, AtomFlags_Data_Binary, 0);
                APar_Unified_atom_Put(genreAtom->AtomicNumber, NULL, UTF8_iTunesStyle_256glyphLimited, 0, 8);
                APar_Unified_atom_Put(genreAtom->AtomicNumber, NULL, UTF8_iTunesStyle_256glyphLimited, (uint32_t) genre_number, 8);

            }
            else {

                AtomicInfo* verboten_genre_atom = APar_FindAtom(standard_genre_atom, false, SIMPLE_ATOM, 0);

                if (verboten_genre_atom != NULL) {
                    if (verboten_genre_atom->AtomicNumber > 5 && verboten_genre_atom->AtomicNumber < atom_number) {
                        if (strncmp(verboten_genre_atom->AtomicName, "gnre", 4) == 0) {
                            APar_RemoveAtom(std_genre_data_atom, VERSIONED_ATOM, 0);
                        }
                    }
                }
                genreAtom = APar_FindAtom(cstm_genre_data_atom, true, VERSIONED_ATOM, 0);
                APar_MetaData_atom_QuickInit(genreAtom->AtomicNumber, AtomFlags_Data_Text, 0);
                APar_Unified_atom_Put(genreAtom->AtomicNumber, atomPayload, UTF8_iTunesStyle_256glyphLimited, 0, 0);
            }
        }
    } //end if (metadata_style == ITUNES_STYLE)
    return;
}

/*----------------------
 APar_MetaData_atomArtwork_Init
 atom_num - the AtomicNumber of the atom in the parsedAtoms array (probably newly created)
 artworkPath - the path that was provided on a (hopefully) existant jpg/png file

 artwork will be inited differently because we need to test a) that the file exists and b) get its size in bytes. This info will be used at the size
 of the 'data' atom under 'covr' - and the path will be carried on AtomicData until write-out time, when the binary contents of the original will be
 copied onto the atom.
 ----------------------*/
void APar_MetaData_atomArtwork_Init(short atom_num, const char* artworkPath) {
    TestFileExistence(artworkPath, false);
    off_t picture_size = findFileSize(artworkPath);

    if (picture_size > 0) {
        APar_MetaData_atom_QuickInit(atom_num, APar_TestArtworkBinaryData(artworkPath), 0, (uint32_t) picture_size);
        FILE* artfile = APar_OpenFile(artworkPath, "rb"); //openSomeFile(artworkPath, true);
        uint32_t bytes_read = APar_ReadFile(parsedAtoms[atom_num].AtomicData + 4, artfile, (uint32_t) picture_size); //+4 for the 4 null bytes
        if (bytes_read > 0)
            parsedAtoms[atom_num].AtomicLength += bytes_read;
        fclose(artfile);
    }
    return;
}

/*----------------------
 APar_MetaData_atomArtwork_Set
 artworkPath - the path that was provided on a (hopefully) existant jpg/png file
 env_PicOptions - picture embedding preferences from a 'export PIC_OPTIONS=foo' setting

 artwork gets stored under a single 'covr' atom, but with many 'data' atoms - each 'data' atom contains the binary data for each picture.
 When the 'covr' atom is found, we create a sparse atom at the end of the existing 'data' atoms, and then perform any of the image manipulation
 features on the image. The path of the file (either original, modified artwork, or both) are returned to use for possible atom creation
 ----------------------*/
void APar_MetaData_atomArtwork_Set(const char* artworkPath, char* env_PicOptions) {
    if (metadata_style == ITUNES_STYLE) {
        const char* artwork_atom = "moov.udta.meta.ilst.covr";
        if (memcmp(artworkPath, "REMOVE_ALL", 10) == 0) {
            APar_RemoveAtom(artwork_atom, SIMPLE_ATOM, 0);

        }
        else {
            APar_Verify__udta_meta_hdlr__atom();

            modified_atoms = true;
            AtomicInfo* desiredAtom = APar_FindAtom(artwork_atom, true, SIMPLE_ATOM, 0);
            AtomicInfo sample_data_atom =
                { 0 };
            APar_CreateSurrogateAtom(&sample_data_atom, "data", 6, VERSIONED_ATOM, 0, NULL, 0);
            desiredAtom =
                    APar_CreateSparseAtom(&sample_data_atom, desiredAtom, APar_FindLastChild_of_ParentAtom(desiredAtom->AtomicNumber));

            //determine if any picture preferences will impact the picture file in any way
            myPicturePrefs = APar_ExtractPicPrefs(env_PicOptions);

#if defined (DARWIN_PLATFORM)
            char* resized_filepath = (char*)calloc(1, sizeof(char)*MAXPATHLEN+1);

            if ( ResizeGivenImage(artworkPath , myPicturePrefs, resized_filepath) ) {
                APar_MetaData_atomArtwork_Init(desiredAtom->AtomicNumber, resized_filepath);

                if (myPicturePrefs.addBOTHpix) {
                    //create another sparse atom to hold the new image data
                    desiredAtom = APar_CreateSparseAtom(&sample_data_atom, desiredAtom, APar_FindLastChild_of_ParentAtom(parent_atom) );
                    APar_MetaData_atomArtwork_Init(desiredAtom->AtomicNumber, artworkPath);
                    if (myPicturePrefs.removeTempPix) remove(resized_filepath);
                }
            }
            else {
                APar_MetaData_atomArtwork_Init(desiredAtom->AtomicNumber, artworkPath);
            }
            free(resized_filepath);
            resized_filepath=NULL;
#else
            //perhaps some libjpeg based resizing/modification for non-Mac OS X based platforms
            APar_MetaData_atomArtwork_Init(desiredAtom->AtomicNumber, artworkPath);
#endif
        }
    } ////end if (metadata_style == ITUNES_STYLE)
    return;
}

/*----------------------
 APar_3GP_Keyword_atom_Format
 keywords_globbed - the globbed string of keywords ('foo1,foo2,foo_you')
 keyword_count - count of keywords in the above globbed string
 set_UTF16_text - whether to encode as utf16
 formed_keyword_struct - the char string that will hold the converted keyword struct (manually formatted)

 3gp keywords are a little more complicated. Since they will be entered separated by some form of punctuation, they need to be separated out
 They also will possibly be converted to utf16 - and they NEED to start with the BOM then. Prefacing each keyword is the 8bit length of the string
 And each keyword needs to be NULL terminated. Technically it would be possible to even have mixed encodings (not supported here).
 ----------------------*/
uint32_t APar_3GP_Keyword_atom_Format(char* keywords_globbed, uint8_t keyword_count, bool set_UTF16_text, char* &formed_keyword_struct) {
    uint32_t formed_string_offset = 0;
    uint32_t string_len = 0;

    char* a_keyword = strsep(&keywords_globbed, ",");

    for (uint8_t i = 1; i <= keyword_count; i++) {
        string_len = strlen(a_keyword);
        if (set_UTF16_text) {

            uint32_t glyphs_req_bytes = mbstowcs(NULL, a_keyword, string_len + 1) * 2; //passing NULL pre-calculates the size of wchar_t needed;

            formed_keyword_struct[formed_string_offset + 1] = 0xFE; //BOM
            formed_keyword_struct[formed_string_offset + 2] = 0xFF; //BOM
            formed_string_offset += 3; //BOM + keyword length that has yet to be set

            int bytes_converted =
                    UTF8ToUTF16BE((unsigned char*) (formed_keyword_struct + formed_string_offset), glyphs_req_bytes, (unsigned char*) a_keyword, string_len);

            if (bytes_converted > 1) {
                formed_keyword_struct[formed_string_offset - 3] = (uint8_t) bytes_converted + 4; //keyword length is NOW set
                formed_string_offset += bytes_converted + 2; //NULL terminator
            }
        }
        else {

            uint32_t string_len = strlen(a_keyword);
            formed_keyword_struct[formed_string_offset] = (uint8_t) string_len + 1; //add the terminating NULL
            formed_string_offset++;
            memcpy(formed_keyword_struct + formed_string_offset, a_keyword, string_len);
            formed_string_offset += (string_len + 1);
        }
        a_keyword = strsep(&keywords_globbed, ",");
    }
    return formed_string_offset;
}

/*----------------------
 APar_uuid_atom_Init
 atom_path - the parent hierarchy of the desired atom (with the location of the specific uuid atom supplied as '=%s')
 uuidName - the name of the atom (possibly provided in a forbidden utf8 - only latin1 aka iso8859 is acceptable)
 dataType - for now text is only supported; really its atom version/flags as used by iTunes
 uuidValue - the string that will get embedded onto the atom
 shellAtom - flag to denote whether the atom may possibly come as utf8 encoded

 uuid atoms are user-supplied/user-defined atoms that allow for extended tagging support. Because a uuid atom is malleable, and defined by the utility
 that created it, any information carried by a uuid is arbitrary, and cannot be guaranteed by a non-originating utility. In AtomicParsley uuid atoms,
 the data is presented much like an iTunes-style atom - except that the information gets carried directly on the uuid atom - no 'data' child atom
 exists. A uuid atom is a special longer type of traditional atom. As a traditional atom, it name is 'uuid' - and the 4 bytes after that represent
 its uuid name. Because the data will be directly sitting on the atom, a different means of finding these atoms exists, as well as creating the
 acutal uuidpath itself. Once created however, placing information on it is very much like any other atom - done via APar_Unified_atom_Put

 //4bytes atom length, 4 bytes 'uuid', 16bytes uuidv5, 4bytes name of uuid in AP namespace, 4bytes versioning, 4bytes NULL, Xbytes data
 ----------------------*/
short APar_uuid_atom_Init(const char* atom_path, char* uuidName, const uint32_t dataType, const char* uuidValue, bool shellAtom) {
    AtomicInfo* desiredAtom = NULL;
    char uuid_path[256];
    char uuid_binary_str[20];
    char uuid_4char_name[10];
    memset(uuid_path, 0, 20);
    memset(uuid_binary_str, 0, 20);
    memset(uuid_4char_name, 0, 10);
    uint16_t path_len = 0;

    if (shellAtom) {
        UTF8Toisolat1((unsigned char*) &uuid_4char_name, 4, (unsigned char*) uuidName, strlen(uuidName));
    }
    else {
        memcpy(uuid_4char_name, uuidName, 4);
    }

    APar_generate_uuid_from_atomname(uuid_4char_name, uuid_binary_str);
    APar_endian_uuid_bin_str_conversion(uuid_binary_str);

    //this will only append (and knock off) %s (anything) at the end of a string
    path_len = strlen(atom_path);
    memcpy(uuid_path, atom_path, path_len - 2);
    memcpy(uuid_path + (path_len - 2), uuid_binary_str, 16);

#if defined(DEBUG_V)
    fprintf(stdout, "debug: APar_uuid_atom_Init desired atom '%s' converted to uuidv5: ", uuidName);
    APar_print_uuid((ap_uuid_t*) (uuid_path + (path_len - 2)));
#endif

    if (uuidValue == NULL || strlen(uuidValue) == 0) {
        APar_RemoveAtom(uuid_path, EXTENDED_ATOM, 0); //find the atom; don't create if it's "" to remove
        return -1;

    }
    else {
        if (!(dataType == AtomFlags_Data_Text || dataType == AtomFlags_Data_uuid_binary)) { //the only supported types
            fprintf(stdout, "AP warning: only text or file types are allowed on uuid atom %s (%u-%u); skipping\n", uuidName, dataType, AtomFlags_Data_Text);
            return -1;
        }
        //uuid atoms won't have 'data' child atoms - they will carry the data directly as opposed to traditional iTunes-style metadata that does store the information on 'data' atoms. But user-defined is user-defined, so that is how it will be defined here.
        modified_atoms = true;

        desiredAtom = APar_FindAtom(uuid_path, true, EXTENDED_ATOM, 0, true);
        desiredAtom->uuid_ap_atomname = (char*) calloc(1, sizeof(char) * 10); //only useful to print out the atom tree midway through an operation
        memcpy(desiredAtom->uuid_ap_atomname, uuid_4char_name, 4); //only useful to print out the atom tree midway through an operation

        if (dataType == AtomFlags_Data_Text)
            APar_MetaData_atom_QuickInit(desiredAtom->AtomicNumber, dataType, 20); //+20 including the 4 NULL bytes preceding any string we set
        //NOTE: setting a file into a uuid atom  (dataType == AtomFlags_Data_uuid_binary) is handled in main.cpp - the length of the file extension, description and file
        //all add up to the amount to malloc AtomicData to, so handle that separately.

        parsedAtoms[desiredAtom->AtomicNumber].AtomicClassification = EXTENDED_ATOM;
    }
    return desiredAtom->AtomicNumber;
}

/*----------------------
 APar_MetaData_atom_QuickInit
 atom_num - the position in the parsedAtoms array (either found in the file or a newly created sparse atom) so AtomicData can be initialized
 atomFlags - the AtomicVerFlags for the iTunes-style metadata atom
 supplemental_length - iTunes-style metadata for 'data' atoms is >= 16bytes long; AtomicParsley created uuid atoms will be +4bytes directly on that atom
 allotment - the bytes of AtomicData to malloc (defaults to MAXDATA_PAYLOAD + 1 (+50) unless changed - like uuids from file)

 Metadata_QuickInit will initialize a pre-found atom to MAXDATA_PAYLOAD so that it can carry info on AtomicData
 ----------------------*/
void APar_MetaData_atom_QuickInit(short atom_num, const uint32_t atomFlags, uint32_t supplemental_length, uint32_t allotment) {
    //this will skip the finding of atoms and just malloc the AtomicData; used by genre & artwork

    parsedAtoms[atom_num].AtomicData = (char*) calloc(1, sizeof(char) * allotment + 50);
    if (parsedAtoms[atom_num].AtomicData == NULL) {
        fprintf(stdout, "AP error: there was insufficient memory available for allocation. Exiting.%c\n", '\a');
        return;
    }

    parsedAtoms[atom_num].AtomicLength = 16 + supplemental_length; // 4bytes atom length, 4 bytes atom length, 4 bytes version/flags, 4 bytes NULL
    parsedAtoms[atom_num].AtomicVerFlags = atomFlags;
    parsedAtoms[atom_num].AtomicContainerState = CHILD_ATOM;
    parsedAtoms[atom_num].AtomicClassification = VERSIONED_ATOM;

    return;
}

/*----------------------
 APar_MetaData_atom_Init
 atom_path - the hierarchical path to the specific atom carrying iTunes-style metadata that will be found (and created if necessary)
 MD_Payload - the information to be carried (also used as a test if NULL to remove the atom)
 atomFlags - the AtomicVerFlags for the atom (text, integer or unsigned integer)

 Metadata_Init will search for and create the necessary hierarchy so that the atom can be initialized to carry the payload data on AtomicData.
 This will provide a shell of an atom with 4bytes length, 4bytes name, 4bytes version/flags, 4bytes NULL + any other data
 ----------------------*/
short APar_MetaData_atom_Init(const char* atom_path, const char* MD_Payload, const uint32_t atomFlags) {
    //this will handle the vanilla iTunes-style metadata atoms; genre will be handled elsewehere because it gets carried on 2 different atoms, and artwork gets special treatment because it can have multiple child data atoms
    if (metadata_style != ITUNES_STYLE) {
        return 0;
    }
    bool retain_atom = true;

    if (strlen(MD_Payload) == 0) {
        retain_atom = false;
    }

    if (retain_atom) {
        APar_Verify__udta_meta_hdlr__atom();
    }

    AtomicInfo* desiredAtom = APar_FindAtom(atom_path, retain_atom, VERSIONED_ATOM, 0); //finds the atom; if not present, creates the atom
    if (desiredAtom == NULL)
        return -1;
    modified_atoms = true;

    if (!retain_atom) {
        AtomicInfo* parent_atom = &parsedAtoms[APar_FindParentAtom(desiredAtom->AtomicNumber, desiredAtom->AtomicLevel)];
        if (desiredAtom->AtomicNumber > 0 && parent_atom->AtomicNumber > 0) {
            APar_EliminateAtom(parent_atom->AtomicNumber, desiredAtom->NextAtomNumber);
            return -1;
        }

    }
    else {
        parsedAtoms[desiredAtom->AtomicNumber].AtomicData = (char*) malloc(sizeof(char) * MAXDATA_PAYLOAD + 1); //puts a hard limit on the length of strings (the spec doesn't)
        memset(parsedAtoms[desiredAtom->AtomicNumber].AtomicData, 0, sizeof(char) * MAXDATA_PAYLOAD + 1);

        parsedAtoms[desiredAtom->AtomicNumber].AtomicLength = 16; // 4bytes atom length, 4 bytes atom length, 4 bytes version/flags, 4 bytes NULL
        parsedAtoms[desiredAtom->AtomicNumber].AtomicVerFlags = atomFlags;
        parsedAtoms[desiredAtom->AtomicNumber].AtomicContainerState = CHILD_ATOM;
        parsedAtoms[desiredAtom->AtomicNumber].AtomicClassification = VERSIONED_ATOM;
    }
    return desiredAtom->AtomicNumber;
}

/*----------------------
 APar_UserData_atom_Init
 atom_path - the hierarchical path to the specific 3gp-style(Quicktime-style) asset atom that will be found (and created if necessary)
 UD_Payload - the information to be carried (also used as a test if NULL to remove the atom)
 UD_lang - the language for the tag (multiple tags with the same name, but different languages are supported for some of these atoms)

 UserData_Init will search for and create the necessary hierarchy so that the atom can be initialized to carry the payload data on AtomicData
 3GP-style assets are different from iTunes-style metadata. 3GP assets are carried directly on the atom (iTunes md is on a child 'data' atom),
 has a packed language code, and can be in utf8 or utf16 which are NULL terminated (not so in iTunes md). The two implementations are separate

 NOTE: the language setting (if supported - yrrc doesn't) occurs in different places in 3GP atoms. Most occur right after atom flags/versioning - but
 in rtng/clsf they occur later. The language is instead stored in binary form amid the data for the atom, but is also put into the parsedAtoms[] array
 (that information is only used in finding atoms not the actual writing of atoms out). Both (storing the language in AtomicData in binary form & put in
 the parsedAtoms[] AtomicInfo array) forms are required to implement multiple language support for 3gp atoms
 ----------------------*/
short APar_UserData_atom_Init(const char* atom_path, const char* UD_Payload, uint16_t UD_lang) {
    //Perhaps there is something wrong with Apple's implementation of 3gp metadata, or I'm loosing my mind.
    //the exact same utf8 string that shows up in a 3gp file as ??? - ??? shows up *perfect* in an mp4 or mov container. encoded as utf16 same problem
    //a sample string using Polish glyphs in utf8 has some gylphs missing with lang=eng. The same string with 'lang=pol', and different glyphs are missing.
    //the problem occurs using unicode.org's ConvertUTF8toUTF16 or using libxmls's UTF8ToUTF16BE (when converting to utf16) in the same places - except for the copyright symbol which unicode.org's ConvertUTF16toUTF8 didn't properly convert - which was the reason why libxml's functions are now used. And at no point can I get the audio protected P-in-a-circle glyph to show up in utf8 or utf16.
    //to summarize, either I am completely overlooking some interplay (of lang somehow changing the utf8 or utf16 standard), the unicode translations are off (which in the case of utf8 is embedded directly on Mac OS X, so that can't be it), or Apple's 3gp implementation is off.

    bool retain_atom = true;
    AtomicInfo* desiredAtom = NULL;

    if (strlen(UD_Payload) == 0) {
        retain_atom = false;
    }
    uint8_t atom_type = PACKED_LANG_ATOM;
    if (UD_lang == 0) {
        atom_type = VERSIONED_ATOM;
    }

    if (!retain_atom) {
        APar_RemoveAtom(atom_path, atom_type, atom_type == VERSIONED_ATOM ? 1 : UD_lang); //find the atom; don't create if it's "" to remove
        return -1;
    }
    else {
        modified_atoms = true;
        desiredAtom = APar_FindAtom(atom_path, true, atom_type, UD_lang);

        parsedAtoms[desiredAtom->AtomicNumber].AtomicData = (char*) malloc(sizeof(char) * MAXDATA_PAYLOAD); //puts a hard limit on the length of strings (the spec doesn't)
        memset(parsedAtoms[desiredAtom->AtomicNumber].AtomicData, 0, sizeof(char) * MAXDATA_PAYLOAD);

        parsedAtoms[desiredAtom->AtomicNumber].AtomicLength = 12; // 4bytes atom length, 4 bytes atom length, 4 bytes version/flags (NULLs)
        parsedAtoms[desiredAtom->AtomicNumber].AtomicVerFlags = 0;
        parsedAtoms[desiredAtom->AtomicNumber].AtomicContainerState = CHILD_ATOM;
        parsedAtoms[desiredAtom->AtomicNumber].AtomicClassification = atom_type;
        parsedAtoms[desiredAtom->AtomicNumber].AtomicLanguage = UD_lang;
    }
    return desiredAtom->AtomicNumber;
}

void APar_StandardTime(char* &formed_time) {
    //ISO 8601 Coordinated Universal Time (UTC)
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(formed_time, 100, "%Y-%m-%dT%H:%M:%SZ", timeinfo); //that hanging Z is there; denotes the UTC

    return;
}

/*----------------------
 APar_ISO_UserData_Set
 iso_atom_name - the name of the atom that will be set/removed
 iso_payload - the information the atom is to carry (possibly of zero length, meaning remove)
 iso_container - specifies where to place/find the atom: movie level, all tracks, or a single track
 iso_track - if the container will be a single track, this will carry exactly which track to work on
 packed_lang - the packed language code for the atom (defaults to packed 'eng')
 set_UTF16_text - controls whether to set iso_payload as utf16 encoded text

 Gather the number of tracks first, then based on which container (moov.udta or moov.trak.udta) construct the atom path using the index into which trak
 atom. Loop through either once (for a specific track or at movie level) or for all tracks if setting the notification for all tracks. If whatever is
 to be set is "", then remove that atom (removal takes the language into consideration). If the playload is not "", then find & create the sparse atom,
 initialize its AtomicData, and send the atom off with the payload to be set in APar_Unified_atom_Put.

 TODO NOTE: the track modification date should change because of this
 ----------------------*/
void APar_ISO_UserData_Set(char* iso_atom_name, const char* iso_payload, uint8_t iso_container, uint8_t iso_track, uint16_t packed_lang, bool set_UTF16_text) {
    uint8_t total_tracks = 0;
    uint8_t total_settings = 0;
    uint8_t a_track = 0; //unused
    short an_atom = 0; //unused
    AtomicInfo* desiredAtom = NULL;

    APar_TrackInfo(total_tracks, a_track, an_atom); //With track_num set to 0, it will return the total trak atom into total_tracks here.

    if (iso_track > total_tracks || (iso_track == 0 && iso_container == SINGLE_TRACK_ATOM)) {
        APar_assert(false, 5, iso_atom_name);
        return;
    }

    if (iso_container == MOVIE_LEVEL_ATOM) {
        total_settings = 1;
    }
    else if (iso_container == ALL_TRACKS_ATOM) {
        total_settings = total_tracks;
    }
    else if (iso_container == SINGLE_TRACK_ATOM) {
        total_settings = 1;
    }
    char* iso_atom_path = (char*) malloc(sizeof(char) * 400);

    for (uint8_t i = 1; i <= total_settings; i++) {
        memset(iso_atom_path, 0, 400);
        if (iso_container == MOVIE_LEVEL_ATOM) {
            memcpy(iso_atom_path, "moov.udta.", 10);
            memcpy(iso_atom_path + 10, iso_atom_name, 4);
        }
        else if (iso_container == ALL_TRACKS_ATOM) {
            sprintf(iso_atom_path, "moov.trak[%u].udta.%s", i, iso_atom_name);
        }
        else if (iso_container == SINGLE_TRACK_ATOM) {
            sprintf(iso_atom_path, "moov.trak[%u].udta.%s", iso_track, iso_atom_name);
        }

        if (strlen(iso_payload) == 0) {
            APar_RemoveAtom(iso_atom_path, PACKED_LANG_ATOM, packed_lang); //find the atom; don't create if it's "" to remove
        }
        else {
            modified_atoms = true;
            if (iso_container != MOVIE_LEVEL_ATOM)
                prevent_update_using_padding = true; //because updating via padding works off of 'moov.udta', a full rewrite is req for tracks.

            desiredAtom = APar_FindAtom(iso_atom_path, true, PACKED_LANG_ATOM, packed_lang);

            desiredAtom->AtomicData = (char*) malloc(sizeof(char) * MAXDATA_PAYLOAD); //puts a hard limit on the length of strings (the spec doesn't)
            memset(desiredAtom->AtomicData, 0, sizeof(char) * MAXDATA_PAYLOAD);

            desiredAtom->AtomicLength = 12; // 4bytes atom length, 4 bytes atom length, 4 bytes version/flags (NULLs)
            desiredAtom->AtomicVerFlags = 0;
            desiredAtom->AtomicContainerState = CHILD_ATOM;
            desiredAtom->AtomicClassification = PACKED_LANG_ATOM;
            desiredAtom->AtomicLanguage = packed_lang;
            APar_Unified_atom_Put(desiredAtom->AtomicNumber, iso_payload, (
                    set_UTF16_text ? UTF16_3GP_Style : UTF8_3GP_Style), (uint32_t) packed_lang, 16);
        }
    }
    free(iso_atom_path);
    iso_atom_path = NULL;
    return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                         'stco' Atom Calculations                                  //
///////////////////////////////////////////////////////////////////////////////////////

//determine if our mdat atom has moved at all...
uint32_t APar_DetermineMediaData_AtomPosition() {
    uint32_t mdat_position = 0;
    short thisAtomNumber = 0;

    //loop through each atom in the struct array (which holds the offset info/data)
    while (parsedAtoms[thisAtomNumber].NextAtomNumber != 0) {

        if (strncmp(parsedAtoms[thisAtomNumber].AtomicName, "mdat", 4) == 0
                && parsedAtoms[thisAtomNumber].AtomicLevel == 1) {
            if (parsedAtoms[thisAtomNumber].AtomicLength <= 1 || parsedAtoms[thisAtomNumber].AtomicLength > 75) {
                break;
            }
        }
        else if (parsedAtoms[thisAtomNumber].AtomicLevel == 1
                && parsedAtoms[thisAtomNumber].AtomicLengthExtended == 0) {
            mdat_position += parsedAtoms[thisAtomNumber].AtomicLength;
        }
        else {
            //part of the pseudo 64-bit support
            mdat_position += parsedAtoms[thisAtomNumber].AtomicLengthExtended;
        }
        thisAtomNumber = parsedAtoms[thisAtomNumber].NextAtomNumber;
    }
    return mdat_position;
}

uint32_t APar_SimpleSumAtoms(short stop_atom) {
    uint32_t byte_sum = 0;
    //first, find the first mdat after this initial 'tfhd' atom to get the sum relative to that atom
    while (true) {
        if (strncmp(parsedAtoms[stop_atom].AtomicName, "mdat", 4) == 0) {
            stop_atom--; //don't include the fragment's mdat, just the atoms prior to it
            break;
        }
        else {
            if (parsedAtoms[stop_atom].NextAtomNumber != 0) {
                stop_atom = parsedAtoms[stop_atom].NextAtomNumber;
            }
            else {
                break;
            }
        }
    }
    byte_sum += 8; //the 'tfhd' points to the byte in mdat where the fragment data is - NOT the atom itself (should always be +8bytes with a fragment)
    while (true) {
        if (parsedAtoms[stop_atom].AtomicLevel == 1) {
            byte_sum +=
                    (parsedAtoms[stop_atom].AtomicLength == 1 ? (uint32_t) parsedAtoms[stop_atom].AtomicLengthExtended : parsedAtoms[stop_atom].AtomicLength);
        }
        if (stop_atom == 0) {
            break;
        }
        else {
            stop_atom = APar_FindPrecedingAtom(stop_atom);
        }
    }
    return byte_sum;
}

bool APar_Readjust_CO64_atom(uint32_t mdat_position, short co64_number) {
    bool co64_changed = false;
    APar_AtomicRead(co64_number);
    parsedAtoms[co64_number].AtomicVerFlags = 0;
    bool deduct = false;
    //readjust

    char* co64_entries = (char *) malloc(sizeof(char) * 4 + 1);
    memset(co64_entries, 0, sizeof(char) * 4 + 1);

    memcpy(co64_entries, parsedAtoms[co64_number].AtomicData, 4);
    uint32_t entries = UInt32FromBigEndian(co64_entries);

    char* a_64bit_entry = (char *) malloc(sizeof(char) * 8 + 1);
    memset(a_64bit_entry, 0, sizeof(char) * 8 + 1);

    for (uint32_t i = 1; i <= entries; i++) {
        //read 8 bytes of the atom into a 8 char uint64_t a_64bit_entry to eval it
        for (int c = 0; c <= 7; c++) {
            //first co64 entry (32-bit uint32_t) is the number of entries; every other one is an actual offset value
            a_64bit_entry[c] = parsedAtoms[co64_number].AtomicData[4 + (i - 1) * 8 + c];
        }
        uint64_t this_entry = UInt64FromBigEndian(a_64bit_entry);

        if (i == 1 && mdat_supplemental_offset == 0) { //for the first chunk, and only for the first *ever* entry, make the global mdat supplemental offset
            if (this_entry - removed_bytes_tally > mdat_position) {
                mdat_supplemental_offset = (uint64_t) mdat_position
                        - ((uint64_t) this_entry - (uint64_t) removed_bytes_tally);
                bytes_into_mdat = this_entry - bytes_before_mdat - removed_bytes_tally;
                deduct = true;
            }
            else {
                mdat_supplemental_offset = mdat_position - (this_entry - removed_bytes_tally);
                bytes_into_mdat = this_entry - bytes_before_mdat - removed_bytes_tally;
            }

            if (mdat_supplemental_offset == 0) {
                break;
            }
        }

        if (mdat_supplemental_offset != 0) {
            co64_changed = true;
        }

        if (deduct) { //crap, uint32_t's were so nice to flip over by themselves to subtract nicely. going from 32-bit to 64-bit prevents that flipping
            this_entry += mdat_supplemental_offset - (bytes_into_mdat * -1); // + bytes_into_mdat;
        }
        else {
            this_entry += mdat_supplemental_offset + bytes_into_mdat; //this is where we add our new mdat offset difference
        }
        char8TOuint64(this_entry, a_64bit_entry);
        //and put the data back into AtomicData...
        for (int d = 0; d <= 7; d++) {
            //first stco entry is the number of entries; every other one is an actual offset value
            parsedAtoms[co64_number].AtomicData[4 + (i - 1) * 8 + d] = a_64bit_entry[d];
        }
    }

    free(a_64bit_entry);
    free(co64_entries);
    a_64bit_entry = NULL;
    co64_entries = NULL;
    //end readjustment
    return co64_changed;
}

bool APar_Readjust_TFHD_fragment_atom(uint32_t mdat_position, short tfhd_number) {
    static bool tfhd_changed = false;
    static bool determined_offset = false;
    static uint64_t base_offset = 0;

    APar_AtomicRead(tfhd_number);
    char* tfhd_atomFlags_scrap = (char *) malloc(sizeof(char) * 10);
    memset(tfhd_atomFlags_scrap, 0, 10);
    //parsedAtoms[tfhd_number].AtomicVerFlags = APar_read32(tfhd_atomFlags_scrap, source_file, parsedAtoms[tfhd_number].AtomicStart+8);

    if (parsedAtoms[tfhd_number].AtomicVerFlags & 0x01) { //seems the atomflags suggest bitpacking, but the spec doesn't specify it; if the 1st bit is set...
        memset(tfhd_atomFlags_scrap, 0, 10);
        memcpy(tfhd_atomFlags_scrap, parsedAtoms[tfhd_number].AtomicData, 4);

        uint64_t tfhd_offset = UInt64FromBigEndian(parsedAtoms[tfhd_number].AtomicData + 4);

        if (!determined_offset) {
            determined_offset = true;
            base_offset = APar_SimpleSumAtoms(tfhd_number) - tfhd_offset;
            if (base_offset != 0) {
                tfhd_changed = true;
            }
        }

        tfhd_offset += base_offset;
        char8TOuint64(tfhd_offset, parsedAtoms[tfhd_number].AtomicData + 4);
    }
    return tfhd_changed;
}

bool APar_Readjust_STCO_atom(uint32_t mdat_position, short stco_number) {
    bool stco_changed = false;
    APar_AtomicRead(stco_number);
    parsedAtoms[stco_number].AtomicVerFlags = 0;
    //readjust

    char* stco_entries = (char *) malloc(sizeof(char) * 4 + 1);
    memset(stco_entries, 0, sizeof(char) * 4 + 1);

    memcpy(stco_entries, parsedAtoms[stco_number].AtomicData, 4);
    uint32_t entries = UInt32FromBigEndian(stco_entries);

    char* an_entry = (char *) malloc(sizeof(char) * 4 + 1);
    memset(an_entry, 0, sizeof(char) * 4 + 1);

    for (uint32_t i = 1; i <= entries; i++) {
        //read 4 bytes of the atom into a 4 char uint32_t an_entry to eval it
        for (int c = 0; c <= 3; c++) {
            //first stco entry is the number of entries; every other one is an actual offset value
            an_entry[c] = parsedAtoms[stco_number].AtomicData[i * 4 + c];
        }

        uint32_t this_entry = UInt32FromBigEndian(an_entry);

        if (i == 1 && mdat_supplemental_offset == 0) { //for the first chunk, and only for the first *ever* entry, make the global mdat supplemental offset

            mdat_supplemental_offset = (uint64_t) (mdat_position - (this_entry - removed_bytes_tally));
            bytes_into_mdat = this_entry - bytes_before_mdat - removed_bytes_tally;

            if (mdat_supplemental_offset == 0) {
                break;
            }
        }

        if (mdat_supplemental_offset != 0) {
            stco_changed = true;
        }

        this_entry += mdat_supplemental_offset + bytes_into_mdat;
        char4TOuint32(this_entry, an_entry);
        //and put the data back into AtomicData...
        for (int d = 0; d <= 3; d++) {
            //first stco entry is the number of entries; every other one is an actual offset value
            parsedAtoms[stco_number].AtomicData[i * 4 + d] = an_entry[d];
        }
    }

    free(an_entry);
    free(stco_entries);
    an_entry = NULL;
    stco_entries = NULL;
    //end readjustment
    return stco_changed;
}

///////////////////////////////////////////////////////////////////////////////////////
//                            Reorder / Padding                                      //
///////////////////////////////////////////////////////////////////////////////////////

void APar_ForcePadding(uint32_t padding_amount) {
    if (tree_display_only || padding_amount == 0) {
        return;
    }

    if (udta_dynamics.free_atom_repository) {
        parsedAtoms[udta_dynamics.free_atom_repository].AtomicLength = padding_amount;

        parsedAtoms[udta_dynamics.free_atom_repository].AtomicData = (char*) malloc(sizeof(char) * padding_amount); //allocate memory to write the NULL space out)
        memset(parsedAtoms[udta_dynamics.free_atom_repository].AtomicData, 0, sizeof(char) * padding_amount);
    }
    else if (udta_dynamics.free_atom_secondary_repository) {
        parsedAtoms[udta_dynamics.free_atom_secondary_repository].AtomicLength = padding_amount;

        parsedAtoms[udta_dynamics.free_atom_secondary_repository].AtomicData = (char*) malloc(sizeof(char)
                * padding_amount);
        memset(parsedAtoms[udta_dynamics.free_atom_secondary_repository].AtomicData, 0, sizeof(char) * padding_amount);
    }
    else {
        APar_InterjectNewAtom("free", CHILD_ATOM, SIMPLE_ATOM, padding_amount, 0, 0, 1, APar_FindLastChild_of_ParentAtom(udta_dynamics.moov_atom));
    }
    new_file_size += padding_amount;
    return;
}

/*----------------------
 APar_ConsilidatePadding
 force_padding_amount - the amount of padding to add (typically the default amount)

 If no existing 'free' atom that can be considered padding is available, add the forced amount. If there is some repository of 'free' space, iterate thru deleting
 'free' space, and adding the length to the repository's length - thereby consolidating multiple 'free' atoms into a single 'free' atom. Then malloc the 'free'
 atom's AtomicData to that length so that when writeout time comes, this 'free' will write from memory - and it will be all nulled out.
 ----------------------*/
void APar_ConsilidatePadding(uint32_t force_padding_amount) {
    if (force_padding_amount <= 8 || tree_display_only) { //prevent having an atom of length 0 (which means it goes to EOF)
        return;
    }
    short primary_repository = 0;
    if (udta_dynamics.free_atom_repository) {
        primary_repository = udta_dynamics.free_atom_repository;
    }
    else if (udta_dynamics.free_atom_secondary_repository) {
        primary_repository = udta_dynamics.free_atom_secondary_repository;
    }
    else if (force_padding_amount >= 8) {
        APar_InterjectNewAtom("free", CHILD_ATOM, SIMPLE_ATOM, force_padding_amount, 0, 0, 1, APar_FindLastChild_of_ParentAtom(udta_dynamics.moov_atom));
        new_file_size += force_padding_amount;
        return;
    }

    short iter = parsedAtoms[udta_dynamics.udta_atom].NextAtomNumber;
    while (true) {
        //when we eliminate atoms in this loop, the NextAtomNumber becomes -1; next_atom ensures the list is followed
        short next_atom = parsedAtoms[iter].NextAtomNumber;

        if (memcmp(parsedAtoms[iter].AtomicName, "free", 4) == 0 && iter != primary_repository) {
            if (iter == udta_dynamics.last_udta_child_atom) {
                udta_dynamics.last_udta_child_atom = APar_FindPrecedingAtom(iter);
            }
            APar_EliminateAtom(iter, parsedAtoms[iter].NextAtomNumber);
        }

        if (iter == udta_dynamics.first_postfree_level1_atom) { //this makes sure that anything in udta is processed...
            break;
        }
        iter = next_atom;
    }
    parsedAtoms[primary_repository].AtomicLength = force_padding_amount;

    parsedAtoms[primary_repository].AtomicData = (char*) malloc(sizeof(char) * force_padding_amount);
    memset(parsedAtoms[primary_repository].AtomicData, 0, sizeof(char) * force_padding_amount);

    return;
}

/*----------------------
 APar_ForcePadding_sans_udta

 Handles adding padding to a  file without a 'udta' atom (say when using --ISO-copyright). Only handles 'free' & 'skip' atoms, but only at file level (1).
 ----------------------*/
void APar_ForcePadding_sans_udta() {
    short total_file_level_atoms = APar_ReturnChildrenAtoms(0, 0); //'ftyp' (atom 0) is level 1, so this finds the count of level 1 atoms
    short eval_atom = 0;
    short moov_atom = 0;
    short significant_atom = 0;
    uint32_t free_padding_space = 0;

    //scan through all top level atoms; fragmented files won't be optimized
    for (uint8_t i = 1; i <= total_file_level_atoms; i++) {
        eval_atom = APar_ReturnChildrenAtoms(0, i);
        if (memcmp(parsedAtoms[eval_atom].AtomicName, "moov", 4) == 0) {
            moov_atom = eval_atom; //note moov atom so that a 'free' or 'skip' after it can be summed
        }
        if (memcmp(parsedAtoms[eval_atom].AtomicName, "mdat", 4) == 0
                || memcmp(parsedAtoms[eval_atom].AtomicName, "moof", 4) == 0) {
            significant_atom = eval_atom; //note moov atom so that a 'free' or 'skip' after it *will NOT* be summed
        }
        if (memcmp(parsedAtoms[eval_atom].AtomicName, "free", 4) == 0
                || memcmp(parsedAtoms[eval_atom].AtomicName, "skip", 4) == 0) {
            if (moov_atom > 0 && significant_atom == 0 && parsedAtoms[eval_atom].AtomicLength != 1) {
                free_padding_space += parsedAtoms[eval_atom].AtomicLength;
            }
            if (moov_atom > 0 && significant_atom == 0 && parsedAtoms[eval_atom].AtomicLength == 1) {
                free_padding_space += (uint32_t) parsedAtoms[eval_atom].AtomicLengthExtended;
            }
        }
    }
    if (free_padding_space <= pad_prefs.minimum_required_padding_size && pad_prefs.default_padding_size >= 8) {
        APar_InterjectNewAtom("free", CHILD_ATOM, SIMPLE_ATOM, pad_prefs.default_padding_size, 0, 0, 1, APar_FindLastChild_of_ParentAtom(moov_atom));
        new_file_size += pad_prefs.default_padding_size;
    }
    return;
}

/*----------------------
 APar_DetermineDynamicUpdate
 initial_pass - inital pass = consolidate 'free' atoms into one globbed 'free' repository; 2nd pass to determine the filler amount

 Find 'udta' & the last child of udta; work through all the atoms from udta to the end of udta (and past) as long as its a 'free' atom; a 'free' atom will
 have its length noted in the initial pass. In the initial pass, if padding does not fall within the padding pref parameters, it is forced. The entire tree
 will have its atom lengths redetermined due to adding & removing of atoms & data. If the file is found to warrant dynamic updating via padding, a second
 visit to this function will be issued. In the 2nd pass (and having udta's length adjusted), figure out the appropriate length of the missing space between
 the last thing in 'udta' (which has already been reordered to be last in 'moov') and the first level 1 atom. Either adjust length, or make a new 'free'
 atom to hold that filler. Run the atom hierarchy through APar_DetermineAtomLengths to adjust ilst, meta, udta & moov lengths. A specific exception for
 psp mpeg4 files is made to allow non-optimized files, but still have metadata dynamically updated (at the end of the file).

 TODO: only 'free' is used here; the free_type is defined as 'free' or 'skip' - 'skip' isn't used as padding here
 ----------------------*/
void APar_DetermineDynamicUpdate(bool initial_pass) {
    udtaAtom = APar_FindAtom("moov.udta", false, SIMPLE_ATOM, 0);

    //if there is no 'udta' atom, but we still want any available padding listed - there is none
    if (udtaAtom == NULL && !modified_atoms) {
        udta_dynamics.max_usable_free_space = 0;
        return;
    }
    else if (udtaAtom == NULL) {
        //TODO: how to handle this so a full rewrite doesn't occur, because a segfault happens a few lines down with udtaAtom->AtomicNumber
        udta_dynamics.max_usable_free_space = 0; //for now
        APar_ForcePadding_sans_udta(); //useless currently for AP since it can't consider padding without a 'udta' atom present
        return; //for now
    }

    if (!udta_dynamics.dynamic_updating) {
        udta_dynamics.udta_atom = udtaAtom->AtomicNumber;
        udta_dynamics.last_udta_child_atom = APar_FindLastChild_of_ParentAtom(udtaAtom->AtomicNumber);
        udta_dynamics.first_postfree_level1_atom = 0;
        udta_dynamics.contained_free_space = 0;
        udta_dynamics.max_usable_free_space = 0;
        udta_dynamics.free_atom_repository = 0;
    }
    bool transited_udta_metadata = false;
    short iter = udtaAtom->NextAtomNumber;

    if (!initial_pass && !psp_brand) {
        if (udta_dynamics.free_atom_repository == 0) {
            //find or create a 'top level 'free' atom after 'udta'
            udta_dynamics.free_atom_repository =
                    APar_InterjectNewAtom("free", CHILD_ATOM, SIMPLE_ATOM, udta_dynamics.max_usable_free_space
                            - (udtaAtom->AtomicLength - udta_dynamics.original_udta_size), 0, 0, 1, udta_dynamics.last_udta_child_atom);

        }
        else {
            parsedAtoms[udta_dynamics.free_atom_repository].AtomicLength += udta_dynamics.contained_free_space
                    - (udtaAtom->AtomicLength - udta_dynamics.original_udta_size);
        }
    }

    while (true) {
        //when we eliminate atoms in this loop, the NextAtomNumber becomes -1; next_atom ensures the list is followed
        short next_atom = parsedAtoms[iter].NextAtomNumber;

        if (memcmp(parsedAtoms[iter].AtomicName, "free", 4) == 0) {

            if (initial_pass) {
                udta_dynamics.max_usable_free_space += parsedAtoms[iter].AtomicLength;
                //a primary preferred repository is on level 1; this is where all 'free' atoms will eventually wind up in a dynamic update
                if (parsedAtoms[iter].AtomicLevel == 1 && udta_dynamics.free_atom_repository == 0) {
                    udta_dynamics.free_atom_repository = iter;

                    // a tracked secondary respository is a child atom & not file level; track so that if a full rewrite is required this 'free' meets default_padding_size
                }
                else if (parsedAtoms[iter].AtomicLevel > 1) {
                    if (udta_dynamics.free_atom_secondary_repository == 0) {
                        udta_dynamics.free_atom_secondary_repository = iter;
                    }
                    else if (parsedAtoms[iter].AtomicLength
                            > parsedAtoms[udta_dynamics.free_atom_secondary_repository].AtomicLength) {
                        udta_dynamics.free_atom_secondary_repository = iter;
                    }
                }
            }
            else {
                if (iter != udta_dynamics.free_atom_repository) {
                    APar_EliminateAtom(iter, parsedAtoms[iter].NextAtomNumber);
                }
            }
            //udta is already the last atom in moov because of APar_Optimize, only things left are:
            //             mdat(s), fragments, uuid(s), skip/free(s) & possible mpeg7 metadata in a L1 meta
            if (!transited_udta_metadata) {
                if (initial_pass) {
                    udta_dynamics.contained_free_space += parsedAtoms[iter].AtomicLength;
                }
                else {
                    //since it will be eliminated next, make the last_udta_child_atom the preceding atom to this 'free' atom
                    if (iter == udta_dynamics.last_udta_child_atom) {
                        udta_dynamics.last_udta_child_atom = APar_FindPrecedingAtom(iter);
                    }
                    APar_EliminateAtom(iter, parsedAtoms[iter].NextAtomNumber);
                }
            }

            //any other atom in or after 'udta'
        }
        else {
            if (transited_udta_metadata) {
                udta_dynamics.first_postfree_level1_atom = parsedAtoms[iter].AtomicNumber;
                break;
            }
        }
        if (iter == udta_dynamics.last_udta_child_atom || iter == 0) { //this makes sure that anything in udta is processed...
            transited_udta_metadata = true;
            if (iter == 0) {
                break;
            }
        }
        iter = next_atom;
        if (iter == udta_dynamics.free_atom_repository) {
            transited_udta_metadata = true;
        }
    }

    if (initial_pass) {
        int userdata_difference = udtaAtom->AtomicLength - udta_dynamics.original_udta_size; //if metadata became shorter or removed, free space would be created
        //+8 so that 'free' can be accommodated; can't write a 'free' atom of length = 5 - min is 8; OR it disappears entirely
        if (((int) udta_dynamics.max_usable_free_space >= userdata_difference + 8) || (userdata_difference <= -8)
                || (((int) udta_dynamics.max_usable_free_space >= 8) && (-8 < userdata_difference)
                        && (userdata_difference < 0))
                || (int) udta_dynamics.max_usable_free_space == userdata_difference) {

            if (!moov_atom_was_mooved) { //only allow dynamic updating when moov precedes any mdat atoms...
                udta_dynamics.dynamic_updating = true;
            }
            else {
                //if there is insufficient padding when moov is rearranged to precede mdat, add default padding
                if ((pad_prefs.minimum_required_padding_size < udta_dynamics.max_usable_free_space)
                        && (udta_dynamics.max_usable_free_space < pad_prefs.default_padding_size)) {
                    APar_ForcePadding(pad_prefs.default_padding_size);
                }
                APar_DetermineAtomLengths();
                return;
            }
            if ((pad_prefs.minimum_required_padding_size < udta_dynamics.max_usable_free_space)
                    && (udta_dynamics.max_usable_free_space < pad_prefs.default_padding_size)) {
                APar_ForcePadding(pad_prefs.default_padding_size);
            }
            if (pad_prefs.minimum_required_padding_size > udta_dynamics.max_usable_free_space) {
                APar_ConsilidatePadding(pad_prefs.minimum_required_padding_size);
                udta_dynamics.dynamic_updating = false;
            }
            //if 'free' padding space is greater than what we allow, reduce it here (reduce anything over max to default_padding_size)
            if (udta_dynamics.max_usable_free_space > pad_prefs.maximum_present_padding_size) {
                udta_dynamics.dynamic_updating = false;
                APar_ConsilidatePadding(pad_prefs.default_padding_size);
            }
            //say 'covr' is erased yielding 140,000 bytes of 'free' - take care of it here by forcing a rewrite (erasing an atom != conversion to 'free')
            if (abs(userdata_difference) > pad_prefs.maximum_present_padding_size) {
                udta_dynamics.dynamic_updating = false;
                APar_ConsilidatePadding(pad_prefs.default_padding_size);
            }
        }
        else {
            //if the file has no functional padding, add a default amount of padding when the file needs to be completely rewritten
            if (udta_dynamics.max_usable_free_space <= pad_prefs.minimum_required_padding_size) {
                if (psp_brand) {
                    udta_dynamics.dynamic_updating = true;
                }
                else if (pad_prefs.default_padding_size >= 8) {
                    APar_InterjectNewAtom("free", CHILD_ATOM, SIMPLE_ATOM, pad_prefs.default_padding_size, 0, 0, 1, APar_FindLastChild_of_ParentAtom(udta_dynamics.moov_atom));
                    new_file_size += pad_prefs.default_padding_size; //used in shell progress bar; easier to just outright add it than go through the whole tree
                }
            }
            else if (!udta_dynamics.dynamic_updating
                    && udta_dynamics.max_usable_free_space < pad_prefs.default_padding_size) {
                APar_ConsilidatePadding(pad_prefs.default_padding_size);
            }
        }
    }
    else {
        //if (pad_prefs.minimum_required_padding_size < udta_dynamics.max_usable_free_space < pad_prefs.default_padding_size) {
        //	APar_ForcePadding(pad_prefs.default_padding_size);
        //}
    }
    if (!tree_display_only) { //APar_DetermineAtomLengths doesn't handle the atoms under 'stsd' any more; for atom setting/removal, 'stsd' parsing is skipped
        APar_DetermineAtomLengths();
    }
    return;
}

/*----------------------
 APar_Optimize

 mdat_test_only - the only info desired (if true, when printing the tree) is to know whether mdat precedes moov (and nullifing the concept of padding)

 Try to rearrange the atoms so that moov comes before mdat(s) & to reorder moov's children atoms so that 'udta' is the last child of moov. Top level atoms
 are sequentially fetched and names are tested. Special accommodation is made for top level 'free' atoms that follow 'moov' but before any other significant
 atoms - these 'free' atoms are tracked in padding_atoms and their atomic levels are temporarily shifted to make them a child of moov so that these 'free'
 atoms follow 'moov' at is is being shuffled around so as to preserve the functionality of padding. Prevent rearranging on fragmented files; the moov atom
 discovered first goes onto to have its children sequentially tested to ensure that 'udta' comes last in moov's hieararchy (excepting 'free' only).
 ----------------------*/
void APar_Optimize(bool mdat_test_only) {
    short total_file_level_atoms = APar_ReturnChildrenAtoms(0, 0);
    short eval_atom = 0;

    short first_mdat_atom = 0;
    short moov_atom = 0;
    short last_noteworthy_atom = 0;
    short padding_atoms[6] =
        { 0, 0, 0, 0, 0, 0 };

    //scan through all top level atoms; fragmented files won't be optimized
    for (uint8_t i = 1; i <= total_file_level_atoms; i++) {
        eval_atom = APar_ReturnChildrenAtoms(0, i);
        if (memcmp(parsedAtoms[eval_atom].AtomicName, "moof", 4) == 0
                || memcmp(parsedAtoms[eval_atom].AtomicName, "mfra", 4) == 0) {
            move_moov_atom = false; //moov reordering won't be occurring on fragmented files, but it should have moov first anyway (QuickTime does at least)
        }
    }

    for (uint8_t iii = 1; iii <= total_file_level_atoms; iii++) {
        eval_atom = APar_ReturnChildrenAtoms(0, iii);
        if (memcmp(parsedAtoms[eval_atom].AtomicName, "mdat", 4) == 0) {
            if (first_mdat_atom == 0) {
                first_mdat_atom = eval_atom;
            }
        }

        if (memcmp(parsedAtoms[eval_atom].AtomicName, "moov", 4) == 0) {
            moov_atom = eval_atom;
            udta_dynamics.moov_atom = eval_atom;
        }

        //keep track of any (where any = up to 5) 'free' atoms that come after moov but before anything else; TODO: also 'skip' atoms
        if (memcmp(parsedAtoms[eval_atom].AtomicName, "free", 4) != 0) {
            last_noteworthy_atom = eval_atom;
        }
        if (memcmp(parsedAtoms[last_noteworthy_atom].AtomicName, "moov", 4) == 0
                && memcmp(parsedAtoms[eval_atom].AtomicName, "free", 4) == 0) {
            if (padding_atoms[0] < 5) {
                padding_atoms[0]++;
                padding_atoms[padding_atoms[0]] = eval_atom;
            }
        }
    }

    //for each padding 'free' atom after moov (but before anything else significant), increase the AtomicLevel so that it becomes a child to moov (so it follows moov when it gets rearranged)
    if (move_moov_atom && first_mdat_atom > 0 && moov_atom > 0 && moov_atom > first_mdat_atom) {
        if (mdat_test_only) {
            moov_atom_was_mooved = true; //this is all the interesting info required (during APar_PrintAtomicTree)
            return;
        }
        else {
            for (uint8_t jj = 1; jj <= padding_atoms[0]; jj++) {
                parsedAtoms[padding_atoms[jj]].AtomicLevel++;
            }
            APar_MoveAtom(moov_atom, first_mdat_atom); //move 'moov' in front of 'mdat'
            moov_atom_was_mooved = true;
            //reinstate the padding atoms as level 1 'free' atoms now that moov is rearranged.
            for (uint8_t kk = 1; kk <= padding_atoms[0]; kk++) {
                parsedAtoms[padding_atoms[kk]].AtomicLevel--;
            }
        }
    }

    /* -----------moving udta to last child of moov--------- */

    if (moov_atom > 0) {
        short udta_atom = 0;
        short any_child_atom = 0;
        short total_moov_child_atoms = APar_ReturnChildrenAtoms(moov_atom, 0);

        for (uint8_t moov_i = 1; moov_i <= total_moov_child_atoms; moov_i++) {
            eval_atom = APar_ReturnChildrenAtoms(moov_atom, moov_i);
            if (memcmp(parsedAtoms[eval_atom].AtomicName, "udta", 4) == 0 && parsedAtoms[eval_atom].AtomicLevel == 2) {
                udta_atom = eval_atom;
            }
            else {
                if (memcmp(parsedAtoms[eval_atom].AtomicName, "free", 4) != 0) { //don't reorder just to move udta behind free
                    any_child_atom = eval_atom;
                }
            }
        }

        if (any_child_atom > udta_atom && !tree_display_only && udta_atom > 0) { //memcmp(parsedAtoms[udta_atom].AtomicName, "udta", 4) == 0 works too
            APar_MoveAtom(udta_atom, any_child_atom);
        }
    }
    initial_optimize_pass = false;
    return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          Determine Atom Length                                    //
///////////////////////////////////////////////////////////////////////////////////////

void APar_DetermineNewFileLength() {
    new_file_size = 0;
    short thisAtomNumber = 0;
    while (true) {
        if (parsedAtoms[thisAtomNumber].AtomicLevel == 1) {
            if (parsedAtoms[thisAtomNumber].AtomicLengthExtended == 0) {
                //normal 32-bit number when AtomicLengthExtended == 0 (for run-o-the-mill mdat & mdat.length=0)
                new_file_size += parsedAtoms[thisAtomNumber].AtomicLength; //used in progressbar
            }
            else {
                //pseudo 64-bit mdat length
                new_file_size += parsedAtoms[thisAtomNumber].AtomicLengthExtended; //used in progressbar
            }
            if (parsedAtoms[thisAtomNumber].AtomicLength == 0) {
                new_file_size += (uint32_t) file_size - parsedAtoms[thisAtomNumber].AtomicStart; //used in progressbar; mdat.length = 1
            }
        }
        if (parsedAtoms[thisAtomNumber].NextAtomNumber == 0) {
            break;
        }
        thisAtomNumber = parsedAtoms[thisAtomNumber].NextAtomNumber;
    }
    return;
}

void APar_DetermineAtomLengths() {

    if (!udta_dynamics.dynamic_updating && !psp_brand && initial_optimize_pass) {
        APar_Optimize(false);
    }
    else {
        //'moov' & the last child of 'udta' are discovered in APar_Optimize and must be known to do dynamic updating (for PSP files)
        udta_dynamics.moov_atom = APar_FindAtom("moov", false, SIMPLE_ATOM, 0)->AtomicNumber;
        AtomicInfo* udta_atom = APar_FindAtom("moov.udta", false, SIMPLE_ATOM, 0);
        if (udta_atom != NULL) { //its possible it was erased...
            udta_dynamics.last_udta_child_atom = APar_FindLastChild_of_ParentAtom(udta_atom->AtomicNumber);
        }
    }

    short rev_atom_loop = APar_FindLastAtom();

    //To determine the lengths of the atoms, and of each parent for EVERY atom in the hierarchy (even atoms we haven't touched), we start at the end
    //
    //Progressing backward, we evaluate & look at each atom; if it is at the end of its hierarchy then its length is what it already states
    //if the atom after our eval atom is a child, sum the lengths of atoms who are 1 level below to our length; also parent atoms have a lengh of 8
    //which are taken care of in the case statement; iterate backwards through tree taking note of odball atoms as they occur.

    while (true) {
        short next_atom = 0;
        uint32_t atom_size = 0;

        if (rev_atom_loop == 0) {
            break; //we seem to have hit the first atom
        }

        uint32_t _atom_ = UInt32FromBigEndian(parsedAtoms[rev_atom_loop].AtomicName);
        switch (_atom_) {
        case 0x6D657461: //'meta'
            atom_size += 12; //meta has 4 bytes length, 4 bytes name & 4 bytes NULL space (...it could be versioned atom...)
            break;

        case 0x73747364: //'stsd'
            atom_size += 16;
            break;

        default:
            atom_size += 8; //all atoms have *at least* 4bytes length & 4 bytes name
            break;
        }

        if (parsedAtoms[rev_atom_loop].NextAtomNumber != 0) {
            next_atom = parsedAtoms[rev_atom_loop].NextAtomNumber;
        }

        while (parsedAtoms[next_atom].AtomicLevel > parsedAtoms[rev_atom_loop].AtomicLevel) { // eval all child atoms....
            if (parsedAtoms[rev_atom_loop].AtomicLevel == (parsedAtoms[next_atom].AtomicLevel - 1)) { // only child atoms 1 level down
                atom_size += parsedAtoms[next_atom].AtomicLength;
            }
            else if (parsedAtoms[next_atom].AtomicLevel < parsedAtoms[rev_atom_loop].AtomicLevel) {
                break;
            }
            next_atom = parsedAtoms[next_atom].NextAtomNumber; //increment to eval next atom
            parsedAtoms[rev_atom_loop].AtomicLength = atom_size;
        }

        if (_atom_ == 0x75647461
                && parsedAtoms[rev_atom_loop].AtomicLevel
                        > parsedAtoms[parsedAtoms[rev_atom_loop].NextAtomNumber].AtomicLevel) { //udta with no child atoms; get here by erasing the last asset in a 3gp file, and it won't quite erase because udta thinks its the former AtomicLength
            parsedAtoms[rev_atom_loop].AtomicLength = 8;
        }
        if (_atom_ == 0x6D657461
                && parsedAtoms[rev_atom_loop].AtomicLevel
                        != parsedAtoms[parsedAtoms[rev_atom_loop].NextAtomNumber].AtomicLevel - 1) { //meta with no child atoms; get here by erasing the last existing uuid atom.
            parsedAtoms[rev_atom_loop].AtomicLength = 12;
        }
        if (_atom_ == 0x696C7374
                && parsedAtoms[rev_atom_loop].AtomicLevel
                        != parsedAtoms[parsedAtoms[rev_atom_loop].NextAtomNumber].AtomicLevel - 1) { //ilst with no child atoms; get here by erasing the last piece of iTunes style metadata
            parsedAtoms[rev_atom_loop].AtomicLength = 8;
        }

        rev_atom_loop = APar_FindPrecedingAtom(parsedAtoms[rev_atom_loop].AtomicNumber);

    }
    APar_DetermineNewFileLength();
    //APar_SimpleAtomPrintout();
    //APar_PrintAtomicTree();
    return;
}

///////////////////////////////////////////////////////////////////////////////////////
//                          Atom Writing Functions                                   //
///////////////////////////////////////////////////////////////////////////////////////

/*----------------------
 APar_ValidateAtoms

 A gaggle of tests go on here - to TRY to make sure that files are not corrupted.

 1. because there is a limit to the number of atoms, test to make sure we haven't hit MAX_ATOMS (probably only likely on a 300MB fragmented file ever 2 secs)
 2. test that the atom name is at least 4 letters long. So far, only quicktime atoms have NULLs in their names.
 3. For files over 300k, make sure that no atom can present larger than the filesize (which would be bad); handy for when the file isn't parsed correctly
 4. Test to make sure 'mdat' is at file-level. That is the only place it should ever be.
 5. (A crude) Test to see if 'trak' atoms have a child 'tkhd' atom. If setting a copyright notice on a track at index built with some compilers faux 'trak's are made
 6. If the file shunk below 90% (after accounting for additions or removals), error out - something went awry.
 ----------------------*/
void APar_ValidateAtoms() {
    bool atom_name_with_4_characters = true;
    short iter = 0;
    uint64_t simple_tally = 0;

    if (atom_number > MAX_ATOMS) {
        fprintf(stderr, "AtomicParsley error: amount of atoms exceeds internal limit. Aborting.\n");
        return;
    }

    while (true) {
        // there are valid atom names that are 0x00000001 - but I haven't seen them in MPEG-4 files, but they could show up, so this isn't a hard error
        if (strlen(parsedAtoms[iter].AtomicName) < 4) {
            atom_name_with_4_characters = false;
        }

        //test for atoms that are going to be greater than out current file size; problem is we could be adding a 1MB pix to a 200k 3gp file; only fail for a file > 300k file; otherwise there would have to be more checks (like artwork present, but a zealous tagger could make moov.lengt > filzesize)
        if (parsedAtoms[iter].AtomicLength > (uint32_t) file_size && file_size > 300000) {
            if (parsedAtoms[iter].AtomicData == NULL) {
                fprintf(stderr, "AtomicParsley error: an atom was detected that presents as larger than filesize. Aborting. %c\n", '\a');
#if defined (_MSC_VER) /* apparently, long long is forbidden there*/
                fprintf(stderr, "atom %s is %u bytes long which is greater than the filesize of %llu\n", parsedAtoms[iter].AtomicName, parsedAtoms[iter].AtomicLength, (long unsigned int)file_size);
#else
                fprintf(stderr, "atom %s is %u bytes long which is greater than the filesize of %llu\n", parsedAtoms[iter].AtomicName, parsedAtoms[iter].AtomicLength, (long long unsigned int) file_size);
#endif
                return; //its conceivable to repair such an off length by the surrounding atoms constrained by file_size - just not anytime soon; probly would catch a foobar2000 0.9 tagged file
            }
        }

        if (parsedAtoms[iter].AtomicLevel == 1) {
            if (parsedAtoms[iter].AtomicLength == 0 && strncmp(parsedAtoms[iter].AtomicName, "mdat", 4) == 0) {
                simple_tally = (uint64_t) file_size - parsedAtoms[iter].AtomicStart;
            }
            else {
                simple_tally +=
                        parsedAtoms[iter].AtomicLength == 1 ? parsedAtoms[iter].AtomicLengthExtended : parsedAtoms[iter].AtomicLength;
            }
        }

        if (strncmp(parsedAtoms[iter].AtomicName, "mdat", 4) == 0 && parsedAtoms[iter].AtomicLevel != 1) {
            fprintf(stderr, "AtomicParsley error: mdat atom was found at an illegal (not at top level). Aborting. %c\n", '\a');
            return; //the error which forced this was some bad atom length redetermination; probably won't be fixed
        }

        if (memcmp(parsedAtoms[iter].AtomicName, "trak", 4) == 0 && parsedAtoms[iter + 1].NextAtomNumber != 0) { //prevent writing any malformed tracks
            if (memcmp(parsedAtoms[parsedAtoms[iter].NextAtomNumber].AtomicName, "tkhd", 4) != 0) {
                fprintf(stderr, "AtomicParsley error: incorrect track structure. %c\n", '\a');
                return;
            }
        }

        iter = parsedAtoms[iter].NextAtomNumber;
        if (iter == 0) {
            break;
        }
    }

    double perdiff = (float) ((float) ((uint32_t) simple_tally) * 100.0 / (double) (file_size - removed_bytes_tally));
    int percentage_difference = (int) lroundf((float) perdiff);

    if (percentage_difference < 90 && file_size > 300000) { //only kick in when files are over 300k & 90% of the size
        fprintf(stderr, "AtomicParsley error: total existing atoms present as larger than filesize. Aborting. %c\n", '\a');
        //APar_PrintAtomicTree();
        fprintf(stdout, "%i %llu\n", percentage_difference, simple_tally);
        return;
    }

    if (!atom_name_with_4_characters) {
        fprintf(stdout, "AtomicParsley warning: atom(s) were detected with atypical names containing NULLs\n");
    }
    return;
}

#if defined (DARWIN_PLATFORM)

//this bit of code is central to eliminating the need to set file extension for mpeg4 files (at least for the currently know codec types).
//we will count how many 'trak' atoms there are; for each one drill down to 'stsd' and return the the atom right after that
//the atom after 'stsd' will carry the 4cc code of the codec for the track - for example jpeg, alac, avc1 or mp4a
//if he have 'avc1', 'drmi', 'mp4v' for any of the returned atoms, then we have a VIDEO mpeg-4 file

//then using Cocoa calls, the Mac OS X specific TYPE/CREATOR code will be set to indicate to the OS/Finder/Applications/iTunes that
//this file is a audio/video file without having to change its extension.

void APar_TestTracksForKind() {
    uint8_t total_tracks = 0;
    uint8_t track_num = 0;
    short codec_atom = 0;

    //With track_num set to 0, it will return the total trak atom into total_tracks here.
    APar_TrackInfo(total_tracks, track_num, codec_atom);

    if (total_tracks > 0) {
        while (total_tracks > track_num) {
            track_num+= 1;

            //Now total_tracks != 0; this ships off which trak atom to test, and returns codec_atom set to the atom after stsd.
            APar_TrackInfo(total_tracks, track_num, codec_atom);

            //now test this trak's stsd codec against these 4cc codes:
            switch(parsedAtoms[codec_atom].stsd_codec) {
                //video types
                case 0x61766331 :// "avc1"
                track_codecs.has_avc1 = true;
                break;
                case 0x6D703476 :// "mp4v"
                track_codecs.has_mp4v = true;
                break;
                case 0x64726D69 :// "drmi"
                track_codecs.has_drmi = true;
                break;

                //audio types
                case 0x616C6163 :// "alac"
                track_codecs.has_alac = true;
                break;
                case 0x6D703461 :// "mp4a"
                track_codecs.has_mp4a = true;
                break;
                case 0x64726D73 :// "drms"
                track_codecs.has_drms = true;
                break;

                //podcast types
                case 0x74657874 :// "text"
                track_codecs.has_timed_text = true;
                break;
                case 0x6A706567 :// "jpeg"
                track_codecs.has_timed_jpeg = true;
                break;

                //either podcast type (audio-only) or a QT-unsupported video file with subtitles
                case 0x74783367 :// "tx3g"
                track_codecs.has_timed_tx3g = true;
                break;

                //other
                case 0x6D703473 :// "mp4s"
                track_codecs.has_mp4s = true;
                break;
                case 0x72747020 :// "rtp "
                track_codecs.has_rtp_hint = true;
                break;
            }
        }
    }
    return;
}
#endif

void APar_DeriveNewPath(const char *filePath, char* temp_path, int output_type, const char* file_kind, const char* forced_suffix, bool random_filename =
        true) {
    const char* suffix = NULL;
    if (forced_suffix == NULL) {
        suffix = strrchr(filePath, '.');
    }
    else {
        suffix = forced_suffix;
    }

    size_t filepath_len = strlen(filePath);
    size_t base_len = filepath_len - strlen(suffix);
    if (output_type >= 0) {
        memcpy(temp_path, filePath, base_len);
        memcpy(temp_path + base_len, file_kind, strlen(file_kind));

    }
    else if (output_type == -1) { //make the output file invisible by prefacing the filename with '.'
#if defined (_MSC_VER)
            memcpy(temp_path, filePath, base_len);
            memcpy(temp_path + base_len, file_kind, strlen(file_kind));
#else
        const char* file_name = strrchr(filePath, '/');
        size_t file_name_len = strlen(file_name);
        memcpy(temp_path, filePath, filepath_len - file_name_len + 1);
        memcpy(temp_path + strlen(temp_path), ".", 1);
        memcpy(temp_path + strlen(temp_path), file_name + 1, file_name_len - strlen(suffix) - 1);
        memcpy(temp_path + strlen(temp_path), file_kind, strlen(file_kind));
#endif
    }

    if (random_filename) {
        char randstring[6];
        srand((int) time(NULL)); //Seeds rand()
        int randNum = rand() % 100000;
        sprintf(randstring, "%i", randNum);
        memcpy(temp_path + strlen(temp_path), randstring, strlen(randstring));
    }

    if (forced_suffix_type == FORCE_M4B_TYPE) {
        memcpy(temp_path + strlen(temp_path), ".m4b", 5);
    }
    else {
        memcpy(temp_path + strlen(temp_path), suffix, strlen(suffix));
    }
    return;
}

void APar_MetadataFileDump(const char* m4aFile) {
    char* dump_file_name = (char*) malloc(sizeof(char) * (strlen(m4aFile) + 12 + 1));
    memset(dump_file_name, 0, sizeof(char) * (strlen(m4aFile) + 12 + 1));

    FILE* dump_file;
    AtomicInfo* userdata_atom = APar_FindAtom("moov.udta", false, SIMPLE_ATOM, 0);

    //make sure that the atom really exists
    if (userdata_atom != NULL) {
        char* dump_buffer = (char*) malloc(sizeof(char) * userdata_atom->AtomicLength + 1);
        memset(dump_buffer, 0, sizeof(char) * userdata_atom->AtomicLength + 1);

        APar_DeriveNewPath(m4aFile, dump_file_name, 1, "-dump-", ".raw");
        dump_file = APar_OpenFile(dump_file_name, "wb");
        if (dump_file != NULL) {
            //body of atom writing here

            fseeko(source_file, userdata_atom->AtomicStart, SEEK_SET);
            fread(dump_buffer, 1, (size_t) userdata_atom->AtomicLength, source_file);

            fwrite(dump_buffer, (size_t) userdata_atom->AtomicLength, 1, dump_file);
            fclose(dump_file);

            fprintf(stdout, " Metadata dumped to %s\n", dump_file_name);
        }
        free(dump_buffer);
        dump_buffer = NULL;

    }
    else {
        fprintf(stdout, "AtomicParsley error: no moov.udta atom was found to dump out to file.\n");
    }

    return;
}

void APar_ShellProgressBar(uint32_t bytes_written) {
    if (udta_dynamics.dynamic_updating) {
        return;
    }
    strcpy(file_progress_buffer, " Progress: ");

    double dispprog = (double) bytes_written / (double) new_file_size * 100.0 * ((double) max_display_width / 100.0);
    int display_progress = (int) lroundf((float) dispprog);
    double percomp = 100.0 * (double) ((double) bytes_written / (double) new_file_size);
    int percentage_complete = (int) lroundf((float) percomp);

    for (int i = 0; i <= max_display_width; i++) {
        if (i < display_progress) {
            strcat(file_progress_buffer, "=");
        }
        else if (i == display_progress) {
            sprintf(file_progress_buffer, "%s>%d%%", file_progress_buffer, percentage_complete);
        }
        else {
            strcat(file_progress_buffer, "-");
        }
    }
    if (percentage_complete < 100) {
        strcat(file_progress_buffer, "-");
        if (percentage_complete < 10) {
            strcat(file_progress_buffer, "-");
        }
    }

    strcat(file_progress_buffer, "|");

    fprintf(stdout, "%s\r", file_progress_buffer);
    fflush(stdout);
    return;
}

void APar_MergeTempFile(FILE* dest_file, FILE *src_file, uint32_t src_file_size, uint32_t dest_position, char* &buffer) {
    uint32_t file_pos = 0;
    while (file_pos <= src_file_size) {
        if (file_pos + max_buffer <= src_file_size) {
            fseeko(src_file, file_pos, SEEK_SET);
            fread(buffer, 1, (size_t) max_buffer, src_file);

            //fseek(dest_file, dest_position + file_pos, SEEK_SET);
#if defined(_MSC_VER)
            fpos_t file_offset = dest_position + file_pos;
#elif defined(__GLIBC__)
            fpos_t file_offset =
                { 0 };
            file_offset.__pos = dest_position + file_pos;
#else
            off_t file_offset = dest_position + file_pos;
#endif
            fsetpos(dest_file, &file_offset);
            fwrite(buffer, (size_t) max_buffer, 1, dest_file);
            file_pos += max_buffer;

        }
        else {
            fseeko(src_file, file_pos, SEEK_SET);
            fread(buffer, 1, (size_t) (src_file_size - file_pos), src_file);
#if defined(_MSC_VER)
            fpos_t file_offset = dest_position + file_pos;
#elif defined(__GLIBC__)
            fpos_t file_offset =
                { 0 };
            file_offset.__pos = dest_position + file_pos;
#else
            off_t file_offset = dest_position + file_pos;
#endif
            fsetpos(dest_file, &file_offset);
            fwrite(buffer, (size_t) (src_file_size - file_pos), 1, dest_file);
            file_pos += src_file_size - file_pos;
            break;
        }
    }
    return;
}

uint32_t APar_WriteAtomically(FILE* source_file, FILE* temp_file, bool from_file, char* &buffer, char* &conv_buffer, uint32_t bytes_written_tally, short this_atom) {
    uint32_t bytes_written = 0;

    if (parsedAtoms[this_atom].AtomicLength > 1 && parsedAtoms[this_atom].AtomicLength < 8) { //prevents any spurious atoms from appearing
        return bytes_written;
    }
    if (memcmp(parsedAtoms[this_atom].AtomicName, "free", 4) == 0
            && (udta_dynamics.dynamic_updating || parsedAtoms[this_atom].AtomicLength == 0)) {
        return bytes_written;
    }

    //write the length of the atom first... taken from our tree in memory
    char4TOuint32(parsedAtoms[this_atom].AtomicLength, conv_buffer);
    fseeko(temp_file, bytes_written_tally, SEEK_SET);
    fwrite(conv_buffer, 4, 1, temp_file);
    bytes_written += 4;

    //since we have already writen the length out to the file, it can be changed now with impunity
    if (parsedAtoms[this_atom].AtomicLength == 0) { //the spec says if an atom has a length of 0, it extends to EOF
        parsedAtoms[this_atom].AtomicLength = (uint32_t) file_size - parsedAtoms[this_atom].AtomicLength;
    }
    else if (parsedAtoms[this_atom].AtomicLength == 1) {
        //part of the pseudo 64-bit support
        parsedAtoms[this_atom].AtomicLength = (uint32_t) parsedAtoms[this_atom].AtomicLengthExtended;
    }

    if (from_file) {
        // here we read in the original atom into the buffer. If the length is greater than our buffer length,
        // we loop, reading in chunks of the atom's data into the buffer, and immediately write it out, reusing the buffer.

        while (bytes_written <= parsedAtoms[this_atom].AtomicLength) {
            if (bytes_written + max_buffer <= parsedAtoms[this_atom].AtomicLength) {
                //read&write occurs from & including atom name through end of atom
                fseeko(source_file, (bytes_written + parsedAtoms[this_atom].AtomicStart), SEEK_SET);
                fread(buffer, 1, (size_t) max_buffer, source_file);

                fseeko(temp_file, (bytes_written_tally + bytes_written), SEEK_SET);
                fwrite(buffer, (size_t) max_buffer, 1, temp_file);
                bytes_written += max_buffer;

                APar_ShellProgressBar(bytes_written_tally + bytes_written);

            }
            else { //we either came up on a short atom (most are), or the last bit of a really long atom
                fseeko(source_file, (bytes_written + parsedAtoms[this_atom].AtomicStart), SEEK_SET);
                fread(buffer, 1, (size_t) (parsedAtoms[this_atom].AtomicLength - bytes_written), source_file);

                fseeko(temp_file, (bytes_written_tally + bytes_written), SEEK_SET);
                fwrite(buffer, (size_t) (parsedAtoms[this_atom].AtomicLength - bytes_written), 1, temp_file);
                bytes_written += parsedAtoms[this_atom].AtomicLength - bytes_written;

                APar_ShellProgressBar(bytes_written_tally + bytes_written);

                break;
            }
        }

    }
    else { // we are going to be writing not from the file, but directly from the tree (in memory).
        uint32_t atom_name_len = 4;

        fseeko(temp_file, (bytes_written_tally + bytes_written), SEEK_SET);

        if (parsedAtoms[this_atom].AtomicClassification == EXTENDED_ATOM) {
            fwrite("uuid", 4, 1, temp_file);
            atom_name_len = 16; //total of 20 bytes for a uuid atom
        }

        fwrite(parsedAtoms[this_atom].AtomicName, atom_name_len, 1, temp_file);
        bytes_written += atom_name_len;
        if (parsedAtoms[this_atom].AtomicClassification == VERSIONED_ATOM
                || parsedAtoms[this_atom].AtomicClassification == PACKED_LANG_ATOM) {
            char4TOuint32((uint32_t) parsedAtoms[this_atom].AtomicVerFlags, conv_buffer);
            fwrite(conv_buffer, 4, 1, temp_file);
            bytes_written += 4;
        }

        uint32_t atom_data_size = 0;
        switch (parsedAtoms[this_atom].AtomicContainerState) {
        case PARENT_ATOM:
        case SIMPLE_PARENT_ATOM: {
            atom_data_size = 0;
            break;
        }
        case DUAL_STATE_ATOM: {
            switch (UInt32FromBigEndian(parsedAtoms[this_atom].AtomicName)) {
            case 0x6D657461: { //meta
                break;
            }
            case 0x73747364: { //stsd
                atom_data_size = parsedAtoms[this_atom].AtomicLength - 12;
                break;
            }
                //no need to accommodate 'schi' here because it only exists inside the monolithic (unparsed) 'stsd'; same for all the codecs under 'stsd'
            }
            break;
        }
        case UNKNOWN_ATOM_TYPE:
        case CHILD_ATOM: {
            if (parsedAtoms[this_atom].AtomicClassification == EXTENDED_ATOM) {
                //4bytes length, 4 bytes 'uuid', 4bytes name, 4bytes NULL (AP writes its own uuid atoms - not those copied - iTunes style with atom versioning)
                atom_data_size = parsedAtoms[this_atom].AtomicLength - (16 + 12); //16 uuid; 16 = 4bytes * ('uuid', ap_uuid_name, verflag, 4 NULL bytes)
            }
            else if (parsedAtoms[this_atom].AtomicClassification == VERSIONED_ATOM
                    || parsedAtoms[this_atom].AtomicClassification == PACKED_LANG_ATOM) {
                //4bytes legnth, 4bytes name, 4bytes flag&versioning (language would be 2 bytes, but because its in different places, it gets stored as data)
                atom_data_size = parsedAtoms[this_atom].AtomicLength - 12;
            }
            else {
                //just 4bytes length, 4bytes name and then whatever data
                atom_data_size = parsedAtoms[this_atom].AtomicLength - 8;
            }
            break;
        }
        }

        if (parsedAtoms[this_atom].AtomicClassification == EXTENDED_ATOM) {
            //AP writes uuid atoms much like iTunes style metadata; with version/flags to connote what type of data is being carried
            //4bytes atom length, 4 bytes 'uuid', 16bytes uuidv5, 4bytes name of uuid in AP namespace, 4bytes versioning, 4bytes NULL, Xbytes data
            fwrite(parsedAtoms[this_atom].uuid_ap_atomname, 4, 1, temp_file);
            bytes_written += 4;

            char4TOuint32((uint32_t) parsedAtoms[this_atom].AtomicVerFlags, conv_buffer);
            fwrite(conv_buffer, 4, 1, temp_file);
            bytes_written += 4;
        }

        if (atom_data_size > 0) {
            fwrite(parsedAtoms[this_atom].AtomicData, atom_data_size, 1, temp_file);
            bytes_written += atom_data_size;

            APar_ShellProgressBar(bytes_written_tally + bytes_written);
        }
    }
    return bytes_written;
}

/*----------------------
 APar_copy_gapless_padding
 mp4file - destination file
 last_atom_pos - the last byte in the destination file that is contained by any atom (in parsedAtoms[] array)
 buffer - a buffer that will be used to set & write out from the NULLs used in gapless padding

 Add the discovered amount of already present gapless void padding at the end of the file (which is *not* contained by any atom at all) back into the destination
 file.

 Update: it would seem that this gapless void padding at the end of the file is not critical to gapless playback. In my 1 test of the thing, it seemed to work
 regardless of whether this NULL space was present or not, 'pgap' seemed to work. But, since Apple put it in for some reason, it will be left there unless explicity
 directed not to (via AP_PADDING). Although tying ordinary padding to this gapless padding may reduce flexibility - the assumption is that someone interested in
 squeezing out wasted space would want to eliminate this wasted space too (and so far, it does seem wasted).
 ----------------------*/
void APar_copy_gapless_padding(FILE* mp4file, uint32_t last_atom_pos, char* buffer) {
    uint32_t gapless_padding_bytes_written = 0;
    while (gapless_padding_bytes_written < gapless_void_padding) {
        if (gapless_padding_bytes_written + max_buffer <= gapless_void_padding) {
            memset(buffer, 0, max_buffer);

            fseeko(mp4file, (last_atom_pos + gapless_padding_bytes_written), SEEK_SET);
            fwrite(buffer, (size_t) max_buffer, 1, mp4file);
            gapless_padding_bytes_written += max_buffer;

        }
        else { //less then 512k of gapless padding (here's hoping we get here always)
            memset(buffer, 0, (gapless_void_padding - gapless_padding_bytes_written));

            fseeko(mp4file, (last_atom_pos + gapless_padding_bytes_written), SEEK_SET);
            fwrite(buffer, (size_t) (gapless_void_padding - gapless_padding_bytes_written), 1, mp4file);
            gapless_padding_bytes_written += (gapless_void_padding - gapless_padding_bytes_written);
            break;
        }
    }
    return;
}

void APar_WriteFile(const char* m4aFile, const char* outfile, bool rewrite_original) {
    char* temp_file_name = (char*) malloc(sizeof(char) * 3500);
    char* file_buffer = (char*) malloc(sizeof(char) * max_buffer + 1);
    char* data = (char*) malloc(sizeof(char) * 4 + 1);
    FILE* temp_file;
    uint32_t temp_file_bytes_written = 0;
    short thisAtomNumber = 0;
    char* originating_file = NULL;
    bool free_modified_name = false;
    memset(temp_file_name, 0, sizeof(char) * 3500);
    memset(file_buffer, 0, sizeof(char) * max_buffer + 1);
    memset(data, 0, sizeof(char) * 4 + 1);

    if (!complete_free_space_erasure) {
        APar_DetermineDynamicUpdate(true); //test whether it can happen
    }
    else {
        APar_ForcePadding_sans_udta(); //force padding without the logistics of summing 'free' space & that overhead since all 'free' atoms were erased anyway
    }

    if (!rewrite_original || prevent_update_using_padding) {
        udta_dynamics.dynamic_updating = false;
    }

    APar_ValidateAtoms();

    //whatever atoms/space comes before mdat has to be added/removed before this point, or chunk offsets (in stco, co64, tfhd) won't be properly determined
    uint32_t mdat_position = APar_DetermineMediaData_AtomPosition();

    if (udta_dynamics.dynamic_updating) {
        APar_DeriveNewPath(m4aFile, temp_file_name, -1, "-data-", NULL);
        temp_file = APar_OpenFile(temp_file_name, "wb");
#if defined (_MSC_VER)
        char* invisi_command=(char*)malloc(sizeof(char)*2*MAXPATHLEN);
        memset( invisi_command, 0, 2*MAXPATHLEN+1);
        memcpy( invisi_command, "ATTRIB +S +H ", 13);
        memcpy( invisi_command + strlen(invisi_command), temp_file_name, strlen(temp_file_name) );

        if ( IsUnicodeWinOS() && UnicodeOutputStatus == WIN32_UTF16) {
            wchar_t* invisi_command_long = Convert_multibyteUTF8_to_wchar(invisi_command);

            _wsystem(invisi_command_long);

            free(invisi_command_long);
            invisi_command_long = NULL;
        }
        else {
            system(invisi_command);
        }
        free(invisi_command);
        invisi_command = NULL;
#endif
        APar_DetermineDynamicUpdate(false); //consolidate 'free' atom space; redetermine atom sizes again

    }
    else if (!outfile) {
        APar_DeriveNewPath(m4aFile, temp_file_name, 0, "-temp-", NULL);
        temp_file = APar_OpenFile(temp_file_name, "wb");

#if defined (DARWIN_PLATFORM)
        APar_SupplySelectiveTypeCreatorCodes(m4aFile, temp_file_name, forced_suffix_type); //provide type/creator codes for ".mp4" for randomly named temp files
#endif

    }
    else {
        //case-sensitive compare means "The.m4a" is different from "THe.m4a"; on certiain Mac OS X filesystems a case-preservative but case-insensitive FS exists &
        //AP probably will have a problem there. Output to a uniquely named file as I'm not going to poll the OS for the type of FS employed on the target drive.
        if (strncmp(m4aFile, outfile, strlen(outfile)) == 0 && (strlen(outfile) == strlen(m4aFile))) {
            //er, nice try but you were trying to ouput to the exactly named file of the original. Y'all ain't so slick
            APar_DeriveNewPath(m4aFile, temp_file_name, 0, "-temp-", NULL);
            temp_file = APar_OpenFile(temp_file_name, "wb");

#if defined (DARWIN_PLATFORM)
            APar_SupplySelectiveTypeCreatorCodes(m4aFile, temp_file_name, forced_suffix_type); //provide type/creator codes for ".mp4" for a fall-through randomly named temp files
#endif

        }
        else {
            temp_file = APar_OpenFile(outfile, "wb");

#if defined (DARWIN_PLATFORM)
            APar_SupplySelectiveTypeCreatorCodes(m4aFile, outfile, forced_suffix_type); //provide type/creator codes for ".mp4" for a user-defined output file
#endif

        }
    }

    if (temp_file != NULL) {
        //body of atom writing here

        if (udta_dynamics.dynamic_updating) { //dynamic updating will just start at udta and write out that hierarchy (minus any 'free' atoms); mdat doesn't move, so no stco/co64 changes. Possibly moov's length will change.
            thisAtomNumber = udta_dynamics.udta_atom;
            fprintf(stdout, "\n Updating metadata... ");
        }
        else {
            fprintf(stdout, "\n Started writing to temp file.\n");
        }

        while (true) {

            AtomicInfo* thisAtom = &parsedAtoms[thisAtomNumber];
            if (thisAtom->AtomicNumber == -1)
                break;

            //the loop where the critical determination is made
            if (memcmp(thisAtom->AtomicName, "mdat", 4) == 0 && udta_dynamics.dynamic_updating)
                break;
            //if (thisAtom->AtomicLevel == 1 && udta_dynamics.dynamic_updating) break; //TODO

            if (memcmp(thisAtom->AtomicName, "stco", 4) == 0) {
                bool readjusted_stco = APar_Readjust_STCO_atom(mdat_position, thisAtomNumber);

                temp_file_bytes_written +=
                        APar_WriteAtomically(source_file, temp_file, !readjusted_stco, file_buffer, data, temp_file_bytes_written, thisAtomNumber);

            }
            else if (memcmp(thisAtom->AtomicName, "co64", 4) == 0) {
                bool readjusted_co64 = APar_Readjust_CO64_atom(mdat_position, thisAtomNumber);

                temp_file_bytes_written +=
                        APar_WriteAtomically(source_file, temp_file, !readjusted_co64, file_buffer, data, temp_file_bytes_written, thisAtomNumber);

            }
            else if (memcmp(thisAtom->AtomicName, "tfhd", 4) == 0) {
                bool readjusted_tfhd = APar_Readjust_TFHD_fragment_atom(mdat_position, thisAtomNumber);

                temp_file_bytes_written +=
                        APar_WriteAtomically(source_file, temp_file, !readjusted_tfhd, file_buffer, data, temp_file_bytes_written, thisAtomNumber);

            }
            else if (thisAtom->AtomicData != NULL || memcmp(thisAtom->AtomicName, "meta", 4) == 0) {
                temp_file_bytes_written +=
                        APar_WriteAtomically(source_file, temp_file, false, file_buffer, data, temp_file_bytes_written, thisAtomNumber);

            }
            else {
                //write out parent atoms (the standard kind that are only offset & name from the tree in memory (total: 8bytes)
                if (thisAtom->AtomicContainerState <= SIMPLE_PARENT_ATOM) {
                    temp_file_bytes_written +=
                            APar_WriteAtomically(source_file, temp_file, false, file_buffer, data, temp_file_bytes_written, thisAtomNumber);
                    //or its a child (they invariably contain some sort of data.
                }
                else {
                    temp_file_bytes_written +=
                            APar_WriteAtomically(source_file, temp_file, true, file_buffer, data, temp_file_bytes_written, thisAtomNumber);
                }
            }
            if (thisAtom->NextAtomNumber == 0) { //if (parsedAtoms[thisAtomNumber].NextAtomNumber == 0) {
                break;
            }
            if (udta_dynamics.dynamic_updating && thisAtomNumber == udta_dynamics.last_udta_child_atom) {
                break; //exit writing out atoms if doing a dynamic update in situ - only the full 'udta' hierarchy will be written out
            }

            //prevent any looping back to atoms already written
            thisAtom->AtomicNumber = -1;
            thisAtomNumber = thisAtom->NextAtomNumber;
        }
        if (!udta_dynamics.dynamic_updating) {
            if (gapless_void_padding > 0 && pad_prefs.default_padding_size > 0) { //only when some sort of padding is wanted will the gapless null padding be copied
                APar_copy_gapless_padding(temp_file, temp_file_bytes_written, file_buffer);
            }
            fprintf(stdout, "\n Finished writing to temp file.\n");
            fclose(temp_file);
        }

    }
    else {
        fprintf(stdout, "AtomicParsley error: an error occurred while trying to create a temp file.\n");
        return;
    }

    if (udta_dynamics.dynamic_updating) {
        //copy file into buffer (cyclicly as needed), write into orig file (check for lock status); write a free_padding_size, 'free' & nulls for free_padding_size - 8
        fclose(temp_file);
        temp_file = APar_OpenFile(temp_file_name, "rb");
        fclose(source_file);
        source_file = APar_OpenFile(m4aFile, "r+b"); //source_file = APar_OpenFile(m4aFile, "r+");
        if (source_file == NULL) {
            fclose(temp_file);
            remove(temp_file_name);
            return;
        }

        //update moov's length
        char4TOuint32(parsedAtoms[udta_dynamics.moov_atom].AtomicLength, twenty_byte_buffer);
        fseeko(source_file, parsedAtoms[udta_dynamics.moov_atom].AtomicStart, SEEK_SET);
        fwrite(twenty_byte_buffer, 4, 1, source_file);

        //pesky (psp only) files that don't even have 'udta' in them, force appending after file length
        if (parsedAtoms[udta_dynamics.udta_atom].AtomicStart == 0 && psp_brand) {
            parsedAtoms[udta_dynamics.udta_atom].AtomicStart = file_size;
        }

        APar_MergeTempFile(source_file, temp_file, temp_file_bytes_written, parsedAtoms[udta_dynamics.udta_atom].AtomicStart, file_buffer);

        if (!psp_brand) {
            //psp files with no 'udta' atom will have whacked free_padding_size - which isn't required at EOF in any event
            fseeko(source_file, parsedAtoms[udta_dynamics.udta_atom].AtomicStart + temp_file_bytes_written, SEEK_SET);

            uint32_t free_padding_size = parsedAtoms[udta_dynamics.first_postfree_level1_atom].AtomicStart
                    - (parsedAtoms[udta_dynamics.udta_atom].AtomicStart + temp_file_bytes_written);

            if (free_padding_size >= 8) {
                char4TOuint32(free_padding_size, twenty_byte_buffer);
                fwrite(twenty_byte_buffer, 4, 1, source_file);
                fwrite("free", 4, 1, source_file);
                memset(file_buffer, 0, max_buffer);
                fwrite(file_buffer, 1, free_padding_size - 8, source_file);
            }
        }

        fclose(source_file);
        file_opened = false;

        fclose(temp_file);
        remove(temp_file_name);
        fprintf(stdout, "  completed.\n");

    }
    else if (rewrite_original && !outfile) { //disable overWrite when writing out to a specifically named file; presumably the enumerated output file was meant to be the final destination
        fclose(source_file);
        file_opened = false;

        if (IsUnicodeWinOS() && UnicodeOutputStatus == WIN32_UTF16) {
#if defined (_MSC_VER) /* native windows seems to require removing the file first; rename() on Mac OS X does the removing automatically as needed */
            wchar_t* utf16_filepath = Convert_multibyteUTF8_to_wchar(m4aFile);

            _wremove(utf16_filepath);

            free(utf16_filepath);
            utf16_filepath = NULL;
#endif
        }
        else {
            remove(m4aFile);
        }

        int err = 0;

        if (forced_suffix_type != NO_TYPE_FORCING) {
            originating_file = (char*) calloc(1, sizeof(char) * 3500);
            free_modified_name = true;
            if (forced_suffix_type == FORCE_M4B_TYPE) { //using --stik Audiobook with --overWrite will change the original file's extension
                uint16_t filename_len = strlen(m4aFile);
                const char* suffix = strrchr(m4aFile, '.');
                memcpy(originating_file, m4aFile, filename_len + 1);
                memcpy(originating_file + (filename_len - strlen(suffix)), ".m4b", 5);
            }
        }
        else {
            originating_file = (char*) m4aFile;
        }

        if (IsUnicodeWinOS() && UnicodeOutputStatus == WIN32_UTF16) {
#if defined (_MSC_VER)
            wchar_t* utf16_filepath = Convert_multibyteUTF8_to_wchar(originating_file);
            wchar_t* temp_utf16_filepath = Convert_multibyteUTF8_to_wchar(temp_file_name);

            err = _wrename(temp_utf16_filepath, utf16_filepath);

            free(utf16_filepath);
            free(temp_utf16_filepath);
            utf16_filepath = NULL;
            temp_utf16_filepath = NULL;
#endif
        }
        else {
            err = rename(temp_file_name, originating_file);
        }

        if (err != 0) {
            switch (errno) {

            case ENAMETOOLONG: {
                fprintf(stdout, "Some or all of the orginal path was too long.");
                return;
            }
            case ENOENT: {
                fprintf(stdout, "Some part of the original path was missing.");
                return;
            }
            case EACCES: {
                fprintf(stdout, "Unable to write to a directory lacking write permission.");
                return;
            }
            case ENOSPC: {
                fprintf(stdout, "Out of space.");
                return;
            }
            }
        }
    }

    free(temp_file_name);
    if (free_modified_name)
        free(originating_file);
    temp_file_name = NULL;
    free(file_buffer);
    file_buffer = NULL;
    free(data);
    data = NULL;

    return;
}
