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



// shpout.h
// again a quick hack.  This is not designed;
// it's just the fastest way.
// Create and Open added to support append.
// Don't call these > once.

#ifndef __SHPOUT_H
#define __SHPOUT_H

#include "dbfout.h"

class ShapeFile: public DBaseTable {
  public:
    bool IsOpen() { return hShape && DBaseTable::IsOpen(); }
  
    void Close();
  
  protected:
    ShapeFile(const char * fname, SHPShapeType type, open_mode_t mode = om_append);
    SHPHandle shp() const { return hShape; }
    bool WriteVertices(double *xy, int nvtx);
    bool WriteVertices(double *xy, int nvtx, int *parts, int nparts);
    
    bool WriteVertices(double *xy, double *zed, int nvtx);
    bool WriteVertices(double *xy, double *zed, int nvtx, int *parts, int nparts);
    
    bool InternalOpen(const char *fname, SHPShapeType type, open_mode_t mode = om_append);
  
    ~ShapeFile();
  
  private:
    SHPHandle hShape;
};



inline ShapeFile::~ShapeFile() {
    if (hShape) SHPClose(hShape);
}

inline void ShapeFile::Close() {
    DBaseTable::Close();
    if (hShape) {
        SHPClose(hShape); hShape=0;
    }
}


inline bool ShapeFile::WriteVertices(double *xy, int nvtx) {
    int partOffset = 0;
    return (rec() == SHPWriteVertices(shp(),nvtx,1,&partOffset,xy));
}

inline bool ShapeFile::WriteVertices(double *xy, int nvtx, int *partOffsets, int nparts) {
    return (rec() == SHPWriteVertices(shp(),nvtx,nparts,partOffsets,xy));
}

inline bool ShapeFile::WriteVertices(double *xy, double *zed, int nvtx) {
    int partOffset = 0;
    return (rec() == SHPWriteVerticesEx(shp(),nvtx,1,&partOffset,xy,zed,0));
}

inline bool ShapeFile::WriteVertices(double *xy, double *zed, int nvtx, int *partOffsets, int nparts) {
    return (rec() == SHPWriteVerticesEx(shp(),nvtx,nparts,partOffsets,xy,zed,0));
}


    

// a point file is a shapefile hardwired for points.

class PointFile: public ShapeFile {
  public:
    PointFile(const char * fname, open_mode_t mode = om_append): ShapeFile(fname,SHPT_POINT, mode){}
    //int WriteShape(point<double> xy);
    bool WriteShape(double x, double y);
  
    bool Open(const char *fname, open_mode_t mode = om_append) { return InternalOpen(fname, SHPT_POINT, mode); }
};

inline bool PointFile::WriteShape(double x,double y) {
    double xy[2] = { x,y };
    return WriteVertices(xy,1);
}


class LineFile: public ShapeFile {
  public:
    LineFile(const char * fname, open_mode_t mode = om_append): ShapeFile(fname, SHPT_ARC, mode){}
  
    bool Open(const char *fname, open_mode_t mode = om_append) { return InternalOpen(fname, SHPT_ARC, mode); }
};


class LineFileZ: public ShapeFile {
  public:
    LineFileZ(const char * fname, open_mode_t mode = om_append): ShapeFile(fname, SHPT_ARC_Z, mode){}
  
    bool Open(const char *fname, open_mode_t mode = om_append) { return InternalOpen(fname, SHPT_ARC_Z, mode); }
};


class PolygonFile: public ShapeFile {
  public:
    PolygonFile(const char *fname, open_mode_t mode = om_append): ShapeFile(fname,SHPT_POLYGON, mode){}
  
    bool Open(const char *fname, open_mode_t mode = om_append) { return InternalOpen(fname,SHPT_POLYGON,mode); }
};


#endif
