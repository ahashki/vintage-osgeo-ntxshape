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



#include "ntxshape.h"
#include "geom.h"
#include <stdlib.h>
#include <math.h>






inline long RoundToGranularity(long coord, long grain) {
    // How are coords rounded within a compressed line?
  
    // Normal rounding works for Marta's files:
    // return long( double(coord) / grain + 0.5 ) * grain;
  
    // Normal truncation works for NBDOT's files:
    // return coord - (coord%grain);
  
    // I think maybe you round, but break the tie at 0.5 by rounding down.
    // Quick test:
    // return long( double(coord) / grain + 0.49999 ) * grain;
  
    // Yep, for those two test cases at least.  But to be more general...
  
    register long mod = coord%grain;
    return (mod * 2 <= grain) ? (coord - mod) : (coord - mod + grain);
}



void NTXConverter::CopyCoords(double * outbuf, long *inbuf, int count) {
    for (int i=0; i<count;++i) {
        outbuf[0] = inbuf[0] * xFactor + xShift;
        outbuf[1] = inbuf[1] * yFactor + yShift;
        outbuf+=2; inbuf+=2;
    }
}

void NTXConverter::CopyCoords(double * outbuf, long *inbuf, int count, int reverse) {
    if (reverse) {
        inbuf = inbuf + count*2 - 2;
        for (int i=0; i<count;++i) {
            outbuf[0] = inbuf[0] * xFactor + xShift;
            outbuf[1] = inbuf[1] * yFactor + yShift;
            outbuf+=2; inbuf-=2;
        }
    } else {
        CopyCoords(outbuf, inbuf, count);
    }
}







// let's not worry about being ultra-efficient when grabbing the nodes
// for polygon output.  just read the whole line, and reuse that code 
// in the line output.

// Remove code redundancy: this private method does the vertex extraction 
// for all four different ways to read vertices:
//   int or double, with or without z
//
// The template supports more permutations than that, but those are the
// supported ones.


// A quick template to say that double precision coords will be rescaled,
// but long coords will not.
template <class coord_t> struct vertex_coord_types;
template <> struct vertex_coord_types<long> { enum { rescale = 0 }; };
template <> struct vertex_coord_types<double> { enum { rescale = 1 }; };

