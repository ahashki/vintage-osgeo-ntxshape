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


#ifndef __GEOM_H
#define __GEOM_H

#include <math.h>

#ifndef PI
#define PI (3.1415926535897932384626433832795028842)
#endif


// The functions defined here will work for long or for double.
// They will also work for other types if integer constants will
// implicitly cast to your coordinate type, and if the coordinate
// type will implicitly cast to double.  You may optionally provide
// a specialized geom_traits.

template <class coord_t> struct coord_types {
    typedef coord_t wide_t;
    typedef coord_t real_t;
};
 
template <> 
struct coord_types<long long> {
    typedef long double wide_t;
    typedef long double real_t;
};

template <> 
struct coord_types<long> {
    typedef long long wide_t;
    typedef long double real_t;
};

template <> 
struct coord_types<int> {
    typedef long long wide_t;
    typedef long double real_t;
};
 
template<>
struct coord_types<short> {
    typedef long wide_t;
    typedef double real_t;
};

template<>
struct coord_types<char> {
    typedef int wide_t;
    typedef float real_t;
};


template <class coord_t> void flip_vertices(coord_t *xy, int count) {
    coord_t *xy1, *xy2;
    coord_t x;
    coord_t y;
  
    for (int i=0; i<count/2;++i) {
        xy1 = xy + i*2;
        xy2 = xy + (count-i-1)*2;
    
        x = xy1[0];
        y = xy1[1];
        xy1[0] = xy2[0];
        xy1[1] = xy2[1];
        xy2[0] = x;
        xy2[1] = y;
    }
}



template <class coord_t> void expand_box(coord_t *box, coord_t *xy, int count) {
    coord_t *px, *py;
  
    while (count--) {
        px = xy++; py = xy++;
    
        if (*px < box[0]) box[0] = *px;
        if (*px > box[2]) box[2] = *px;
    
        if (*py < box[1]) box[1] = *py;
        if (*py > box[3]) box[3] = *py;
    }
}

template <class coord_t> inline void init_box(coord_t *box, coord_t *xy, int count) {
    box[2] = box[0] = xy[0];
    box[1] = box[3] = xy[1];
    if (count > 1) {
        expand_box(box, xy+2,count-1);
    }
}



template <class coord_t> int inside_box(coord_t *xy, int count, coord_t *box) {
    coord_t *px, *py;
  
    while (count--) {
        px = xy++; py = xy++;
    
        // this was <= and >= but considering what we learned about
        // CARIS and its label placement scheme, let the perimeter of
        // the box be considered inside.
    
        if (
            (*px < box[0]) || (*px > box[2])
         || (*py < box[1]) || (*py > box[3])
        ) {
            return 0;
        }
    }
    return 1;
}

template <class coord_t> inline int inside_box(coord_t *xy, coord_t *box) {
    return !(
        (xy[0] < box[0]) || (xy[0] > box[2])
     || (xy[1] < box[1]) || (xy[1] > box[3])
    );
}


//from comp.graphics.algorithms FAQ
//int pnpoly(int npol, float *xp, float *yp, float x, float y) {
//int i, j, c = 0;
//for (i = 0, j = npol-1; i < npol; j = i++) {
//  if ((((yp[i]<=y) && (y<yp[j])) ||
//       ((yp[j]<=y) && (y<yp[i]))) &&
//      (x < (xp[j] - xp[i]) * (y - yp[i]) / (yp[j] - yp[i]) + xp[i]))
//    c = !c;
//}
//return c;
//}




/*
template <class coord_t>
inline typename coord_types<coord_t>::real_t __triangle_area_x2(coord_t*a, coord_t*b, coord_t*c) {
    return (
        multiply_coords( ( b[0] - a[0] ) , ( c[1] - a[1] ))
      - multiply_coords( ( c[0] - a[0] ) , ( b[1] - a[1] ))
    );
}
*/

