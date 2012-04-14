//==================================================================//
/*
    AtomicParsley - AP_iconv.h

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

    Copyright ©2005-2006 puck_lock
                                                                   */
//==================================================================//
// utf conversion functions from libxml2
/*

 Copyright (C) 1998-2003 Daniel Veillard.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is fur-
nished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
DANIEL VEILLARD BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CON-
NECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Daniel Veillard shall not
be used in advertising or otherwise to promote the sale, use or other deal-
ings in this Software without prior written authorization from him.

*/

// Original code for IsoLatin1 and UTF-16 by "Martin J. Duerst" <duerst@w3.org>

extern int xmlLittleEndian;

void xmlInitEndianDetection();

int isolat1ToUTF8(unsigned char* out, int outlen, const unsigned char* in, int inlen);
int UTF8Toisolat1(unsigned char* out, int outlen, const unsigned char* in, int inlen);
int UTF16BEToUTF8(unsigned char* out, int outlen, const unsigned char* inb, int inlenb);
int UTF8ToUTF16BE(unsigned char* outb, int outlen, const unsigned char* in, int inlen);
int UTF16LEToUTF8(unsigned char* out, int outlen, const unsigned char* inb, int inlenb);
int UTF8ToUTF16LE(unsigned char* outb, int outlen, const unsigned char* in, int inlen);

/*------------- these functions are independent of any secondary licesing -------------*/

int isUTF8(const char* in_string);
unsigned int utf8_length(const char *in_string, unsigned int char_limit);

int strip_bogusUTF16toRawUTF8 (unsigned char* out, int inlen, wchar_t* in, int outlen);
