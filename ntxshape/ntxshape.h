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




#ifndef __NTXSHAPE_H
#define __NTXSHAPE_H

#include <stdlib.h>
#include "ntx.h"
#include "descattr.h"
#include <string.h>
#include "patmatch.h"
#include "podarray.h"

#if defined(__GNUC__) && !defined(__stdcall)
#define __stdcall __attribute__((stdcall))
#endif



extern "C" {
    // NTXStatusProc is passed the current progress %, and returns
    // the progress % at which it next wants to be called
  
    // The first argument is not meant to be used; it's there mainly to support having more than
    // one NTX file open at once.  (Although, NTXShape is not confirmed to be thread safe!)
    typedef void (__stdcall * NTXStatusProc)(float progress);
  
    typedef void (__stdcall * NTXScanDescProc)(int recno, NTXDescriptor const *desc, void*cbdata);
    typedef int (__stdcall * NTXCustomFilter)(int recno, NTXDescriptor const *desc);
}


typedef pod_array<long,2>   ntx_point_array;
typedef pod_array<double,2> shp_point_array;

enum NTXReadPosition {
    ntxReadFirst = 0,
    ntxReadNext = 1
    //ntxReadCurrent = 2; //unsupported
};


class NTXConverter: protected NTX { //protected because the relationship isn't really IS A
  public:
  
    NTXConverter(const char *fn);
    ~NTXConverter() { free(fcodeFilter); }
    bool IsValid() { return NTX::IsValid(); }
  
    // These are exposed so they can be called from VB
    float GetProgress() { return NTX::GetProgress(); }
    void GotoRecord(long newRec) { NTX::GotoRecord(newRec); }
  
    NTXBookmark Bookmark() const { return NTX::Bookmark(); }
    void GotoBookmark(const NTXBookmark &newRec) { NTX::GotoBookmark(newRec); }
  
    // these need not be public but is, temporarily, while debugging ntxbuild
    void NextRecord() { NTX::NextRecord(); }
  
    static bool IsLine(long dtype) { dtype %= 1000; return (dtype == 1) || (dtype == 3) || (dtype == 4); }
    static bool IsPoint(long dtype) { dtype %= 1000; return (dtype == 7) || (dtype == 8) || (dtype == 10) || (dtype == 11); }
    static bool IsSuper(long dtype) { return (dtype >= 1000); }
    static bool IsNoData(long value) { return (value == NTX_NODATA); }
  
    int ScanDescriptors(NTXScanDescProc callback, void *cbdata=0);
  
    int ReadDescriptor(NTXDescriptor &desc, NTXReadPosition descPos = ntxReadNext);
  
    int ConvertDescriptors(const char * dbfOut, bool append=false);
    int ConvertPoints(const char * pntOut, bool append=false);
    int ConvertLines(const char * linOut, bool append=false);
    int ConvertNames(const char * txtOut, bool append=false);
    int ConvertPolygons(const char * shpOut, bool append=false);
    int ConvertLinesZ(const char * linOut, bool append=false);
  
    //This status thing could have been done with virtual functions, I guess.
    //But that would put too much emphasis on a trivial part of the engine.
    void SetStatusCallback(NTXStatusProc Status, int frequency=100) {
        status = Status;
        statusFrequency = (frequency>0 ? frequency : 0);
        ResetStatus();
    }
  
    void SetCustomFilter(NTXCustomFilter Filter) { customFilter = Filter; }
  
    void SetThemeNumber(int theme) { themeNumFilter = (theme != 0) ? theme : NTX_NODATA; }
  
    void SetDataType(int dtype) { dataTypeFilter = DataTypeFlags(dtype); }
    void AddDataType(int dtype) { dataTypeFilter |= DataTypeFlags(dtype); }
  
    void SetFcodePattern(char const *fcode);
  
    void SetDescFlags(int flags, bool matchAny=true) { descFlagMask = flags; descFlagMatchAny = matchAny; }
    void SetExcludeDescFlags(int flags) { descFlagExcludeMask = flags; }
  
    void SetHonourGraphicLink(bool fLink) { useGraphicLink = fLink; }
  
    // I would rather that this were just protected, but the way the
    // polygon output is currently implemented, it needs to be public.
    int ReadLineVertices(ntx_point_array &vertices);
    int ReadLineVertices(shp_point_array &vertices);
    int ReadLineVertices(ntx_point_array &vertices, pod_array<long> &zvalues);
    int ReadLineVertices(shp_point_array &vertices, pod_array<double> &zvalues);
    
  protected:
    double xFactor;
    double xShift;
    double yFactor;
    double yShift;
    double zFactor;
    double zShift;
  
    void CopyCoords(double * outbuf, long *inbuf, int count);
    void CopyCoords(double * outbuf, long *inbuf, int count, int reverse);
  
  
    int themeNumFilter;
    unsigned dataTypeFilter;
    char *fcodeFilter;
  
    int descFlagMask;
    bool descFlagMatchAny;
    int descFlagExcludeMask;
  
    bool useGraphicLink;
  
    enum DataTypeFlagsEnum {
        dtfCompressline = 0x2,  //1
        dtfStreamline = 0x8,    //3
        dtfDashline = 0x10,     //4
        dtfName = 0x80,         //7
        dtfSymbol = 0x100,      //8
        dtfSounding = 0x400,    //10
        dtfSpot = 0x800,        //11
    
        dtfLine = 0x1A,
        dtfPoint = 0xD80,
    
        dtfSuper = 0x10000,
    
        dtfNoData = 0x80000000
    };
  
    static long DataTypeFlags(long dtype) {
        return (dtype > 1000) ? dtfSuper | (1 << ((dtype % 1000) & 0xF)) : (dtype > 0) ? (1 << (dtype & 0xF)) : 0;
    }
  
    void UpdateStatus(float percentDone);
  
    void UpdateStatus() { UpdateStatus(GetProgress()); }
    void ResetStatus() { nextNotify = status ? 0 : 9999; }
  
    bool MatchThemeNumber(long theme) {
        return (themeNumFilter == NTX_NODATA) || (themeNumFilter == theme) || ((themeNumFilter < 0) && (theme != -themeNumFilter));
    }
    bool MatchThemeNumber() { return MatchThemeNumber(GetLong(1)); }
  
    bool MatchDataType(long dtype) {
        return (dataTypeFilter == dtfNoData) || ( dataTypeFilter & DataTypeFlags(dtype) );
    }
  
    bool MatchDataType() { return MatchDataType(GetLong(12)); }
  
    bool MatchDescFlags(long dflag) {
        return (!(dflag & descFlagExcludeMask)) && (descFlagMask ? descFlagMatchAny ? dflag & descFlagMask : dflag & descFlagMask == descFlagMask : 1);
    }
  
    bool MatchDescFlags() { return MatchDescFlags(GetLong(11)); }
  
    bool MatchFcode(char *fcode) {
        return (!fcodeFilter) || patmatchi(fcodeFilter, fcode);
    }
  
    bool MatchCustom(long recno, NTXDescriptor &desc) {
        return (!customFilter) || customFilter(recno,&desc);
    }
  
    NTXCustomFilter customFilter;
  
    NTXStatusProc status;
    long statusFrequency;
    float nextNotify;

  private:
    //Fold ReadLineVertices' internal implementation into one template 
    //member function, rather than two functions that are mostly the
    //same... and add optional z output at the same time.  But this is 
    //an implementation detail so make it private.
    template <class coord_t> int InternalReadLineVertices(pod_array<coord_t,2> &vertices, pod_array<coord_t> *zvalues=NULL);
};


#endif
