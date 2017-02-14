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
#include <string.h>



#include "ntxrecs.h"
#include "pntout.h"



void NTXReadType7(NTX* hntx, NTXLabelData &d) {
    char *keyword = d.keyword();
    int keylen = d.keyword_len();
  
    hntx->AbsOffset( hntx->DescLen() );
  
    int dhlen = hntx->GetLong(0);
    int rglen = hntx->GetLong(1);
  
    d.size = dhlen>5 ? hntx->GetLong(5) : 0;
  
    if (dhlen>7) {
      d.x = hntx->GetLong(6);
      d.y = hntx->GetLong(7);
    }
  
    hntx->RelOffset(dhlen);
  
    long x = hntx->GetLong(0);
    long y = hntx->GetLong(1);
  
    if (x!=NTX_NODATA) {
        d.dispx = x;
        d.dispy = y;
        if ( dhlen<=7 ) {
            d.x = x;
            d.y = y;
        }
    
        //take the angle of the whole string from the 1st character
        int angle = hntx->GetShort(2,0);
        if (angle < 0) {
            angle += (360*60);
        }
        d.angle  = float(angle) / 60;
        
        keyword[0] = hntx->GetChar(2,2);
        int i;
        for (i = 1; i < keylen ;++i) {
            if ( hntx->GetLong(rglen) == NTX_NODATA ) {
                break;
            }
      
            hntx->RelOffset(rglen);
            keyword[i] = hntx->GetChar(2,2);
        }
        keyword[i] = '\0';
    }
}



void  NTXReadType8Group(NTX* hntx, short grplen, NTXPointData &d) {
    d.x = d.dispx = hntx->GetLong(0);
    d.y = d.dispy = hntx->GetLong(1);
    d.elev  = hntx->GetLong(2);
  
    if (grplen > 4) {
        d.angle = float(hntx->GetShort(4,0)) / 60.0;
        d.size  = hntx->GetShort(4,1);
        if (grplen > 5) {
            d.flags = hntx->GetLong(5);
            if (grplen > 8) {
                strcpy(d.keyword(),hntx->GetString(6,0,12));
            }
        }
    }
  
    hntx->RelOffset(grplen);
}




void  NTXReadType10Group(NTX* hntx, short grplen, NTXPointData &d) {
    d.x = d.dispx = hntx->GetLong(0);
    d.y = d.dispy = hntx->GetLong(1);
    d.elev  = hntx->GetLong(2);
    d.flags = hntx->GetLong(3);
  
    if (grplen>4) {
        d.angle = (float)hntx->GetLong(4) / 3600000.0;
        if (grplen > 7) {
            strcpy(d.keyword(),hntx->GetString(5,0,12));
            if (grplen > 10) {
                d.x = hntx->GetLong(9);
                d.y = hntx->GetLong(10);
                //if that turned out to be invalid, undo it.
                if (d.x == 0 || d.y == 0) {
                    d.x = d.dispx;
                    d.y = d.dispy;
                }
            }
        }
    }
  
    hntx->RelOffset(grplen);
}



void  NTXReadType11Group(NTX* hntx, short grplen, NTXPointData &d) {
    d.x = d.dispx = hntx->GetLong(0);
    d.y = d.dispy = hntx->GetLong(1);
    d.elev  = hntx->GetLong(2);
    d.flags = hntx->GetLong(3);
  
    if (grplen>5) {
        d.dispx = hntx->GetLong(4);
        d.dispy = hntx->GetLong(5);
        //if that turned out to be invalid, undo it.
        if (d.dispx == NTX_NODATA || d.dispy == NTX_NODATA) {
            d.dispx = d.x;
            d.dispy = d.y;
        }
    
        if (grplen>10) {
            d.angle = float(hntx->GetLong(10)) / 3600000.0;
      
            if (grplen>13) {
                strcpy(d.keyword(),hntx->GetString(11,0,12));
            }
        }
    }
  
    hntx->RelOffset(grplen);
}



void  NTXReadDescriptor(
    NTX* hntx,
    long*userNum,
    char*fcode,
    long*flags,
    long*datatype,
    char*sourceID
) {
    hntx->AbsOffset(0);
  
    *userNum = hntx->GetLong(1);
    strcpy( fcode, hntx->GetString(2,0,12) );
    *flags = hntx->GetLong(11);
    *datatype  = hntx->GetLong(12);
  
    if ((hntx->DescLen() >= 16) && sourceID) {
        strcpy( sourceID, hntx->GetString(13,0,12) );
    } else {
        strcpy( sourceID, "            ");
    }
  
} // NTXReadDescriptor


