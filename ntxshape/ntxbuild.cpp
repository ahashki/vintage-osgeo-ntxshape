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

/*
 ntxbuild.cpp

 DESIGN NOTES
 1: scan the shapefile.  Gather up the line features,
    placing their edge data in an ntx_edge_lst, and
    their endpoint data in an ntx_end_lst.  These two
    intertwine.  The two end nodes go into the file
    with different end_idx values.  end_id is an offset
    into pEdge->pEnds.
    Theta is a function that increases with angle.
    It could be the angle itself, although if that were
    a bottleneck, it could use something simpler that
    has the same ordering.  (The angle is between the 
    end vertex and its adjacent vertex, measured in the
    direction "toward" the endpoint in all cases.)

 2: Sort this thing by usernum, x, y, and theta.  (It 
    would be best if only one usernum was processed at a 
    time, but if there are polygons from multiple themes 
    being processed at once, we should be able to handle 
    that.)

 3: Scan through the sorted table, and create a "junction"
    table.  This is basically a summary of where the first
    node with each x/y falls, and how many nodes have that
    x/y.

 4: Create a 2d array, with 2 columns and as many rows as
    there are lines.  Populate this with the offsets into
    the junction table.  This doesn't require any searching;
    it's all absolute offsets.
 
 5: Pick an arbitrary node (e.g. the first one).

    Go to the junction, and find the next entry that comes
    to the junction.  This will be the one after this node,
    or if that node is part of a different junction, wrap
    around.

    Repeat until we find that we have come back to the other
    end of the initial line.
  
    Keep track of which nodes were "entered".  These nodes
    need to be written to another table, and after the polygon
    is successfully handled, their record numbers in the node
    structure can be cleared to indicate that this node has been
    used.  (You can only "enter" a line once from each direction,
    so you can only enter each node once.  Exiting a line via
    a node doesn't count as "using" it.  The node that you
    started with has not been "used", but its opposite end,
    which you finish with, has been used.)
 
    So now we have a table that says
    { ring number, input line rec }
 
    With the ring number repeating for each line used.

 6: Repeat for another arbitrary un-used node, until all of the
    nodes have been used.  Now that array (mapping polygon recnos
    to input lines) has lots of entries.
 
 7: Create an array, with as many entries as there are output
    rings.  This is going to hold the polygon areas, as well
    as the bounding box.  Rings with negative area are
    holes.  (If everything looks like a hole, this can be
    fixed by changing the sense of the "theta" part of the
    node order.)
 
 8: The holes need to be cut out of the other polygons.
    For each poly with negative area, scan through the
    rest of the polygons looking for ones with:
    a: larger absolute area
    b: bounding box which contains the hole's bounding box.
    c: point-in-poly containing all the hole's nodes
    d: p-in-p containing all the hole's vertexes
 
    Test "e:" in that series would normally be the
    "no line cross" test, but we are assuming the input is
    topologically correct.
 
    There could be multiple matches, e.g. a lake on an island
    in a lake on an island.  In a case like this the innermost
    polygon is the one that gets the hole.  But the chances of
    this happening can be greatly reduced by processing the
    holes in order from largest (most negative area) to smallest.
    Also if in addition the outer rings are checked in order 
    from smallest to largest, the first "outer" ring encountered 
    will autmatically be the right one for the hole.
 
    As we do this, we'll be constructing a list of rings
    by polygon:
    { poly-number, ring }
 
 9: Write out the polygons to an SHP/SHX
    Since we're not supporting disjoint polygons, it is sufficient
    that the positive-area ring come first, and all of the negative
    rings come after.

 10:Labels are needed next.
 
    For each polygon:
    Scan through the points to find one which:
    a: has not been matched to another polygon
    b: falls within the bounding box
    c: is contained by this polygon's outer ring
    d: is not within any of the polygon's holes.

    - OR -
 
    For each label point:
    Scan through the polygons for one which:
    a: have not yet been matched to a label point
    b: bounding box contains the point
    c: outer shape contains the point
    d: holes do not contain the point

    I think that first way might be better since it avoids some 
    page-thrashing, as the polygon currently being handled is likely 
    to stay in memory.
 
    Either way, assuming 1 point per polygon, the results will be
    the same.  (But, come to think of it, the first way is also a
    bit better if there are duplicate labels - just arbitrarily 
    choose one rather than choosing 
 
 11:Write out the DBF attributes that go with the point that fell
    within the polygon.  If there was no point for a given record,
    just add the DBF record and don't populate it.
    
 IMPLEMENTATION NOTES:
 A: As implemented, steps 9, 10, 11 are done together.
 B: Step 10 is assisted by a simple spatial index based on a KD-tree,
    which is populated earlier by scanning through the file for labels.
    (It's done as part of step 1, rather than as a separate loop.)
 C: Step 8 performs fine without a spatial index, mainly because holes
    are uncommon.

*/








