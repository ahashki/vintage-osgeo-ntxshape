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


#ifndef __STATUSDLG_H
#define __STATUSDLG_H

#define WIN32_MEAN_AND_LEAN
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

void __stdcall OpenProgressDialog(const char *message, HWND hwndParent=NULL);
void __stdcall UpdateProgressDialog(float progress);
void __stdcall CloseProgressDialog();

#ifdef __cplusplus
}
#endif

#endif
