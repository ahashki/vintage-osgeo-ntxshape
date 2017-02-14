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


#include "ntx.h"


void NTX::NextRecord() {
    recStart += recLen;
    recLen = file.at(recStart);
    if (recLen==NTX_NODATA) {
        recLen=0;
    }
  
    ++recNum;
    curPos = recStart;
}



void NTX::GotoRecord(long newRec) {
    // sequential scan.

    if (newRec < recNum) {
        recStart = 0;
        recLen = file[0][0];
        recNum = 0;
    }
    
    for (; recNum < newRec; ++recNum) {
        recStart += recLen;
        recLen = file.at(recStart);
      
        if (recLen==NTX_NODATA) {
            recLen=0;
        }
        
        if (recLen==0) {
            break;
        }
    }
    curPos = recStart;
}


void NTX::GotoBookmark(const NTXBookmark &bmark) {
    recNum   = bmark.recNum;
    recStart = bmark.recStart;
    recLen   = file.at(recStart);
    if (recLen == NTX_NODATA) {
        recLen = 0;
    }
};




char* NTX::GetString(long longoffset, long byteoffset, long nchars) {
    static char buffer[256] = "";
  
    if (nchars > 255) nchars = 255;
  
    if (nchars <= 0) {
        buffer[0] = '\0';
        return buffer;
    }
  
    long absOffset = curPos + longoffset + (byteoffset / 4);
  
    int strOffset = (byteoffset&3);
    char * str = buffer + strOffset;
  
    int numLongs = (strOffset + nchars + 3) / 4;
  
    file.array(absOffset, numLongs , (long*)buffer, 1); // 1=force it to use the buffer
  
#ifdef _BIGENDIAN_MACHINE
    for ( long * bufPtr = (long*)buffer+numLongs-1; bufPtr>=(long*)buffer; --bufPtr) {
        (void)AllSwap( *bufPtr );            //byteswap the whole thing
    }
#endif
  
    str[nchars] = '\0';
  
    // turn trailing blanks into null (deFORTRANize)
    while (buffer[--nchars] == ' ') {
        buffer[nchars] = '\0';
    }
  
    return str;
}
