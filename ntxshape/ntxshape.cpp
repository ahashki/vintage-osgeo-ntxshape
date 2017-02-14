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

//NTXSHAPE.CPP

#include "ntxshape.h"

#include "ntxrecs.h"
#include "dbfout.h"
#include "pntout.h"
#include "lineout.h"
#include "nameout.h"
#include "descattr.h"

#include <math.h>
#include <stdlib.h>

#define NTX_GENERALIZE_NAMES 1


#if (NTX_GENERALIZE_NAMES||NTX_GENERALIZE_LINES)
#include "geom.h"
#endif


static double GetF24(NTX * ntx, long offset) {
    char s[25];
    strcpy(s,ntx->GetString(offset,0,24));
  
    char * findD = strrchr(s,'D');
    if (findD) {
        *findD = 'E';
    }
  
    return atof(s);
}







NTXConverter::NTXConverter(const char *fn)
  : NTX(fn),
    themeNumFilter(NTX_NODATA), 
    dataTypeFilter(dtfNoData), 
    fcodeFilter(NULL),
    descFlagMask(0),
    descFlagMatchAny(false),
    descFlagExcludeMask(1<<5),
    useGraphicLink(true),
    customFilter(NULL),
    status(0),
    nextNotify(9999)
{
    if (IsValid()) {
        xFactor = GetF24(this,28);
        xShift  = GetF24(this,34);
        yFactor = GetF24(this,40);
        yShift  = GetF24(this,46);
        zFactor = GetF24(this,52);
        zShift  = GetF24(this,58);
    }
}



void NTXConverter::SetFcodePattern(char const *fcode) {
    if (fcode && *fcode) {
        fcodeFilter = (char *)realloc(fcodeFilter, strlen(fcode) + 1);
        if (fcodeFilter) {
            strcpy(fcodeFilter,fcode);
        }
    } else {
        free(fcodeFilter);
        fcodeFilter = NULL;
    }
}



void NTXConverter::UpdateStatus(float percentDone) {
    if (status) {
        percentDone = (percentDone>100) ? 100 : (percentDone<0) ? 0 : percentDone;
    
        if (statusFrequency>0) {
            while (percentDone>=nextNotify) {
                status(nextNotify);
                nextNotify += (100.0 / statusFrequency);
            }
        } else {
            status(percentDone);
            nextNotify = percentDone;
        }
    }
}





