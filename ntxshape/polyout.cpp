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


#include "polyout.h"


NTXPolygonFile::NTXPolygonFile(const char *fname, open_mode_t mode)
  : PolygonFile(fname, mode)
{
    if (IsOpen()) {
        if (!AddFields()) {
            Close();
        }
    }
}


bool NTXPolygonFile::AddFields() {
    if (IsOpen()) {
        if (NumFields() == 0) {
            return (
                AddField("RECNO"    ,FTNumber,12)
             && AddField("AREA",     FTNumber,16,4)
             && AddField("DISPX"    ,FTNumber,18,8)
             && AddField("DISPY"    ,FTNumber,18,8)
             && AddField("SIZE"     ,FTNumber,14,6)
             && AddField("ANGLE"    ,FTNumber,8,3)
             && AddField("KEYWORD"  ,FTString,15)
      
            );
        } else {
            return (
                FindField("RECNO") == 0
             && FindField("AREA") == 1
       
             && FindField("DISPX") == 2
             && FindField("DISPY") == 3
             && FindField("SIZE") == 4
             && FindField("ANGLE") == 5
             && FindField("KEYWORD") == 6
              
            );
        }
    } else {
        return false;
    }
}



bool NTXPolygonFile::WriteRecord(
    double *xy, long vtxCount, 
    int*parts, int partCount,
    long recno, double area
) {
  
    if (!IsOpen()) {
        return false;
    }
  
    AddRecord();
    return (
        WriteVertices(xy, vtxCount, parts, partCount)
     && WriteField(0,recno)
     && WriteField(1,area)
    );
}

int NTXPolygonFile::WriteRecord(
    double *xy, long vtxCount, 
    int*parts, int partCount,
    long recno, double area, 
    double dispx, double dispy, 
    double size, double angle,
    char const *keyword
) {
    return (
        WriteRecord(xy,vtxCount,parts,partCount,recno,area)
     && WriteField(2,dispx)
     && WriteField(3,dispy)
     && WriteField(4,size)
     && WriteField(5,angle)
     && WriteField(6,keyword)
    );
}

      