template <class coord_t>
typename coord_types<coord_t>::real_t ring_area(coord_t *xy, int count) {
    typedef typename coord_types<coord_t>::wide_t wide_t; // intermediate calculations
    typedef typename coord_types<coord_t>::real_t real_t; // final result...
  
    // calculate the signed area of the ring by summing the signed
    // areas of all triangles formed between the first vertex and all
    // other pairs of adjacent vertices.
  
    count--;
  
    wide_t area = 0;
  
    coord_t * a = xy, *b = xy, *c = xy + 2;
    for (int i = 1; i < (count-1); i++) {
        // Move to the next line segment witin the ring
    
        b += 2; c += 2; // (2 because x and y are interleaved)
    
        // add the signed area of the triangle ABC, using ArcView's winding order.
        area += (
            (wide_t)(b[1] - a[1]) * (c[0] - a[0])
          - (wide_t)(b[0] - a[0]) * (c[1] - a[1])
        );
    
        // actually that calculates double the signed area.  The whole thing
        // will need to be halfed at the end.
    }
  
    return real_t(area) * 0.5;
}

template <class coord_t> inline bool points_in_line(coord_t *a, coord_t *b, coord_t*c) {
    // this only works for exactly inline - which probably never occurs due to fp rounding.
    // but computing a reasonable threshold would require looking at the lengths ab, ac, bc.
    return ((a[1] - b[1]) * (c[0]-a[0])) == ((b[0] - a[0]) * (c[1] - a[1]));
}

template <class coord_t> int find_extremity(coord_t *xy, int count) {
    int i,m;
    coord_t min[2];
  
    min[0] = xy[0];
    min[1] = xy[1];
    m = 0;
  
    for( i = 1; i < count; i++ ) {
        xy += 2;
    
        if ( (xy[0] < min[0]) || ( (xy[0]==min[0]) && (xy[1] < min[1])) ) {
            m = i;
            min[0] = xy[0];
            min[1] = xy[1];
        }
    }
    return m;
}






template <class coord_t> int point_in_ring(coord_t x, coord_t y, coord_t *ring, int count) {
    typedef typename coord_types<coord_t>::wide_t wide_t;
  
    // I'm departing from the "computational geometry faq" form here, running the infinite 
    // ray in a different direction and playing with the inequalities a bit.  I've also 
    // changed the way I think about the intersection tests, and have gotten rid of division, 
    // so there is no need to switch to floating point.
  
    // The original, which is KNOWN to be bug-free, is renamed franklin_point_in_ring.
    // This version also appears to be bug-free and seems to be more compatible with
    // CARIS's label placement algorithm (which can put a label right on the boundary
    // line.
  
    int inside = 0;
  
    // Run an imaginary ray from (x,y) to (x,+infinity).
    // Find out how many of the line segments making up the ring does this ray
    // cross. (Well, all we really care is whether the number is even or odd.)
  
  
    // prefix operator because there is 1 more vertex than there are segments
    while (--count) {
        // these correspond to components of the segment we're about to look at
    
        enum { x1,y1, x2,y2 }; // 0,1,2,3
    
        // Check whether our x falls within the segment's range [leftx,rightx)
        // Then see if a path from x to the segment's left end, to the segment's
        // right end, back to x, is clockwise.  (Notice the similarity to the
        // formula for a triangle's signed area, as used in ring_area.)  If
        // so, the ray would intersect the segment.  (The 1's and 2's are
        // reversed in the second part because there x1 is the one on the left.)
    
        if ( ((ring[x2] < x) && (x <= ring[x1]))
           ? ((wide_t)(ring[x1] - ring[x2]) * (y - ring[y2])
            < (wide_t)(ring[y1] - ring[y2]) * (x - ring[x2]))
    
           : ((ring[x1] < x) && (x <= ring[x2]) &&
             ((wide_t)(ring[x2] - ring[x1]) * (y - ring[y1])
            < (wide_t)(ring[y2] - ring[y1]) * (x - ring[x1])))
        ) {
            inside = !inside;
        }
    
        /*
        // Experiment shows that, if the test is written as shown
        // below, one returns the same results as CARIS (judging 
        // from test files containing labels which CARIS believes 
        // are in their polygons), except when the label falls on 
        // a vertex.  The version above is just slightly different, 
        // in a way that is probably also correct and might be 
        // more compatible with CARIS.  The version below remains
        // in just in case I need to revert.
    
        if ( ((ring[x2] <= x) && (x < ring[x1]))
           ? ((wide_t)(ring[x1] - ring[x2]) * (y - ring[y2])
            < (wide_t)(ring[y1] - ring[y2]) * (x - ring[x2]))
    
           : ((ring[x1] <= x) && (x < ring[x2]) &&
             ((wide_t)(ring[x2] - ring[x1]) * (y - ring[y1])
            < (wide_t)(ring[y2] - ring[y1]) * (x - ring[x1])))
        ) {
            inside = !inside;
        }
    
        */
    
        ring += 2; // move on to the next segment of the ring
    }
  
  
    return inside;
  
}