#include "ntxshape.h"

#include "dbfout.h"
#include "pntout.h"
#include "lineout.h"
#include "nameout.h"
#include "polyout.h"
#include "descattr.h"
#include "geom.h"
#include "ntxbuild.h"
#include "ntxsearch.h"
#include "ntxrecs.h"

#include <math.h>

#include <algorithm>
using namespace std;










// Although the build process isn't simple and linear like the
// other Convert... functions, I have tried to fudge it in order 
// to make a decent status bar.  The final step where the polys 
// are written seems to take about half of the total time, so 
// that gets 50%.  The other time is distributed between the 
// earlier steps to make the status bar move smoothly in my test 
// data.  It works out to:
// Load & Sort Connectivity (and labels) - 15%
// Walk the nodes - 5%
// Load the rings & calculate areas - 15%
// Associate holes with polygons - 15%
// Associate labels & write to disk - 50%.
//
// The status bar may pause or slow down for a bit in the middle
// but it's close enough.



// theta can be made more efficient, since it doesn't need
// to be the real angle.  (just needs to have the same 
// ordering as the real angle)  However, since this is not
// not a bottleneck in the least, there's no harm in doing
// it nice and carefully, usign the real angle...

static long theta(double dx, double dy) {
    double ang = 0.5; // PI-rads
    if (dx < 0) {
        ang = atan(dy/dx)/PI + 1;
    } else if (dx > 0) {
        ang = atan(dy/dx)/PI;
	if (dy < 0) {
            ang += 2;
        }
    } else if (dy < 0) {
        ang = 1.5;
    }

    return (long)(180000000 * ang); // nanodegrees
}



//ntx_get_ring assumes pRing->vtxCount has been initialized to at least big enough.
//  If not, we will segfault.  (This could be avoided, but it's simpler this way,
//  and I do happen to know the size.)

//The second pod_array passed in is for the vertices of each part of the ring.
//  Performance can be optimized by making sure that this has enough space
//  reserved to hold as any edge.  However, it will grow if needed.

template <class coord_t>
static long ntx_get_ring(NTXConverter *ntx, ntx_ring *pRing, pod_array<coord_t,2> &vertices, pod_array<coord_t,2> &work_vertices) {
    ntx_ring_part *pPart = pRing->pParts;
    coord_t *vtx = vertices.reserve(pRing->vtxCount);
    
    for (int i = 0; i < pRing->partCount; ++i, ++pPart) {
        
        ntx->GotoBookmark(pPart->pEdge->ntxBookmark);
        
        long vtxCount = ntx->ReadLineVertices(work_vertices);
        if (vtxCount > 0) {
            if (pPart->end_id == 0) {
                memcpy(vtx, work_vertices[0], vtxCount * sizeof(*vtx) * 2);
            } else {
                coord_t *xy = work_vertices[0];
                for (int j = 2*(vtxCount-1); j >= 0 ; j-=2, xy += 2) {
                    vtx[j] = xy[0];
                    vtx[j+1] = xy[1];
                }
            }
            vtx += (2*vtxCount - 2);
        } else {
            return -1;
        }
    }
    
    return (vtx + 2 - vertices[0]) / 2;
    return -1;
}


