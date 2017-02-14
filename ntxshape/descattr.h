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




// descattr.h

// NTXPointFileWithDescriptor
// A shapefile containing both point attributes and descriptor
// data is needed.  I want to do this while changing as little
// existing code as possible.

// This can derive from NTXPointFile.
// Code for the descriptor fields can be copied from NTXDescriptor
// Inheriting from NTXDescriptor isn't an option.

// Therefore NTXDescriptorHelper was introduced.  This class can
// be plugged in to either NTXPointFile or NTXLineFile.  It could
// also be plugged into a revised NTXDescriptorTable
// Maybe this would be called NTXBaseTable, and would just
// include RecNo.

// Although putting the descriptor attributes at the start of the
// file would make sense in many ways, putting them at the end
// better matches what people are used to seeing in 1.00,
// where the descriptors are joined on using ArcView.

#ifndef __DESCATTR_H
#define __DESCATTR_H

#ifdef __GNUG__
__attribute__((packed))
#endif
struct NTXDescriptor {
    //Don't change the type to bool (even though that's what it holds)
    //because this structure is used in the VB-callable API, and changing
    //the type could change the layout.  (e.g. in GCC, sizeof(bool) is 1)
    //but go ahead and treat it as bool everywhere else...
    int has_desc; 
    int has_super;
  
    long usernum;
    char fcode[16];
    char sourceid[16];
    long datatype;
    long descflags;
  
    long themenum;
    char indexkey[16];
    long superflags;
  
#ifdef __cplusplus
    void SetDescriptor(
        long usernum,
        const char *fcode,
        long datatype,
        long descflags,
        const char * sourceId
    );
  
    // default constructor would just nullify everything,
    // which ANSI C++ will do anyway.
    
    void SetSuperDescriptor(
        long themenum,
        const char *indexkey,
        long superflags
    );
  
    void Clear() { has_desc = has_super = false; }
    NTXDescriptor() : has_desc(false), has_super(false) {
        fcode[12]=sourceid[12]=indexkey[12]='\0';
    }
#endif
};  


#ifdef __cplusplus

#include "dbfout.h"

class NTXDescriptorWriter {
  public:
    NTXDescriptorWriter(DBaseTable & dbf);
    int Write(NTXDescriptor const &dsc);
  
  protected:
    DBaseTable * pDbf;
    int startingField;
};


#endif

#endif