template <class coord_t> int franklin_point_in_ring(coord_t x, coord_t y, coord_t *xy, int count) {
    --count; // ignore duplicate
    int i, j, c = 0;
    coord_t *ipt, *jpt;
    for (i = 0, j = count-1; i < count; j = i++) {
        ipt = xy+i*2; jpt = xy+j*2;
        if (
            (((ipt[1]<=y) && (y<jpt[1])) || ((jpt[1]<=y) && (y<ipt[1]))) 
         && ( x < double(jpt[0] - ipt[0]) * (y - ipt[1]) / (jpt[1] - ipt[1]) + ipt[0] )
        ) {
            c = !c;
        }
    }
    return c;
}






template <class coord_t> int inside_ring(coord_t *xy, int count, coord_t *ring, int rcount) {
    for (int i=0; i<count; ++i) {
        if (!point_in_ring(xy[0],xy[1], ring, rcount)) {
            return 0;
        }
        xy+=2;
    }
    return 1;
}










template <class coord_t> long generalize_line(coord_t *xy, long numPairs, bool tight = false) {
  
    coord_t * vtx, *lastVtx, *outVtx;
  
    double dx, dy;
    double slope;
    double slopeInv;
  
  
    double lastSlope = 1;
    double lastSlopeInv = 1;
  
    double slopeRatio = 1;
    const double maxSlopeRatio = tight ? 1.0001 : 1.04;
    const double minSlopeRatio = tight ? 0.9999 : 0.96;
  
    long i = 1, outCount = 1;
  
    lastVtx = xy;
    for (i = 1; i<numPairs; i++) {
        vtx = &(xy[i*2]);
    
        dx = vtx[0] - lastVtx[0];
        dy = vtx[1] - lastVtx[1];
    
        if (dx != 0 || dy != 0) {
            slopeInv = ( (dx<0 ? -dx : dx) < (dy<0 ?-dy : dy) );
            slope = slopeInv ? (dx / dy) : (dy / dx);
      
            outVtx = lastVtx + 2;
            if (lastVtx != xy) {
                if (slopeInv == lastSlopeInv) {
                    if (lastSlope != 0) {
                        slopeRatio = slope / lastSlope;
                    } else if (slope != 0) {
                        slopeRatio = lastSlope / slope;
                    } else {
                        slopeRatio = 1;
                    }
                } else {
                    slopeRatio = slope * lastSlope;
                }
        
                if (slopeRatio > minSlopeRatio && slopeRatio < maxSlopeRatio) {
                    outVtx = lastVtx;
                }
            }
      
            if (outVtx != vtx) {
                outVtx[0] = vtx[0];
                outVtx[1] = vtx[1];
            }
      
            if (outVtx != lastVtx) {
                outCount++;
                lastSlopeInv = slopeInv;
                lastSlope = slope;
                lastVtx = outVtx;
            } else {
                vtx = lastVtx - 2;
                dx = lastVtx[0] - vtx[0];
                dy = lastVtx[1] - vtx[1];
        
                lastSlopeInv = ( (dx<0 ? -dx : dx) < (dy<0 ? -dy : dy) );
                lastSlope = lastSlopeInv ? (dx / dy) : (dy / dx);
            }
        }
    }
    return outCount;
}




// remove _adjacent_ duplicate points (first and last point are not considered adjacent.
template <class coord_t> long remove_duplicate_points(coord_t *xy, long numPairs) {
    coord_t * vtx, *lastVtx, *outVtx;
  
    long i = 1, outCount = 1;
  
    lastVtx = xy;
    for (i = 1; i<numPairs; i++) {
        vtx = &(xy[i*2]);
    
        if ((vtx[0] != lastVtx[0]) || (vtx[1] != lastVtx[1])) {
            outCount++;
      
            outVtx = lastVtx + 2;
      
            if (outVtx != vtx) {
                outVtx[0] = vtx[0];
                outVtx[1] = vtx[1];
            }
            lastVtx = outVtx;
    
        }
    }
    return outCount;
}





#endif
