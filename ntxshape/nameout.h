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


#ifndef __NAMEOUT_H
#define __NAMEOUT_H

#include "shpout.h"


#define MAX_NAME_LEN 200


// NTXNameFile is a linefile just for annotation (names)

class NTXNameFile: public LineFile {
  public:
    NTXNameFile(const char * fname = NULL, open_mode_t mode=om_append);
  
    bool AddFields();
  
    bool WriteRecord(
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
        char *text
    );
};



#endif
