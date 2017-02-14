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

//dbfout.cpp


#include "dbfout.h"
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>


#if defined(unix) || defined(__CYGWIN__)
#include <unistd.h>
#else
#include <io.h>
#include <errno.h>
#define PATH_SEPARATOR '\\'

#define F_OK 0
#endif





static void ChangeExtension(char *outfname, const char *infname, const char *ext) {
    //This is the same formula used in the shapefile routines, minus
    //the needless malloc and sprintf stuff


    int slen = strlen(infname);
  
    strcpy(outfname,infname);
  
    int i;
    for(i = slen-1;i > 0 && outfname[i] != '/' && outfname[i] != '\\'; --i ) {
        if (outfname[i] == '.') {
            outfname[i] = '\0';
            break;
        }
    }

    if (ext && *ext) {
        if (ext[0] != '.') {
            strcat(outfname,".");
        }
        
        strcat(outfname,ext);
    }
}



DBaseTable::DBaseTable(const char * fname, open_mode_t mode):hTable(0), recno(-1) {
    // remember the Create and Open will not be polymorphic here
  
    if (fname && *fname) {
        Open(fname, mode);
    }
}


// a little less complexity in Open, removing boilerplate code from the callers.
// TODO: check that I got this right.

bool DBaseTable::Open(const char * fname, open_mode_t mode) {
    Close();
    
    char dbfName[255];
    ChangeExtension(dbfName,fname,".dbf");
    
    if ((mode != om_read) && ((mode == om_replace) || (access(dbfName,(F_OK))!=0))) {
        hTable = DBFCreate(dbfName);
    }
    
    if ((hTable == 0) && (mode != om_create) && (access(dbfName,(F_OK))==0)) {
        hTable = DBFOpen(dbfName, mode == om_read ? "rb" : "rb+");
    }
  
    return IsOpen();
}


int DBaseTable::FindField(const char *fld) {
    int i;
  
    char ucaseFld[12];
    for (i=0; i < 10 && fld[i] && fld[i] != ' '; ++i) {
        ucaseFld[i] = toupper(fld[i]);
    }
    ucaseFld[i] = '\0';

    int count = NumFields();
    for (int i = 0; i < count; i++) {
        char fieldName[12];
        DBFGetFieldInfo( hTable, i, fieldName, 0, 0);
        
        if (strcmp(fieldName, ucaseFld) == 0) {
            return i;
        }
    }
  
    return -1;
}




NTXTable::NTXTable(const char *fname, open_mode_t mode): DBaseTable(fname, mode) {
    if (IsOpen()) {
        if (!AddFields()) {
            Close();
        }
    }
}


bool NTXTable::AddFields() {
    if (IsOpen()) {
        if (NumFields() == 0) {
            return ( AddField("RECNO",FTNumber,12) );
        } else {
            return ( FindField("RECNO") == 0 );
        }
    } else {
        return false;
    }
}



bool NTXTable::WriteRecord(long recno) {
    if (!IsOpen()) {
        return false;
    }

    AddRecord();
  
    return WriteField(0,recno);
}