int NTXConverter::ConvertPoints(const char * shpOut, bool append) {
    ResetStatus();
  
    void (*grpProc)(NTX *,short,NTXPointData&);
  
    GotoRecord(0);
  
    if (!IsValid()) {
        return 1;
    }
  
  
    NTXPointFile shp(shpOut, append ? om_append : om_replace);
    
    NTXDescriptorWriter descwriter(shp);
  
    if (!shp.IsOpen()) {
        return 1;
    }

    
    NTXDescriptor desc;
    NTXPointData data(32);
    
    long ntxSize;
  
    double x,y,dispx,dispy;
    double elevation=0;
  
    double size=0;
    char keyword[33];
    memset(keyword,' ',32);
    keyword[32] = '\0';
  
    //superdescriptor
    long themenum;
    long superflags;
    char indexkey[13];
  
    //descriptor
    long usernum;
    long descflags;
    char fcode   [13];
    char sourceid[13];
  
    long datatype = 0;
  
    int dataHeadLen = 0;
    int repGrpLen   = 0;
    long scale;
  
  
    NextRecord();
    UpdateStatus();
  
    while (IsValid()) {
        if (MatchThemeNumber()) {
      
            datatype  = GetLong(12);
      
            if (IsPoint(datatype) && MatchDataType(datatype)) {
                if (IsSuper(datatype)) {
                    themenum = GetLong(1);
                    strcpy(indexkey, GetString(2,0,12));
                    superflags = GetLong(11);
                    desc.SetSuperDescriptor(themenum,indexkey,superflags);
                } else {
        
                    usernum = GetLong(1);
                    strcpy(fcode, GetString(2,0,12));
          
                    descflags = GetLong(11);
          
                    if (MatchFcode(fcode) && MatchDescFlags(descflags)) {
            
                        if (DescLen() >= 16) {
                            strcpy(sourceid, GetString(13,0,12));
                        }
            
                        desc.SetDescriptor(usernum,fcode,datatype,descflags,sourceid);
            
                        if (MatchCustom(Recno(), desc)) {
                            AbsOffset(descLen);
                            dataHeadLen = GetLong(0);
                            scale = dataHeadLen > 2 ? GetLong(2) : 0;
              
                            if (datatype == 7) {
                                data.clear();
                
                                NTXReadType7(this,data); //ntxX,&ntxY,&ntxDispX,&ntxDispY,&ntxSize,&angle,keyword,32);
                
                                x = data.x * xFactor + xShift;
                                y = data.y * yFactor + yShift;
                
                                dispx = data.dispx * xFactor + xShift;
                                dispy = data.dispy * xFactor + xShift;
                
                                size = data.size * yFactor;
                
                                shp.WriteRecord(x,y,Recno(),0,0,dispx,dispy,size,data.angle,scale,data.keyword());
                                descwriter.Write(desc);
                
                            } else {
              
                                repGrpLen = GetLong(1);
                
                                // Initialize the optional fields, in case they're
                                // not present in the repeating groups.
                                data.clear();
                
                                if (datatype==8) {
                                    grpProc = NTXReadType8Group;
                                } else if (datatype==10) {
                                    grpProc = NTXReadType10Group;
                                } else {
                                    grpProc = NTXReadType11Group;
                  
                                    // Soundings have a size in the header.  Use it.
                                    if (dataHeadLen>5) {
                                        ntxSize = GetLong(5);
                                    }
                                }
                
                
                                RelOffset(dataHeadLen);
                
                                while( NTX_NODATA != GetLong(0) ) {
                                    grpProc(this,repGrpLen,data);
                  
                                    x = data.x * xFactor + xShift;
                                    y = data.y * yFactor + yShift;
                  
                                    dispx = data.dispx * xFactor + xShift;
                                    dispy = data.dispy * yFactor + yShift;
                  
                                    size = data.size * yFactor;
                  
                                    elevation = (data.elev != NTX_NODATA) ? double(data.elev) * zFactor + zShift : -9999;
                  
                                    shp.WriteRecord(x,y,Recno(), elevation, data.flags, dispx,dispy,size,data.angle,scale,data.keyword());
                                    descwriter.Write(desc);
                                }
                            }
                        } else {
                            desc.Clear();
                        }
                    } else { //fcode does not match
                        desc.Clear();
                    }
                } //end branch super vs normal
      
            } else { //datatype does not match
                desc.Clear();
            }
        }
    
        NextRecord();
        UpdateStatus();
    }
  
    GotoRecord(0);
    UpdateStatus(100);
    return 0;
}




















int NTXConverter::ConvertLines(const char * shpOut, bool append) {
    ResetStatus();
  
    GotoRecord(0);
    if (!IsValid()) {
        return 1;
    }
  
  
    NTXLineFile shp(shpOut, append ? om_append : om_replace);
  
    NTXDescriptorWriter descwriter(shp);
    
    if (!shp.IsOpen()) {
        return 1;
    }

    
    NTXDescriptor desc;
  
    shp_point_array vertices(300);
    double * vtx;
    long vtxCount = 0;
  
    long datatype = 0;
    long dataHeadLen=0;
    long startRecno;
  
    //superdescriptor
    long themenum;
    long superflags;
    char indexkey[13];
  
    //descriptor
    long usernum;
    long descflags;
    char fcode   [13];
    char sourceid[13];
  
  
  
    long elev=0;
    double elevation=0;
    double lineweight = 0;
    long scale;
  
    NextRecord();
    UpdateStatus();
  
    while (IsValid()) {
    
        if (MatchThemeNumber()) {
      
            datatype  = GetLong(12);
            if (IsLine(datatype) && MatchDataType(datatype)) {
                if (IsSuper(datatype)) {
        
                    themenum = GetLong(1);
                    strcpy(indexkey, GetString(2,0,12));
                    superflags = GetLong(11);
                    desc.SetSuperDescriptor(themenum,indexkey,superflags);
        
                } else {
        
                    usernum = GetLong(1);
                    strcpy(fcode, GetString(2,0,12));
          
                    descflags = GetLong(11);
          
                    if (MatchFcode(fcode) && MatchDescFlags(descflags)) {
          
                        if (DescLen() >= 16) {
                            strcpy(sourceid, GetString(13,0,12));
                        }
            
                        desc.SetDescriptor(usernum,fcode,datatype,descflags,sourceid);
                        if (MatchCustom(Recno(), desc)) {
                            elev = GetLong(9);
              
                            elevation = (elev != NTX_NODATA) ? double(elev) * zFactor + zShift : -9999;
              
                            vtxCount=0;
                            startRecno = Recno();
              
                            descflags = GetLong(11);
                            AbsOffset(descLen);
              
                            scale = (dataHeadLen > 2) ? GetLong(2) : 0;
                            lineweight = (dataHeadLen > 3) ? GetLong(3) : 0;
              
                            vtxCount = ReadLineVertices(vertices);
              
                            if (vtxCount > 0) {
                                vtx = vertices[0];
                
#if NTX_GENERALIZE_LINES
                                vtxCount = generalize_lines(vtx,vtxCount);
#endif
                
                                shp.WriteRecord(vtx,vtxCount,startRecno,elev,lineweight,scale);
                                descwriter.Write(desc);
                
                                // if we joined together multiple records, the superdesc needs to
                                // be discarded because all the records for which it was
                                // valid have been processed by ReadLineVertices.
                                if (useGraphicLink) {
                                    desc.Clear();
                                }
                            }
                        } else {
                            desc.Clear();
                        }
                    } else { //fcode no match
                        desc.Clear();
                    }
                  } // end branch superdesc vs. line
        
              } else { //datatype no match
                desc.Clear();
            }
      
        }
    
        NextRecord();
        UpdateStatus();
    }
  
    GotoRecord(0);
    UpdateStatus(100);
    return 0;
}







