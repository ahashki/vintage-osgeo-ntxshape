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

/* stripext.c */

#include <string.h>

#ifdef unix
#define PATH_SEPARATOR '/'
#else
#define PATH_SEPARATOR '\\'
#endif


#if !defined(MSDOS) /* multiple extensions allowed.  strip from right */
char * stripext(char * fname) {
    char * pc;
    char * ext_start;
    ext_start = fname + strlen(fname);
    for( pc = ext_start-1; pc >= fname && *pc != PATH_SEPARATOR; --pc) {
        if (*pc == '.') {
            *pc = '\0';
            ext_start = pc+1;
            break;
        }
    }
  
    return ext_start;
}

#else  /*only one extension allowed.  strip from left */

char * stripext(char * fname) {
    char * pc;
    char * ext_start;
  
    pc = strrchr(fname,PATH_SEPARATOR);
    if (!pc) {
        pc = fname;
    }
    
    ext_start = strchr(pc,'.');
    
    if (ext_start) {
        *ext_start++ = '\0';
    } else {
        ext_start = fname + strlen(fname);
    }
    
    return ext_start;
}

#endif
