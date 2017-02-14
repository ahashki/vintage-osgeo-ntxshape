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



/**
 *	patmatch.c
 *
 *	Unix glob-style pattern matching routine.
 *	Simpler than regexp.
 *
 *      Extracted from:
 *	setargv.c - parse arguments
 *
 *	The code of setargv.c was placed in the public domain on 24-Jan-95 
 *      by the original author, Frank Whaley (few@autodesk.com).  The
 *      modified version included in NTXShape is not in the public domain.
**/


#include <string.h>
#include <ctype.h>




int haswildcard(char *s) {
    while (*s) {
        if ( (*s=='?') || (*s=='*') || (*s=='[') || (*s=='|') ) {
            return 1;
        } else if ( *(s++) == '\\' && *s) {
            s++;
        }
    }

    return 0;
}

static char * findnextbar(char *pat) {
    if (*pat) {
        ++pat;
    }
    
    while (*pat && (*pat != '|')) {
        if ( *(pat++) == '\\' && *pat) {
            pat++;
        }
    }
    return pat;
}


/* match pattern against text, case sensitive */
int patmatch(char *pat, char *txt) {
    int ret;
  
    int ccLast, ccMatched, ccInvert;
  
    char *txtStart = txt;
  
    for ( ; *pat; pat++ ) {
      
        if (*pat == '|') {
            /* bar encountered.  at end of text? */
            if (*txt == '\0') {
                return 1;
            } else {
                /* previous pattern did not match; start over
                   with the next part of the pattern */
                txt = txtStart;
                continue;
            }
            
        } else if ( (*txt == '\0') && (*pat != '*') ) {
            /* ran out of text and there are more pattern characters. */
            /* we know we're not at a '|' right now */
            /* but scan forward for next the next one, if any */
            if (*(pat=findnextbar(pat))) {
                txt = txtStart;
                continue;
            } else {
                return 0;
            }
        }
        
        
        switch (*pat) {
          case '*':	/*  match zero or more chars  */
            /*  trailing '*' matches everything  */
            if (*++pat == '\0') {
                return 1;
            }
            
            /*  try matching balance of pattern  */
            while ((ret = patmatch(pat, txt++)) == 0) {
                /* no-op */
            }
            
            return ret;

          case '[': /*  match character class */
            if ( (ccInvert = (pat[1] == '!')) != 0 ) {
                pat++;
            }
            
            for (
                ccLast = 256, ccMatched = 0;
                *++pat && (*pat != ']');
                ccLast = *pat
            ) {
                if (
                    (*pat == '-')
                  ? (*txt <= *++pat) && (*txt >= ccLast)
                  : (*txt == *pat)
                ) {
                    ccMatched = 1;
                }
            }
                
            if (ccMatched == ccInvert) {
                /* did I get this right? */
                if (*(pat = findnextbar(pat))) {
                    txt = txtStart;
                    continue;
                } else {
                    return 0;
                }
            }
            break;

          case '?':     /*  match any char  */
            break;

          case '\\':    /*  literal match next char  */
            pat++;
            /* fall through */
          default:      /*  match single char  */
            if ( *txt != *pat ) {
                if (*(pat = findnextbar(pat))) {
                    txt = txtStart;
                    continue;
                } else {
                    return 0;
                }
            }
        }
        
        txt++;
    }

    /*  matched if end of pattern and text  */
    return ( *txt == '\0' );
}



/*
 *	match pattern against text, case insensitive
 */
int patmatchi(char *pat, char *txt) {
    int ret;
    
    int ccLast, ccMatched, ccInvert;
  
    char *txtStart = txt;

    for ( ; *pat; pat++ ) {
        if (*pat == '|') {
            /* bar encountered.  at end of text? */
            if (*txt == '\0') {
                return 1;
            } else {
                /* previous pattern did not match; start over
                   with the next part of the pattern */
                txt = txtStart;
                continue;
            }
            
        } else if ( (*txt == '\0') && (*pat != '*') ) {
            /* ran out of text and there are more pattern characters. */
            /* we know we're not at a '|' right now */
            /* but scan forward for next the next one, if any */
            if (*(pat=findnextbar(pat))) {
                txt = txtStart;
                continue;
            } else {
                return 0;
            }
        }

        
        switch ( *pat ) {
          case '*':	/*  match zero or more chars  */
            /*  trailing '*' matches everything  */
            if ( *++pat == '\0' ) {
                return 1;
            }
            /*  try matching balance of pattern  */
            while ( (ret = patmatchi(pat, txt++)) == 0 ) {
                /* no-op */
            }

            return ret;

          case '[':	/*  match character class  */
            if ( (ccInvert = (pat[1] == '!')) != 0 ) {
                pat++;
            }
            
            for (
                ccLast = 256, ccMatched = 0;
                *++pat && (*pat != ']');
                ccLast = *pat
            ) {
                if (
                    (*pat == '-')
                  ? (tolower(*txt) <= tolower(*++pat)) && (tolower(*txt) >= tolower((char)ccLast))
                  : (tolower(*txt) == tolower(*pat))
                ) {
                    ccMatched = 1;
                }
            }
            
            if (ccMatched == ccInvert) {
                /* did I get this right? */
                if (*(pat = findnextbar(pat))) {
                    txt = txtStart;
                    continue;
                } else {
                    return 0;
                }
            }
            break;
          case '?':     /*  match any char  */
            break;
          case '\\':    /*  literal match next char  */
            pat++;
            /*fall through*/
          default:      /*  match single char  */
            if ( tolower(*txt) != tolower(*pat) ) {
                if (*(pat = findnextbar(pat))) {
                    txt = txtStart;
                    continue;
                } else {
                    return 0;
                }
            }
        }
        
        txt++;
    }

    /*  matched if end of pattern and text  */
    return ( *txt == '\0' );
}


