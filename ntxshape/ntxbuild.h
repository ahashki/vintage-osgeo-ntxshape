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



#ifndef __NTXBUILD_H
#define __NTXBUILD_H



struct ntx_edge;
struct ntx_end;
struct ntx_junc;
struct ntx_ring_part;
struct ntx_ring;

struct ntx_box {
    long xmin;
    long ymin;
    long xmax;
    long ymax;
};

struct ntx_edge {
    NTXBookmark ntxBookmark;
    long userNum;
    long vtxCount;
    
    ntx_end *pEnds[2];
};

struct ntx_end {
    ntx_edge *pEdge;
    int flags;
    long x;
    long y;
    long theta;
    ntx_end *pNext;
  
    int end_id() { return flags & 1; }
    int is_used() { return flags & 2; }
    void set_used() { flags |= 2; }
    void clear_used() { flags &= ~2; }
  
    void fixup_edge() { pEdge->pEnds[flags&1] = this; }
    ntx_end * this_end() const { return pEdge->pEnds[flags&1]; }
    ntx_end *other_end() const { return pEdge->pEnds[1 - (flags&1)]; }
};


struct ntx_ring_part {
    ntx_edge *pEdge;
  
    int end_id;
  
    ntx_end * from_end() { return pEdge ? pEdge->pEnds[end_id] : 0; }
    ntx_end * other_end() { return pEdge ? pEdge->pEnds[1 - (end_id&1)] : 0; }
};

struct ntx_ring {
    ntx_ring_part *pParts;
    int userNum;
    int vtxCount;
    int partCount;
    long extent[4];
    double area;
    long *coords;
    ntx_ring *owner;  // for holes.
};

typedef pod_array<ntx_edge> ntx_edge_lst;
typedef pod_array<ntx_end>  ntx_end_lst;
typedef pod_array<ntx_junc> ntx_junc_lst;
typedef pod_array<ntx_ring_part> ntx_ring_part_lst;
typedef pod_array<ntx_ring> ntx_ring_lst;

#endif
