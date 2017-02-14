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


// pntout.h

#ifndef __PNTOUT_H
#define __PNTOUT_H


#include "shpout.h"
#include <stdlib.h>
#include <string.h>

#ifndef NTX_NODATA
#define NTX_NODATA ((long)(0x80000000))
#endif


// NTXPointFile is a PointFile with its fields hardwired.

// NTXLabelData can be used "by value" (plain ol' data)
// if the key length is less than 16.  This choice totally
// takes advantage of "inside knowledge": 15 is enough for
// label points, and label points are what are loaded into
// a search structure.
struct NTXLabelData {
    union { struct { long x; long y; }; long xy[2]; };
  
    union { struct { long dispx; long dispy;}; long dispxy[2]; };
  
    long size;
    float angle;
  
  protected:
    int keylen_;
    char *keyword_;
    char short_keyword[16];
  
  public:
    char *keyword() { return keyword_ ? keyword_ : short_keyword; }
    char &keyword(int i) { return keyword()[i]; }
    char const*keyword() const { return keyword_ ? keyword_ : short_keyword; }
    char keyword(int i) const { return keyword()[i]; }
  
    int keyword_len() const { return keylen_; }
  
    NTXLabelData(int keylen=32)
      : x(NTX_NODATA), 
        y(NTX_NODATA),
        dispx(NTX_NODATA), dispy(NTX_NODATA),
        size(0), angle(0),
        keylen_(keylen > 15 ? keylen : 15)
    {
        keyword_ = (keylen_ > 15) ? (char*)malloc(keylen_+ 1) : 0;
        memset(keyword(),' ', keylen_);
        keyword(keylen_) = '\0';
    }
  
  
    ~NTXLabelData() { free(keyword_); }
  
    NTXLabelData(const NTXLabelData &copy);
    NTXLabelData & operator=(const NTXLabelData &copy);
    void clear();
};



struct NTXPointData: NTXLabelData {
    long flags;
    long elev;
  
    NTXPointData(int keylen=32): NTXLabelData(keylen), flags(0), elev(NTX_NODATA) {}
  
    void clear() {
        NTXLabelData::clear();
        flags = 0; elev = NTX_NODATA;
    }
};




class NTXPointFile: public PointFile {
  public:
    NTXPointFile(const char * fname = NULL, open_mode_t mode = om_append);
  
    bool AddFields();
    bool WriteRecord(
        double x,
        double y,
        long recno,
        double elevation,
        long dataflags,
        double dispx,
        double dispy,
        double size,
        double angle,
        long scale,
        const char *keyword
    );
};




#endif
