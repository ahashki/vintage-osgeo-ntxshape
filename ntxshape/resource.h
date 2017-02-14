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


#ifndef __RESOURCE_H
#define __RESOURCE_H


#define IDD_ProgressDialog                              100
#define IDC_StaticTaskName                             3002
#define IDC_ProgressBar                                3003

/**
 * NOTE: the name of the DLL project, and therefore the name of the DLL,
 * will change in future decimal-place releases (1.5, 2.0, etc.).  This
 * is because a requirement if supporting access from ArcGIS/VB/COM
 * is that the DLL be placed in the system path, e.g. in SYSTEM32.  To
 * reduce the risk of incompatibilities between a version installed for
 * ArcView 3, and for other products, the version number is incorporated
 * into the filename.
**/

#define NTX_VERSION_QUAD 1,4,2,101
#define NTX_VERSION_NUMBER 1.4

#define NTX_VERSION_BASE "1.4"
#define NTX_VERSION_PATCH "b-pre"
#define NTX_VERSION NTX_VERSION_BASE NTX_VERSION_PATCH

#define NTX_DLL_NAME "NTXAPI14.DLL"
#define NTX_COPYRIGHT_DATE "1997-2009"

#endif
