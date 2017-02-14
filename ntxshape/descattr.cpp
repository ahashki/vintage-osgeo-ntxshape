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

// descattr.cpp

#include "descattr.h"
#include <string.h>

NTXDescriptorWriter::NTXDescriptorWriter(DBaseTable &dbf)
  : pDbf(&dbf)
{

    if (pDbf->IsOpen()) {
    
        startingField = dbf.FindField("USERNUM");
    
        if (startingField == -1) {
            startingField = dbf.NumFields();
      
            if (!(
                pDbf->FindField("FCODE") == -1
             && pDbf->FindField("DATATYPE") == -1
             && pDbf->FindField("DESCFLAGS") == -1
             && pDbf->FindField("THEMENUM") == -1
             && pDbf->FindField("INDEXKEY") == -1
             && pDbf->FindField("SUPERFLAGS") == -1
             && pDbf->FindField("SOURCEID") == -1
        
             && pDbf->AddField("USERNUM",FTNumber,12)
             && pDbf->AddField("FCODE",FTString,12)
             && pDbf->AddField("DATATYPE",FTNumber,4)
             && pDbf->AddField("DESCFLAGS",FTNumber,12)
             && pDbf->AddField("THEMENUM",FTNumber,12)
             && pDbf->AddField("INDEXKEY",FTString,12)
             && pDbf->AddField("SUPERFLAGS",FTNumber,12)
             && pDbf->AddField("SOURCEID",FTString,12)
            
            )) {
                pDbf->Close();
            }
        } else {
            if (!(
              
                pDbf->FindField("FCODE") == startingField+1
             && pDbf->FindField("DATATYPE") == startingField+2
             && pDbf->FindField("DESCFLAGS") == startingField+3
             && pDbf->FindField("THEMENUM") == startingField+4
             && pDbf->FindField("INDEXKEY") == startingField+5
             && pDbf->FindField("SUPERFLAGS") == startingField+6
             && pDbf->FindField("SOURCEID") == startingField+7
            
            )) {
                pDbf->Close();
            }
        }
    
    }

}



void NTXDescriptor::SetDescriptor(
    long usernum_,
    const char *fcode_,
    long datatype_,
    long descflags_,
    const char * sourceId_
) {

    //if the datatype is < 1000 this is a plain descriptor,
    //otherwise it's a superdescriptor.
  
    //if the _previous_ record doesn't have its "continues"
    //flag set, clear the super descriptor
    if ((descflags&1)==0) {
        has_super = false;
    }
  
    usernum = usernum_;
    strncpy(fcode,fcode_,12);
    strncpy(sourceid, sourceId_, 12);
    datatype = datatype_;
    descflags = descflags_;
  
    has_desc = true;
}





void NTXDescriptor::SetSuperDescriptor(
    long themenum_,
    const char *indexkey_,
    long superflags_
) {
    has_desc = false;
  
    descflags = 1;     //to force it to apply to the next record
  
  
    // descriptor
    themenum = themenum_;
    if (superflags_ & (1<<16)) {
        strncpy(indexkey,indexkey_,12);
    } else {
        //skip the indexkey since the flags don't say it's valid.
        indexkey[0]=0;
    }
    superflags = superflags_;
    has_super = true;
}






int NTXDescriptorWriter::Write(NTXDescriptor const &d) {
    int success = true;
  
    if (d.has_desc) {
        success = (
            pDbf->WriteField(startingField  ,d.usernum)
         && pDbf->WriteField(startingField+1,d.fcode)
         && pDbf->WriteField(startingField+2,d.datatype)
         && pDbf->WriteField(startingField+3,d.descflags)
      
        );
    }
  
    if (d.has_super && success) {
      success = (
          pDbf->WriteField(startingField+4,d.themenum)
       && pDbf->WriteField(startingField+5,d.indexkey)
       && pDbf->WriteField(startingField+6,d.superflags)
      
      );
    }
  
  
    if (d.has_desc && success) {
        success = pDbf->WriteField(startingField+7,d.sourceid);
    }
    
    return success; //or failure
}