template <class coord_t>
int NTXConverter::InternalReadLineVertices(pod_array<coord_t,2> &vertices, pod_array<coord_t> *zvalues) {
    enum { rescale = vertex_coord_types<coord_t>::rescale }; //shortcut
    
    AbsOffset(0);
    
    long x = NTX_NODATA, y = NTX_NODATA, z = NTX_NODATA;
  
    coord_t *vtx=0;
    coord_t *zed=0;
  
    long vtxCount = -1;
  
    long xPrev=NTX_NODATA, yPrev=NTX_NODATA, zPrev=NTX_NODATA;
    
    long zContour = NTX_NODATA;
    
    // For compressed lines:
    long xStart,yStart,xEnd,yEnd,zStart,zEnd;
    long xIncFactor,yIncFactor,zIncFactor;
    long xInc,yInc,zInc;
    
    long datatype, descflags;
    long dataHeadLen,repGrpLen,grpStart,grpOffset;
    
  
    datatype  = GetLong(12);
    if (IsLine(datatype) && !IsSuper(datatype)) {
        vtxCount=0;
    
        while(IsValid()) {
            
            if (zvalues) {
                long zmax = GetLong(9);
                long zmin = GetLong(10);
        
                zContour = (zmin == zmax) ? zmax : NTX_NODATA;
            }

            descflags = GetLong(11);
            
            AbsOffset(descLen);
      
            dataHeadLen = GetLong(0);
            repGrpLen = GetLong(1);
            
            bool ntxHasZ = (repGrpLen >= 3) && (descflags & (1<<13));
            
            if (datatype!=1) {
                
                z = ntxHasZ ? NTX_NODATA : zContour;
        
                grpStart = descLen+dataHeadLen;
        
                // If vtxCount>0 that means we're joining many lines.
                // In this case the last vertex of the previous line is the same
                // as the first vertex of the next line.  So I was removing it here by
                // dropping the vertex count by one.
        
                //if (vtxCount > 0) --vtxCount;
        
                // HOWEVER I later added a test to remove adjacent duplicate vertices
                // within a line (in case of digitising errors).  When I dropped the last
                // vertex, I neglected to reset the xPrev and yPrev, so both the vertex
                // from the end of the previous line, and the vertex from the start of the
                // next line, were lost.
        
                for(;;) {
                    AbsOffset(grpStart);
          
                    x = GetLong(0);
                    if (x==NTX_NODATA) {
                        break;
                    }
          
                    y = GetLong(1);
                    
                    if (zvalues && ntxHasZ) {
                        z = GetLong(2); // otherwize stay at zContour or nodata
                    }
          
                    if ((x!=xPrev)||(y!=yPrev)||(z!=zPrev)) {
                        vtx = vertices[vtxCount];
                        if (!vtx) {
                            return -1;
                        }
                        
                        if (rescale) {
                            vtx[0] = (coord_t) (x * xFactor + xShift);
                            vtx[1] = (coord_t) (y * yFactor + yShift);
                        } else {
                            vtx[0] = (coord_t) x;
                            vtx[1] = (coord_t) y;
                        }
                        
                        
                        xPrev = x;
                        yPrev = y;
                        
                        if (zvalues) {
                            zed = (*zvalues)[vtxCount];
                            if (!zed) {
                                return -1;
                            }
                            
                            if (!rescale) {
                                *zed = (coord_t) z;
                            } else if (z!=NTX_NODATA) {
                                *zed = (coord_t) (z * zFactor + zShift);
                            } else {
                                // I tried NaN, but ArcMap didn't seem to handle it properly.
                                // Probably the best thing is to rescale NTX_NODATA as 0,
                                // even though that could be ambiguous.
                                *zed = (coord_t) 0;
                            }
                            
                            zPrev = z;
                        }
                        
                        ++vtxCount;
                    }
          
                    grpStart += repGrpLen;
                }
            } else {
                
                // type 1 line - compressed
      
                xStart = GetLong(4);
                yStart = GetLong(5);
        
                xEnd = GetLong(6);
                yEnd = GetLong(7);
        
                xIncFactor = (dataHeadLen > 8) ? GetLong(8) : 1;
                yIncFactor = (dataHeadLen > 9) ? GetLong(9) : 1;
                
                if (dataHeadLen > 12 && zvalues && ntxHasZ) {
                    zStart = GetLong(11);
                    zEnd = GetLong(12);
                    zIncFactor = (dataHeadLen > 13) ? GetLong(13) : 1;
                } else {
                    zStart = zEnd = zContour; // which is nodata if z was not constant.
                    zIncFactor = 0;
                }
                
        
                // In compressed mode, I just don't bother to write the first
                // vertex (from the data header) except for the first line in
                // the series.  So I didn't have the same error here as I had
                // with non-compressed lines
        
                if (vtxCount == 0) {
                    vtx = vertices[0];
                    if (!vtx) {
                        return -1;
                    }
                        
                    if (rescale) {
                        vtx[0] = (coord_t) (xStart * xFactor + xShift);
                        vtx[1] = (coord_t) (yStart * yFactor + yShift);
                    } else {
                        vtx[0] = (coord_t) xStart;
                        vtx[1] = (coord_t) yStart;
                    }
                    
                    if (zvalues) {
                        zed = (*zvalues)[vtxCount];
                        if (!zed) {
                            return -1;
                        }
                        
                        if (!rescale) {
                            *zed = (coord_t) zStart;
                        } else if (zStart!=NTX_NODATA) {
                            *zed = (coord_t) (zStart * zFactor + zShift);
                        } else {
                            *zed = (coord_t) 0;
                        }
                    }
                    
                    vtxCount = 1;
                }
        
                // OK.  Here's the catch.  CARIS doesn't make the
                // offsets relative to xStart@yStart.  Instead,
                // they are relative to the nearest coordinate in
                // the grid defined by xIncFactor, with ties broken
                // in favour of rounding DOWN.
        
                // Also, the last compressed coord is irrelevant because it
                // is replaced by xEnd@yEnd.
        
                x = RoundToGranularity(xStart,labs(xIncFactor));
                y = RoundToGranularity(yStart,labs(yIncFactor));
                
                //zIncFactor implies ntxHasZ; see about 50 lines up.
                z = zIncFactor ? RoundToGranularity(zStart,labs(zIncFactor)) : zStart;
        
                AbsOffset(descLen + dataHeadLen);
        
                grpOffset = 0;
                for (;;) {
                    if (grpOffset & 1) { //odd (probably never happens)
                        xInc = GetChar(0,grpOffset);
                        yInc = GetChar(0,grpOffset+1);
                    } else {
                        char *xyPair = GetCharPair(0,grpOffset);
                        xInc = xyPair[0];
                        yInc = xyPair[1];
                    }
                    
                    if (xInc <= -128) {
                        break;
                    }
                    
                    x += (xInc * xIncFactor);
                    y += (yInc * yIncFactor);

                    if (zIncFactor) { // zIncFactor implies ntxHasZ
                        zInc = GetChar(0,grpOffset+2);
                        z += (zInc * zIncFactor);
                    }
          
                    vtx = vertices[vtxCount];
                    if (!vtx) {
                        return -1;
                    }
          
                    if (rescale) {
                        vtx[0] = (coord_t) (x * xFactor + xShift);
                        vtx[1] = (coord_t) (y * yFactor + yShift);
                    } else {
                        vtx[0] = (coord_t) x;
                        vtx[1] = (coord_t) y;
                    }
                    
                    if (zvalues) {
                        zed = (*zvalues)[vtxCount];
                        
                        if (!zed) {
                            return -1;
                        }

                        if (!rescale) {
                            *zed = (coord_t) z;
                        } else if (z!=NTX_NODATA) {
                            *zed = (coord_t) (z * zFactor + zShift);
                        } else {
                            *zed = (coord_t) (0);
                        }
                    }
                    
                    ++vtxCount;
                    
                    grpOffset += repGrpLen;
                }
          
                if (grpOffset == 0) {
                    // There were no groups??  I doubt that ever can happen, 
                    // but if it did, we need to grow the buffers to make room
                    // for the end coordinate (normally, there's already space
                    // since I just overwrite the last compressed coordinate).
                    
                    vtx = vertices[vtxCount];
                    if (!vtx) {
                        return -1;
                    }
                    
                    if (zvalues) {
                        zed = (*zvalues)[vtxCount];
                        if (!zed) {
                            return -1;
                        }
                    }
                    
                    ++vtxCount;
                }
          
                if (rescale) {
                    vtx[0] = (coord_t) (xEnd * xFactor + xShift);
                    vtx[1] = (coord_t) (yEnd * yFactor + yShift);
                } else {
                    vtx[0] = (coord_t) xEnd;
                    vtx[1] = (coord_t) yEnd;
                }
                    
                if (zvalues) {
                    if (!rescale) {
                        *zed = (coord_t) zEnd;
                    } else if (zEnd!=NTX_NODATA) {
                        *zed = (coord_t) (zEnd * zFactor + zShift);
                    } else {
                        *zed = (coord_t) (0);
                    }
                }
            }
      
            if (! (useGraphicLink && (descflags & 1)) ) {
                break;
            }
            NextRecord();
        }
  
    }
    return vtxCount;
}


int NTXConverter::ReadLineVertices(ntx_point_array & vertices) {
    return InternalReadLineVertices(vertices);
}

int NTXConverter::ReadLineVertices(shp_point_array & vertices) {
    return InternalReadLineVertices(vertices);
}

int NTXConverter::ReadLineVertices(ntx_point_array & vertices, pod_array<long> &zvalues) {
    return InternalReadLineVertices(vertices, &zvalues);
}

int NTXConverter::ReadLineVertices(shp_point_array & vertices, pod_array<double> &zvalues) {
    return InternalReadLineVertices(vertices, &zvalues);
}
