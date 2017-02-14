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



// dbfout.h
// Produces the dbase files.  Hardwired.  Quick hack.
// I have no interest in making this pretty.
// makes use of a shapefile library of unknown origin.
// That shapefile library is not particularly good imo,
// but it is finished and will save me some time.  I'm
// going to retro-fit it to the existing functions
// used for the avenue version


#ifndef __DBFOUT_H
#define __DBFOUT_H


#include "shpfile.h"

enum open_mode_t {
    om_read = 0,
    om_create = 1,
    om_append = 2,
    om_replace = 3
};

class DBaseTable {
  public:
    DBaseTable(const char *fname = NULL, open_mode_t mode=om_append);
    virtual ~DBaseTable();
    bool IsOpen() { return hTable?true:false; }
  
    bool AddField(const char *nm,DBFFieldType ft,int width, int decimals);
    int NumFields();
    int NumRecords();
    int FindField(const char *nm);
  
    long AddRecord();
    bool WriteField(int,const char *);
    bool WriteField(int,long);
    bool WriteField(int,double);
  
    virtual bool Open(const char *fname, open_mode_t mode=om_append);
    virtual void Close();
  
  private: //prevent
    DBaseTable(const DBaseTable &); //not implemented
  
  
  protected:
    DBFHandle dbf() const { return hTable; }
    long rec() const { return recno; }
  
  private: //implementation
    DBFHandle hTable;
    long recno;
};


inline bool DBaseTable::AddField(const char *nm,DBFFieldType ft,int width, int decimals = 0) {
    return DBFAddField(hTable,nm,ft,width,decimals);
}

inline void DBaseTable::Close() {
    if (hTable) {
        DBFClose(hTable);
        hTable = 0;
    }
    recno = -1;
}

inline DBaseTable::~DBaseTable() {
    if (hTable) {
        DBFClose(hTable);
    }
}

inline long DBaseTable::AddRecord() {
    return (recno = NumRecords());
}


inline int DBaseTable::NumFields() {
    return hTable ? hTable->nFields : -1;
}

inline int DBaseTable::NumRecords() {
    return hTable ? hTable->nRecords : -1;
}

inline bool DBaseTable::WriteField(int fieldNum, const char *val) {
    return DBFWriteStringAttribute(hTable,recno,fieldNum,val);
}

inline bool DBaseTable::WriteField(int fieldNum, long val) {
    return DBFWriteIntegerAttribute(hTable,recno,fieldNum,val);
}

inline bool DBaseTable::WriteField(int fieldNum, double val) {
    return DBFWriteDoubleAttribute(hTable,recno,fieldNum,val);
}





class NTXTable: public DBaseTable {
  public:
    NTXTable(const char *fname = NULL, open_mode_t mode=om_append); //makes a new table complete with all fields.
  
    bool AddFields();
    bool WriteRecord(long recno);
};





#endif