// This is a copy/paste of ConvertLines so there should be ample
// opportunity for refactoring later (even more than in the others)

int NTXConverter::ConvertLinesZ(const char * shpOut, bool append) {
    ResetStatus();
  
    GotoRecord(0);
    if (!IsValid()) {
        return 1;
    }
  
  
    NTXLineFileZ shp(shpOut, append ? om_append : om_replace);
  
    NTXDescriptorWriter descwriter(shp);
    
    if (!shp.IsOpen()) {
        return 1;
    }

    
    NTXDescriptor desc;
  
    shp_point_array vertices(300);
    pod_array<double> zvalues(300);
    
    double * vtx;
    double * zed;
    long vtxCount = 0;
  
    long datatype = 0;
    long dataHeadLen=0;
    long startRecno;
  
    //superdescriptor
    long themenum;
    long superflags;
    char indexkey[13];
  
    //descriptor
    long usernum;
    long descflags;
    char fcode   [13];
    char sourceid[13];
  
  
  
    long elev=0;
    double elevation=0;
    double lineweight = 0;
    long scale;
  
    NextRecord();
    UpdateStatus();
  
    while (IsValid()) {
    
        if (MatchThemeNumber()) {
      
            datatype  = GetLong(12);
            if (IsLine(datatype) && MatchDataType(datatype)) {
                if (IsSuper(datatype)) {
        
                    themenum = GetLong(1);
                    strcpy(indexkey, GetString(2,0,12));
                    superflags = GetLong(11);
                    desc.SetSuperDescriptor(themenum,indexkey,superflags);
        
                } else {
        
                    usernum = GetLong(1);
                    strcpy(fcode, GetString(2,0,12));
          
                    descflags = GetLong(11);
          
                    if (MatchFcode(fcode) && MatchDescFlags(descflags)) {
          
                        if (DescLen() >= 16) {
                            strcpy(sourceid, GetString(13,0,12));
                        }
            
                        desc.SetDescriptor(usernum,fcode,datatype,descflags,sourceid);
                        if (MatchCustom(Recno(), desc)) {
                            elev = GetLong(9);
              
                            elevation = (elev != NTX_NODATA) ? double(elev) * zFactor + zShift : -9999;
              
                            vtxCount=0;
                            startRecno = Recno();
              
                            descflags = GetLong(11);
                            AbsOffset(descLen);
              
                            scale = (dataHeadLen > 2) ? GetLong(2) : 0;
                            lineweight = (dataHeadLen > 3) ? GetLong(3) : 0;
              
                            vtxCount = ReadLineVertices(vertices, zvalues);
              
                            if (vtxCount > 0) {
                                vtx = vertices[0];
                                zed = zvalues[0];
                
                                shp.WriteRecord(vtx,zed,vtxCount,startRecno,elev,lineweight,scale);
                                descwriter.Write(desc);
                
                                // if we joined together multiple records, the superdesc needs to
                                // be discarded because all the records for which it was
                                // valid have been processed by ReadLineVertices.
                                if (useGraphicLink) {
                                    desc.Clear();
                                }
                            }
                        } else {
                            desc.Clear();
                        }
                    } else { //fcode no match
                        desc.Clear();
                    }
                  } // end branch superdesc vs. line
        
              } else { //datatype no match
                desc.Clear();
            }
      
        }
    
        NextRecord();
        UpdateStatus();
    }
  
    GotoRecord(0);
    UpdateStatus(100);
    return 0;
}











