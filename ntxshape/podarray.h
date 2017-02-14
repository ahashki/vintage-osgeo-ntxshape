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

// podarray.h

// pod_array: an auto-growing array
// which can hold plain only plain ol' data
// since it uses realloc.
//
// ncolumns must be >= 1, or bad things will happen.
//
//
// This module is slated for removal in favor of stl vectors
// but for now it does the job.


#ifndef __PODARRAY_H
#define __PODARRAY_H

#include <malloc.h>

template <class elt_t, size_t ncolumns=1> class pod_array {
  public:
    pod_array(size_t sz = 0):pArray(0),size(0) { resize(sz); }
    ~pod_array() { resize(0); }
  
    bool resize(size_t newsize); //manually resizes the array.
    size_t storage() const { return size; }
  
    elt_t* operator[](size_t offset); //resizes if needed
    elt_t const * operator[](size_t offset) const;
    elt_t* reserve(size_t need_size);
    elt_t* detach() { elt_t *ptr = pArray; pArray = 0; size = 0; return ptr; }

  private:
    elt_t* pArray;
    size_t size;
  
    //not implemented:
    pod_array(const pod_array&);
    void operator=(const pod_array&);
};

template <class elt_t, size_t ncolumns>
bool pod_array<elt_t,ncolumns>::resize(size_t newsize) {
    elt_t * newArray = (elt_t*)realloc(pArray, newsize * sizeof(elt_t) * ncolumns);
    if (newArray) {
        pArray = newArray;
        size = newsize;
        return true;
    } else if (!newsize) {
        pArray = 0;
        size = 0;
        return true;
    } else {
        return false;
    }
}

template <class elt_t, size_t ncolumns>
elt_t * pod_array<elt_t,ncolumns>::reserve(size_t needsize) {
    if (needsize>size) {
        size_t newsize = (size+1)*2;
        while (needsize>newsize) {
            newsize *=2;
        }
        return resize(newsize) ? pArray : (elt_t*)0;
    }
    return pArray;
}


template <class elt_t, size_t ncolumns>
elt_t * pod_array<elt_t,ncolumns>::operator[](size_t offset) {
    if (offset>=size) {
        size_t newsize = (size+1)*2;
        while (offset>=newsize) {
            newsize *=2;
        }
    
        if (!resize(newsize)) {
            return (elt_t*)0;
        }
    }
  
    return pArray + (offset * ncolumns);
}


template <class elt_t, size_t ncolumns>
elt_t const * pod_array<elt_t,ncolumns>::operator[](size_t offset) const {
    if (offset>=size) {
        return (elt_t const*)0;
    }
    return (elt_t const *)(pArray + (offset * ncolumns));
}

template <class elt_t> class pod_array<elt_t,0> { 
  //0 columns would be invalid.  prevent accidents.
  protected:
    pod_array();
};

#endif
