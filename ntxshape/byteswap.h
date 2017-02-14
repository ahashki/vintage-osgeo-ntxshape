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



// BYTESWAP.H
// Defines a few inlines for swapping bytes.
// These functions pass by reference, so the variable passed in is
// changed.  They also return that variable by reference for use in
// nested expressions.

// This would also work, maybe for both signed, unsigned long.
// (( (unsigned long)val >> 24) | (val >> 8 & 0xFF00) | (val & 0xFF00 << 8) | (val<<24 ))



#ifndef __BYTESWAP_H
#define __BYTESWAP_H


inline unsigned long & ByteSwap(unsigned long & val) {
    val = ( (val>>8) & 0x00FF00FFUL ) | ( (val<<8) & 0xFF00FF00UL );
    return val;
}


inline unsigned long & ShortSwap(unsigned long & val) {
    val = ( (val>>16) & 0x0000FFFFUL ) | ( (val<<16) & 0xFFFF0000UL );
    return val;
}


inline unsigned long & AllSwap(unsigned long & val) {
    return ShortSwap(ByteSwap(val));
}



inline long & ByteSwap(long & val) {
    val = ( (val>>8) & 0x00FF00FFL ) | ( (val<<8) & 0xFF00FF00L );
    return val;
}


inline long & ShortSwap(long & val) {
   val = ( (val>>16) & 0x0000FFFFL ) | ( (val<<16) & 0xFFFF0000L );
   return val;
}


inline long & AllSwap(long & val) {
   return ShortSwap(ByteSwap(val));
}


#endif
