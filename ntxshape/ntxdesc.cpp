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




#include "ntxshape.h"

int NTXConverter::ConvertDescriptors(const char * dbfOut, bool append) {
    ResetStatus();
  
    GotoRecord(0);
    if (!IsValid()) {
        return 1;
    }
  
    NTXTable dbf(dbfOut, append ? om_append : om_replace);
  
    NTXDescriptorWriter descwriter(dbf);

    if (! dbf.IsOpen()) {
        return 1;
    }

    NTXDescriptor desc;
  
    long usernum = 0;
    char fcode[13] = "";
    long datatype = 0;
    long descflags = 0;
    long themenum = 0;
    char indexkey[13] = "";
    long superflags = 0;
  
    char sourceid[13] = "";
  
    NextRecord(); //skip header
    UpdateStatus();
  
    while (IsValid()) {
        if ( MatchThemeNumber() && MatchDataType()) {
            datatype  = GetLong(12);
            if (IsSuper(datatype)) {
                themenum = GetLong(1);
                strcpy(indexkey, GetString(2,0,12));
                superflags = GetLong(11);
                desc.SetSuperDescriptor(themenum,indexkey,superflags);
            } else {
                usernum = GetLong(1);
                strcpy(fcode, GetString(2,0,12));
                descflags = GetLong(11);
        
                if (MatchFcode(fcode) && MatchDescFlags(descflags)) {
          
                    if (DescLen() >= 16) {
                        strcpy(sourceid, GetString(13,0,12));
                    }
          
                    desc.SetDescriptor(usernum,fcode,datatype,descflags,sourceid);
          
                    if (MatchCustom(Recno(), desc)) {
                        dbf.WriteRecord(Recno());
                        descwriter.Write(desc);
                    } else {
                        desc.Clear();
                    }
                } else { //fcode does not match
                    desc.Clear();
                }
            } //end branch super vs normal
    
        } else { //datatype or themenum does not match
            desc.Clear();
        }
    
        NextRecord();
        UpdateStatus();
    }
  
    GotoRecord(0);
    UpdateStatus(100);
    return 0;
}


// ScanDescriptors is a copy/paste with the
// write replaced with a callback.  It could
// be implemented using a callback that knows how
// to write.




int NTXConverter::ScanDescriptors(NTXScanDescProc callback, void*cbdata) {
    ResetStatus();
  
    GotoRecord(0);
    if (!IsValid()) {
        return 1;
    }
  
    NTXDescriptor desc;
  
    long usernum = 0;
    char fcode[13] = "";
    long datatype = 0;
    long descflags = 0;
    long themenum = 0;
    char indexkey[13] = "";
    long superflags = 0;
  
    char sourceid[13] = "";
  
    NextRecord(); //skip header
    UpdateStatus();
  
    while (IsValid()) {
        if ( MatchThemeNumber() && MatchDataType()) {
            datatype  = GetLong(12);
            if (IsSuper(datatype)) {
                themenum = GetLong(1);
                strcpy(indexkey, GetString(2,0,12));
                superflags = GetLong(11);
                desc.SetSuperDescriptor(themenum,indexkey,superflags);
            } else {
                usernum = GetLong(1);
                strcpy(fcode, GetString(2,0,12));
                descflags = GetLong(11);
        
                if (MatchFcode(fcode) && MatchDescFlags(descflags)) {
          
                    if (DescLen() >= 16) {
                        strcpy(sourceid, GetString(13,0,12));
                    }
          
                    desc.SetDescriptor(usernum,fcode,datatype,descflags,sourceid);
          
                    if (MatchCustom(Recno(), desc)) {
                        callback(Recno(),&desc,cbdata);
                    } else {
                        desc.Clear();
                    }
                } else { //fcode does not match
                    desc.Clear();
                }
            } //end branch super vs normal
    
        } else { //datatype or themenum does not match
            desc.Clear();
        }
    
        NextRecord();
        UpdateStatus();
    }
  
    GotoRecord(0);
    UpdateStatus(100);
    return 0;
}











// ReadDescriptor is similar to ScanDescriptors, but it works by "pull"
// rather than "push" processing, which turns out to be easier to 
// understand when writing a filter or a scanner.

// It is a documented, intentional side effect of ReadDescriptor that
// it updates the current-record pointer, so that we can get at the
// data portion of the descriptor and data.  Also, it uses the current
// record pointer when run in read-next mode, so if something like
// ReadLineVertices is called (which scans forward to join graphically
// linked features), the joined features will be skipped.


int NTXConverter::ReadDescriptor(NTXDescriptor &desc, NTXReadPosition readPos) {
    if (readPos==ntxReadFirst) {
        GotoRecord(0);
    
        desc.Clear();
    
        if (!IsValid()) {
            return NTX_NODATA;
        }
    }
  
    NextRecord();
  
    long usernum = 0;
    char fcode[13] = "";
    long datatype = 0;
    long descflags = 0;
    long superflags= 0;
    long themenum = 0;
    char indexkey[13] = "";
    char sourceid[13] = "";
  
  
    while (IsValid()) {
  
        if ( MatchThemeNumber() && MatchDataType()) {
            datatype  = GetLong(12);
            if (IsSuper(datatype)) {
                themenum = GetLong(1);
                strcpy(indexkey, GetString(2,0,12));
                superflags = GetLong(11);
        
                desc.SetSuperDescriptor(themenum,indexkey,superflags);
            } else {
                usernum = GetLong(1);
                strcpy(fcode, GetString(2,0,12));
                descflags = GetLong(11);
        
                if (MatchFcode(fcode) && MatchDescFlags(descflags)) {
                    if (DescLen() >= 16) {
                        strcpy(sourceid, GetString(13,0,12));
                    }
          
                    desc.SetDescriptor(usernum,fcode,datatype,descflags,sourceid);
          
                    if (MatchCustom(Recno(), desc)) {
                        return Recno();
                    } else {
                        desc.Clear();
                    }
                } else { //fcode does not match
                  desc.Clear();
                }
            } //end branch super vs normal
        } else { //datatype or themenum does not match
            desc.Clear();
        }
    
        NextRecord();
    }
  
    GotoRecord(0);
    return NTX_NODATA;
}
