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


#include "shpout.h"


ShapeFile::ShapeFile(const char *fname, SHPShapeType type, open_mode_t mode) : hShape(0) {
    if (fname && *fname) {
        InternalOpen(fname,type,mode);
    }
}


bool ShapeFile::InternalOpen(const char *fname, SHPShapeType shpType, open_mode_t mode) {
    if (DBaseTable::Open(fname, mode)) {
        
        if ( (mode==om_read) || (mode==om_append) ) {
            hShape = SHPOpen(fname, mode==om_append ? "rb+" : "rb");
            
        }
        
        if ( (mode!=om_read) && (hShape==0) ) {
            // note: create and replace are the same, because the DBaseTable::Open would
            // have already blocked us from overwriting when in create mode.
            hShape = SHPCreate(fname, shpType);
        }

        if ((hShape==0) || (hShape->nShapeType != shpType)) {
            Close(); // also closes DBF
        }
        
        return IsOpen();
    } else {
        return false;
    }
}
