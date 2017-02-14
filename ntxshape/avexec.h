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


/* avexec.h */
/* The idea here is to support a late-bound AVExec,
   so that a DLL can be ArcView-enabled without _requiring_ 
   that it be called from ArcView.

   The interface is the same as the regular ArcView AVExec.  However
   there is an optional pair of supporting functions, LoadAVExec
   and UnloadAVExec.  The idea is that, if you're going to call
   AVExec many times, calling LoadAVExec before will speed it up.
   There is a reference count, so all calls to LoadAVExec should be
   matched with a call to UnloadAVExec!

   You could also call LoadAVExec/UnloadAVExec from DllMain
   if you know it is being called from ArcView, e.g. if
   GetModuleHandle(NULL) == GetModuleHandle("ARCVIEW.EXE").

   Note:
   Since ArcView is single-threaded, no effort was made to make AVExec's
   Load/Unload routines MT-safe.
*/

#ifndef __AVEXEC_H
#define __AVEXEC_H


#if defined (__cplusplus)
extern "C" {
#endif

    /* LoadAVExec loads the DLL.  Returns true if successful*/
    
    int LoadAVExec();
    
    
    /* UnloadAVExec unloads the DLL.  A reference count is used.
       Call this once for every call to LoadAVExec */
    
    int UnloadAVExec();
    
    
    /* AVExec compiles and executes an Avenue script, and returns the
       result as a string.  Internally, it also calls Load and Unload,
       so the above functions provide no speed advantage for a single
       isolated avenue script
    */
    
    char *AVExec(char* source);

#if defined (__cplusplus)
}
#endif

#endif