static void ring_clear_coords(ntx_ring *pRing, long ringCount) {
    for (int j = 0; j < ringCount; ++pRing, ++j) {
        free(pRing->coords);
        pRing->coords=0;
    }
}


static bool ends_meet(ntx_end *e1, ntx_end *e2) {
    return (
        (e1 && e2)
     && (e1->pEdge->userNum == e2->pEdge->userNum)
     && (e1->x == e2->x) && (e1->y == e2->y)
    );
}




static bool end_order(ntx_end const &e1, ntx_end const &e2) {
    // group the ends by theme, x, y; sort within groups by theta
  
    int order = e1.pEdge->userNum - e2.pEdge->userNum;
    if (!order) {
        order = e1.x - e2.x;
        if (!order) {
            order = e1.y - e2.y;
            if (!order) {
                order = e1.theta - e2.theta;
            }
        }
    }
    return order < 0; //for arcview order.
}
    


static bool ring_order_recno(ntx_ring const &r1, ntx_ring const &r2) {
    return (
        r1.pParts[0].pEdge->ntxBookmark.Recno() 
      < r2.pParts[0].pEdge->ntxBookmark.Recno()
    );
}

static bool ring_order_area(ntx_ring const &r1, ntx_ring const &r2) {
    // Order the rings by area, so the negative areas come first.
    // Then, break ties ordering by location.
    // (Why location?  No reason in particular; it's just expedient.)
    //
    // Area alone is enough to make the "find holes" algorithm work, really.
    // However, by trying to break ties in a deterministic way, we increase
    // the odds the records will be written in the same order from one STL 
    // sort() implementation to the next.  This enables one to do a binary 
    // comparison between the SHXs as a quick test of integrity.
  
    return (
        (r1.area < r2.area) || (
            (r1.area == r2.area) && (
                (r1.extent[0] != r2.extent[0])
              ? (r1.extent[0] < r2.extent[0])
              : (r1.extent[1] < r2.extent[1])
            )
        )
    );
}

    
static bool hole_order_owner(ntx_ring const &r1, ntx_ring const &r2) {
    // Group the rings by their containers, retaining "area order" within
    // the groups.  If two holes are the same size, it doesn't matter which
    // comes first since this doesn't affect the SHX.  (The SHPs aren't
    // going to come out identical in a binary compare anyway, because of
    // floating point round-off differences.)
    return ((r1.owner < r2.owner) || ((r1.owner == r2.owner) && (r1.area < r2.area)));
}
    
static bool label_order_size(NTXLabelSearch::label_t const*v1, NTXLabelSearch::label_t const*v2) {
      return (v1->data.size < v2->data.size);
}





