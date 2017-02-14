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

#include "ntxsearch.h"

#include <new>
#include <algorithm>

using namespace std;


void NTXLabelSearch::insert(
    long rec,
    NTXDescriptor const &desc,
    NTXLabelData const &data
) {
    int newIdx = lblCount++;
    label_node *lbl = labels[newIdx];

    lbl->recno = rec;
  
 
    // Now that I am using a standard-compliant compiler
    // I should go back and excise this garbage in favor
    // of STL, but for now.. placement new instead of
    // assignment operator.
    new (&(lbl->desc)) NTXDescriptor(desc);
    new (&(lbl->data)) NTXLabelData(data);
 
    lbl->child[0] = -1;
    lbl->child[1] = -1;
 
    label_node *pLabels = labels[0];
    label_node *nullNode = pLabels - 1;
    label_node *node = pLabels;
    label_node *parent = nullNode;
 
    int x_or_y = 0;  // (x)
    int lbl_less;
 
    // average depth to reach a node may give an idea whether the
    // tree is healthy.
    // int depth = 0;
 
    do {
        lbl_less = (lbl->data.xy[x_or_y] < node->data.xy[x_or_y]);
        parent = node;
        node = pLabels + node->child[!lbl_less];
        x_or_y = !x_or_y;
    } while (node != nullNode);
 
    parent->child[!lbl_less] = newIdx;
 
    //if other child is empty, the parent used to be a leaf but is now unbalanced.
    //otherwise, the parent used to be unbalanced but is now complete.
    //in either case the just-inserted label starts out as a leaf node.
}


// IMPORTANT: If compiling with Symantec C++, optimize for space,
// not speed.  Some versions of SC (including DM8.22) are overly
// agressive and will optimize this recursive function to the point
// where it crashes.

void NTXLabelSearch::srch_range(label_node *node, int x_or_y) {
 
    if (node >= srch_base) {
  
        if (srch_box[x_or_y] < node->data.xy[x_or_y]) {
            srch_range(srch_base + node->child[0], !x_or_y);
        }
  
        if ((srch_usernum == node->desc.usernum) && inside_box(node->data.xy, srch_box)) {
            *(matches[matchCount++]) = node;
        }
  
        if (node->data.xy[x_or_y] <= srch_box[x_or_y + 2]) {
            srch_range(srch_base + node->child[1], !x_or_y);
        }
  
    }
}


void NTXLabelSearch::sortCandidates(bool (*compare)(label_t const*,label_t const*)) {
    label_t ** ppMatches = (matches[0]);
    sort(ppMatches, ppMatches + matchCount, compare);
}



#if 0


void NTXLabelSearch::printTreeStats() {
    int leafCount = 0, unbalancedCount = 0;
 
    for (int j=0; j<lblCount; ++j) {
        if ((labels[j]->child[0] == -1) && (labels[j]->child[1] == -1)) {
            leafCount++;
        } else if ((labels[j]->child[0] == -1) || (labels[j]->child[1] == -1)) {
            unbalancedCount++;
        }
    }
 
 
    printf("\nLabel index statistics:\n\n");
    printf("\nNodes, Leaves, Unbalanced: %d, %d, %d\n", lblCount, leafCount, unbalancedCount);
 
    long box[4] = {0, 0, 2000000000, 2000000000};
 
    search(1001,box);
 
    NTXLabelSearch test;
 
    for (int i=0; i < matchCount; ++i) {
        label_t const *lbl = candidate(i);
        test.insert(lbl->recno, lbl->desc, lbl->data);
    }
 
    leafCount = unbalancedCount = 0;
    for (int j = 0; j<lblCount; ++i) {
        if ((test.labels[j]->child[0] == -1) && (test.labels[j]->child[1] == -1)) {
            leafCount++;
        } else if ((test.labels[j]->child[0] == -1) || (test.labels[j]->child[1] == -1)) {
            unbalancedCount++;
        }
    }
 
    printf("\nWorse case:\n\n");
    printf("\nNodes, Leaves, Unbalanced: %d, %d, %d\n", test.lblCount, leafCount, unbalancedCount);
}





void balance() {
    // walk the tree in-order to get a more or less sorted result.
    // insert the nodes in the same order they'd be visited by
    // a binary tree search.  try to do this in-place
    // this might only be called for if the tree has a lot of nodes
    // with one child
}

#endif
