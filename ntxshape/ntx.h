/**
 * The contents of this file are subject to the Mozilla Public License 
 * Version 1.1 (the "License"); you may not use this file except in 
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" 
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See 
 * the License for the specific language governing rights and limitations 
 * under the License.
 *
 * The Original Code is NTXShape - NTX to Shapefile Converter.
 *
 * The Initial Developer of the Original Code is ESRI Canada Limited.
 * Portions created by ESRI Canada are Copyright (C) 1997-2003 
 * ESRI Canada Limited.  All Rights Reserved.
 *
 * Contributor(s):
 *  Bruce Dodson <bdodson@esricanada.com>
**/


#ifndef __NTX_H
#define __NTX_H

#include "intxfile.h"

#ifdef _BIGENDIAN_MACHINE
//On unix/mac we'll have to unswap the strings
#include "byteswap.h"
#endif


class NTXBookmark;

struct NTX {
  public:
    NTX(const char * fname): file(fname), recNum(0), recStart(0), curPos(0) {
        recLen = file[0][0];
        descLen = file[0][1];
    }
  
    //BuildIndex has been removed; use Bookmark instead.
  
    void NextRecord();
    void GotoRecord(long newRec);
  
    NTXBookmark Bookmark() const;
    void GotoBookmark(const NTXBookmark &bookmark);
  
  
    void AbsOffset(long off) { curPos = recStart + off; }
  
    void RelOffset(long off) { curPos += off; }
  
    int  DescLen() const { return descLen; }
  
    //no integrity checking here
    void SetOffsetData() { curPos = recStart + descLen + file.at(recStart+descLen); }
  
    long GetLong(long offset) { return file.at(curPos + offset); }
  
    long GetShort(long longoffset,long shortoffset) {
        long value = GetLong(longoffset + shortoffset/2);
        if (value != NTX_NODATA) {
#ifdef _BIGENDIAN_MACHINE
            ShortSwap(value);
#endif
            value = ((short*)&value)[ shortoffset  & 1 ];
        }
        return value;
    }
  
    long GetChar(long longoffset,long byteoffset) {
        long value = GetLong(longoffset + (byteoffset>>2));
    
        if (value != NTX_NODATA) {
#ifdef _BIGENDIAN_MACHINE
            AllSwap(value);
#endif
            value = ((char*)&value)[ byteoffset & 3 ];
        }
        return value;
    }
  
  
    // GetCharPair assumes the byteoffset is even.  It is mainly an optimization:
    // half the number of GetChars that compresed lines require.  Those
    // calls were cheap, but were taking a significant fraction (about 10%) of
    // ReadVertices' running time.
    char * GetCharPair(long longoffset, long byteoffset) {
        static long value;
    
        value = GetLong(longoffset + (byteoffset>>2));
    
        if (value == NTX_NODATA) {
            value = -0x80808080;
        }
        
#ifdef _BIGENDIAN_MACHINE
        AllSwap(value);
#endif
        return ((char*)&value) + (byteoffset & 3);
    }
  
  
    char *GetString(long longoff, long byteoff, long nchars);
  
    bool IsValid() const { return file.isValid() && curPos < recStart+recLen; }
  
    float GetProgress() const { return file.isValid() ? float((100.0/256.0) * curPos / file.count()) : 0; }
    long Recno() const { return recNum; }
  
  protected:
    iNTXfile file;
  
    long descLen;  //these are included for convenience
  
    long recNum; //these two are commented out since derive from bookmark.
    long recStart;
    long recLen;
    long curPos;
  
};


// NTX /could/ inherit from NTXBookmark... but no
class NTXBookmark {
  public:
    NTXBookmark(): recNum(0), recStart(0) {}
    long Recno() const { return recNum; }
  
  protected:
    friend NTXBookmark NTX::Bookmark() const;
    friend void NTX::GotoBookmark(const NTXBookmark &);
  
    NTXBookmark(long rn, long rs): recNum(rn), recStart(rs) {}
  
    long recNum;
    long recStart;
};

inline NTXBookmark NTX::Bookmark() const {
    return NTXBookmark(recNum, recStart);
}

#endif
