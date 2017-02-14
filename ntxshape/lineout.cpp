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


#include "lineout.h"

NTXLineFile::NTXLineFile(const char *fname, open_mode_t mode)
  : LineFile(fname, mode)
{
    if (IsOpen()) {
        if (!AddFields()) {
            Close();
        }
    }
}


NTXLineFileZ::NTXLineFileZ(const char *fname, open_mode_t mode)
  : LineFileZ(fname, mode)
{
    if (IsOpen()) {
        if (!AddFields()) {
            Close();
        }
    }
}





static bool _AddFields(DBaseTable &dbf) {
    if (dbf.NumFields() == 0) {
        return (
            dbf.AddField("RECNO"     ,FTNumber,12)
         && dbf.AddField("ELEVATION" ,FTNumber,16,6)
         && dbf.AddField("LINEWEIGHT",FTNumber,12,3)
         && dbf.AddField("SCALE"     ,FTNumber,12)
        );
    } else {
        return (
            dbf.FindField("RECNO") == 0
         && dbf.FindField("ELEVATION") == 1
         && dbf.FindField("LINEWEIGHT") == 2
         && dbf.FindField("SCALE") == 3
        );
    }
}


bool NTXLineFile::AddFields() {
    return IsOpen() && _AddFields(*this);
}


bool NTXLineFileZ::AddFields() {
    return IsOpen() && _AddFields(*this);
}






bool NTXLineFile::WriteRecord(
    double *xy,
    long vtxCount,
    long recno,
    double elevation,
    double lineweight,
    long scale
) {

    if (!IsOpen()) {
        return false;
    }

    AddRecord();

    return (
        WriteVertices(xy,vtxCount)
     && WriteField(0,recno)
     && WriteField(1,elevation)
     && WriteField(2,lineweight)
     && WriteField(3,scale)
    );
}


bool NTXLineFileZ::WriteRecord(
    double *xy,
    double *zed,
    long vtxCount,
    long recno,
    double elevation,
    double lineweight,
    long scale
) {

    if (!IsOpen()) {
        return false;
    }

    AddRecord();

    return (
        WriteVertices(xy,zed,vtxCount)
     && WriteField(0,recno)
     && WriteField(1,elevation)
     && WriteField(2,lineweight)
     && WriteField(3,scale)
    );
}