int NTXConverter::ConvertNames(const char * shpOut, bool append) {
    ResetStatus();
  
  
    GotoRecord(0);
  
    if (!IsValid()) {
        return 1;
    }
  
  
    NTXNameFile shp(shpOut, append ? om_append : om_replace);
  
    NTXDescriptorWriter descwriter(shp);
  
    if (!shp.IsOpen()) {
        return 1;
    }

    
    NTXDescriptor desc;
  
    long ntxX,ntxY,ntxRefX,ntxRefY;
    long ntxSize;
  
    double refX,refY;
    long font = 0;
    int grpStart;
  
  
  
    double size=0;
    char text[MAX_NAME_LEN+1];
    memset(text, ' ', MAX_NAME_LEN);
    text[MAX_NAME_LEN] = '\0';
  
    double vertices[MAX_NAME_LEN * 2][2];
    double *vtx;
    int vtxCount;

    pod_array<int> partBuf(5);
    *(partBuf[0]) = 0;
    int partCount = 1;

    char *textChar;
    char *textEnd = text+MAX_NAME_LEN;
  
    //superdescriptor
    long themenum;
    long superflags;
    char indexkey[13];
  
    //descriptor
    long usernum;
    long descflags;
    char fcode   [13];
    char sourceid[13];
  
    long datatype = 0;
  
    int dataHeadLen = 0;
    int repGrpLen   = 0;
    long scale = 0;
  
    NextRecord();
    UpdateStatus();
  
    while (IsValid()) {
        if (MatchThemeNumber()) {
      
            datatype  = GetLong(12);
      
            if (datatype == 1007) {
                themenum = GetLong(1);
                strcpy(indexkey, GetString(2,0,12));
                superflags = GetLong(11);
                desc.SetSuperDescriptor(themenum,indexkey,superflags);
            } else if (datatype == 7) {
                usernum = GetLong(1);
                strcpy(fcode, GetString(2,0,12));
        
                descflags = GetLong(11);
        
                if (MatchFcode(fcode) && MatchDescFlags(descflags)) {
          
                    if (DescLen() >= 16) {
                        strcpy(sourceid, GetString(13,0,12));
                    }
          
                    desc.SetDescriptor(usernum,fcode,datatype,descflags,sourceid);
          
                    if (MatchCustom(Recno(), desc)) {
            
                        AbsOffset(descLen);
            
                        dataHeadLen = GetLong(0);
                        repGrpLen = GetLong(1);
                        scale = dataHeadLen > 2 ? GetLong(2) : 0;
                        font = dataHeadLen > 4  ? GetLong(4) : 0;
                        ntxSize = dataHeadLen>5 ? GetLong(5) : 0;
            
            
                        if (dataHeadLen>7) {
                            ntxRefX = GetLong(6);
                            ntxRefY = GetLong(7);
                        } else {
                            RelOffset(dataHeadLen);
                            ntxRefX = GetLong(0);
                            ntxRefY = GetLong(1);
                        }
            
                        refX = double(ntxRefX) * xFactor + xShift;
                        refY = double(ntxRefY) * yFactor + yShift;
                        size = ntxSize * yFactor;
            
                        grpStart = descLen+dataHeadLen;
                        vtxCount=0;

                        double lastX = 0;
                        double lastY = 0;
                        double lastOrientationX = 0;
                        double lastOrientationY = 0;

                        // I don't think size will ever be negative or 0 on names, but be safe.
                        double inlineDistance = (size > 1E-20) ? size : 1;
            
                        for(textChar = text; textChar < textEnd; textChar++) {
                            AbsOffset(grpStart);
              
                            ntxX = GetLong(0);
                            if (ntxX==NTX_NODATA) {
                                break;
                            }
              
                            ntxY = GetLong(1);
              
                            vtx = vertices[vtxCount++];
                            vtx[0] = double(ntxX) * xFactor + xShift;
                            vtx[1] = double(ntxY) * yFactor + yShift;
                            
                            if (vtxCount > 1) { // need to prime lastX/lastY etc. with the first character.
                                // analyse position of this character vs. last character, compared to the orientation of
                                // the last character.  if the next character isn't more or less in line with the direction 
                                // indicated for the current one, assume it is a new-line.
                                
                                double dx = vtx[0] - lastX;
                                double dy = vtx[1] - lastY;
                                
                                // use dot product for this - a negative dot product indicates an abrupt change in direction
                                // two characters in a row with the same xy would have 0/0 in dx/dy which results in a dot
                                // product of 0 and is not counted as a new line.
                                
                                if ((dx*lastOrientationX + dy*lastOrientationY) < 0) {
                                    // insert trailing point.
                                    vertices[vtxCount][0] = vtx[0];
                                    vertices[vtxCount][1] = vtx[1];
                                    
                                    vtx[0] = lastX + lastOrientationX * inlineDistance;
                                    vtx[1] = lastY + lastOrientationY * inlineDistance;
                                    
                                    if (textChar+1 == textEnd) { // not enough room for the buffer to continue.
                                        break;
                                    }
                                    
                                    vtx = vertices[vtxCount++]; // already populated above
                                    
                                    // starting another line.
                                    *(textChar++) = '\n';
                                    
                                    *(partBuf[partCount++]) = vtxCount - 1;
                                }
                            }
                            
                            *textChar = GetChar(2,2);

                            lastX = vtx[0];
                            lastY = vtx[1];
                            
                            double angle = GetShort(2,0) * PI / 10800.0; // minutes to rads
                            
                            lastOrientationX = cos(angle);
                            lastOrientationY = sin(angle);
                            
                            grpStart += repGrpLen;
                        }
            
                        *textChar = '\0';
                        
                        if (vtxCount > 0) {
                            vtx = vertices[vtxCount++];
                            vtx[0] = lastX + lastOrientationX * inlineDistance;
                            vtx[1] = lastY + lastOrientationY * inlineDistance;
                        }
                        
                        //vtx = vertices[0];
                        int *parts = partBuf[0];
            
#if NTX_GENERALIZE_NAMES
                        // This had to get more complicated to deal with multi-part lines and with the
                        // trailing vertices generated via code, to make sure they don't skew anything
                        
                        int partEnd = vtxCount;
                        for (int partIndex = partCount - 1; partIndex >= 0; --partIndex) {
                            int partStart = parts[partIndex];
                            
                            int beforeGeneralizeCount = partEnd - partStart;
                            
                            if (beforeGeneralizeCount > 2) {
                                vtx = vertices[partStart];
                                
                                //int afterGeneralizeCount = generalize_line(vtx, beforeGeneralizeCount, true);
                                
                                // ignore the generated final vertex for this first pass
                                int afterGeneralizeCount = generalize_line(vtx, beforeGeneralizeCount - 1) + 1;
                                
                                // put the generated final vertex back in place - this is handled separately from
                                // the loop below to allow it to be generalized out (with tighter tolerances)
                                if (afterGeneralizeCount < beforeGeneralizeCount) {
                                    double *vtxSrc = vertices[partStart + beforeGeneralizeCount - 1];
                                    double *vtxDest =  vertices[partStart + afterGeneralizeCount - 1];
                                    vtxDest[0] = vtxSrc[0];
                                    vtxDest[1] = vtxSrc[1];
                                }
                                
                                afterGeneralizeCount += generalize_line(vertices[partStart + afterGeneralizeCount - 3], 3, true) - 3;
                                
                                int removedVtxCount = beforeGeneralizeCount - afterGeneralizeCount;

                                if (removedVtxCount > 0) {
                                    // move rest of vertices and part offsets back.
                                    for (int vtxToMove = partEnd; vtxToMove < vtxCount; ++vtxToMove) {
                                        double *vtxDest = vertices[vtxToMove - removedVtxCount];
                                        vtxDest[0] = vertices[vtxToMove][0];
                                        vtxDest[1] = vertices[vtxToMove][1];
                                    }
                                    for (int partToMove = partIndex+1; partToMove < partCount; ++partToMove) {
                                        parts[partToMove] -= removedVtxCount;
                                    }
                                    
                                    vtxCount -= removedVtxCount;
                                }
                                    
                            }
                            
                            partEnd = partStart;
                        }
                        
#endif
            
                        shp.WriteRecord(vertices[0],vtxCount, parts, partCount, Recno(),font,size,refX,refY,scale,text);
                        descwriter.Write(desc);
                    } else {
                        desc.Clear();
                    }
                } else { //fcode does not match
                    desc.Clear();
                }
            } else { //datatype does not match
                desc.Clear();
            }
        }
    
        NextRecord();
        UpdateStatus();
    
  
    }
  
    GotoRecord(0);
    UpdateStatus(100);
    return 0;
}
