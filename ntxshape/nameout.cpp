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
 * Portions created by ESRI Canada are Copyright (C) 1997-2009
 * ESRI Canada Limited.  All Rights Reserved.
 *
 * Contributor(s):
 *  Bruce Dodson <bdodson@esricanada.com>
**/


#include "nameout.h"


NTXNameFile::NTXNameFile(const char *fname, open_mode_t mode)
  : LineFile(fname, mode)
{
    if (IsOpen()) {
        if (!AddFields()) {
            Close();
        }
    }
}


bool NTXNameFile::AddFields() {
    if (IsOpen()) {
        if (NumFields() == 0) {
            return (
                AddField("RECNO"     ,FTNumber,12)
             && AddField("FONT"      ,FTNumber,10)
             && AddField("SIZE"      ,FTNumber,14,6)
             && AddField("REFX"      ,FTNumber,18,8)
             && AddField("REFY"      ,FTNumber,18,8)
             && AddField("SCALE"     ,FTNumber,12)
             && AddField("TEXT"      ,FTString, MAX_NAME_LEN)
            );
        } else {
            return (
                FindField("RECNO")==0
             && FindField("FONT") == 1
             && FindField("SIZE") == 2
             && FindField("REFX") == 3
             && FindField("REFY") == 4
             && FindField("SCALE")== 5
             && FindField("TEXT") == 6
            );
        }
    } else {
        return false;
    }
}





bool NTXNameFile::WriteRecord(
    double *xy,
    long vtxCount,
    int *parts,
    int partCount,
    long recno,
    long font,
    double size,
    double refx,
    double refy,
    long scale,
    char * text
) {

    if (!IsOpen()) {
        return false;
    }

    AddRecord();

    return (
        WriteVertices(xy,vtxCount, parts, partCount)
     && WriteField(0,recno)
     && WriteField(1,font)
     && WriteField(2,size)
     && WriteField(3,refx)
     && WriteField(4,refy)
     && WriteField(5,scale)
     && WriteField(6,text)
    
    );
}

