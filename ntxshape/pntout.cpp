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


// pntout.cpp
#include "pntout.h"

NTXLabelData::NTXLabelData(const NTXLabelData &copy) {
    memcpy(this, &copy, sizeof(copy));
    if (copy.keyword_) {
        keyword_ = (char*)malloc(keylen_+1);
        memcpy(keyword_, copy.keyword_, keylen_+1);
    }
}


NTXLabelData & NTXLabelData::operator=(const NTXLabelData &copy) {
    if (this != &copy) {
        free(keyword_);
        memcpy(this, &copy, sizeof(copy));
  
        if (copy.keyword_) {
            keyword_ = (char*)malloc(keylen_+1);
            memcpy(keyword_, copy.keyword_, keylen_+1);
        }
    }
    return *this;
}


void NTXLabelData::clear() {
    x = y = dispx = dispy = NTX_NODATA;
    size = 0; angle = 0;
    memset(keyword(),' ', keylen_);
    keyword(keylen_) = 0;
}


NTXPointFile::NTXPointFile(const char *fname, open_mode_t mode)
  : PointFile(fname, mode)
{
    if (IsOpen()) {
        if (!AddFields()) {
            Close();
        }
    }
}


bool NTXPointFile::AddFields() {
    if (IsOpen()) {
        if (NumFields() == 0) {
            return (
                AddField("RECNO"    ,FTNumber,12)
             && AddField("ELEVATION",FTNumber,16,6)
             && AddField("DATAFLAGS",FTNumber,12)
             && AddField("DISPX"    ,FTNumber,18,8)
             && AddField("DISPY"    ,FTNumber,18,8)
             && AddField("SIZE"     ,FTNumber,14,6)
             && AddField("ANGLE"    ,FTNumber,8,3)
             && AddField("SCALE"    ,FTNumber,12)
             && AddField("KEYWORD"  ,FTString,32)
            );
        } else {
            return (
                FindField("RECNO") == 0
             && FindField("ELEVATION") == 1
             && FindField("DATAFLAGS") == 2
             && FindField("DISPX")     == 3
             && FindField("DISPY")     == 4
             && FindField("SIZE")      == 5
             && FindField("ANGLE")     == 6
             && FindField("SCALE")     == 7
             && FindField("KEYWORD")   == 8
            );
        }
    } else {
        return false;
    }
}



bool NTXPointFile::WriteRecord(
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
) {
  
    if (!IsOpen()) {
        return false;
    }
  
    AddRecord();
  
    return (
        WriteShape(x,y)
     && WriteField(0,recno)
     && WriteField(1,elevation)
     && WriteField(2,dataflags)
     && WriteField(3,dispx)
     && WriteField(4,dispy)
     && WriteField(5,size) 
     && WriteField(6,angle)
     && WriteField(7,scale)
     && WriteField(8,keyword)
    );
}
