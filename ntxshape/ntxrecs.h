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



//NTXRECS.H

#ifndef __NTXRECS_H
#define __NTXRECS_H

struct NTX;
struct NTXLabelData;
struct NTXPointData;


void NTXReadType7(NTX* hntx, NTXLabelData &pntData);


// For the "group" functions below,
// any optional attributes which aren't present
// in a given NTX record are left as they are;
// thus you must initialize the following vars
// before looping through the repeating group:
// size, angle, flags, and keyword.

void NTXReadType8Group(NTX* hntx, short grplen, NTXPointData &pntData);
void NTXReadType10Group(NTX* hntx,short grplen, NTXPointData &pntData);
void NTXReadType11Group(NTX* hntx,short grplen, NTXPointData &pntData);



void NTXReadDescriptor(
    NTX* hntx,
    long*userNum,
    char*fcode,
    long*descFlags,
    long*dataType,
    char*sourceID
);

#endif