int NTXConverter::ConvertPolygons(const char * shpOut, bool append) {

    ResetStatus();

    GotoRecord(0);
    if (!IsValid()) {
        return 1;
    }
    
    
    NTXPolygonFile shp(shpOut, append ? om_append : om_replace);
    
    NTXDescriptorWriter descwriter(shp);

    if (! shp.IsOpen()) {
        return 1;
    }
    
    
    int i;

    long datatype = 0;
    long startRecno;
    NTXBookmark startBookmark;

 //superdescriptor
    long themenum;
    long superflags;
    char indexkey[13];

 //descriptor
    long usernum;
    long descflags;
    char fcode   [13];
    char sourceid[13];

    long *vtx;
    ntx_point_array vertices(300);
    long vtxCount;
    long maxEdgeVertexCount = 0;

    ntx_end_lst ends(2048);
    ntx_end *pEnd, *pPrevEnd, *pNextEnd, *pOtherEnd, *pInitEnd;
    long endCount = 0;

    ntx_edge_lst edges(1024);
    ntx_edge *pEdge;

    long edgeCount = 0;
    
    ntx_end *pJuncStart = 0;

    //ntx_junc_lst junctions(512);
    //ntx_junc *pJunc = 0;
    //long juncCount = 0;

    ntx_ring_lst rings(384);
    ntx_ring *pRing = 0;
    long ringCount = 0;

    ntx_ring_part_lst parts;
    ntx_ring_part *pPart = 0;
    long partCount = 0;

    long dx,dy;

    long *xyFirst=0, *xyNext=0, *xyPrev=0, *xyLast=0;
    long thetaFirst, thetaLast;
    
    NTXLabelData   labelData(15);
    NTXDescriptor  labelDesc;
    NTXLabelSearch labelSearch;


    float progressOffset = 0;
    float progressScale = 12.5;


    NextRecord();
    UpdateStatus(progressOffset + progressScale  * GetProgress() / 100);

    while (IsValid()) {
        if (MatchThemeNumber()) {
            datatype  = GetLong(12);

            if (IsLine(datatype) && !IsSuper(datatype)) {
                labelDesc.Clear();
 
                usernum = GetLong(1);
                descflags = GetLong(11);
 
                // line bearing topology?
                // I am also checking that the delete flag is not set
                // although this is probably not needed (I would assume that 
                // CARIS does the right thing, and only marks a line as having 
                // topology if it was used as a polygon boundary.  A deleted 
                // line wouldn't do that.)
                if ((descflags & (1<<10)) && !(descflags&(1<<5))) {
                    vtxCount=0;
                    startRecno = Recno();
                    startBookmark = Bookmark();
  
                    vtxCount = ReadLineVertices(vertices);

                    if (vtxCount > 1) {
                        int vtxIdx;
   
                        vtx = vertices[0];
                        xyFirst = vertices[0];
                        //xyNext = vertices[1];
                        for (vtxIdx = 1; vtxIdx < vtxCount; ++vtxIdx) {
                            xyNext = vertices[vtxIdx];
                            if (0 != memcmp(xyNext, xyFirst, 8)) break;
                        }
   
                        xyLast =  vertices[vtxCount-1];
                        //xyPrev = vertices[vtxCount-2];
                        for (vtxIdx = vtxCount-2; vtxIdx >= 0; --vtxIdx) {
                            xyPrev = vertices[vtxIdx];
                            if (0 != memcmp(xyPrev, xyLast, 8)) {
                                break;
                            }
                        }
  
                        dx = xyNext[0] - xyFirst[0];
                        dy = xyNext[1] - xyFirst[1];
   
                        if ( dx || dy ) {
                            thetaFirst = theta(dx*xFactor,dy*yFactor);
                            dx = xyPrev[0] - xyLast[0];
                            dy = xyPrev[1] - xyLast[1];
    
                            if ( dx || dy ) {
                                thetaLast = theta(dx*xFactor,dy*yFactor);
                                pEdge = edges[edgeCount++];
                                pEdge->ntxBookmark = startBookmark;
                                pEdge->userNum = usernum;
                                pEdge->pEnds[0] = NULL;
                                pEdge->pEnds[1] = NULL;
                                pEdge->vtxCount = vtxCount;
                                //init_box(pEdge->extent, vtx, vtxCount);
                                
                                if (vtxCount > maxEdgeVertexCount) {
                                    maxEdgeVertexCount = vtxCount;
                                }
                                
                                pEnd = ends[endCount++];
                                pEnd->pEdge = NULL;
                                pEnd->flags = 0; // from end
                                pEnd->x = xyFirst[0];
                                pEnd->y = xyFirst[1];
                                pEnd->theta = thetaFirst;
                                pEnd->pNext = NULL;
     
                                pEnd = ends[endCount++];
                                pEnd->pEdge = NULL;
                                pEnd->flags = 1; // to end
                                pEnd->x = xyLast[0];
                                pEnd->y = xyLast[1];
                                pEnd->theta = thetaLast;
                                pEnd->pNext = NULL;
                                
                            } // end if good end-theta
                        } // end if good start-theta
                    } // end if got vertices
                } // end if topological
            } else if (datatype == 1007) {
                themenum = GetLong(1);
                strcpy(indexkey, GetString(2,0,12));
                superflags = GetLong(11);
                labelDesc.SetSuperDescriptor(themenum,indexkey,superflags);
            } else if (datatype == 7) {
                usernum = GetLong(1);
                strcpy(fcode, GetString(2,0,12));
                descflags = GetLong(11);
                if (MatchFcode(fcode) && (descflags&1<<11) && !(descflags&(1<<5))) {
                    if (DescLen() >= 16) {
                        strcpy(sourceid, GetString(13,0,12));
                    }
 
                    labelDesc.SetDescriptor(usernum,fcode,datatype,descflags,sourceid);
 
                    labelData.clear();
                    NTXReadType7(this, labelData);
                    labelSearch.insert(Recno(), labelDesc, labelData);
 
                } else { // fcode or descflags mismatch
                    labelDesc.Clear();
                }
            } else { // datatype match
                labelDesc.Clear();
            }
            
        } else { // theme number match
            labelDesc.Clear();
        }

        NextRecord();
        UpdateStatus(progressOffset + progressScale  * GetProgress() / 100);
    } //end while valid
    
    vertices.resize(0);
    
    progressOffset += progressScale;
    


    labelSearch.compact();

    // labels are ready to be used when we need them.
    // forget about them for now (gathering them above
    // was just an optimization to avoid another scan of
    // the file, since RAM is cheap and the labels are
    // fairly compact in memory).
    
    



    if (edgeCount <= 0) {
        return NTX_NODATA; //didn't get anything useable.
    }

    edges.resize(edgeCount);
    ends.resize(endCount);  // used to reserve room for "guard" end.
    
    
    // Also, now the ends and edges are more or less ready.
    // A little more processing is needed to index better.
    

    // First, now that the edges are all loaded, go back and
    // fill in the pEdge pointers.  (Also used to find an edge
    // that was at some extremity, but this is now commented out.)
    pEnd = ends[0];
    pEdge = edges[0];

    for (i = 0; i < edgeCount ; ++i, ++pEdge) {
        pEnd[0].pEdge = pEdge;
        pEnd[1].pEdge = pEdge;

        pEnd+=2;
    }
    

    // sort by usernum,x,y,theta.
    // theta is used when walking from one line to the next
    // usernum,x,y will define distinct junctions (usernum
    // included so that separate themes are built separately; a
    // line from theme 1002 can't connect to one in 1003, even if
    // they have the same x,y)

    pEnd = ends[0];
    sort(pEnd, pEnd + endCount, end_order);


    // Now they are sorted, but there is more processing to do.

    progressScale = 2.5;
    
    pPrevEnd = pJuncStart = NULL;
    
    for (i = 0; i < endCount ; ++i, ++pEnd) {

        //Fill in the pointers from edges to ends.
 
        pEnd->fixup_edge();
        
        //Build a circular linked list at each junction
        
        //ends_meet returns false if either end-pointer is NULL, by the way.
        if (ends_meet(pEnd, pPrevEnd)) {
            pPrevEnd->pNext = pEnd;
        } else {
            pJuncStart = pEnd;
        }
        
        //this will be overwritten, usually, but avoids a special case at the last end.
        pEnd->pNext = pJuncStart;
        
        pPrevEnd = pEnd;
 
        UpdateStatus(progressOffset+progressScale*i/endCount);
    }
    

    // pre-alloc room for the parts.
    parts.resize(endCount*2);

    progressOffset += progressScale;
    progressScale = 5;
    
    

    for (i = 0, pInitEnd = ends[0]; i < endCount ; ++i, ++pInitEnd) {
 
        if (pInitEnd->is_used()) {
            continue;
        }
 
        pPart = parts[partCount++];
        pRing = rings[ringCount++];
 
        //pPart->ring_id = ringCount; // debugging
        pPart->pEdge = pInitEnd->pEdge;
        pPart->end_id = pInitEnd->end_id();
 
        pRing->pParts = pPart;
        pRing->partCount = 1;
        pRing->userNum = pPart->pEdge->userNum;
        pRing->vtxCount = pPart->pEdge->vtxCount;
        pRing->coords = 0;
        pRing->owner = 0;
 
        pEnd = pInitEnd;
 
        pOtherEnd = pEnd->other_end();
        pNextEnd = pOtherEnd->pNext;
 
        if (pNextEnd->is_used()) {
            // bad topology - two lines with same geometry?
            partCount = pRing->pParts - parts[0];
            ringCount--;
            pRing = NULL;
            continue;
        }
 
        pEnd = pNextEnd;
        pEnd->set_used();
 
 
        while (pEnd != pInitEnd) {
            pPart = parts[partCount++];
            pPart->pEdge = pEnd->pEdge;
            pPart->end_id = pEnd->end_id();
  
            pRing->partCount++;
            pRing->vtxCount += (pPart->pEdge->vtxCount - 1);
  
            pOtherEnd = pEnd->other_end();
            pNextEnd = pOtherEnd->pNext;
  
            if (pNextEnd->is_used()) {
                // bad topology - two lines with same geometry?
             
                partCount = pRing->pParts - parts[0];
                ringCount--;
   
                pRing = NULL;
                break;
            }
            pEnd = pNextEnd;
            pEnd->set_used();
        }
 
        //if (!pRing) continue;
        UpdateStatus(progressOffset+progressScale*i/endCount);
    }

    progressOffset += progressScale;

    // ends are no longer needed.
    ends.resize(0);

    // NOTE: THE ENDS REFERENCED IN THE PARTS ARRAY ARE NOW INVALID
    
    // Now we have a set of rings, which form closed loops.
    // But some of them might be holes.  We need to calculate
    // their signed areas.  May as well keep the vertices in
    // memory after this, since they'll be needed in later
    // steps and for the output.

    
    ntx_point_array ring_vertices;
    long ring_vtxCount = 0;

    int holeCount = 0;

    progressScale = 15;

    // this is not needed, but it'll save a few reallocs.  see ntx_get_ring.
    vertices.resize(maxEdgeVertexCount);

    for (i = 0, pRing = rings[0]; i < ringCount; ++i, ++pRing) {
        ring_vtxCount = ntx_get_ring(this, pRing, ring_vertices, vertices);
        ring_vtxCount = remove_duplicate_points(ring_vertices[0], ring_vtxCount);
        
        if (ring_vtxCount > 3) {
            // greater than because first and last vertex are same, so a triangle
            // has four vertices.
            
            ring_vertices.resize(ring_vtxCount);
            pRing->coords = ring_vertices.detach();
            
            pRing->area = ring_area(pRing->coords, ring_vtxCount) * xFactor * yFactor;
            if (pRing->area < 0) {
                ++holeCount;
            }

            pRing->vtxCount = ring_vtxCount;
            
            init_box(pRing->extent, pRing->coords, pRing->vtxCount);
            
        } else {
            // degenerate ring, e.g. a non-closed line segment, where the ring
            // is formed from both sides of the edge.
            pRing->area = 0;
            pRing->coords = 0;
            pRing->vtxCount = 0;

            // There might be other degenerate cases, e.g. a degenerate polygon
            // with more than 3 vertices.  These will also have 0 area, too,
            // and will be filtered out later.
        }
 
        UpdateStatus(progressOffset+progressScale*i/ringCount);
    }

    progressOffset += progressScale;

    // bug is before this point

    // invalidate parts and edges; they are no longer needed
    // since the vertices have been collected into memory.
    parts.resize(0);
    edges.resize(0);


    sort(rings[0], rings[0]+ringCount, ring_order_area);


    // Now we have the rings in order, so the largest hole is at index 0, 
    // and the largest area is at index ringCount-1.

    // Associate the holes to the outer rings, by finding the smallest 
    // polygon that contains each hole.  It should be OK to do this
    // "which owner" search by sequential scan, with some short-circuit
    // tests, since the number of holes is likely to be small.


    progressScale = 15;

    if (holeCount > 1) { //first hole is the world poly.
        pRing = rings[0];
 
        ntx_ring *pSmallestRing = rings[holeCount];
        ntx_ring *pHole = rings[1];
        // first hole is always a hole in the world poly
        // (just an optimization; if there are other holes in
        // the world poly, they'll be skipped inside the loop)
        //
 
        for (i = 1; i<holeCount; ++i, ++pHole) {
            pRing = pSmallestRing;
            for (int j = holeCount; j < ringCount; ++pRing, ++j) {
                //degenerate polygons short-circuited here due to zero area.
                if ((pRing->userNum == pHole->userNum) && (pRing->area > (-pHole->area)) ) {
                    if (inside_box(pHole->extent,2,pRing->extent)) {
                        if (inside_ring(pHole->coords, pHole->vtxCount, pRing->coords, pRing->vtxCount)) {
                            pHole->owner = pRing;
                            break; 
                            // break because hole may only be part of 1 polygon,
                            // and we just found the right one
                        }
                    }
                }
            }
 
            UpdateStatus(progressOffset+progressScale*i/holeCount);
 
        }
 
        pHole = rings[0];
        sort(pHole, pHole + holeCount, hole_order_owner);

    }

    progressOffset += progressScale;


    // Now the holes are sorted by container, so we won't have to
    // scan through the entire list again when writing out the
    // polygons.
    
    
    
    // DELETE UN-ASSIGNED HOLES (PROBABLY HOLES IN THE WORLD POLY)
    // These are always at the start of the holes list since we just
    // sorted by owner.

    ntx_ring *pHole = rings[0];

    for (int k=0; (k<holeCount) && (pHole->owner==0); ++k, ++pHole) {
        free(pHole->coords);
        pHole->coords=0;
    }




    // It's time to associate the points with the polygons.  This is the 
    // other potentially slow part.  To make it slightly less slow, the
    // points are loaded into a search structure (that was done up in
    // the initial scan through the file, in order to avoid having to 
    // scan the file again here.



    shp_point_array shpPoints(500);

    pod_array<int> partBuf(5);
    *(partBuf[0]) = 0;

    double *outbuf;
    
    // NTXPolygonFile shp has already been created / opened.
    // This was not needed until right now, but it was opened at the start
    // to be consistent with the other output types (i.e. in case of or no
    // polygons to output, an empty shapefile should be produced).
    
    // For the same reason, NTXDescriptorWriter descwriter has been 
    // initialized and connected to shp.
    

    NTXDescriptor descNoLabel;

    ntx_ring *pFirstHole = rings[0];
    ntx_ring *pSmallestRing = pFirstHole + holeCount;
    pRing = pSmallestRing;

    double polyArea;

    // when I was writing out unassigned holes, it was:
    // pRing = pHole+1;
    // for (int j = 1; ...
    //   if (pRing->owner) continue;
    //   ...
    //   CopyCoords( ... , pRing->area < 0);
    // and hole-association and label-association were skipped

    // The polygons are written in reverse order, starting at
    // the end of the ring array, and at the end of the holes
    // array, and counting down.  This is slightly less
    // intuitive but it means the largest polygons are stored
    // first in the output shapefile.  This makes the drawing
    // appear faster.

    progressScale = 50;


    pHole = pFirstHole + holeCount - 1;
    pRing = rings[ringCount-1];
    for (int j = ringCount-1; j >= holeCount; --pRing, --j) {
        if (pRing->coords==0) {
            continue;
        }
 
        vtxCount = pRing->vtxCount;
 
        outbuf = shpPoints.reserve(vtxCount);
        partCount = 1;
 
        CopyCoords(outbuf, pRing->coords, pRing->vtxCount);
        polyArea = pRing->area;
 
 
        //skip any holes that weren't used (shouldn't be any,
        //except holes in world poly).  The pHole>=pFirstHole
        //is slightly paraniod, since the first hole always has
        //NULL as the owner (it's a hole in the world polygon).
 
        while ((pHole >= pFirstHole) && (pHole->owner > pRing)) {
            --pHole;
        }
 
        //process holes associated with this polygon
        while ((pHole >= pFirstHole) && (pHole->owner == pRing)) {
            *(partBuf[partCount++]) = vtxCount;
            vtxCount += pHole->vtxCount;
  
            shpPoints.reserve(vtxCount);
            outbuf = shpPoints[vtxCount - pHole->vtxCount];
  
            CopyCoords(outbuf, pHole->coords, pHole->vtxCount);
            polyArea += pHole->area;
  
            --pHole;
        }
 
        //note, pHole is now 1 before the start of the associated holes.
        //this is right where we want it, so we can refer to the holes
        //by partIndex using this pointer (except the 0th part which
        //is the outer ring).
        //This is also where we want to begin for the next polygon, so
        //do not change the value of pHole in the rest of this loop.
 
        // for degenerate polygons which are "all hole"?
        if (polyArea < 1E-20) {
            continue;
        }
 
 
 
        // now go find some attributes...
 
        NTXLabelSearch::label_t const * pLabel = NULL;
 
        int lblCount = labelSearch.search(pRing->userNum, pRing->extent);
 
        if (lblCount > 0) {
            if (lblCount > 1) {
                labelSearch.sortCandidates(label_order_size);
            }
  
            for (int lblIdx = 0; lblIdx < lblCount; ++lblIdx) {
                pLabel = labelSearch.candidate(lblIdx);
   
                long lx = pLabel->data.x;
                long ly = pLabel->data.y;
   
                if (point_in_ring(lx,ly, pRing->coords, pRing->vtxCount)) {
   
                 //if (point_in_ring(lx,ly, pRing->coords, pRing->vtxCount)) {
    
                    // now make sure it doesn't fall in any holes
                    for (int partIdx = 1; partIdx < partCount; ++partIdx) {
                        ntx_ring &thisHole = pHole[partIdx];
     
                        if (point_in_ring(lx,ly, thisHole.coords , thisHole.vtxCount)) {
                            pLabel = NULL;
                            break;
                        }
                    } // for each hole
    
                    if (pLabel) break;
                }
   
                pLabel = NULL;
            } // for each candidate label
        }
 
        if (pLabel) {
            const NTXLabelData &data = pLabel->data;
   
            if (shp.WriteRecord(
                shpPoints[0], vtxCount, partBuf[0],
                partCount, pLabel->recno, polyArea,
                data.dispx * xFactor + xShift,
                data.dispy * yFactor + yShift,
                data.size * yFactor, data.angle, data.keyword()
            )) {
                descwriter.Write(pLabel->desc);
            }
 
        } else { //no label
            if (shp.WriteRecord(
                shpPoints[0], vtxCount, partBuf[0],
                partCount, 0, polyArea
            )) {
                // Well, at least we can write the user number since
                // we know that from the ring.
                descNoLabel.SetDescriptor(pRing->userNum,"",0,0,"");
                descwriter.Write(descNoLabel);
            }
        }
 
        UpdateStatus(progressOffset + progressScale * (ringCount-j)/(ringCount-holeCount));
    }

    // since we detached the coords stored within the rings, we have to
    // clean them up manually.  (some have already been cleaned up,
    // but since the pointers were also set to null, calling free()
    // is safe.)

    ring_clear_coords(rings[0], ringCount);

    GotoRecord(0);
    return 0;
}


