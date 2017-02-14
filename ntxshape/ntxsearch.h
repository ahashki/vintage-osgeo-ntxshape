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



#ifndef __NTXSEARCH_H
#define __NTXSEARCH_H

#include "podarray.h"
#include "geom.h"
#include "pntout.h"
#include "descattr.h"


// The idea here is to manage a range search for the potential
// labels for a polygon.  I'll use a 2-d binary tree for this.
// The basic algorithm is taken from Sedgewick, but
// implementation differs in the following ways:
// 1) xy coords are an array, to trade branching for
//    pointer arithmetic in both "insert" and "range"
// 2) left and right children are an array for same reason,
//    although it only affects "insert".
// 3) instead of dynamically allocating each node, the nodes
//    are in a contiguous block of memory (managed by pod_array
//    so it can grow as needed).
// 4) Since the array might move as it grows, the children are
//    represented as offsets from the array base.
// 5) The special "outside" node representing a leaf has an
//    offset of -1, so it points before the array base.  This
//    address is not a node so it must not be dereferenced.

// The search structure manages a selected set.  This is stored
// as a second pod_array containing offsets into the first
// pod_array.
// allocate an NTXLabelSearch
// add() all the labels
// search() by box and usernum
// iterate through candidates.

class NTXLabelSearch {
  public:
    struct label_t {
        long recno;
        NTXDescriptor desc;
        NTXLabelData data;
    };

  protected:
    struct label_node: label_t {
        long child[2];
    };

    typedef pod_array<label_node> label_lst;

    pod_array<label_node> labels;
    long lblCount;

    pod_array<label_t *> matches;
    long matchCount;

    //store search terms in this object to avoid
    //stack wasteage during recursion
    label_node *srch_base;
    long srch_usernum;
    long *srch_box;

  public:
    NTXLabelSearch(int count = 100) : labels(count), lblCount(1) {
        label_node *lbl = labels[0];
        lbl->recno = -1;
        lbl->desc.usernum = -1;
        lbl->data.xy[0] = -1;
        lbl->data.xy[1] = -1;
        lbl->child[0] = -1;
        lbl->child[1] = -1;
    }

    void insert(long rec, NTXDescriptor const &desc, NTXLabelData const &data);

    int search(long usernum, long *box) {
        matchCount = 0;

        srch_base = labels[0];
        srch_usernum = usernum;
        srch_box = box;

        srch_range(srch_base, 0);

        srch_box = NULL;

        return matchCount;
    }

    void clearSearch(int freeMem = 0) {
        matchCount = 0;
        if (freeMem) {
            matches.resize(0);
        }
    }

    void sortCandidates(bool(*compare)(label_t const*,label_t const*));

    label_t const * candidate(int i) const {
        if (i < matchCount) {
            return *(matches[i]);
        }
        return 0;
    }

    int candidateCount() const { return matchCount; }

    void compact() {
        labels.resize(lblCount);
        matches.resize(matchCount);
    }

    void reset(int freeMem = 0) {
        lblCount = 1;
        matchCount = 0;
        if (freeMem) {
            compact();
        }
    }

  protected:
    void srch_range(label_node *node, int x_or_y);
};

#endif
